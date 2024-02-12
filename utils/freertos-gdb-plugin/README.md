# FreeRTOS plugin for gdb


## What is this
This is a plugin for debugging FreeRTOS data structures in gdb debugger.
At the moment it only supports printing stacktrace of FreeRTOS tasks.


## How do I use this
Make sure your gdb supports Python.

Start the debug session as usual and load the plugin:
```
source utils/freertos-gdb-plugin/freertos-gdb-plugin.py
```

After the plugin is loaded, gdb starts providing new command `freertos`.
For details on how to use this command, read the help:
```
help freertos
```

Happy hacking!


## References
https://sourceware.org/gdb/current/onlinedocs/gdb.html/Python.html
https://github.com/espressif/freertos-gdb
