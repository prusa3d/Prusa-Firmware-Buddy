
# marlin_server_types

This is python module, that exposes types that can be used to determine states of marlin server
If you import it, you can decode variables like marlin server state to concrete enum. Also supports FSM and its responses.
Intended usage is for testing the firmware - integration tests and so on.

# Warning

Note that this library need needs to be compiled for specific printer and its settings!

# Usage
`pip install .`

In your python script:

`import marlin_server_types`

Now you have all the variables available, to see the variables, look into `marlin_server_types/__init__.py` or run test in `tests/test_basic.py`, it will print all the variables
