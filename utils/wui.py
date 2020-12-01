#!/usr/bin/env python3

import argparse
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
        self.raw_data_file = Path(__file__).resolve().parent.parent / 'lib' / 'WUI' / 'resources' / 'fsdata_wui_local_1.c'

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
        print('hex from file :', file_path)
        with open(filePath, 'rb') as file:
            hex_data = []
            d = file.read(16)
            while d:
                for ch in d:
                    hex_data.append(hex(ch) + '')
                d = file.read(16)

        separator = ','
        data = separator.join(hex_data)
        print(data)
        return data 
    #endef

    # generate hex raw data from all resources files into a one C file
    def generateRawDataFile(self):
        self.getHex(self.wui_files[0])

        #  for file in self.wui_files:
            #  print(file)
    #endef


# start instance
wf = WUIFiles()


