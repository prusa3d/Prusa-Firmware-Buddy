#!/usr/bin/env python3

import os
from pathlib import Path
import gzip
import shutil

class WUIFiles:
    def __init__(self):
        self.wui_files = []
        # path to resource files directory
        self.wui_path = Path(__file__).resolve().parent.parent / 'lib' / 'WUI' / 'resources' / 'src_local'
        # path to main C file with raw data
        self.main_file_name = 'fsdata_wui_local.c'
        self.raw_data_file = Path(__file__).resolve().parent.parent / 'lib' / 'WUI' / 'resources' / self.main_file_name
        self.raw_data_file_tmp = Path(__file__).resolve().parent.parent / 'lib' / 'WUI' / 'resources' / str(self.main_file_name + '_tmp')

        # start
        self.getFiles()
        self.gzipFiles()
        self.generateRawDataFile()
    #endef

    # store paths to static wui files in array for later use
    def getFiles(self):
        os.chdir(self.wui_path)
        root, dirs, files = next(os.walk(self.wui_path))
        for file in files:
            # *.gz files is ignored
            # if for some reason gziped files is in root dir - we don't want them
            # TODO: check for gziped files and delete them ?!
            if file.find('.gz') < 0:
                self.wui_files.append(os.path.join(root, file));
    #endef

    # gzip wui static files in same directory
    def gzipFiles(self):
        for file in self.wui_files:
            with open(file, 'rb') as f_in:
                with gzip.open(file + '.gz', 'wb') as f_out:
                    shutil.copyfileobj(f_in, f_out)
    #endef

    # get hex from file
    # arg[0] file_path
    # return string of raw hex data
    def getHex(self, file_path):


        with open(file_path, 'rb') as file:
            hex_data = []
            while True:
                hd = file.read(1).hex()
                if len(hd) == 0:
                    break;
                hex_data.append('0x' + hd)
            hex_data.append('0x00')

        ret = ','.join(hex_data)
        return ret



        #  print('hex from file :', file_path)
        #  with open(file_path, 'rb') as file:
            #  hex_data = []
            #  d = file.read(1)
            #  while d:
                #  for ch in d:
                    #  hex_data.append(hex(ord(ch)) + '')
                    #  ch = ch.encode('utf-8')
                    #  hex_data.append(hex(ch) + '')
                    #  hex_data.append(ch.hex() + '')
                #  d = file.read(1)
            #  hex_data.append('0x00')
#
        #  ret = ','.join(hex_data)
        #  return ret
    #endef

    # finish script - replace tmp generated file for origin & delete it
    def finishMainFile(self):
        data = []
        file_num = len(self.wui_files)
        last_file = 'file__' + Path(self.wui_files[file_num - 1]).name
        last_file = last_file.replace('.', '_')

        # add define to file num & root
        data.append('\n#define FS_NUMFILES ' + str(file_num) +'\n')
        data.append('#define FS_ROOT ' + last_file + '\n')

        # write data
        self.writeTmpData(data)
    #endef

    def writeVariables(self):
        data = []

        previous_file = 'file_NULL'
        for file in self.wui_files:
            file_name = Path(file).name.replace('.', '_')
            file_length = len('/' + file_name) + 1

            data.append('const struct fsdata_file file__' + file_name + '[] = { {\n')
            data.append(previous_file + ',\n')
            data.append('data__' + file_name + ',\n')
            data.append('data__' + file_name + ' + ' + str(file_length) + ',\n')
            data.append('sizeof(data__' + file_name + ') - ' + str(file_length) + ',\n')
            data.append('0,\n')
            data.append('} };\n\n')

            previous_file = 'file__' + file_name

        # write data
        self.writeTmpData(data)
    #endef

    def replaceOrigin(self):
        # remove gziped files
        #  for file in self.wui_files:
            #  os.remove(file + '.gz')

        # remove original file and rename tmp file
        os.remove(self.raw_data_file)
        os.rename(self.raw_data_file_tmp, self.raw_data_file)
    #endef
    
    # generate hex raw data from all resources files into a one C file
    def generateRawDataFile(self):
        # tmp file for writing data
        self.createTmpFile()
        # write static data as header
        self.generateHeader()

        # dev test
        #  self.getHex(self.wui_files[0])
        #  self._writeFile(self.wui_files[0])

        # generate hex from files
        for file in self.wui_files:
            self.writeFile(file)

        # add footer with variables
        self.writeVariables()
        # add footer with defines of files
        self.finishMainFile()
        # replace & remove
        self.replaceOrigin()
    #endef

    # write data into a file
    def writeTmpData(self, data):
        tmp_file = open(self.raw_data_file_tmp, 'a')
        tmp_file.writelines(data)
        tmp_file.close()
    #endef

    # read file as binary and wrote hex data into a main file
    def writeFile(self, file):
        # lines to wrote into a tmp file
        data = []
        # file name
        file_name = Path(file).name
        
        # define file array
        data.append('static const unsigned char FSDATA_ALIGN_PRE data__' +file_name.replace('.', '_') + '[] FSDATA_ALIGN_POST = {\n')

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
        hex_data = self.getHex(file + '.gz')
        hex_data += ',\n'
        data.append(hex_data)

        # end of hex data array
        data.append('};\n\n')

        # write data
        self.writeTmpData(data)
    #endef
    
    # create new temporary file to write content into it then,
    # after all content is made, this file will replace the original one
    def createTmpFile(self):
        open(self.raw_data_file_tmp, 'w')
    #endef

    def generateHeader(self):
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
        self.writeTmpData(data)
    #endef
#enclass

# start instance
wf = WUIFiles()

