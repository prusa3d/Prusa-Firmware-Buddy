### Development with LSP-based IDEs (Visual Studio [Code], Vim, Sublime Text, etc.)

1. Build the firmware for the printer(s) you are working on using `build.py`:

   ```bash
   python utils/build.py --preset coreone --build-type debug --bootloader yes
   ```

   This handles bootstrapping dependencies, configuring, and building. Build output (including `compile_commands.json`) is placed under `build/<preset>_<build-type>_<bootloader>/` (e.g. `build/coreone_debug_boot/`).

2. Link the generated `compile_commands.json` file to the project root so your editor's language server can find it:

   ```bash
   ln -s build/coreone_debug_boot/compile_commands.json compile_commands.json
   ```

3. If you have multiple build configurations, you can merge them using `jq`:

   ```bash
   jq -s add build/*/compile_commands.json > compile_commands.json
   ```

   Re-run this after rebuilding if the compile commands change.

4. Install some spell checker (optional but recommended).
