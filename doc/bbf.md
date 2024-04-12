BBF is a file format used to distribute firmware for buddy platform.
It contains a header, firmware for the main board and littlefs image.

To unpack a BBF file, use
```
utils/unpack_bbf.py --input-file firmware.bbf
```

To unpack littlefs image, use
```
littlefs-python unpack --block-size 4096 --block-count 512 --image resources-image.lfs resources-image.lfs.unpacked
```

Make sure you are using correct version of littlefs-python.
This is best achieved using python virtual environment
```
source .venv/bin/activate
```
