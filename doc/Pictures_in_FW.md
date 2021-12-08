# Pictures in firmware

## How to get picture to firmware
There are few steps you need to do to get your picture to firmware
1. Get the picture in PNG format
2. convert it to .c file with binary representation of the picture
3. put the png and .c file to its corresponding folders in src/res
4. include the .c file in resource.c
5. Create resource endtry in resource.c add the entry to enum in resource.h as IDR_name_of_the_picture
6. Use the picture under its resource name

## How to use png2cc.py

### To use the script you need to have installed:

- python interpreter 3.7 or higher
-pip 1.5 or higher
- Wand package
  - [link to installation guide](https://docs.wand-py.org/en/0.6.7/guide/install.html)
- image magick at least version 7
  - haven't found image magick version 7 on ubuntu, so on ubuntu you need to build it from source
    - didn't check other distros, so please leave your experiences under this line
  - Windows and macOS should be OK with just following the instructions on the site

### How to use the script
The script has two operation modes file and folder mode
#### File mode
In file mode it expects two inputs source png file
```shell
python png2cc.py src_file.png
```

#### Folder mode
In folder mode the script processes all png files in the first level of the folder. You need to specify the source and destination folder
```shell
python png2cc.py src_folder --folder=dst_folder
```
