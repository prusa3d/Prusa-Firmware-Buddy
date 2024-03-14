####################################################
# Shared functions/settings for Buddy-Firmware & GDB
# gdb entry point: setup directories and source init

import gdb
import os.path
import shlex

# include the current directory and source init.gdb
gdb.execute('directory ' + shlex.quote(
    os.path.realpath(os.path.join(os.path.dirname(__file__), '../..'))))
gdb.execute('source -s utils/gdb/init.gdb')
