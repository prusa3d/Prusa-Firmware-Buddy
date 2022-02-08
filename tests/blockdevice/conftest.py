def pytest_addoption(parser):
    # yapf: disable
    parser.addoption(
        '--device',
        type=str,
    )
