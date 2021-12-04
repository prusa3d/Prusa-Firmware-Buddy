# Integration Tests

## Requirements

1. Python 3.7
2. Install Python dependencies using `pip install -r tests/integration/requirements.txt`
3. Make sure you have the project's dependencies downloaded if you haven't built the project before (`python utils/bootstrap.py`)

## Running the suite

Run `pytest tests/integration --firmware <firmware.bin to test>`

> ⚠️  The first time you run the tests, an OCR library will have to download its models first. This might take some time and you won't see any progress.
> ⚠️  The firmware has to support the AUTO.GCO feature. Compile the firmware with `-DCUSTOM_COMPILE_OPTIONS:STRING=-DAUTOSTART_GCODE=1`

Some tips:
- See [pytest's documentation](https://docs.pytest.org/en/latest/contents.html#toc)
- Do you want to see what is happening on the simulator's screen? `--enable-graphic` is your friend
- Beware that the tests cache prepared eeprom content. In case you are debugging some test issues, use `--cache-clear` option to clear the cache.
- You ran the suite, a test failed, you fixed and now want to continue running the suite? Use `--failed-first` flag [doc](https://docs.pytest.org/en/6.2.x/cache.html)
- If you want to see all the logs while the tests are running, use -- for example -- `--log-cli-level info`, or `-s` to disable output capturing.
- Use `-x` to stop on the first failed test. `--pdb` to enter debugger on failure.
- `--simulator <qemu-system-buddy>` lets you specify the simulator's binary to use
