"""pytest configuration"""

# default pytest vylues
URL = "http://10.25.247.256:5000"
USER = "pi"
PASSWORD = "raspberry"
API_KEY = "EAFBC7A359E048AB836ADD36D44209B9"


def pytest_addoption(parser):
    """Append new options for py.test command tool."""
    parser.addoption("--url",
                     action="store",
                     type="string",
                     default=URL,
                     help="Endpoint url")
    parser.addoption("--user",
                     action="store",
                     type="string",
                     default=USER,
                     help="HTTP User Name")
    parser.addoption("--password",
                     action="store",
                     type="string",
                     default=PASSWORD,
                     help="HTTP User Password")
    parser.addoption("--api-key",
                     action="store",
                     type="string",
                     default=API_KEY,
                     help="HTTP API KEY")
