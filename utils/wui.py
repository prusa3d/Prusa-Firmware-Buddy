#!/usr/bin/env python3

import os
from pathlib import Path
import gzip
import shutil
import argparse


class WUI_Files:
    def __init__(self, input_folder: Path, output_folder: Path, filename):
        self.wui_files = []
        self.main_file_name = filename

        self.wui_path = input_folder
        # path to main C file with raw data
        self.raw_data_file = os.path.join(output_folder, self.main_file_name)
        self.raw_data_file_tmp = os.path.join(output_folder,
                                              f'{self.main_file_name}.tmp')

    """ store paths to static wui files in array for later use """

    def get_files(self):
        root, dirs, files = next(os.walk(self.wui_path))
        for file in files:
            self.wui_files.append(os.path.join(root, file))

    """ gzip wui static files in same directory """

    def gzip_files(self):
        for file in self.wui_files:
            with open(file, 'rb') as f_in:
                with gzip.open(file + '.gz', 'wb') as f_out:
                    shutil.copyfileobj(f_in, f_out)

    """
    Get content of file, compress it using gzip and return as raw hex data.
    """

    def get_hex(self, file_path):
        with open(file_path, 'rb') as file:
            data = file.read()
        return ','.join(map(lambda b: hex(b), gzip.compress(data)))

    """ finish script - replace tmp generated file for origin & delete it """

    def finish_main_file(self):
        data = []
        file_num = len(self.wui_files)
        last_file = 'file__' + Path(self.wui_files[file_num - 1]).name
        last_file = last_file.replace('.', '_')

        # add define to file num & root
        data.append('\n')
        data.append('#define FS_NUMFILES ' + str(file_num) + '\n')
        data.append('#define FS_ROOT ' + last_file + '\n')

        # write data
        self.write_tmp_data(data)

    """ generate variables at the end of the file """

    def write_variables(self):
        data = []

        previous_file = 'file_NULL'
        for file in self.wui_files:
            file_name = Path(file).name.replace('.', '_')
            file_length = len('/' + file_name) + 1

            data.append('const struct fsdata_file file__' + file_name +
                        '[] = { {\n')
            data.append(previous_file + ',\n')
            data.append('data__' + file_name + ',\n')
            data.append('data__' + file_name + ' + ' + str(file_length) +
                        ',\n')
            data.append('sizeof(data__' + file_name + ') - ' +
                        str(file_length) + ',\n')
            data.append('0,\n')
            data.append('} };\n\n')

            previous_file = 'file__' + file_name

        # write data
        self.write_tmp_data(data)

    """
    remove created gziped files,
    remove original main file with raw data and replace it with TMP file as new origin
    """

    def replace_origin(self):
        # remove gziped files
        #        for file in self.wui_files:
        #            os.remove(file + '.gz')

        # remove original file and rename tmp file
        os.replace(self.raw_data_file_tmp, self.raw_data_file)

    """ generate hex raw data from all resources files into a one C file """

    def generate_raw_data_file(self):
        # tmp file for writing data
        self.create_tmp_file()
        # write static data as header
        self.generate_header()

        # dev test
        #  self.getHex(self.wui_files[0])
        #  self._writeFile(self.wui_files[0])

        # generate hex from files
        for file in self.wui_files:
            self.write_file(file)

        # add footer with variables
        self.write_variables()
        # add footer with defines of files
        self.finish_main_file()
        # replace & remove
        self.replace_origin()

    """ write data into a file """

    def write_tmp_data(self, data):
        tmp_file = open(self.raw_data_file_tmp, 'a')
        tmp_file.writelines(data)
        tmp_file.close()

    """ read file as binary and wrote hex data into a main file """

    def write_file(self, file):
        # lines to wrote into a tmp file
        data = []
        # file name
        file_name = Path(file).name

        # define file array
        data.append('static const unsigned char FSDATA_ALIGN_PRE data__' +
                    file_name.replace('.', '_') + '[] FSDATA_ALIGN_POST = {\n')

        # get hex of file name, add into text data array
        file_name = '/' + file_name
        hex_data = []
        for ch in file_name:
            ch_hex = '0x' + ch.encode('utf-8').hex()
            hex_data.append(ch_hex)

        # add null terminator
        hex_data.append('0x00')
        # generate string
        hex_data = ','.join(hex_data)
        hex_data += ',\n'
        data.append(hex_data)

        # hex file data
        hex_data = self.get_hex(file)
        hex_data += ',\n'
        data.append(hex_data)

        # end of hex data array
        data.append('};\n\n')

        # write data
        self.write_tmp_data(data)

    """
    create new temporary file to write content into it then,
    after all content is made, this file will replace the original one
    """

    def create_tmp_file(self):
        open(self.raw_data_file_tmp, 'w')

    """ generate and write static makros and includes as a header of main file with raw data """

    def generate_header(self):
        # lines to wrote into a tmp file
        data = []

        data.append('#include "lwip/apps/fs.h"\n')
        data.append('#include "lwip/def.h"\n\n')

        data.append('#define file_NULL (struct fsdata_file *)NULL\n\n')

        data.append('#ifndef FS_FILE_FLAGS_HEADER_INCLUDED\n')
        data.append('#define FS_FILE_FLAGS_HEADER_INCLUDED 1\n')
        data.append('#endif\n')
        data.append('#ifndef FS_FILE_FLAGS_HEADER_PERSISTENT\n')
        data.append('#define FS_FILE_FLAGS_HEADER_PERSISTENT 0\n')
        data.append('#endif\n')
        data.append('#ifndef FSDATA_FILE_ALIGNMENT\n')
        data.append('#define FSDATA_FILE_ALIGNMENT 0\n')
        data.append('#endif\n')
        data.append('#ifndef FSDATA_ALIGN_PRE\n')
        data.append('#define FSDATA_ALIGN_PRE\n')
        data.append('#endif\n')
        data.append('#ifndef FSDATA_ALIGN_POST\n')
        data.append('#define FSDATA_ALIGN_POST\n')
        data.append('#endif\n')
        data.append('#if FSDATA_FILE_ALIGNMENT == 2\n')
        data.append('#include "fsdata_alignment.h"\n')
        data.append('#endif\n\n')
        #  data.append('')

        # write data
        self.write_tmp_data(data)


parser = argparse.ArgumentParser(description='Web.....')
parser.add_argument('--input-folder', type=Path, help='input folder')
parser.add_argument('--output-folder', type=Path, help='output folder')
parser.add_argument('filename', help='output file')
args = parser.parse_args()

# start instance
wf = WUI_Files(args.input_folder, args.output_folder, args.filename)
wf.get_files()
wf.generate_raw_data_file()
