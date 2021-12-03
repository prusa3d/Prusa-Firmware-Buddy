# -*- coding: utf-8 -*-
from sys import stderr
from warnings import warn

from requests import Request, Session
from requests.exceptions import RequestException
from openapi_core.schema.operations.exceptions import InvalidOperation

from .openapi import OpenAPIRequest, OpenAPIResponse

IGNORED_URL = []


class TestError(RuntimeError):
    """Support exception."""


class Found(BaseException):
    """Support exception for founding entity in generator."""


def check(method, url, status_code=200, **kwargs):
    """Do HTTP request and check status_code."""
    session = kwargs.pop("session", None)
    if not session:
        session = Session()
    try:
        request = Request(method, url, cookies=session.cookies, **kwargs)
        response = session.send(request.prepare())
        if isinstance(status_code, int):
            status_code = [status_code]
        assert response.status_code in status_code
        return response
    except RequestException:
        pass
    raise ConnectionError("Not response")


def check_get(url, status_code=200, **kwargs):
    """Do HTTP GET request and check status_code."""
    return check('GET', url, status_code, **kwargs)


def check_post(url, status_code=200, **kwargs):
    """Do HTTP POST request and check status_code."""
    return check('POST', url, status_code, **kwargs)


def check_api(method,
              url,
              status_code=200,
              path_pattern=None,
              response_validator=None,
              **kwargs):
    """Do HTTP API request and check status_code."""
    assert response_validator, "response_validator must be set"
    session = kwargs.pop("session", None)
    if not session:  # nechceme vytvářet session nadarmo
        session = Session()
    try:
        request = Request(method, url, **kwargs)
        # je potreba pro pouziti/zachovani cookies!
        response = session.send(session.prepare_request(request))
        # bude vytisten jen pri chybe
        print("Response:\n", response.headers, "\n", response.text)
        if isinstance(status_code, int):
            status_code = [status_code]
        assert response.status_code in status_code
        api_request = OpenAPIRequest(request, path_pattern)
        result = response_validator.validate(api_request,
                                             OpenAPIResponse(response))
        if result.errors:
            to_raise = False
            for error in result.errors:
                if isinstance(error, InvalidOperation):
                    if response.status_code == 404:
                        continue
                    if api_request.path in IGNORED_URL:
                        continue
                    warn(UserWarning("Not API definition for %s!" % url))
                    continue
                stderr.write("API output error: %s" % str(error))
                to_raise = True
            if to_raise:
                raise TestError("API errors not zero: %d" % len(result.errors))
        return response
    except RequestException as err:
        print(err)
    raise ConnectionError("Not response")


def check_api_head(url, status_code=200, **kwargs):
    """Do HTTP HEAD API request and check status_code."""
    return check_api('HEAD', url, status_code, **kwargs)


def check_api_get(url, status_code=200, **kwargs):
    """Do HTTP GET API request and check status_code."""
    return check_api('GET', url, status_code, **kwargs)


def check_api_post(url, status_code=200, **kwargs):
    """Do HTTP POST API request and check status_code."""
    return check_api('POST', url, status_code, **kwargs)


def check_api_put(url, status_code=200, **kwargs):
    """Do HTTP PUT API request and check status_code."""
    return check_api('PUT', url, status_code, **kwargs)


def check_api_delete(url, status_code=200, **kwargs):
    """Do HTTP DELETE API request and check status_code."""
    return check_api('DELETE', url, status_code, **kwargs)
