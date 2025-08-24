#!/usr/bin/python3 -s
# -*- coding:utf-8 -*-
#
# A report generator for gcov 3.4
#
# This routine generates a format that is similar to the format generated
# by the Python coverage.py module.  This code is similar to the
# data processing performed by lcov's geninfo command.  However, we
# don't worry about parsing the *.gcna files, and backwards compatibility for
# older versions of gcov is not supported.
#
# Outstanding issues
#   - verify that gcov 3.4 or newer is being used
#   - verify support for symbolic links
#
# gcovr is a FAST project.  For documentation, bug reporting, and
# updates, see https://software.sandia.gov/trac/fast/wiki/gcovr
#
#  _________________________________________________________________________
#
#  Gcovr: A parsing and reporting tool for gcov
#  Copyright (c) 2013 Sandia Corporation.
#  This software is distributed under the BSD License.
#  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#  the U.S. Government retains certain rights in this software.
#  For more information, see the README.md file.
#  
#  ******* YOUR ATTENTION PLEASE! ********
#  This version has been hacked up by VintagePC (http://github.com/VintagePC) 
#  in order to emit Github-flavoured markdown for the purposes of commenting on PRs.
#  Your mileage may vary and there are exactly ZERO guarantees it will work for your use case. 
#
# _________________________________________________________________________
#
# $Revision$
# $Date$
#

try:
    import html
except:
    import cgi as html
import copy
import os
import re
import subprocess
import sys
import time
import xml.dom.minidom
import datetime
import posixpath
import itertools
import zlib

from optparse import OptionParser
from string import Template
from os.path import normpath

try:
    xrange
except NameError:
    xrange = range

# In Python3, zip is equivalent to izip
try:
    izip = itertools.izip
except AttributeError:
    izip = zip

medium_coverage = 75.0
high_coverage = 90.0
low_color = "LightPink"
medium_color = "#FFFF55"
high_color = "LightGreen"
covered_color = "LightGreen"
uncovered_color = "LightPink"
takenBranch_color = "Green"
notTakenBranch_color = "Red"

__version__ = "3.3"
src_revision = "$Revision$"

output_re = re.compile("[Cc]reating [`'](.*)'$")
source_re = re.compile("[Cc]annot open (source|graph) file")

starting_dir = os.getcwd()

exclude_line_flag = "_EXCL_"
exclude_line_pattern = re.compile('([GL]COVR?)_EXCL_(LINE|START|STOP)')

c_style_comment_pattern = re.compile('/\*.*?\*/')
cpp_style_comment_pattern = re.compile('//.*?$')


def version_str():
    ans = __version__
    m = re.match('\$Revision:\s*(\S+)\s*\$', src_revision)
    if m:
        ans = ans + " (r%s)" % (m.group(1))
    return ans


#
# Container object for coverage statistics
#
class CoverageData(object):

    def __init__(
            self, fname, uncovered, uncovered_exceptional, covered, branches,
            noncode):
        self.fname = fname
        # Shallow copies are cheap & "safe" because the caller will
        # throw away their copies of covered & uncovered after calling
        # us exactly *once*
        self.uncovered = copy.copy(uncovered)
        self.uncovered_exceptional = copy.copy(uncovered_exceptional)
        self.covered = copy.copy(covered)
        self.noncode = copy.copy(noncode)
        # But, a deep copy is required here
        self.all_lines = copy.deepcopy(uncovered)
        self.all_lines.update(uncovered_exceptional)
        self.all_lines.update(covered.keys())
        self.branches = copy.deepcopy(branches)

    def update(
            self, uncovered, uncovered_exceptional, covered, branches,
            noncode):
        self.all_lines.update(uncovered)
        self.all_lines.update(uncovered_exceptional)
        self.all_lines.update(covered.keys())
        self.uncovered.update(uncovered)
        self.uncovered_exceptional.update(uncovered_exceptional)
        self.noncode.intersection_update(noncode)
        for k in covered.keys():
            self.covered[k] = self.covered.get(k, 0) + covered[k]
        for k in branches.keys():
            for b in branches[k]:
                d = self.branches.setdefault(k, {})
                d[b] = d.get(b, 0) + branches[k][b]
        self.uncovered.difference_update(self.covered.keys())
        self.uncovered_exceptional.difference_update(self.covered.keys())

    def uncovered_str(self, exceptional):
        if options.show_branch:
            #
            # Don't do any aggregation on branch results
            #
            tmp = []
            for line in self.branches.keys():
                for branch in self.branches[line]:
                    if self.branches[line][branch] == 0:
                        tmp.append(line)
                        break
            tmp.sort()
            return ",".join([str(x) for x in tmp]) or ""

        if exceptional:
            tmp = list(self.uncovered_exceptional)
        else:
            tmp = list(self.uncovered)
        if len(tmp) == 0:
            return ""

        #
        # Walk through the uncovered lines in sorted order.
        # Find blocks of consecutive uncovered lines, and return
        # a string with that information.
        #
        tmp.sort()
        first = None
        last = None
        ranges = []
        for item in tmp:
            if last is None:
                first = item
                last = item
            elif item == (last + 1):
                last = item
            else:
                #
                # Should we include noncode lines in the range of lines
                # to be covered???  This simplifies the ranges summary, but it
                # provides a counterintuitive listing.
                #
                #if len(self.noncode.intersection(range(last+1,item))) \
                #       == item - last - 1:
                #    last = item
                #    continue
                #
                if first == last:
                    ranges.append(str(first))
                else:
                    ranges.append(str(first) + "-" + str(last))
                first = item
                last = item
        if first == last:
            ranges.append(str(first))
        else:
            ranges.append(str(first) + "-" + str(last))
        return ",".join(ranges)

    def coverage(self):
        if options.show_branch:
            total = 0
            cover = 0
            for line in self.branches.keys():
                for branch in self.branches[line].keys():
                    total += 1
                    cover += self.branches[line][branch] > 0 and 1 or 0
        else:
            total = len(self.all_lines)
            cover = len(self.covered)

        percent = total and str(int(100.0 * cover / total)) or "--"
        return (total, cover, percent)

    def summary(self):
        tmp = options.root_filter.sub('', self.fname)
        if not self.fname.endswith(tmp):
            # Do no truncation if the filter does not start matching at
            # the beginning of the string
            tmp = self.fname
        tmp = tmp.ljust(40)
        if len(tmp) > 40:
            tmp = tmp + "\n" + " " * 40

        (total, cover, percent) = self.coverage()
        uncovered_lines = "" #self.uncovered_str(False)
        if not options.show_branch:
            t = self.uncovered_str(True)
            if len(t):
                uncovered_lines += "" # " [* " + t + "]"
        return (total, cover,
                tmp.strip() + " | " + str(total) + " | " + str(cover) + " | " + percent + "%") # + uncovered_lines)


def resolve_symlinks(orig_path):
    """
    Return the normalized absolute path name with all symbolic links resolved
    """
    return os.path.realpath(orig_path)
    # WEH - why doesn't os.path.realpath() suffice here?
    #
    drive, tmp = os.path.splitdrive(os.path.abspath(orig_path))
    if not drive:
        drive = os.path.sep
    parts = tmp.split(os.path.sep)
    actual_path = [drive]
    while parts:
        actual_path.append(parts.pop(0))
        if not os.path.islink(os.path.join(*actual_path)):
            continue
        actual_path[-1] = os.readlink(os.path.join(*actual_path))
        tmp_drive, tmp_path = os.path.splitdrive(
            resolve_symlinks(os.path.join(*actual_path)))
        if tmp_drive:
            drive = tmp_drive
        actual_path = [drive] + tmp_path.split(os.path.sep)
    return os.path.join(*actual_path)


#
# Class that creates path aliases
#
class PathAliaser(object):

    def __init__(self):
        self.aliases = {}
        self.master_targets = set()
        self.preferred_name = {}

    def path_startswith(self, path, base):
        return path.startswith(base) and (
            len(base) == len(path) or path[len(base)] == os.path.sep)

    def master_path(self, path):
        match_found = False
        while True:
            for base, alias in self.aliases.items():
                if self.path_startswith(path, base):
                    path = alias + path[len(base):]
                    match_found = True
                    break
            for master_base in self.master_targets:
                if self.path_startswith(path, master_base):
                    return path, master_base, True
            if match_found:
                sys.stderr.write(
                    "(ERROR) violating fundamental assumption while walking "
                    "directory tree.\n\tPlease report this to the gcovr "
                    "developers.\n")
            return path, None, match_found

    def unalias_path(self, path):
        path = resolve_symlinks(path)
        path, master_base, known_path = self.master_path(path)
        if not known_path:
            return path
        # Try and resolve the preferred name for this location
        if master_base in self.preferred_name:
            return self.preferred_name[master_base] + path[len(master_base):]
        return path

    def add_master_target(self, master):
        self.master_targets.add(master)

    def add_alias(self, target, master):
        self.aliases[target] = master

    def set_preferred(self, master, preferred):
        self.preferred_name[master] = preferred

aliases = PathAliaser()


# This is UGLY.  Here's why: UNIX resolves symbolic links by walking the
# entire directory structure.  What that means is that relative links
# are always relative to the actual directory inode, and not the
# "virtual" path that the user might have traversed (over symlinks) on
# the way to that directory.  Here's the canonical example:
#
#   a / b / c / testfile
#   a / d / e --> ../../a/b
#   m / n --> /a
#   x / y / z --> /m/n/d
#
# If we start in "y", we will see the following directory structure:
#   y
#   |-- z
#       |-- e
#           |-- c
#               |-- testfile
#
# The problem is that using a simple traversal based on the Python
# documentation:
#
#    (os.path.join(os.path.dirname(path), os.readlink(result)))
#
# will not work: we will see a link to /m/n/d from /x/y, but completely
# miss the fact that n is itself a link.  If we then naively attempt to
# apply the "c" relative link, we get an intermediate path that looks
# like "/m/n/d/e/../../a/b", which would get normalized to "/m/n/a/b"; a
# nonexistant path.  The solution is that we need to walk the original
# path, along with the full path of all links 1 directory at a time and
# check for embedded symlinks.
#
#
# NB:  Users have complained that this code causes a performance issue.
# I have replaced this logic with os.walk(), which works for Python >= 2.6
#
def link_walker(path):
    if sys.version_info >= (2, 6):
        for root, dirs, files in os.walk(
            os.path.abspath(path), followlinks=True
        ):
            for exc in options.exclude_dirs:
                for d in dirs:
                    m = exc.search(d)
                    if m is not None:
                        dirs[:] = [d for d in dirs if d is not m.group()]
            yield (os.path.abspath(os.path.realpath(root)), dirs, files)
    else:
        targets = [os.path.abspath(path)]
        while targets:
            target_dir = targets.pop(0)
            actual_dir = resolve_symlinks(target_dir)
            #print "target dir: %s  (%s)" % (target_dir, actual_dir)
            master_name, master_base, visited = aliases.master_path(actual_dir)
            if visited:
                #print "  ...root already visited as %s" % master_name
                aliases.add_alias(target_dir, master_name)
                continue
            if master_name != target_dir:
                aliases.set_preferred(master_name, target_dir)
                aliases.add_alias(target_dir, master_name)
            aliases.add_master_target(master_name)
            #print "  ...master name = %s" % master_name
            #print "  ...walking %s" % target_dir
            for root, dirs, files in os.walk(target_dir, topdown=True):
                #print "    ...reading %s" % root
                for d in dirs:
                    tmp = os.path.abspath(os.path.join(root, d))
                    #print "    ...checking %s" % tmp
                    if os.path.islink(tmp):
                        #print "      ...buffering link %s" % tmp
                        targets.append(tmp)
                yield (root, dirs, files)


def search_file(expr, path):
    """
    Given a search path, recursively descend to find files that match a
    regular expression.
    """
    ans = []
    pattern = re.compile(expr)
    if path is None or path == ".":
        path = os.getcwd()
    elif not os.path.exists(path):
        raise IOError("Unknown directory '" + path + "'")
    for root, dirs, files in link_walker(path):
        for name in files:
            if pattern.match(name):
                name = os.path.join(root, name)
                if os.path.islink(name):
                    ans.append(os.path.abspath(os.readlink(name)))
                else:
                    ans.append(os.path.abspath(name))
    return ans


def commonpath( files ):

    if len(files) == 1:
        return os.path.join( os.path.relpath( os.path.split( files[0] )[0] ), '' )

    common_path = os.path.realpath(files[0])
    common_dirs = common_path.split( os.path.sep )

    for f in files[1:]:
        path = os.path.realpath(f)
        dirs = path.split( os.path.sep )
        common = []
        for a, b in izip( dirs, common_dirs ):
            if a == b:
                common.append( a )
            elif common:
                common_dirs = common
                break
            else:
                return ''

    return os.path.join( os.path.relpath( os.path.sep.join( common_dirs ) ), '' )


#
# Get the list of datafiles in the directories specified by the user
#
def get_datafiles(flist, options):
    allfiles = set()
    for dir_ in flist:
        if options.gcov_files:
            if options.verbose:
                sys.stdout.write(
                    "Scanning directory %s for gcov files...\n" % (dir_, )
                )
            files = search_file(".*\.gcov$", dir_)
            gcov_files = [file for file in files if file.endswith('gcov')]
            if options.verbose:
                sys.stdout.write(
                    "Found %d files (and will process %d)\n" %
                    (len(files), len(gcov_files))
                )
            allfiles.update(gcov_files)
        else:
            if options.verbose:
                sys.stdout.write(
                    "Scanning directory %s for gcda/gcno files...\n" % (dir_, )
                )
            files = search_file(".*\.gc(da|no)$", dir_)
            # gcno files will *only* produce uncovered results; however,
            # that is useful information for the case where a compilation
            # unit is never actually exercised by the test code.  So, we
            # will process gcno files, but ONLY if there is no corresponding
            # gcda file.
            gcda_files = [
                filenm for filenm in files if filenm.endswith('gcda')
            ]
            tmp = set(gcda_files)
            gcno_files = [
                filenm for filenm in files if
                filenm.endswith('gcno') and filenm[:-2] + 'da' not in tmp
            ]
            if options.verbose:
                sys.stdout.write(
                    "Found %d files (and will process %d)\n" %
                    (len(files), len(gcda_files) + len(gcno_files)))
            allfiles.update(gcda_files)
            allfiles.update(gcno_files)
    return allfiles


noncode_mapper = dict.fromkeys(ord(i) for i in '}{)(;')
def is_non_code(code):
    if sys.version_info < (3,0):
        code = code.strip().translate(None, '}{)(;')
    else:
        code = code.strip().translate(noncode_mapper)
    return len(code) == 0 or code.startswith("//") or code == 'else'


#
# Process a single gcov datafile
#
def process_gcov_data(data_fname, covdata, source_fname, options):
    INPUT = open(data_fname, "r")
    #
    # Get the filename
    #
    line = INPUT.readline()
    segments = line.split(':', 3)
    if len(segments) != 4 or not \
            segments[2].lower().strip().endswith('source'):
        raise RuntimeError(
            'Fatal error parsing gcov file, line 1: \n\t"%s"' % line.rstrip()
        )
    #
    # Find the source file
    #
    currdir = os.getcwd()
    root_dir = os.path.abspath(options.root)
    if source_fname is None:
        common_dir = os.path.commonprefix([data_fname, currdir])
        if sys.version_info >= (2, 6):
            fname = aliases.unalias_path(os.path.join(common_dir, segments[-1].strip()))
        else:
            fname = aliases.unalias_path(os.path.join(common_dir, segments[-1]).strip())
    else:
        #     1. Try using the path to common prefix with the root_dir as the source directory
        fname = os.path.join(root_dir, segments[-1].strip())
        if not os.path.exists(fname):
            # 2. Try using the path to the gcda file as the source directory
            fname = os.path.join(os.path.dirname(source_fname), os.path.basename(segments[-1].strip()))

    if options.verbose:
        print("Finding source file corresponding to a gcov data file")
        print('  currdir      '+currdir)
        print('  gcov_fname   '+data_fname)
        print('               '+str(segments))
        print('  source_fname '+source_fname)
        print('  root         '+root_dir)
        #print('  common_dir   '+common_dir)
        #print('  subdir       '+subdir)
        print('  fname        '+fname)

    if options.verbose:
        sys.stdout.write("Parsing coverage data for file %s\n" % fname)
    #
    # Return if the filename does not match the filter
    #
    filtered_fname = None
    for i in range(0, len(options.filter)):
        if options.filter[i].match(fname):
            filtered_fname = options.root_filter.sub('', fname)
            break
    if filtered_fname is None:
        if options.verbose:
            sys.stdout.write("  Filtering coverage data for file %s\n" % fname)
        return
    #
    # Return if the filename matches the exclude pattern
    #
    for exc in options.exclude:
        if (filtered_fname is not None and exc.match(filtered_fname)) or \
                exc.match(fname) or \
                exc.match(os.path.abspath(fname)):
            if options.verbose:
                sys.stdout.write(
                    "  Excluding coverage data for file %s\n" % fname
                )
            return
    #
    # Parse each line, and record the lines that are uncovered
    #
    excluding = []
    noncode = set()
    uncovered = set()
    uncovered_exceptional = set()
    covered = {}
    branches = {}
    #first_record=True
    lineno = 0
    last_code_line = ""
    last_code_lineno = 0
    last_code_line_excluded = False
    for line in INPUT:
        segments = line.split(":", 2)
        #print "\t","Y", segments
        tmp = segments[0].strip()
        if len(segments) > 1:
            try:
                lineno = int(segments[1].strip())
            except:
                pass  # keep previous line number!

        if exclude_line_flag in line:
            excl_line = False
            for header, flag in exclude_line_pattern.findall(line):
                if flag == 'START':
                    excluding.append((header, lineno))
                elif flag == 'STOP':
                    if excluding:
                        _header, _line = excluding.pop()
                        if _header != header:
                            sys.stderr.write(
                                "(WARNING) %s_EXCL_START found on line %s "
                                "was terminated by %s_EXCL_STOP on line %s, "
                                "when processing %s\n"
                                % (_header, _line, header, lineno, fname)
                            )
                    else:
                        sys.stderr.write(
                            "(WARNING) mismatched coverage exclusion flags.\n"
                            "\t%s_EXCL_STOP found on line %s without "
                            "corresponding %s_EXCL_START, when processing %s\n"
                            % (header, lineno, header, fname)
                        )
                elif flag == 'LINE':
                    # We buffer the line exclusion so that it is always
                    # the last thing added to the exclusion list (and so
                    # only ONE is ever added to the list).  This guards
                    # against cases where puts a _LINE and _START (or
                    # _STOP) on the same line... it also guards against
                    # duplicate _LINE flags.
                    excl_line = True
            if excl_line:
                excluding.append(False)

        is_code_statement = False
        if tmp[0] == '-' or (excluding and tmp[0] in "#=0123456789"):
            is_code_statement = True
            if len(segments) < 3:
                noncode.add(lineno)
                continue
                
            code = segments[2].strip()
            # remember certain non-executed lines
            if excluding or is_non_code(segments[2]):
                noncode.add(lineno)
        elif tmp[0] == '#':
            is_code_statement = True
            if is_non_code(segments[2]):
                noncode.add( lineno )
            else:
                uncovered.add( lineno )
        elif tmp[0] == '=':
            is_code_statement = True
            uncovered_exceptional.add(lineno)
        elif tmp[0] in "0123456789":
            is_code_statement = True
            covered[lineno] = int(segments[0].strip().rstrip('*'))
        elif tmp.startswith('branch'):
            exclude_branch = False
            if options.exclude_unreachable_branches and \
                    lineno == last_code_lineno:
                if last_code_line_excluded:
                    exclude_branch = True
                    exclude_reason = "marked with exclude pattern"
                else:
                    code = last_code_line
                    code = re.sub(cpp_style_comment_pattern, '', code)
                    code = re.sub(c_style_comment_pattern, '', code)
                    code = code.strip()
                    code_nospace = code.replace(' ', '')
                    exclude_branch = \
                        code in ['', '{', '}'] or code_nospace == '{}'
                    exclude_reason = "detected as compiler-generated code"

            if exclude_branch:
                if options.verbose:
                    sys.stdout.write(
                        "Excluding unreachable branch on line %d "
                        "in file %s (%s).\n"
                        % (lineno, fname, exclude_reason)
                    )
            else:
                fields = line.split()
                try:
                    count = int(fields[3])
                except:
                    count = 0
                branches.setdefault(lineno, {})[int(fields[1])] = count
        elif tmp.startswith('call'):
            pass
        elif tmp.startswith('function'):
            pass
        elif tmp[0] == 'f':
            pass
            #if first_record:
                #first_record=False
                #uncovered.add(prev)
            #if prev in uncovered:
                #tokens=re.split('[ \t]+',tmp)
                #if tokens[3] != "0":
                    #uncovered.remove(prev)
            #prev = int(segments[1].strip())
            #first_record=True
        else:
            sys.stderr.write(
                "(WARNING) Unrecognized GCOV output: '%s'\n"
                "\tThis is indicitive of a gcov output parse error.\n"
                "\tPlease report this to the gcovr developers." % tmp
            )

        # save the code line to use it later with branches
        if is_code_statement:
            last_code_line = "".join(segments[2:])
            last_code_lineno = lineno
            last_code_line_excluded = False
            if excluding:
                last_code_line_excluded = True

        # clear the excluding flag for single-line excludes
        if excluding and not excluding[-1]:
            excluding.pop()

    if options.verbose:
        print('uncovered '+str(uncovered))
        #print('covered ',+covered)
        #print('branches ',+str(branches))
        #print('noncode ',+str(noncode))
    #
    # If the file is already in covdata, then we
    # remove lines that are covered here.  Otherwise,
    # initialize covdata
    #
    if not fname in covdata:
        covdata[fname] = CoverageData(
            fname, uncovered, uncovered_exceptional, covered, branches, noncode
        )
    else:
        covdata[fname].update(
            uncovered, uncovered_exceptional, covered, branches, noncode
        )
    INPUT.close()

    for header, line in excluding:
        sys.stderr.write("(WARNING) The coverage exclusion region start flag "
                         "%s_EXCL_START\n\ton line %d did not have "
                         "corresponding %s_EXCL_STOP flag\n\t in file %s.\n"
                         % (header, line, header, fname))

#
# Process a datafile (generated by running the instrumented application)
# and run gcov with the corresponding arguments
#
# This is trickier than it sounds: The gcda/gcno files are stored in the
# same directory as the object files; however, gcov must be run from the
# same directory where gcc/g++ was run.  Normally, the user would know
# where gcc/g++ was invoked from and could tell gcov the path to the
# object (and gcda) files with the --object-directory command.
# Unfortunately, we do everything backwards: gcovr looks for the gcda
# files and then has to infer the original gcc working directory.
#
# In general, (but not always) we can assume that the gcda file is in a
# subdirectory of the original gcc working directory, so we will first
# try ".", and on error, move up the directory tree looking for the
# correct working directory (letting gcov's own error codes dictate when
# we hit the right directory).  This covers 90+% of the "normal" cases.
# The exception to this is if gcc was invoked with "-o ../[...]" (i.e.,
# the object directory was a peer (not a parent/child) of the cwd.  In
# this case, things are really tough.  We accept an argument
# (--object-directory) that SHOULD BE THE SAME as the one povided to
# gcc.  We will then walk that path (backwards) in the hopes of
# identifying the original gcc working directory (there is a bit of
# trial-and-error here)
#
def process_datafile(filename, covdata, options):
    if options.verbose:
        print("Processing file: "+filename)
    #
    # Launch gcov
    #
    abs_filename = os.path.abspath(filename)
    dirname, fname = os.path.split(abs_filename)

    potential_wd = []
    errors = []
    Done = False

    if options.objdir:
        #print "X - objdir"
        src_components = abs_filename.split(os.sep)
        components = normpath(options.objdir).split(os.sep)
        idx = 1
        while idx <= len(components):
            if idx > len(src_components):
                break
            if components[-1 * idx] != src_components[-1 * idx]:
                break
            idx += 1
        if idx > len(components):
            pass  # a parent dir; the normal process will find it
        elif components[-1 * idx] == '..':
            # NB: os.path.join does not re-add leading '/' characters!?!
            dirs = [
                os.path.sep.join(src_components[:len(src_components) - idx])
            ]
            while idx <= len(components) and components[-1 * idx] == '..':
                tmp = []
                for d in dirs:
                    for f in os.listdir(d):
                        x = os.path.join(d, f)
                        if os.path.isdir(x):
                            tmp.append(x)
                dirs = tmp
                idx += 1
            potential_wd = dirs
        else:
            if components[0] == '':
                # absolute path
                tmp = [options.objdir]
            else:
                # relative path: check relative to both the cwd and the
                # gcda file
                tmp = [
                    os.path.join(x, options.objdir) for x in
                    [os.path.dirname(abs_filename), os.getcwd()]
                ]
            potential_wd = [
                testdir for testdir in tmp if os.path.isdir(testdir)
            ]
            if len(potential_wd) == 0:
                errors.append("ERROR: cannot identify the location where GCC "
                              "was run using --object-directory=%s\n" %
                              options.objdir)

    # no objdir was specified (or it was a parent dir); walk up the dir tree
    if len(potential_wd) == 0:
        potential_wd.append(root_dir)
        #print "X - potential_wd", root_dir
        wd = os.path.split(abs_filename)[0]
        while True:
            potential_wd.append(wd)
            wd = os.path.split(wd)[0]
            if wd == potential_wd[-1]:
                #
                # Stop at the root of the file system
                #
                break

    #
    # If the first element of cmd - the executable name - has embedded spaces it probably
    # includes extra arguments.
    #
    cmd = options.gcov_cmd.split(' ') + [
        abs_filename,
        "--branch-counts", "--branch-probabilities", "--preserve-paths",
        '--object-directory', dirname
    ]

    # NB: Currently, we will only parse English output
    env = dict(os.environ)
    env['LC_ALL'] = 'en_US'

    #print "HERE", potential_wd
    while len(potential_wd) > 0 and not Done:
        # NB: either len(potential_wd) == 1, or all entires are absolute
        # paths, so we don't have to chdir(starting_dir) at every
        # iteration.

        #
        # Iterate from the end of the potential_wd list, which is the root
        # directory
        #
        dir_ = potential_wd.pop(0)
        #print "X DIR:", dir_
        os.chdir(dir_)

        if options.verbose:
            sys.stdout.write(
                "Running gcov: '%s' in '%s'\n" % (' '.join(cmd), os.getcwd())
            )
        out, err = subprocess.Popen(
            cmd, env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE).communicate()
        out = out.decode('utf-8')
        err = err.decode('utf-8')

        # find the files that gcov created
        gcov_files = {'active' : [], 'filter' : [], 'exclude' : []}
        for line in out.splitlines():
            found = output_re.search(line.strip())
            if found is not None:
                fname = found.group(1)
                if not options.gcov_filter.match(fname):
                    if options.verbose:
                        sys.stdout.write("Filtering gcov file %s\n" % fname)
                    gcov_files['filter'].append(fname)
                    continue
                exclude = False
                for exc in options.gcov_exclude:
                    if exc.match(options.gcov_filter.sub('', fname)) or \
                            exc.match(fname) or \
                            exc.match(os.path.abspath(fname)):
                        exclude = True
                        break
                if not exclude:
                    gcov_files['active'].append(fname)
                elif options.verbose:
                    sys.stdout.write("Excluding gcov file %s\n" % fname)
                    gcov_files['exclude'].append(fname)

        #print "HERE", err, "XXX", source_re.search(err)
        if source_re.search(err):
            #
            # gcov tossed errors: try the next potential_wd
            #
            errors.append(err)
        else:
            #
            # Process *.gcov files
            #
            for fname in gcov_files['active']:
                process_gcov_data(fname, covdata, abs_filename, options)
            Done = True

        if not options.keep:
            for group in gcov_files.values():
                for fname in group:
                    if os.path.exists(fname):
                        # Only remove files that actually exist.
                        os.remove(fname)

    os.chdir(starting_dir)
    if options.delete:
        if not abs_filename.endswith('gcno'):
            os.remove(abs_filename)

    if not Done:
        sys.stderr.write(
            "(WARNING) GCOV produced the following errors processing %s:\n"
            "\t   %s"
            "\t(gcovr could not infer a working directory that resolved it.)\n"
            % (filename, "\t   ".join(errors))
        )


#
#  Process Already existing gcov files
#
def process_existing_gcov_file(filename, covdata, options):
    #
    # Ignore this file if it does not match the gcov filter
    #
    if not options.gcov_filter.match(filename):
        if options.verbose:
            sys.stdout.write("This gcov file does not match the filter: %s\n" % filename)
        return
    #
    # Ignore this file if it matches one of the exclusion regex's
    #
    for exc in options.gcov_exclude:
        if exc.match(options.gcov_filter.sub('', filename)) or \
                exc.match(filename) or \
                exc.match(os.path.abspath(filename)):
            if options.verbose:
                sys.stdout.write("Excluding gcov file: %s\n" % filename)
            return
    #
    # Process the gcov data file
    #
    process_gcov_data(filename, covdata, None, options)
    #
    # Remove the file unless the user has indicated that we keep gcov data files
    #
    if not options.keep:
        #
        # Only remove files that actually exist.
        #
        if os.path.exists(filename):
            os.remove(filename)

#
# Produce the classic gcovr text report
#
def print_text_report(covdata):
    def _num_uncovered(key):
        (total, covered, percent) = covdata[key].coverage()
        return total - covered

    def _percent_uncovered(key):
        (total, covered, percent) = covdata[key].coverage()
        if covered:
            return -1.0 * covered / total
        else:
            return total or 1e6

    def _alpha(key):
        return key

    if options.output:
        OUTPUT = open(options.output, 'w')
    else:
        OUTPUT = sys.stdout
    total_lines = 0
    total_covered = 0

    # Header
    OUTPUT.write("## Automated Test Code Coverage Report\n")
    OUTPUT.write("\n<details><summary>View details... </summary>\n<p>\n\n")

    a = options.show_branch and "Branches" or "Lines"
    b = options.show_branch and "Taken" or "Exec"
    c = ""
    OUTPUT.write(
        "File | " + a + " | " + b + " | Cover   " + c + "\n"
    )
    OUTPUT.write("-----|---------|--------|--------\n")

    # Data
    keys = list(covdata.keys())
    keys.sort(
        key=options.sort_uncovered and _num_uncovered or
        options.sort_percent and _percent_uncovered or _alpha
    )
    for key in keys:
        (t, n, txt) = covdata[key].summary()
        total_lines += t
        total_covered += n
        OUTPUT.write(txt + '\n')

    # Footer & summary
    percent = total_lines and str(int(100.0 * total_covered / total_lines)) \
        or "--"
    OUTPUT.write(
        "**TOTAL** | **" + str(total_lines) + "** | **" + str(total_covered) + "** | **" + str(percent) + "%**" + '\n'
    )

    OUTPUT.write("\n</details>\n\n")

    OUTPUT.write(
        "**TOTAL: " + str(total_lines) + " lines of code, " + str(total_covered) + " lines executed, " + str(percent) + "% covered.**" + '\n'
    )


    # Close logfile
    if options.output:
        OUTPUT.close()


#
# Prints a small report to the standard output
#
def print_summary(covdata):
    lines_total = 0
    lines_covered = 0
    branches_total = 0
    branches_covered = 0

    keys = list(covdata.keys())

    for key in keys:
        options.show_branch = False
        (t, n, txt) = covdata[key].coverage()
        lines_total += t
        lines_covered += n

        options.show_branch = True
        (t, n, txt) = covdata[key].coverage()
        branches_total += t
        branches_covered += n

    percent = lines_total and (100.0 * lines_covered / lines_total)
    percent_branches = branches_total and \
        (100.0 * branches_covered / branches_total)

    lines_out = "lines: %0.1f%% (%s out of %s)\n" % (
        percent, lines_covered, lines_total
    )
    branches_out = "branches: %0.1f%% (%s out of %s)\n" % (
        percent_branches, branches_covered, branches_total
    )

    sys.stdout.write(lines_out)
    sys.stdout.write(branches_out)

#
# CSS declarations for the HTML output
#
css = Template('''
    body
    {
      color: #000000;
      background-color: #FFFFFF;
    }

    /* Link formats: use maroon w/underlines */
    a:link
    {
      color: navy;
      text-decoration: underline;
    }
    a:visited
    {
      color: maroon;
      text-decoration: underline;
    }
    a:active
    {
      color: navy;
      text-decoration: underline;
    }

    /*** TD formats ***/
    td
    {
      font-family: sans-serif;
    }
    td.title
    {
      text-align: center;
      padding-bottom: 10px;
      font-size: 20pt;
      font-weight: bold;
    }

    /* TD Header Information */
    td.headerName
    {
      text-align: right;
      color: black;
      padding-right: 6px;
      font-weight: bold;
      vertical-align: top;
      white-space: nowrap;
    }
    td.headerValue
    {
      text-align: left;
      color: blue;
      font-weight: bold;
      white-space: nowrap;
    }
    td.headerTableEntry
    {
      text-align: right;
      color: black;
      font-weight: bold;
      white-space: nowrap;
      padding-left: 12px;
      padding-right: 4px;
      background-color: LightBlue;
    }
    td.headerValueLeg
    {
      text-align: left;
      color: black;
      font-size: 80%;
      white-space: nowrap;
      padding-left: 10px;
      padding-right: 10px;
      padding-top: 2px;
    }

    /* Color of horizontal ruler */
    td.hr
    {
      background-color: navy;
      height:3px;
    }
    /* Footer format */
    td.footer
    {
      text-align: center;
      padding-top: 3px;
      font-family: sans-serif;
    }

    /* Coverage Table */

    td.coverTableHead
    {
      text-align: center;
      color: white;
      background-color: SteelBlue;
      font-family: sans-serif;
      font-size: 120%;
      white-space: nowrap;
      padding-left: 4px;
      padding-right: 4px;
    }
    td.coverFile
    {
      text-align: left;
      padding-left: 10px;
      padding-right: 20px;
      color: black;
      background-color: LightBlue;
      font-family: monospace;
      font-weight: bold;
      font-size: 110%;
    }
    td.coverBar
    {
      padding-left: 10px;
      padding-right: 10px;
      background-color: LightBlue;
    }
    td.coverBarOutline
    {
      background-color: white;
    }
    td.coverValue
    {
      padding-top: 2px;
      text-align: right;
      padding-left: 10px;
      padding-right: 10px;
      font-family: sans-serif;
      white-space: nowrap;
      font-weight: bold;
    }

    /* Link Details */
    a.detail:link
    {
      color: #B8D0FF;
      font-size:80%;
    }
    a.detail:visited
    {
      color: #B8D0FF;
      font-size:80%;
    }
    a.detail:active
    {
      color: #FFFFFF;
      font-size:80%;
    }

    .graphcont{
        color:#000;
        font-weight:700;
        float:left
    }

    .graph{
        float:left;
        background-color: white;
        position:relative;
        width:280px;
        padding:0
    }

    .graph .bar{
        display:block;
        position:relative;
        border:black 1px solid;
        text-align:center;
        color:#fff;
        height:10px;
        font-family:Arial,Helvetica,sans-serif;
        font-size:12px;
        line-height:1.9em
    }

    .graph .bar span{
        position:absolute;
        left:1em
    }

    td.coveredLine,
    span.coveredLine
    {
        background-color: ${covered_color}!important;
    }

    td.uncoveredLine,
    span.uncoveredLine
    {
        background-color: ${uncovered_color}!important;
    }

    .linebranch, .linecount
    {
        border-right: 1px gray solid;
        background-color: lightgray;
    }

    span.takenBranch
    {
        color: ${takenBranch_color}!important;
        cursor: help;
    }

    span.notTakenBranch
    {
        color: ${notTakenBranch_color}!important;
        cursor: help;
    }

    .src
    {
        padding-left: 12px;
    }

    .srcHeader,
    span.takenBranch,
    span.notTakenBranch
    {
        font-family: monospace;
        font-weight: bold;
    }

    pre
    {
        height : 15px;
        margin-top: 0;
        margin-bottom: 0;
    }

    .lineno
    {
        background-color: #EFE383;
        border-right: 1px solid #BBB15F;
    }
''')

#
# A string template for the root HTML output
#
root_page = Template('''
<html>

<head>
  <meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/>
  <title>${HEAD}</title>
  <style media="screen" type="text/css">
  ${CSS}
  </style>
</head>

<body>

  <table width="100%" border=0 cellspacing=0 cellpadding=0>
    <tr><td class="title">GCC Code Coverage Report</td></tr>
    <tr><td class="hr"></td></tr>

    <tr>
      <td width="100%">
        <table cellpadding=1 border=0 width="100%">
          <tr>
            <td width="10%" class="headerName">Directory:</td>
            <td width="35%" class="headerValue">${DIRECTORY}</td>
            <td width="5%"></td>
            <td width="15%"></td>
            <td width="10%" class="headerValue" style="text-align:right;">Exec</td>
            <td width="10%" class="headerValue" style="text-align:right;">Total</td>
            <td width="15%" class="headerValue" style="text-align:right;">Coverage</td>
          </tr>
          <tr>
            <td class="headerName">Date:</td>
            <td class="headerValue">${DATE}</td>
            <td></td>
            <td class="headerName">Lines:</td>
            <td class="headerTableEntry">${LINES_EXEC}</td>
            <td class="headerTableEntry">${LINES_TOTAL}</td>
            <td class="headerTableEntry" style="background-color:${LINES_COLOR}">${LINES_COVERAGE} %</td>
          </tr>
          <tr>
            <td class="headerName">Legend:</td>
            <td class="headerValueLeg">
              <span style="background-color:${low_color}">low: &lt; ${COVERAGE_MED} %</span>
              <span style="background-color:${medium_color}">medium: &gt;= ${COVERAGE_MED} %</span>
              <span style="background-color:${high_color}">high: &gt;= ${COVERAGE_HIGH} %</span>
            </td>
            <td></td>
            <td class="headerName">Branches:</td>
            <td class="headerTableEntry">${BRANCHES_EXEC}</td>
            <td class="headerTableEntry">${BRANCHES_TOTAL}</td>
            <td class="headerTableEntry" style="background-color:${BRANCHES_COLOR}">${BRANCHES_COVERAGE} %</td>
          </tr>
        </table>
      </td>
    </tr>

    <tr><td class="hr"></td></tr>
  </table>

  <center>
  <table width="80%" cellpadding=1 cellspacing=1 border=0>
    <tr>
      <td width="44%"><br></td>
      <td width="8%"></td>
      <td width="8%"></td>
      <td width="8%"></td>
      <td width="8%"></td>
      <td width="8%"></td>
    </tr>
    <tr>
      <td class="coverTableHead">File</td>
      <td class="coverTableHead" colspan=3>Lines</td>
      <td class="coverTableHead" colspan=2>Branches</td>
    </tr>

    ${ROWS}

    <tr>
      <td width="44%"><br></td>
      <td width="8%"></td>
      <td width="8%"></td>
      <td width="8%"></td>
      <td width="8%"></td>
      <td width="8%"></td>
    </tr>
  </table>
  </center>

  <table width="100%" border=0 cellspacing=0 cellpadding=0>
    <tr><td class="hr"><td></tr>
    <tr><td class="footer">Generated by: <a href="http://gcovr.com">GCOVR (Version ${VERSION})</a></td></tr>
  </table>
  <br>

</body>

</html>
''')

#
# A string template for the source file HTML output
#
source_page = Template('''
<html>

<head>
  <meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/>
  <title>${HEAD}</title>
  <style media="screen" type="text/css">
  ${CSS}
  </style>
</head>

<body>

  <table width="100%" border="0" cellspacing="0" cellpadding="0">
    <tr><td class="title">GCC Code Coverage Report</td></tr>
    <tr><td class="hr"></td></tr>

    <tr>
      <td width="100%">
        <table cellpadding="1" border="0" width="100%">
          <tr>
            <td width="10%" class="headerName">Directory:</td>
            <td width="35%" class="headerValue">${DIRECTORY}</td>
            <td width="5%"></td>
            <td width="15%"></td>
            <td width="10%" class="headerValue" style="text-align:right;">Exec</td>
            <td width="10%" class="headerValue" style="text-align:right;">Total</td>
            <td width="15%" class="headerValue" style="text-align:right;">Coverage</td>
          </tr>
          <tr>
            <td class="headerName">File:</td>
            <td class="headerValue">${FILENAME}</td>
            <td></td>
            <td class="headerName">Lines:</td>
            <td class="headerTableEntry">${LINES_EXEC}</td>
            <td class="headerTableEntry">${LINES_TOTAL}</td>
            <td class="headerTableEntry" style="background-color:${LINES_COLOR}">${LINES_COVERAGE} %</td>
          </tr>
          <tr>
            <td class="headerName">Date:</td>
            <td class="headerValue">${DATE}</td>
            <td></td>
            <td class="headerName">Branches:</td>
            <td class="headerTableEntry">${BRANCHES_EXEC}</td>
            <td class="headerTableEntry">${BRANCHES_TOTAL}</td>
            <td class="headerTableEntry" style="background-color:${BRANCHES_COLOR}">${BRANCHES_COVERAGE} %</td>
          </tr>
        </table>
      </td>
    </tr>

    <tr><td class="hr"></td></tr>
  </table>

  <br>
  <table cellspacing="0" cellpadding="1">
    <tr>
      <td width="5%" align="right" class="srcHeader">Line</td>
      <td width="5%" align="right" class="srcHeader">Branch</td>
      <td width="5%" align="right" class="srcHeader">Exec</td>
      <td width="75%" align="left" class="srcHeader src">Source</td>
    </tr>

    ${ROWS}

  </table>
  <br>

  <table width="100%" border="0" cellspacing="0" cellpadding="0">
    <tr><td class="hr"><td></tr>
    <tr><td class="footer">Generated by: <a href="http://gcovr.com">GCOVR (Version ${VERSION})</a></td></tr>
  </table>
  <br>

</body>

</html>
''')


def calculate_coverage(covered, total):
    return 0.0 if total == 0 else round(100.0 * covered / total, 1)


def coverage_to_color(coverage):
    if coverage < medium_coverage:
        return low_color
    elif coverage < high_coverage:
        return medium_color
    else:
        return high_color


#
# Produce an HTML report
#
def print_html_report(covdata, details):
    def _num_uncovered(key):
        (total, covered, percent) = covdata[key].coverage()
        return total - covered

    def _percent_uncovered(key):
        (total, covered, percent) = covdata[key].coverage()
        if covered:
            return -1.0 * covered / total
        else:
            return total or 1e6

    def _alpha(key):
        return key

    if options.output is None:
        details = False
    data = {}
    data['HEAD'] = "Head"
    data['VERSION'] = version_str()
    data['TIME'] = str(int(time.time()))
    data['DATE'] = datetime.date.today().isoformat()
    data['ROWS'] = []
    data['low_color'] = low_color
    data['medium_color'] = medium_color
    data['high_color'] = high_color
    data['COVERAGE_MED'] = medium_coverage
    data['COVERAGE_HIGH'] = high_coverage
    data['CSS'] = css.substitute(low_color=low_color, medium_color=medium_color, high_color=high_color,
                                 covered_color=covered_color, uncovered_color=uncovered_color,
                                 takenBranch_color=takenBranch_color, notTakenBranch_color=notTakenBranch_color
    )
    data['DIRECTORY'] = ''

    branchTotal = 0
    branchCovered = 0
    options.show_branch = True
    for key in covdata.keys():
        (total, covered, percent) = covdata[key].coverage()
        branchTotal += total
        branchCovered += covered
    data['BRANCHES_EXEC'] = str(branchCovered)
    data['BRANCHES_TOTAL'] = str(branchTotal)
    coverage = calculate_coverage(branchCovered, branchTotal)
    data['BRANCHES_COVERAGE'] = str(coverage)
    data['BRANCHES_COLOR'] = coverage_to_color(coverage)

    lineTotal = 0
    lineCovered = 0
    options.show_branch = False
    for key in covdata.keys():
        (total, covered, percent) = covdata[key].coverage()
        lineTotal += total
        lineCovered += covered
    data['LINES_EXEC'] = str(lineCovered)
    data['LINES_TOTAL'] = str(lineTotal)
    coverage = calculate_coverage(lineCovered, lineTotal)
    data['LINES_COVERAGE'] = str(coverage)
    data['LINES_COLOR'] = coverage_to_color(coverage)

    # Generate the coverage output (on a per-package basis)
    #source_dirs = set()
    files = []
    dirs = []
    filtered_fname = ''
    keys = list(covdata.keys())
    keys.sort(
        key=options.sort_uncovered and _num_uncovered or
        options.sort_percent and _percent_uncovered or _alpha
    )
    for f in keys:
        cdata = covdata[f]
        filtered_fname = options.root_filter.sub('', f)
        files.append(filtered_fname)
        dirs.append(os.path.dirname(filtered_fname) + os.sep)
        cdata._filename = filtered_fname
        ttmp = os.path.abspath(options.output).split('.')
        longname = cdata._filename.replace(os.sep, '_')
        longname_hash = ""
        while True:
            if len(ttmp) > 1:
                cdata._sourcefile = \
                    '.'.join(ttmp[:-1]) + \
                    '.' + longname + longname_hash + \
                    '.' + ttmp[-1]
            else:
                cdata._sourcefile = \
                    ttmp[0] + '.' + longname + longname_hash + '.html'
            # we add a hash at the end and attempt to shorten the
            # filename if it exceeds common filesystem limitations
            if len(os.path.basename(cdata._sourcefile)) < 256:
                break
            longname_hash = "_" + hex(zlib.crc32(longname) & 0xffffffff)[2:]
            longname = longname[(len(cdata._sourcefile) - len(longname_hash)):]

    # Define the common root directory, which may differ from options.root
    # when source files share a common prefix.
    if len(files) > 1:
        commondir = commonpath(files)
        if commondir != '':
            data['DIRECTORY'] = commondir
    else:
        dir_, file_ = os.path.split(filtered_fname)
        if dir_ != '':
            data['DIRECTORY'] = dir_ + os.sep

    for f in keys:
        cdata = covdata[f]
        class_lines = 0
        class_hits = 0
        class_branches = 0
        class_branch_hits = 0
        for line in sorted(cdata.all_lines):
            hits = cdata.covered.get(line, 0)
            class_lines += 1
            if hits > 0:
                class_hits += 1
            branches = cdata.branches.get(line)
            if branches is None:
                pass
            else:
                b_hits = 0
                for v in branches.values():
                    if v > 0:
                        b_hits += 1
                coverage = 100 * b_hits / len(branches)
                class_branch_hits += b_hits
                class_branches += len(branches)

        lines_covered = 100.0 if class_lines == 0 else \
            100.0 * class_hits / class_lines
        branches_covered = 100.0 if class_branches == 0 else \
            100.0 * class_branch_hits / class_branches

        data['ROWS'].append(html_row(
            details, cdata._sourcefile,
            directory=data['DIRECTORY'],
            filename=os.path.relpath( os.path.realpath( cdata._filename ), data['DIRECTORY'] ),
            LinesExec=class_hits,
            LinesTotal=class_lines,
            LinesCoverage=lines_covered,
            BranchesExec=class_branch_hits,
            BranchesTotal=class_branches,
            BranchesCoverage=branches_covered
        ))
    data['ROWS'] = '\n'.join(data['ROWS'])

    if data['DIRECTORY'] == '':
        data['DIRECTORY'] = "."

    htmlString = root_page.substitute(**data)

    if options.output is None:
        sys.stdout.write(htmlString + '\n')
    else:
        OUTPUT = open(options.output, 'w')
        OUTPUT.write(htmlString + '\n')
        OUTPUT.close()

    # Return, if no details are requested
    if not details:
        return

    #
    # Generate an HTML file for every source file
    #
    for f in keys:
        cdata = covdata[f]

        data['FILENAME'] = cdata._filename
        data['ROWS'] = ''

        options.show_branch = True
        branchTotal, branchCovered, tmp = cdata.coverage()
        data['BRANCHES_EXEC'] = str(branchCovered)
        data['BRANCHES_TOTAL'] = str(branchTotal)
        coverage = calculate_coverage(branchCovered, branchTotal)
        data['BRANCHES_COVERAGE'] = str(coverage)
        data['BRANCHES_COLOR'] = coverage_to_color(coverage)

        options.show_branch = False
        lineTotal, lineCovered, tmp = cdata.coverage()
        data['LINES_EXEC'] = str(lineCovered)
        data['LINES_TOTAL'] = str(lineTotal)
        coverage = calculate_coverage(lineCovered, lineTotal)
        data['LINES_COVERAGE'] = str(coverage)
        data['LINES_COLOR'] = coverage_to_color(coverage)

        data['ROWS'] = []
        currdir = os.getcwd()
        os.chdir(root_dir)
        INPUT = open(data['FILENAME'], 'r')
        ctr = 1
        for line in INPUT:
            data['ROWS'].append(
                source_row(ctr, line.rstrip(), cdata)
            )
            ctr += 1
        INPUT.close()
        os.chdir(currdir)
        data['ROWS'] = '\n'.join(data['ROWS'])

        htmlString = source_page.substitute(**data)
        OUTPUT = open(cdata._sourcefile, 'w')
        OUTPUT.write(htmlString + '\n')
        OUTPUT.close()


def source_row(lineno, source, cdata):
    rowstr = Template('''
    <tr>
    <td align="right" class="lineno"><pre>${lineno}</pre></td>
    <td align="right" class="linebranch">${linebranch}</td>
    <td align="right" class="linecount ${covclass}"><pre>${linecount}</pre></td>
    <td align="left" class="src ${covclass}"><pre>${source}</pre></td>
    </tr>''')
    kwargs = {}
    kwargs['lineno'] = str(lineno)
    if lineno in cdata.covered:
        kwargs['covclass'] = 'coveredLine'
        kwargs['linebranch'] = ''
        # If line has branches them show them with ticks or crosses
        if lineno in cdata.branches.keys():
            branches = cdata.branches.get(lineno)
            branchcounter = 0
            for branch in branches:
                if branches[branch] > 0:
                    kwargs['linebranch'] += '<span class="takenBranch" title="Branch ' + str(branch) + ' taken ' + str(branches[branch]) + ' times">&check;</span>'
                else:
                    kwargs['linebranch'] += '<span class="notTakenBranch" title="Branch ' + str(branch) + ' not taken">&cross;</span>'
                branchcounter += 1
                # Wrap at 4 branches to avoid too wide column
                if (branchcounter > 0) and ((branchcounter % 4) == 0):
                    kwargs['linebranch'] += '<br/>'
        kwargs['linecount'] = str(cdata.covered.get(lineno, 0))
    elif lineno in cdata.uncovered:
        kwargs['covclass'] = 'uncoveredLine'
        kwargs['linebranch'] = ''
        kwargs['linecount'] = ''
    else:
        kwargs['covclass'] = ''
        kwargs['linebranch'] = ''
        kwargs['linecount'] = ''
    kwargs['source'] = html.escape(source)
    return rowstr.substitute(**kwargs)

#
# Generate the table row for a single file
#
nrows = 0


def html_row(details, sourcefile, **kwargs):
    if options.relative_anchors:
        sourcefile = os.path.basename(sourcefile)
    rowstr = Template('''
    <tr>
      <td class="coverFile" ${altstyle}>${filename}</td>
      <td class="coverBar" align="center" ${altstyle}>
        <table border=0 cellspacing=0 cellpadding=1><tr><td class="coverBarOutline">
                <div class="graph"><strong class="bar" style="width:${LinesCoverage}%; ${BarBorder}background-color:${LinesBar}"></strong></div>
                </td></tr></table>
      </td>
      <td class="CoverValue" style="font-weight:bold; background-color:${LinesColor};">${LinesCoverage}&nbsp;%</td>
      <td class="CoverValue" style="font-weight:bold; background-color:${LinesColor};">${LinesExec} / ${LinesTotal}</td>
      <td class="CoverValue" style="background-color:${BranchesColor};">${BranchesCoverage}&nbsp;%</td>
      <td class="CoverValue" style="background-color:${BranchesColor};">${BranchesExec} / ${BranchesTotal}</td>
    </tr>
''')
    global nrows
    nrows += 1
    if nrows % 2 == 0:
        kwargs['altstyle'] = 'style="background-color:LightSteelBlue"'
    else:
        kwargs['altstyle'] = ''
    if details:
        kwargs['filename'] = '<a href="%s">%s</a>' % (
            sourcefile, kwargs['filename']
        )
    kwargs['LinesCoverage'] = round(kwargs['LinesCoverage'], 1)
    # Disable the border if the bar is too short to see the color
    if kwargs['LinesCoverage'] < 1e-7:
        kwargs['BarBorder'] = "border:white; "
    else:
        kwargs['BarBorder'] = ""
    if kwargs['LinesCoverage'] < medium_coverage:
        kwargs['LinesColor'] = low_color
        kwargs['LinesBar'] = 'red'
    elif kwargs['LinesCoverage'] < high_coverage:
        kwargs['LinesColor'] = medium_color
        kwargs['LinesBar'] = 'yellow'
    else:
        kwargs['LinesColor'] = high_color
        kwargs['LinesBar'] = 'green'

    kwargs['BranchesCoverage'] = round(kwargs['BranchesCoverage'], 1)
    if kwargs['BranchesCoverage'] < medium_coverage:
        kwargs['BranchesColor'] = low_color
        kwargs['BranchesBar'] = 'red'
    elif kwargs['BranchesCoverage'] < high_coverage:
        kwargs['BranchesColor'] = medium_color
        kwargs['BranchesBar'] = 'yellow'
    else:
        kwargs['BranchesColor'] = high_color
        kwargs['BranchesBar'] = 'green'

    return rowstr.substitute(**kwargs)


#
# Produce an XML report in the Cobertura format
#
def print_xml_report(covdata):
    branchTotal = 0
    branchCovered = 0
    lineTotal = 0
    lineCovered = 0

    options.show_branch = True
    for key in covdata.keys():
        (total, covered, percent) = covdata[key].coverage()
        branchTotal += total
        branchCovered += covered

    options.show_branch = False
    for key in covdata.keys():
        (total, covered, percent) = covdata[key].coverage()
        lineTotal += total
        lineCovered += covered

    impl = xml.dom.minidom.getDOMImplementation()
    docType = impl.createDocumentType(
        "coverage", None,
        "http://cobertura.sourceforge.net/xml/coverage-03.dtd"
    )
    doc = impl.createDocument(None, "coverage", docType)
    root = doc.documentElement
    root.setAttribute(
        "line-rate", lineTotal == 0 and '0.0' or
        str(float(lineCovered) / lineTotal)
    )
    root.setAttribute(
        "branch-rate", branchTotal == 0 and '0.0' or
        str(float(branchCovered) / branchTotal)
    )
    root.setAttribute(
        "timestamp", str(int(time.time()))
    )
    root.setAttribute(
        "version", "gcovr %s" % (version_str(),)
    )

    # Generate the <sources> element: this is either the root directory
    # (specified by --root), or the CWD.
    sources = doc.createElement("sources")
    root.appendChild(sources)

    # Generate the coverage output (on a per-package basis)
    packageXml = doc.createElement("packages")
    root.appendChild(packageXml)
    packages = {}
    source_dirs = set()

    keys = list(covdata.keys())
    keys.sort()
    for f in keys:
        data = covdata[f]
        directory = options.root_filter.sub('', f)
        if f.endswith(directory):
            src_path = f[:-1 * len(directory)]
            if len(src_path) > 0:
                while directory.startswith(os.path.sep):
                    src_path += os.path.sep
                    directory = directory[len(os.path.sep):]
                source_dirs.add(src_path)
        else:
            # Do no truncation if the filter does not start matching at
            # the beginning of the string
            directory = f
        directory, fname = os.path.split(directory)

        package = packages.setdefault(
            directory, [doc.createElement("package"), {}, 0, 0, 0, 0]
        )
        c = doc.createElement("class")
        # The Cobertura DTD requires a methods section, which isn't
        # trivial to get from gcov (so we will leave it blank)
        c.appendChild(doc.createElement("methods"))
        lines = doc.createElement("lines")
        c.appendChild(lines)

        class_lines = 0
        class_hits = 0
        class_branches = 0
        class_branch_hits = 0
        for line in sorted(data.all_lines):
            hits = data.covered.get(line, 0)
            class_lines += 1
            if hits > 0:
                class_hits += 1
            l = doc.createElement("line")
            l.setAttribute("number", str(line))
            l.setAttribute("hits", str(hits))
            branches = data.branches.get(line)
            if branches is None:
                l.setAttribute("branch", "false")
            else:
                b_hits = 0
                for v in branches.values():
                    if v > 0:
                        b_hits += 1
                coverage = 100 * b_hits / len(branches)
                l.setAttribute("branch", "true")
                l.setAttribute(
                    "condition-coverage",
                    "%i%% (%i/%i)" % (coverage, b_hits, len(branches))
                )
                cond = doc.createElement('condition')
                cond.setAttribute("number", "0")
                cond.setAttribute("type", "jump")
                cond.setAttribute("coverage", "%i%%" % (coverage))
                class_branch_hits += b_hits
                class_branches += float(len(branches))
                conditions = doc.createElement("conditions")
                conditions.appendChild(cond)
                l.appendChild(conditions)

            lines.appendChild(l)

        className = fname.replace('.', '_')
        c.setAttribute("name", className)
        c.setAttribute("filename", os.path.join(directory, fname))
        c.setAttribute(
            "line-rate",
            str(class_hits / (1.0 * class_lines or 1.0))
        )
        c.setAttribute(
            "branch-rate",
            str(class_branch_hits / (1.0 * class_branches or 1.0))
        )
        c.setAttribute("complexity", "0.0")

        package[1][className] = c
        package[2] += class_hits
        package[3] += class_lines
        package[4] += class_branch_hits
        package[5] += class_branches

    keys = list(packages.keys())
    keys.sort()
    for packageName in keys:
        packageData = packages[packageName]
        package = packageData[0]
        packageXml.appendChild(package)
        classes = doc.createElement("classes")
        package.appendChild(classes)
        classNames = list(packageData[1].keys())
        classNames.sort()
        for className in classNames:
            classes.appendChild(packageData[1][className])
        package.setAttribute("name", packageName.replace(os.sep, '.'))
        package.setAttribute(
            "line-rate", str(packageData[2] / (1.0 * packageData[3] or 1.0))
        )
        package.setAttribute(
            "branch-rate", str(packageData[4] / (1.0 * packageData[5] or 1.0))
        )
        package.setAttribute("complexity", "0.0")

    # Populate the <sources> element: this is either the root directory
    # (specified by --root), or relative directories based
    # on the filter, or the CWD
    if options.root is not None:
        source = doc.createElement("source")
        source.appendChild(doc.createTextNode(options.root.strip()))
        sources.appendChild(source)
    elif len(source_dirs) > 0:
        cwd = os.getcwd()
        for d in source_dirs:
            source = doc.createElement("source")
            if d.startswith(cwd):
                reldir = d[len(cwd):].lstrip(os.path.sep)
            elif cwd.startswith(d):
                i = 1
                while normpath(d) != \
                        normpath(os.path.join(*tuple([cwd] + ['..'] * i))):
                    i += 1
                reldir = os.path.join(*tuple(['..'] * i))
            else:
                reldir = d
            source.appendChild(doc.createTextNode(reldir.strip()))
            sources.appendChild(source)
    else:
        source = doc.createElement("source")
        source.appendChild(doc.createTextNode('.'))
        sources.appendChild(source)

    if options.prettyxml:
        import textwrap
        lines = doc.toprettyxml(" ").split('\n')
        for i in xrange(len(lines)):
            n = 0
            while n < len(lines[i]) and lines[i][n] == " ":
                n += 1
            lines[i] = "\n".join(textwrap.wrap(
                lines[i], 78,
                break_long_words=False,
                break_on_hyphens=False,
                subsequent_indent=" " + n * " "
            ))
        xmlString = "\n".join(lines)
        #print textwrap.wrap(doc.toprettyxml(" "), 80)
    else:
        xmlString = doc.toprettyxml(indent="")
    if options.output is None:
        sys.stdout.write(xmlString + '\n')
    else:
        OUTPUT = open(options.output, 'w')
        OUTPUT.write(xmlString + '\n')
        OUTPUT.close()


##
## MAIN
##

#
# Create option parser
#
parser = OptionParser()
parser.add_option(
    "--version",
    help="Print the version number, then exit",
    action="store_true",
    dest="version",
    default=False
)
parser.add_option(
    "-v", "--verbose",
    help="Print progress messages",
    action="store_true",
    dest="verbose",
    default=False
)
parser.add_option(
    '--object-directory',
    help="Specify the directory that contains the gcov data files.  gcovr "
         "must be able to identify the path between the *.gcda files and the "
         "directory where gcc was originally run.  Normally, gcovr can guess "
         "correctly.  This option overrides gcovr's normal path detection and "
         "can specify either the path from gcc to the gcda file (i.e. what "
         "was passed to gcc's '-o' option), or the path from the gcda file to "
         "gcc's original working directory.",
    action="store",
    dest="objdir",
    default=None
)
parser.add_option(
    "-o", "--output",
    help="Print output to this filename",
    action="store",
    dest="output",
    default=None
)
parser.add_option(
    "-k", "--keep",
    help="Keep the temporary *.gcov files generated by gcov.  "
         "By default, these are deleted.",
    action="store_true",
    dest="keep",
    default=False
)
parser.add_option(
    "-d", "--delete",
    help="Delete the coverage files after they are processed.  "
         "These are generated by the users's program, and by default gcovr "
         "does not remove these files.",
    action="store_true",
    dest="delete",
    default=False
)
parser.add_option(
    "-f", "--filter",
    help="Keep only the data files that match this regular expression",
    action="append",
    dest="filter",
    default=[]
)
parser.add_option(
    "-e", "--exclude",
    help="Exclude data files that match this regular expression",
    action="append",
    dest="exclude",
    default=[]
)
parser.add_option(
    "--gcov-filter",
    help="Keep only gcov data files that match this regular expression",
    action="store",
    dest="gcov_filter",
    default=None
)
parser.add_option(
    "--gcov-exclude",
    help="Exclude gcov data files that match this regular expression",
    action="append",
    dest="gcov_exclude",
    default=[]
)
parser.add_option(
    "-r", "--root",
    help="Defines the root directory for source files.  "
         "This is also used to filter the files, and to standardize "
         "the output.",
    action="store",
    dest="root",
    default=None
)
parser.add_option(
    "-x", "--xml",
    help="Generate XML instead of the normal tabular output.",
    action="store_true",
    dest="xml",
    default=False
)
parser.add_option(
    "--xml-pretty",
    help="Generate pretty XML instead of the normal dense format.",
    action="store_true",
    dest="prettyxml",
    default=False
)
parser.add_option(
    "--html",
    help="Generate HTML instead of the normal tabular output.",
    action="store_true",
    dest="html",
    default=False
)
parser.add_option(
    "--html-details",
    help="Generate HTML output for source file coverage.",
    action="store_true",
    dest="html_details",
    default=False
)
parser.add_option(
    "--html-absolute-paths",
    help="Set the paths in the HTML report to be absolute instead of relative",
    action="store_false",
    dest="relative_anchors",
    default=True
)
parser.add_option(
    "-b", "--branches",
    help="Tabulate the branch coverage instead of the line coverage.",
    action="store_true",
    dest="show_branch",
    default=None
)
parser.add_option(
    "-u", "--sort-uncovered",
    help="Sort entries by increasing number of uncovered lines.",
    action="store_true",
    dest="sort_uncovered",
    default=None
)
parser.add_option(
    "-p", "--sort-percentage",
    help="Sort entries by decreasing percentage of covered lines.",
    action="store_true",
    dest="sort_percent",
    default=None
)
parser.add_option(
    "--gcov-executable",
    help="Defines the name/path to the gcov executable [defaults to the "
         "GCOV environment variable, if present; else 'gcov'].",
    action="store",
    dest="gcov_cmd",
    default=os.environ.get('GCOV', 'gcov')
)
parser.add_option(
    "--exclude-unreachable-branches",
    help="Exclude from coverage branches which are marked to be excluded by "
         "LCOV/GCOV markers or are determined to be from lines containing "
         "only compiler-generated \"dead\" code.",
    action="store_true",
    dest="exclude_unreachable_branches",
    default=False
)
parser.add_option(
    "--exclude-directories",
    help="Exclude directories from search path that match this regular expression",
    action="append",
    dest="exclude_dirs",
    default=[]
)
parser.add_option(
    "-g", "--use-gcov-files",
    help="Use preprocessed gcov files for analysis.",
    action="store_true",
    dest="gcov_files",
    default=False
)
parser.add_option(
    "-s", "--print-summary",
    help="Prints a small report to stdout with line & branch "
         "percentage coverage",
    action="store_true",
    dest="print_summary",
    default=False
)
parser.usage = "gcovr [options]"
parser.description = \
    "A utility to run gcov and generate a simple report that summarizes " \
    "the coverage"
#
# Process options
#
options, args = parser.parse_args(args=sys.argv)

if options.version:
    sys.stdout.write(
        "gcovr %s\n"
        "\n"
        "Copyright (2013) Sandia Corporation. Under the terms of Contract\n"
        "DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government\n"
        "retains certain rights in this software.\n"
        % (version_str(), )
    )
    sys.exit(0)

if options.objdir is not None:
    if not options.objdir:
        sys.stderr.write(
            "(ERROR) empty --object-directory option.\n"
            "\tThis option specifies the path to the object file "
            "directory of your project.\n"
            "\tThis option cannot be an empty string.\n"
        )
        sys.exit(1)
    tmp = options.objdir.replace('/', os.sep).replace('\\', os.sep)
    while os.sep + os.sep in tmp:
        tmp = tmp.replace(os.sep + os.sep, os.sep)
    if normpath(options.objdir) != tmp:
        sys.stderr.write(
            "(WARNING) relative referencing in --object-directory.\n"
            "\tthis could cause strange errors when gcovr attempts to\n"
            "\tidentify the original gcc working directory.\n")
    if not os.path.exists(normpath(options.objdir)):
        sys.stderr.write(
            "(ERROR) Bad --object-directory option.\n"
            "\tThe specified directory does not exist.\n")
        sys.exit(1)

if options.output is not None:
    options.output = os.path.abspath(options.output)

if options.root is not None:
    if not options.root:
        sys.stderr.write(
            "(ERROR) empty --root option.\n"
            "\tRoot specifies the path to the root "
            "directory of your project.\n"
            "\tThis option cannot be an empty string.\n"
        )
        sys.exit(1)
    root_dir = os.path.abspath(options.root)
else:
    root_dir = starting_dir

#
# Setup filters
#
options.root_filter = re.compile(re.escape(root_dir + os.sep))
for i in range(0, len(options.exclude)):
    options.exclude[i] = re.compile(options.exclude[i])

if options.exclude_dirs is not None:
    for i in range(0, len(options.exclude_dirs)):
        options.exclude_dirs[i] = re.compile(options.exclude_dirs[i])

for i in range(0, len(options.filter)):
    options.filter[i] = re.compile(options.filter[i])
if len(options.filter) == 0:
    options.filter.append(options.root_filter)

for i in range(0, len(options.gcov_exclude)):
    options.gcov_exclude[i] = re.compile(options.gcov_exclude[i])
if options.gcov_filter is not None:
    options.gcov_filter = re.compile(options.gcov_filter)
else:
    options.gcov_filter = re.compile('')
#
# Get data files
#
if len(args) == 1:
    if options.root is None:
        search_paths = ["."]
    else:
        search_paths = [options.root]

    if options.objdir is not None:
        search_paths.append(options.objdir)

    datafiles = get_datafiles(search_paths, options)
else:
    datafiles = get_datafiles(args[1:], options)
#
# Get coverage data
#
covdata = {}
for file_ in datafiles:
    if options.gcov_files:
        process_existing_gcov_file(file_, covdata, options)
    else:
        process_datafile(file_, covdata, options)
if options.verbose:
    sys.stdout.write(
        "Gathered coveraged data for " + str(len(covdata)) + " files\n"
    )
#
# Print report
#
if options.xml or options.prettyxml:
    print_xml_report(covdata)
elif options.html:
    print_html_report(covdata, options.html_details)
else:
    print_text_report(covdata)

if options.print_summary:
    print_summary(covdata)
