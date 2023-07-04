# Integration Tests

## Requirements

1. Python 3.7
2. Make sure you have the project's dependencies downloaded if you haven't built the project before (`python utils/bootstrap.py`)

## Running the suite

Activate the virtualenv, if needed:
```
source .venv/bin/activate
```

Run the test suite
```
pytest tests/integration --firmware <firmware.bin to test>
```

-  Currently supported firmware builds are:
    - MK4, noboot variant
- The first time you run the tests, an OCR library will have to download its models first. This might take some time and you might not see any progress.

### Tips
- See [pytest's documentation](https://docs.pytest.org/en/latest/contents.html#toc)
- Do you want to see what is happening on the simulator's screen? `--enable-graphic` is your friend
- Beware that the tests cache some content (bootstrapped xflash for example). In case you are debugging some issues, clearing the cache using the `--cache-clear` might help.
- You ran the suite, a test failed, you fixed and now want to continue running the suite? Use `--failed-first` flag [doc](https://docs.pytest.org/en/6.2.x/cache.html)
- If you want to see all the logs while the tests are running, use -- for example -- `--log-cli-level info`, or `-s` to disable output capturing.
- Use `-x` to stop on the first failed test. `--pdb` to enter debugger on failure. Use `--gdb` to make the QEMU's GDB server available on port (1234).
- `--simulator <qemu-system-buddy>` lets you specify the simulator's binary to use.
