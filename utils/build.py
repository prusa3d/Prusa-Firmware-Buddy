#!/usr/bin/env python3
import argparse
import os
import platform
import random
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET
from abc import ABC, abstractmethod, abstractproperty
from copy import deepcopy
from enum import Enum
from functools import lru_cache
from pathlib import Path
from typing import Dict, List, Optional
from uuid import uuid4

project_root = Path(__file__).resolve().parent.parent
dependencies_dir = project_root / '.dependencies'


def bootstrap(*args, interactive=False, check=False):
    """Run the bootstrap script."""
    bootstrap_py = project_root / 'utils' / 'bootstrap.py'
    result = subprocess.run([sys.executable, str(bootstrap_py)] + list(args),
                            check=False,
                            encoding='utf-8',
                            stdout=None if interactive else subprocess.PIPE,
                            stderr=None if interactive else subprocess.PIPE)
    return result


def project_version():
    """Return current project version (e. g. "4.0.3")"""
    with open(project_root / 'version.txt', 'r') as f:
        return f.read().strip()


@lru_cache()
def get_dependency(name):
    install_dir = Path(
        bootstrap('--print-dependency-directory', name,
                  check=True).stdout.strip())
    suffix = '.exe' if platform.system() == 'Windows' else ''
    if name == 'ninja':
        return install_dir / ('ninja' + suffix)
    elif name == 'cmake':
        return install_dir / 'bin' / ('cmake' + suffix)
    else:
        return install_dir


class Printer(Enum):
    """Represents the -DPRINTER CMake option."""

    MINI = 'MINI'


class Bootloader(Enum):
    """Represents the -DBOOTLOADER CMake option."""

    NO = 'NO'
    EMPTY = 'EMPTY'
    YES = 'YES'

    @property
    def file_component(self):
        if self == Bootloader.NO:
            return 'NOBOOT'
        elif self == Bootloader.EMPTY:
            return 'EMPTYBOOT'
        elif self == Bootloader.YES:
            return 'BOOT'
        else:
            raise NotImplementedError


class BuildType(Enum):
    """Represents the -DCONFIG CMake option."""

    DEBUG = 'DEBUG'
    RELEASE = 'RELEASE'


class HostTool(Enum):
    """Known host tools."""

    png2font = "png2font"
    bin2cc = "bin2cc"
    hex2dfu = "hex2dfu"
    makefsdata = "makefsdata"


class BuildConfiguration(ABC):
    @abstractmethod
    def get_cmake_cache_entries(self):
        """Convert the build configuration to CMake cache entries."""

    @abstractmethod
    def get_cmake_flags(self, build_dir: Path) -> List[str]:
        """Return all CMake command-line flags required to build this configuration."""

    @abstractproperty
    def name(self):
        """Name of the configuration."""

    def __hash__(self):
        return hash(self.name)


class FirmwareBuildConfiguration(BuildConfiguration):
    def __init__(self,
                 printer: Printer,
                 bootloader: Bootloader,
                 build_type: BuildType,
                 toolchain: Path = None,
                 generator: str = None,
                 generate_dfu: bool = False,
                 generate_bbf: bool = False,
                 signing_key: Path = None,
                 version_suffix: str = None,
                 version_suffix_short: str = None,
                 custom_entries: List[str] = None):
        self.printer = printer
        self.bootloader = bootloader
        self.build_type = build_type
        self.toolchain = toolchain or FirmwareBuildConfiguration.default_toolchain(
        )
        self.generator = generator
        self.generate_dfu = generate_dfu
        self.generate_bbf = generate_bbf
        self.signing_key = signing_key
        self.version_suffix = version_suffix
        self.version_suffix_short = version_suffix_short
        self.custom_entries = custom_entries or []

    @staticmethod
    def default_toolchain() -> Path:
        return Path(
            __file__).resolve().parent.parent / 'cmake/GccArmNoneEabi.cmake'

    def get_cmake_cache_entries(self):
        if self.generate_bbf and self.bootloader == Bootloader.EMPTY:
            generate_bbf = True
            signing_key_flg = self.signing_key.resolve(
            ) if self.signing_key else ''
        else:
            generate_bbf = False
            signing_key_flg = ''
        entries = []
        if self.generator.lower() == 'ninja':
            entries.append(('CMAKE_MAKE_PROGRAM', 'FILEPATH',
                            str(get_dependency('ninja'))))
        entries.extend([
            ('CMAKE_MAKE_PROGRAM', 'FILEPATH', str(get_dependency('ninja'))),
            ('PRINTER', 'STRING', self.printer.value),
            ('BOOTLOADER', 'STRING', self.bootloader.value),
            ('GENERATE_BBF', 'STRING', str(generate_bbf).upper()),
            ('GENERATE_DFU', 'BOOL', 'ON' if self.generate_dfu else 'OFF'),
            ('SIGNING_KEY', 'FILEPATH', str(signing_key_flg)),
            ('CMAKE_TOOLCHAIN_FILE', 'FILEPATH', str(self.toolchain)),
            ('CMAKE_BUILD_TYPE', 'STRING', self.build_type.value.title()),
            ('PROJECT_VERSION_SUFFIX', 'STRING', self.version_suffix or ''),
            ('PROJECT_VERSION_SUFFIX_SHORT', 'STRING',
             self.version_suffix_short or ''),
        ])
        entries.extend(self.custom_entries)
        return entries

    def get_cmake_flags(self, build_dir: Path) -> List[str]:
        cache_entries = self.get_cmake_cache_entries()
        flags = ['-D{}:{}={}'.format(*entry) for entry in cache_entries]
        flags += ['-G', self.generator or 'Ninja']
        flags += ['-S', str(Path(__file__).resolve().parent.parent)]
        flags += ['-B', str(build_dir)]
        return flags

    @property
    def name(self):
        components = [
            self.printer.name,
            self.build_type.value,
            self.bootloader.file_component,
        ]
        return '_'.join(components)


class HostToolBuildConfiguration(BuildConfiguration):
    def __init__(self,
                 build_type: BuildType,
                 tool: HostTool,
                 generator: str = None):
        self.build_type = build_type
        self.tool = tool
        self.generator = generator

    def get_cmake_cache_entries(self):
        entries = []
        if self.generator.lower() == 'ninja':
            entries.append(('CMAKE_MAKE_PROGRAM', 'FILEPATH',
                            str(get_dependency('ninja'))))
        entries.extend((tool.value.upper() + '_ENABLE', 'BOOL',
                        'YES' if tool == self.tool else 'NO')
                       for tool in HostTool)
        return entries

    def get_cmake_flags(self, build_dir: Path) -> List[str]:
        cache_entries = self.get_cmake_cache_entries()
        flags = ['-D{}:{}={}'.format(*entry) for entry in cache_entries]
        flags += ['-G', self.generator or 'Ninja']
        flags += ['-S', str(Path(__file__).resolve().parent.parent)]
        flags += ['-B', str(build_dir)]
        return flags

    @property
    def name(self):
        return '_'.join([self.tool.value, self.build_type.value.lower()])


class BuildResult:
    """Represents a result of an attempt to build the project."""

    def __init__(self, config_returncode: int, build_returncode: Optional[int],
                 stdout: Path, stderr: Path, products: List[Path]):
        self.config_returncode = config_returncode
        self.build_returncode = build_returncode
        self.stdout = stdout
        self.stderr = stderr
        self.products = products

    @property
    def configuration_failed(self):
        return self.config_returncode != 0

    @property
    def build_failed(self):
        return self.build_returncode != 0 and self.build_returncode is not None

    @property
    def is_failure(self):
        return self.configuration_failed or self.build_failed

    def __str__(self):
        return '<BuildResult config={self.config_returncode} build={self.build_returncode}>'.format(
            self=self)


def build(configuration: BuildConfiguration,
          build_dir: Path,
          configure_only=False,
          output_to_file=True) -> BuildResult:
    """Build a project with a single configuration."""
    flags = configuration.get_cmake_flags(build_dir=build_dir)

    # create the build directory
    build_dir.mkdir(parents=True, exist_ok=True)
    products = []

    if output_to_file:
        # stdout and stderr are saved to a file in the build directory
        stdout_path = build_dir / 'stdout.txt'
        stderr_path = build_dir / 'stderr.txt'
        stdout = open(stdout_path, 'w')
        stderr = open(stderr_path, 'w')
    else:
        stdout_path, stderr_path = None, None
        stdout, stderr = None, None

    # prepare the build
    config_process = subprocess.run([str(get_dependency('cmake'))] + flags,
                                    stdout=stdout,
                                    stderr=stderr,
                                    check=False)
    if not configure_only and config_process.returncode == 0:
        cmd = [
            str(get_dependency('cmake')), '--build',
            str(build_dir), '--config',
            configuration.build_type.value.lower()
        ]
        build_process = subprocess.run(cmd,
                                       stdout=stdout,
                                       stderr=stderr,
                                       check=False)
        build_returncode = build_process.returncode
        products.extend(build_dir / fname for fname in [
            'firmware', 'firmware.bin', 'firmware.bbf', 'firmware.dfu',
            'firmware.map'
        ] if (build_dir / fname).exists())
    else:
        build_returncode = None

    if stdout:
        stdout.close()
    if stderr:
        stderr.close()

    # collect the result and return
    return BuildResult(config_returncode=config_process.returncode,
                       build_returncode=build_returncode,
                       stdout=stdout_path,
                       stderr=stderr_path,
                       products=products)


class CProjectGenerator:
    @staticmethod
    def create_cmake_def(name, value_type, value) -> ET:
        definition = ET.Element('def')
        definition.attrib['name'] = name
        definition.attrib['type'] = value_type
        definition.attrib['val'] = value
        return definition

    @staticmethod
    def generate_language_settings(cconfigurations):
        """Generate .settings/language.settings.xml file"""
        settings = ET.parse(project_root / 'utils' / 'cproject' /
                            'template_language_settings.xml')
        project = settings.getroot()
        template = project.find('./configuration')
        project.remove(template)

        # create a `configuration` element for each `cconfiguration` in .cproject
        for cconfiguration in cconfigurations:
            new_config = deepcopy(template)
            managedbuilder_config = cconfiguration.find(
                './storageModule[@buildSystemId="org.eclipse.cdt'
                '.managedbuilder.core.configurationDataProvider"]')
            new_config.attrib['id'] = managedbuilder_config.attrib['id']
            new_config.attrib['name'] = managedbuilder_config.attrib['name']
            project.append(new_config)

        path = project_root / '.settings' / 'language.settings.xml'
        os.makedirs(path.parent, exist_ok=True)
        with open(path, 'wb') as f:
            f.write('<?xml version="1.0" encoding="UTF-8" standalone="no"?>\n'.
                    encode())
            settings.write(f)
        print('generated: .settings/language.settings.xml')

    @staticmethod
    def generate_core_settings():
        """Generate .settings/org.eclipse.cdt.core.prefs file"""
        shutil.copy(
            project_root / 'utils' / 'cproject' /
            'template_org_eclipse_cdt_core.prefs',
            project_root / '.settings' / 'org.eclipse.cdt.core.prefs')
        print('generated: .settings/org.eclipse.cdt.core.prefs')

    @staticmethod
    def generate_project():
        shutil.copy(
            project_root / 'utils' / 'cproject' / 'template_project.xml',
            project_root / '.project')
        print('generated: .project')

    @staticmethod
    def generate_cconfiguration(template: ET,
                                configuration: BuildConfiguration) -> ET:
        cconfiguration = deepcopy(template)
        cache_entries = configuration.get_cmake_cache_entries()
        cmake_defines = [
            CProjectGenerator.create_cmake_def(*entry)
            for entry in cache_entries
        ]
        name = configuration.name.upper()
        if isinstance(configuration, FirmwareBuildConfiguration):
            build_subdir = 'cproject-fw'
        else:
            build_subdir = 'cproject-host'

        # generate identifier for the configuration
        identifier = 'com.st.stm32cube.ide.mcu.gnu.managedbuild.config.exe.debug.'
        identifier += str(uuid4()).replace('-', '')
        cconfiguration.attrib['id'] = identifier

        # update name and identifier in configurationDataProvider
        managedbuilder_config = cconfiguration.find(
            './storageModule[@buildSystemId="org.eclipse.cdt'
            '.managedbuilder.core.configurationDataProvider"]')
        managedbuilder_config.attrib['id'] = identifier
        managedbuilder_config.attrib['name'] = name

        cdt_build_system = cconfiguration.find(
            './storageModule[@moduleId="cdtBuildSystem"]')
        build_config = cdt_build_system.find('configuration')
        build_config.attrib['name'] = name
        build_config.attrib['id'] = identifier
        folder_info = build_config.find('folderInfo')
        folder_info.attrib['id'] = identifier + '.'
        builder = folder_info.find('toolChain').find('builder')
        builder.attrib['buildPath'] = '/build/' + build_subdir

        cmake_config = cconfiguration.find(
            './storageModule[@moduleId="de.marw.cdt.cmake.core.settings"]')
        cmake_config.attrib['buildDir'] = 'build/' + build_subdir
        cmake_config.find('defs').extend(cmake_defines)
        # set our cmake from .dependencies
        cmake_path = str(get_dependency('cmake'))
        for platform_name in ('linux', 'win32'):
            cmake_config.find(platform_name).attrib['command'] = cmake_path
        return cconfiguration

    @staticmethod
    def generate(configurations: List[BuildConfiguration]):
        """Generate a .cproject and .project with given configurations."""
        template = ET.parse(project_root / 'utils' / 'cproject' /
                            'template_cproject.xml')
        cproject = template.getroot()
        coresettings = cproject.find(
            './storageModule[@moduleId="org.eclipse.cdt.core.settings"]')
        cconfiguration_all = coresettings.findall('cconfiguration')
        assert len(cconfiguration_all) == 1
        cconfiguration = cconfiguration_all[0]
        coresettings.remove(cconfiguration)
        for configuration in configurations:
            cconfig = CProjectGenerator.generate_cconfiguration(
                template=cconfiguration, configuration=configuration)
            coresettings.append(cconfig)
        with open(project_root / '.cproject', 'wb') as f:
            f.write('<?xml version="1.0" encoding="UTF-8" standalone="no"?>\n'.
                    encode())
            f.write('<?fileVersion 4.0.0?>\n'.encode())
            template.write(f)
        print('generated: .cproject')
        CProjectGenerator.generate_project()
        CProjectGenerator.generate_language_settings(coresettings)
        CProjectGenerator.generate_language_settings(coresettings)
        CProjectGenerator.generate_core_settings()


def store_products(products: List[Path], build_config: BuildConfiguration,
                   products_dir: Path):
    """Copy build products to a shared products directory."""
    products_dir.mkdir(parents=True, exist_ok=True)
    for product in products:
        is_firmware = isinstance(build_config, FirmwareBuildConfiguration)
        has_custom_suffix = is_firmware and (build_config.version_suffix !=
                                             '<auto>')
        if has_custom_suffix:
            version = project_version()
            name = build_config.name.lower(
            ) + '_' + version + build_config.version_suffix
        else:
            name = build_config.name.lower()
        destination = products_dir / (name + product.suffix)
        shutil.copy(product, destination)


def list_of(EnumType):
    """Create an argument-parser for comma-separated list of values of some Enum subclass."""

    def convert(val):
        if val == '':
            return []
        values = [p.lower() for p in val.split(',')]
        if 'all' in values:
            return list(EnumType)
        else:
            return [EnumType(v.upper()) for v in values]

    convert.__name__ = EnumType.__name__
    return convert


def cmake_cache_entry(arg):
    match = re.fullmatch(r'(.*):(.*)=(.*)', arg)
    if not match:
        raise ValueError('invalid cmake entry; must be <NAME>:<TYPE>=<VALUE>')
    return (match.group(1), match.group(2), match.group(3))


def main():
    parser = argparse.ArgumentParser()
    # yapf: disable
    parser.add_argument(
        '--printer',
        type=list_of(Printer),
        default=list(Printer),
        help='Printer type (default: {default}).'.format(
            default=','.join(str(p.value.lower()) for p in Printer)))
    parser.add_argument(
        '--build-type',
        type=list_of(BuildType),
        default='release',
        help=('Build type (debug or release; default: release; '
              'default for --generate-cproject: debug,release).'))
    parser.add_argument(
        '--bootloader',
        type=list_of(Bootloader),
        default='all',
        help='What bootloader mode to use ("yes", "no" or "empty"; default: "empty").')
    parser.add_argument(
        '--signing-key',
        type=Path,
        help='A PEM private key to be used to generate .bbf version.')
    parser.add_argument(
        '--version-suffix',
        type=str,
        default='<auto>',
        help='Version suffix (e.g. -BETA+1035.PR111.B4)')
    parser.add_argument(
        '--version-suffix-short',
        type=str,
        default='<auto>',
        help='Version suffix (e.g. +1035)')
    parser.add_argument(
        '--final',
        action='store_true',
        help='Set\'s --version-suffix and --version-suffix-short to empty string.')
    parser.add_argument(
        '--build-dir',
        type=Path,
        help='Specify a custom build directory to be used.')
    parser.add_argument(
        '--products-dir',
        type=Path,
        help='Directory to store built firmware (default: <build-dir>/products).')
    parser.add_argument(
        '-G', '--generator',
        type=str,
        default='Ninja',
        help='Generator to be used by CMake (default=Ninja).')
    parser.add_argument(
        '--toolchain',
        type=Path,
        help='Path to a CMake toolchain file to be used.')
    parser.add_argument(
        '--generate-cproject',
        action='store_true',
        help='Generate .cproject and .project files and exit without building.',
    )
    parser.add_argument(
        '--generate-bbf',
        action='store_true',
        help=('Generate .bbf versions of the firmware.'
              ' Use --signing-key to specify a private key to be used for signing.')
    )
    parser.add_argument(
        '--generate-dfu',
        action='store_true',
        help='Generate .dfu versions of the firmware.'
    )
    parser.add_argument(
        '--host-tools',
        action='store_true',
        help=('Build host tools (png2font and others). '
              'Turned on by default with --generate-cproject only.')
    )
    parser.add_argument(
        '--no-build',
        action='store_true',
        help='Do not build, configure the build only.'
    )
    parser.add_argument(
        '--no-store-output',
        dest='store_output',
        action='store_const',
        const=False,
        help='Do not write build output to files - print it to console instead.'
    )
    parser.add_argument(
        '--store-output',
        dest='store_output',
        action='store_const',
        const=True,
        help='Write build output to files - print it to console instead.'
    )
    parser.add_argument(
        '-D', '--cmake-def',
        action='append', type=cmake_cache_entry,
        help='Custom CMake cache entries (e.g. -DCUSTOM_COMPILE_OPTIONS:STRING=-Werror)'
    )
    args = parser.parse_args(sys.argv[1:])
    # yapf: enable

    build_dir_root = args.build_dir or Path(
        __file__).resolve().parent.parent / 'build'
    products_dir_root = args.products_dir or (build_dir_root / 'products')

    if args.final:
        args.version_suffix = ''
        args.version_suffix_short = ''

    if args.generate_cproject:
        args.build_type = list(BuildType)
        args.host_tools = True
        args.no_build = True

    # Check all dependencis are installed
    if bootstrap(interactive=True).returncode != 0:
        print('bootstrap.py failed.')
        sys.exit(1)

    # prepare configurations
    configurations = [
        FirmwareBuildConfiguration(
            printer=printer,
            bootloader=bootloader,
            build_type=build_type,
            generate_dfu=args.generate_dfu,
            generate_bbf=args.generate_bbf,
            signing_key=args.signing_key,
            version_suffix=args.version_suffix,
            version_suffix_short=args.version_suffix_short,
            generator=args.generator,
            custom_entries=args.cmake_def) for printer in args.printer
        for build_type in args.build_type for bootloader in args.bootloader
    ]
    if args.host_tools:
        configurations.extend([
            HostToolBuildConfiguration(tool=tool,
                                       build_type=build_type,
                                       generator=args.generator)
            for tool in HostTool for build_type in args.build_type
        ])

    # generate .cproject if requested
    if args.generate_cproject:
        CProjectGenerator.generate(configurations)
        sys.exit(0)

    # build everything
    results: Dict[BuildConfiguration, BuildResult] = dict()
    for configuration in configurations:
        build_dir = build_dir_root / configuration.name.lower()
        print('Building ' + configuration.name.lower())
        result = build(configuration,
                       build_dir=build_dir,
                       configure_only=args.no_build,
                       output_to_file=args.store_output
                       if args.store_output is not None else False)
        store_products(result.products, configuration, products_dir_root)
        results[configuration] = result

    # print results
    print()
    print('Building finished: {} success, {} failure(s).'.format(
        sum(1 for result in results.values() if not result.is_failure),
        sum(1 for result in results.values() if result.is_failure)))
    failure = False
    max_configname_len = max(len(config.name) for config in results)
    for config, result in results.items():
        if result.configuration_failed:
            status = 'project configuration FAILED'
            failure = True
        elif result.build_failed:
            status = 'build FAILED'
            failure = True
        else:
            status = 'SUCCESS'

        print(' {} {}'.format(
            config.name.lower().ljust(max_configname_len, ' '), status))
    if failure:
        sys.exit(1)


if __name__ == "__main__":
    main()
