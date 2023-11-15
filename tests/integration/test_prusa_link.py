import aiohttp
import asyncio
import logging
import pytest
import struct

from .actions import encoder, screen, temperature, network, utils
from simulator import MachineType, Thermistor, Printer

PRUSALINK_PASSWORD = '0123456789123456'
PRUSALINK_EEPROM = {
    'PrusaLink Password':
    struct.pack('<16s', bytearray(PRUSALINK_PASSWORD.encode('utf-8')))
}

pytestmark = [
    pytest.mark.asyncio,
    pytest.mark.parametrize('specific_eeprom_variables', [PRUSALINK_EEPROM])
]
gcode_filename = 'box'
gcode_file = gcode_filename + '.gcode'


def wui_base_url(printer):
    return f'http://localhost:{network.proxy_http_port_get(printer)}'


@pytest.fixture
async def wui_client(printer):
    # Make sure the printer is running before returning the client.
    await utils.wait_for_bootstrap(printer)

    async with aiohttp.ClientSession(
            base_url=wui_base_url(printer)) as session:
        yield session


def valid_headers():
    return {'X-Api-Key': PRUSALINK_PASSWORD}


async def test_web_interface_is_accessible(wui_client: aiohttp.ClientSession):
    response = await wui_client.get('/')
    assert response.ok
    # Check we can actually download the whole page and that it looks htmley a
    # bit.
    body = await response.text()
    assert '<html>' in body


async def test_not_found(wui_client: aiohttp.ClientSession):
    for non_existent in ['/nonsense', '/whatever/not']:
        response = await wui_client.get(non_existent)
        assert response.status == 404

    # The whole /api is behind an authentication and _doesn't_ show even what
    # exists and what doesn't.
    response = await wui_client.get('/api/not')
    assert response.status == 401

    # But when authenticated, it prefers the 404 error for non-existing bits.
    response = await wui_client.get('/api/not', headers=valid_headers())
    assert response.status == 404


async def test_auth(wui_client: aiohttp.ClientSession):
    # Not getting in when no X-Api-Kep is present.
    all_endpoints = ['version', 'printer', 'job']
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


async def test_idle_version(wui_client: aiohttp.ClientSession):
    version_r = await wui_client.get('/api/version', headers=valid_headers())
    version = await version_r.json()
    for exp in ["text", "hostname", "api", "server"]:
        assert exp in version


async def test_idle_printer_api(wui_client: aiohttp.ClientSession):
    printer_r = await wui_client.get('/api/printer', headers=valid_headers())
    printer_j = await printer_r.json()
    tele = printer_j["telemetry"]
    assert tele["material"] == "---"
    assert tele["print-speed"] == 100
    temp = printer_j["temperature"]
    assert temp["tool0"]["target"] == 0
    assert temp["bed"]["target"] == 0
    assert printer_j["state"]["text"] == "Operational"
    assert not printer_j["state"]["flags"]["printing"]


async def test_idle_job(wui_client: aiohttp.ClientSession):
    job_r = await wui_client.get('/api/job', headers=valid_headers())
    job = await job_r.json()
    assert job["state"] == "Operational"
    assert job["job"] is None
    assert job["progress"] is None


@pytest.fixture
async def printer_with_files(printer_factory, printer_flash_dir, data_dir):
    gcode_name = gcode_file
    gcode = (data_dir / gcode_name).read_bytes()
    (printer_flash_dir / gcode_name).write_bytes(gcode)

    async with printer_factory() as printer:
        printer: Printer
        # Wait for boot
        await utils.wait_for_bootstrap(printer)
        # Wait for mounting of the USB
        await screen.wait_for_text(printer, 'Print')

        client = aiohttp.ClientSession(base_url=wui_base_url(printer))

        yield client


@pytest.fixture
async def running_printer_client(printer_factory, printer_flash_dir, data_dir):
    gcode_name = gcode_file
    gcode = (data_dir / gcode_name).read_bytes()
    (printer_flash_dir / gcode_name).write_bytes(gcode)

    async with printer_factory() as printer:
        printer: Printer
        # Wait for boot
        await utils.wait_for_bootstrap(printer)
        # Wait for mounting of the USB
        await screen.wait_for_text(printer, 'Print')

        # "Preheat" the printer
        await temperature.set(printer, Thermistor.BED, 60)
        await temperature.set(printer, Thermistor.NOZZLE, 170)

        tmp = await temperature.get(printer, Thermistor.NOZZLE)
        print(tmp, type(tmp))

        # Start a print
        await encoder.click(printer)
        # For some reason, box.gcode is not discovered, the OCR places a space
        # in there.
        await screen.wait_for_text(printer, 'gcode')
        await encoder.click(printer)
        await screen.wait_for_text(printer, 'Print')
        await encoder.click(printer)
        await screen.wait_for_text(printer, 'Tune')

        client = aiohttp.ClientSession(base_url=wui_base_url(printer))

        # Wait for the printer to go into the printing state. This might take a
        # little bit of time, so try few times before giving up
        for attempt in range(1, 10):
            printer_r = await client.get('/api/printer',
                                         headers=valid_headers())
            printer_j = await printer_r.json()
            if printer_j["temperature"]["bed"]["target"] > 0 and printer_j[
                    "state"]["flags"]["printing"]:
                break
            logging.info("Didn't start print yet? " + str(printer_j))
            await asyncio.sleep(0.3)
        else:
            assert False  # Didn't reach the part where there's a temperature

        yield client


async def test_printing_telemetry(running_printer_client):
    """
    Ask for telemetry information during a print and get something useful.
    """
    printer_r = await running_printer_client.get('/api/printer',
                                                 headers=valid_headers())
    printer_j = await printer_r.json()
    assert printer_j["temperature"]["bed"]["target"] == 60
    assert printer_j["temperature"]["tool0"]["target"] == 170
    assert printer_j["temperature"]["tool0"]["display"] == 170
    assert printer_j["telemetry"]["temp-bed"] > 40
    assert printer_j["telemetry"]["temp-bed"] == printer_j["temperature"][
        "bed"]["actual"]


async def test_printing_job(running_printer_client):
    job_r = await running_printer_client.get('/api/job',
                                             headers=valid_headers())
    job = await job_r.json()
    logging.info("Received job info " + str(job))
    assert job["state"] == "Printing"
    # Hopefully the test won't take that long
    assert job["progress"]["printTime"] < 300
    assert job["job"]["file"]["name"] == gcode_file
    assert job["job"]["file"]["display"] == gcode_file
    assert job["job"]["file"]["path"] == f"/usb/BOX~1.GCO"
    # It's something around 25 minutes...
    # FIXME: The following sometimes fail too. It seems marlin_vars are not
    # always properly updated?
    assert job["progress"]["printTimeLeft"] > 1000
    assert job["job"]["estimatedPrintTime"] > 1000
    assert job["job"]["estimatedPrintTime"] == job["progress"][
        "printTime"] + job["progress"]["printTimeLeft"]


async def test_download_gcode(printer_with_files, data_dir):
    """
    Test downloading the gcode from the printer.

    Test both human readable and "short" file names (the latter are used within
    the job API so they are the ones the client may discover on its own).
    """
    gcode = (data_dir / gcode_file).read_bytes()
    for fname in [f'BOX~1.GCO', gcode_file]:
        download_r = await printer_with_files.get('/usb/' + fname,
                                                  headers=valid_headers())
        assert download_r.status == 200
        download = await download_r.read()
        assert download == gcode


async def test_thumbnails(printer_with_files):
    metadata_r = await printer_with_files.get(f'/api/files/usb/BOX~1.GCO',
                                              headers=valid_headers())
    assert metadata_r.status == 200
    metadata = await metadata_r.json()
    refs = metadata["refs"]
    for thumb in (refs["thumbnailSmall"], refs["thumbnailBig"]):
        thumb_r = await printer_with_files.get(thumb, headers=valid_headers())
        assert thumb_r.status == 200
        data = await thumb_r.read()
        # Check PNG "file magic"
        assert data[1:4] == b"PNG"


async def test_delete_project_printing(running_printer_client):
    fname = f'/api/files/usb/BOX~1.GCO'
    heads = valid_headers()
    # We are printing the file right now
    printing_attempt = await running_printer_client.delete(fname,
                                                           headers=heads)
    assert printing_attempt.status == 409

    stop = await running_printer_client.post('/api/job',
                                             headers=heads,
                                             data='{"command":"cancel"}')
    assert stop.status == 204


async def test_delete_project(printer_with_files):
    fname = f'/api/files/usb/BOX~1.GCO'
    heads = valid_headers()
    idle_attempt = await printer_with_files.delete(fname, headers=heads)
    assert idle_attempt.status == 204

    # The file actually disappeared
    listing_r = await printer_with_files.get('/api/files', headers=heads)
    assert listing_r.status == 200
    listing = await listing_r.json()
    assert listing["files"][0]["children"] == []


async def test_list_files(printer_with_files):
    listing_r = await printer_with_files.get('/api/files',
                                             headers=valid_headers())
    assert listing_r.status == 200
    listing = await listing_r.json()
    file = listing["files"][0]["children"][0]
    assert file["name"] == f"BOX~1.GCO"
    assert file["display"] == gcode_file
    refs = file["refs"]
    assert refs["thumbnailSmall"] == f"/thumb/s/usb/BOX~1.GCO"
    download = await printer_with_files.get(refs["resource"],
                                            headers=valid_headers())
    assert download.status == 200


async def test_caching(printer_with_files):
    path = f'/thumb/s/usb/BOX~1.GCO'
    h = valid_headers()
    get1 = await printer_with_files.get(path, headers=h)
    assert get1.status == 200
    print(dict(get1.headers))
    etag = get1.headers['Etag']

    # Match
    h['If-None-Match'] = etag
    get2 = await printer_with_files.get(path, headers=h)
    assert get2.status == 304  # Not Modified

    # Cache miss
    h['If-None-Match'] = "hello-123"
    get3 = await printer_with_files.get(path, headers=h)
    assert get3.status == 200


# See below, needs investigation
@pytest.mark.skip()
async def test_upload(wui_client: aiohttp.ClientSession):
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


async def test_upload_notauth(wui_client: aiohttp.ClientSession):
    data = aiohttp.FormData()
    data.add_field('file', b'', filename='empty.gcode')
    response = await wui_client.post('/api/files/sdcard', data=data)
    assert response.status == 401
