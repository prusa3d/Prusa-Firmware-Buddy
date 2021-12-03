"""requests library object Wrapper for openapi_core library objects."""
from urllib.parse import urlparse, parse_qs
from cgi import parse_header

import json

from openapi_core import create_spec
from openapi_core.wrappers.mock import MockRequest, MockResponse
from openapi_core.shortcuts import ResponseValidator
from openapi_spec_validator.loaders import ExtendedSafeLoader

import yaml


class OpenAPIRequest(MockRequest):
    """requests.Request wrapper for openapi_core."""

    def __init__(self, request, path_pattern=None):
        url = urlparse(request.url)
        self.path_pattern = path_pattern or url.path
        query = parse_qs(url.query) if url.query else {}
        # when args have one value, that is the value
        args = tuple((key, val[0] if len(val) < 2 else val)
                     for key, val in query.items())
        ctype = parse_header(request.headers.get('Content-Type', ''))

        super(OpenAPIRequest,
              self).__init__("{url.scheme}://{url.netloc}/".format(url=url),
                             request.method.lower(),
                             url.path,
                             path_pattern=path_pattern or url.path,
                             args=args,
                             headers=request.headers,
                             cookies=request.cookies,
                             data=request.data,
                             mimetype=ctype[0])


class OpenAPIResponse(MockResponse):
    """requests.Response wrapper for openapi_core."""

    def __init__(self, response):
        ctype = parse_header(response.headers.get('Content-Type', ''))
        super(OpenAPIResponse, self).__init__(response.text,
                                              response.status_code, ctype[0])


def response_validator_json(filename):
    """Initialization response_validator for openapi.json."""
    with open(filename, "r") as openapi:
        spec = create_spec(json.load(openapi))
        return ResponseValidator(spec)


def response_validator_yaml(filename):
    """Initialization response_validator for openapi.yaml."""
    with open(filename, "r") as openapi:
        spec = create_spec(yaml.load(openapi, ExtendedSafeLoader))
        print("spec:", spec)
        return ResponseValidator(spec)


__all__ = ["response_validator_json", "OpenAPIRequest", "OpenAPIResponse"]
