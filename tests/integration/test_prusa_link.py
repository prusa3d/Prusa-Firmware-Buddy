import aiohttp
import pytest

from .actions import encoder, screen, temperature, network
from simulator import MachineType, Thermistor, Printer

pytestmark = pytest.mark.asyncio


@pytest.fixture
def wui_base_url(printer):
    return f'http://localhost:{network.proxy_http_port_get(printer)}'


@pytest.fixture
async def wui_client(wui_base_url):
    async with aiohttp.ClientSession(base_url=wui_base_url) as session:
        yield session


def valid_headers():
    return {'X-Api-Key': '0123456789'}


async def test_web_interface_is_accessible(printer: Printer,
                                           wui_client: aiohttp.ClientSession):
    await screen.wait_for_text(printer, 'HOME')
    response = await wui_client.get('/')
    assert response.ok
    # Check we can actually download the whole page and that it looks htmley a
    # bit.
    body = await response.text()
    assert '<html>' in body


async def test_not_found(printer: Printer, wui_client: aiohttp.ClientSession):
    await screen.wait_for_text(printer, 'HOME')

    for non_existent in ['/nonsense', '/api/not']:
        response = await wui_client.get(non_existent)
        assert response.status == 404


async def test_auth(printer: Printer, wui_client: aiohttp.ClientSession):
    await screen.wait_for_text(printer, 'HOME')
    # Not getting in when no X-Api-Kep is present.
    all_endpoints = ['version', 'printer', 'job', 'files']
    for endpoint in all_endpoints:
        response = await wui_client.get('/api/' + endpoint)
        assert response.status == 401

    # Not getting in when the api key is wrong
    # We try some that are _close_ too.
    for wrong_pass in ["123456789", "012345678", "0123456789abc"]:
        headers = {"X-Api-Key": wrong_pass}
        for endpoint in all_endpoints:
            response = await wui_client.get('/api/' + endpoint,
                                            headers=headers)
            assert response.status == 401

    # And correct password lets one come in and returns a json
    for endpoint in all_endpoints:
        response = await wui_client.get('/api/' + endpoint,
                                        headers=valid_headers())
        assert response.ok
        assert response.headers["CONTENT-TYPE"] == 'application/json'
        await response.json()  # Tries to parse and throws if fails decoding


# See below, needs investigation
@pytest.mark.skip()
async def test_upload(printer: Printer, wui_client: aiohttp.ClientSession):
    await screen.wait_for_text(printer, 'HOME')
    data = aiohttp.FormData()
    data.add_field('file', b'', filename='empty.gcode')
    # TODO: Why does this send the "printer" to bluescreen? Doesn't happen on a
    # real one.
    response = await wui_client.post('/api/files/sdcard',
                                     data=data,
                                     headers=valid_headers())
    assert response.ok
    assert response.headers["CONTENT-TYPE"] == 'application/json'
    await response.json()  # Tries to parse and throws if fails decoding
    # See if we get the one-click-print screen with the just-uploaded file
    await screen.wait_for_text(printer, 'empty.gcode')
    # TODO: Turn off printer and see that the file appeared on the flash drive.


# Known issue: Returns 404 instead of 401
@pytest.mark.skip()
async def test_upload_notauth(printer: Printer,
                              wui_client: aiohttp.ClientSession):
    await screen.wait_for_text(printer, 'HOME')
    data = aiohttp.FormData()
    data.add_field('file', b'', filename='empty.gcode')
    response = await wui_client.post('/api/files/sdcard', data=data)
    assert response == 401
