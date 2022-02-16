### Development using Vim

1. Configure your [Neo]Vim to support Language Server Protocol -- I recommend (early 2020) [coc.nvim](https://github.com/neoclide/coc.nvim).
2. Install [ccls](https://github.com/MaskRay/ccls) on your system.
3. Configure your LSP plugin in Vim to use `ccls`. In case of `coc.nvim`, the configuration looks as follows:

    ```JSON
    "languageserver": {
        "ccls": {
            "command": "ccls",
            "filetypes": [
                "c",
                "cpp"
            ],
            "settings": {},
            "rootPatterns": [
                ".ccls",
                "compile_commands.json",
                ".vim/",
                ".git/",
                ".hg/"
            ],
            "initializationOptions": {
                "cache": {
                    "directory": "/tmp/ccls"
                },
                "highlight": {
                    "lsRanges": true
                }
            }
    ```
4. Create a build folder and build the project using cmake:
    ```bash
    $ mkdir build-vim
    $ cd build-vim
    $ cmake .. \
        --preset mini \
        -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE=../cmake/GccArmNoneEabi.cmake \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE \
        -DGENERATE_BBF=YES ... and other cmake flags
    $ ninja
    ```
5. Locate a `compile_commands.json` file for `ccls`. By default, `ccls` searches in project's root directory, but `ninja` produces it in the build folder. There are two options:
    - Update `ccls` config to let `ccls` know, that it should search for it in the `build-vim` subfolder, or
    - make a symbolic link in project's root directory: `ln -s build-vim/compile_commands.json`.
6. Install some spell checker (optional but recommended).
7. Get to work! 💪
