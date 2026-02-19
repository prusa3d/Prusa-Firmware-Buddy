### Development using Zed

1. Follow the steps in [LSP-based IDEs](lsp-based-ides.md) to build the firmware and generate a `compile_commands.json`.
2. Create a `.zed/settings.json` in the project root to configure `clangd` with `--query-driver`, so it can find the ARM GCC toolchain's standard library headers:

   ```JSON
   {
     "lsp": {
       "clangd": {
         "binary": {
           "arguments": [
             "--query-driver=/absolute/path/to/repo/.dependencies/gcc-arm-none-eabi-*/bin/arm-none-eabi-*"
           ]
         }
       }
     }
   }
   ```

   Note: `--query-driver` requires an absolute path; relative paths are not supported.

   Without this, `clangd` won't find headers like `<optional>` that ship with the ARM GCC toolchain.

3. Get to work!
