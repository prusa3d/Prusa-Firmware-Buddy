# integrity test for login/logout process
from os import path

from requests import Session

import pytest

from .support import check_post, check_get, check_api_post
from .openapi import response_validator_yaml


@pytest.fixture
def api_url(request):
    return request.config.option.url + "/api"


@pytest.fixture
def user(request):
    return request.config.option.user


@pytest.fixture
def password(request):
    return request.config.option.password


@pytest.fixture
def api_key(request):
    return request.config.option.api_key


@pytest.fixture
def session(api_url, user, password):
    session = Session()
    req = check_post(api_url + "/login",
                     session=session,
                     json={
                         "user": user,
                         "pass": password,
                         "remember": False
                     })
    assert req.json()["session"]
    return session


@pytest.fixture
def validator():
    cache = getattr(validator, "cache", None)
    if cache:
        return cache

    validator.cache = response_validator_yaml(
        path.join(path.dirname(__file__), path.pardir, "doc", "openapi.yaml"))

    return validator.cache


class TestLogin:

    def test_loging(self, session):
        pass  # test wil done by session fixture

    def test_logout(self, session, api_url):
        check_post(api_url + "/logout", status_code=204, session=session)

    def test_printer(self, session, api_url):
        check_get(api_url + "/printer", session=session)

    def test_connection(self, session, api_url):
        check_get(api_url + "/connection", session=session)


class TestLoginApi:

    def test_loging(self, api_url, user, password, validator):
        check_api_post(api_url + "/login",
                       json={
                           "user": user,
                           "pass": password,
                           "remember": False
                       },
                       response_validator=validator)

    def test_logout(self, api_url, session, validator):
        check_api_post(api_url + "/logout",
                       status_code=204,
                       session=session,
                       response_validator=validator)


class TestApiKey:

    def test_printer(self, api_url, api_key):
        check_get(api_url + "/printer", headers={'X-Api-Key': api_key})

    def test_connection(self, api_url, api_key):
        check_get(api_url + "/connection", headers={'X-Api-Key': api_key})
