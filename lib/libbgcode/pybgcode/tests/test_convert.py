"""Convert test functions."""
import filecmp

import pytest

import pybgcode
from pybgcode import (
    EResult,
    EThumbnailFormat,
    read_thumbnails,
    read_metadata,
    read_connect_metadata,
    connect_metadata_keys
)

# pylint: disable=missing-function-docstring
TEST_GCODE = "test.gcode"
TEST_REVERSE_GCODE = "test_reverse.gcode"
TEST_BGCODE = "test.bgcode"
TEST_THUMBNAILS = 2

TEST_PRINTER_METADATA = {'printer_model': 'MINI', 'filament_type': 'PETG',
                         'nozzle_diameter': '0.4', 'bed_temperature': '90',
                         'brim_width': '0', 'fill_density': '15%',
                         'layer_height': '0.15', 'temperature': '240',
                         'ironing': '0', 'support_material': '0',
                         'max_layer_z': '18.05', 'extruder_colour': '""',
                         'filament used [mm]': '986.61',
                         'filament used [cm3]': '2.37',
                         'filament used [g]': '3.01', 'filament cost': '0.08',
                         'estimated printing time (normal mode)': '32m 6s'}
TEST_FILE_METADATA = {'Producer': 'PrusaSlicer 2.6.0'}
TEST_PRINT_METADATA = {
    'filament used [mm]': '986.61', 'filament used [cm3]': '2.37',
    'filament used [g]': '3.01', 'filament cost': '0.08',
    'total filament used [g]': '3.01', 'total filament cost': '0.08',
    'estimated printing time (normal mode)': '32m 6s',
    'estimated first layer printing time (normal mode)': '1m 8s'}
TEST_LEN_SLICER_METADATA = 302



def get_thumbnail_extension(format_id):
    ret = "unknown"
    if format_id == int(EThumbnailFormat.PNG):
        ret = "png"
    elif format_id == int(EThumbnailFormat.JPG):
        ret = "jpg"
    elif format_id == int(EThumbnailFormat.QOI):
        ret = "qoi"

    return ret


def test_main():
    assert pybgcode.__version__ == pybgcode.version()

    in_f = pybgcode.open(TEST_GCODE, "r")
    out_f = pybgcode.open(TEST_BGCODE, "wb")

    assert in_f
    assert out_f

    assert pybgcode.is_open(in_f)
    assert pybgcode.is_open(out_f)

    cfg = pybgcode.get_config()
    cfg.compression.file_metadata = pybgcode.CompressionType.Heatshrink_11_4
    res = pybgcode.from_ascii_to_binary(in_f, out_f, cfg)

    assert res == EResult.Success

    pybgcode.close(out_f)
    pybgcode.close(in_f)

    thumb_f = pybgcode.open(TEST_BGCODE, "rb")
    thumbnails = read_thumbnails(thumb_f)
    assert len(thumbnails) == TEST_THUMBNAILS
    printer_metadata = read_metadata(thumb_f)
    assert printer_metadata == TEST_PRINTER_METADATA
    file_metadata = read_metadata(thumb_f, 'file')
    assert file_metadata == TEST_FILE_METADATA
    print_metadata = read_metadata(thumb_f, 'print')
    assert print_metadata == TEST_PRINT_METADATA
    slicer_metadata = read_metadata(thumb_f, 'slicer')
    assert len(slicer_metadata) == TEST_LEN_SLICER_METADATA

    all_metadata = read_connect_metadata(thumb_f)
    # ; extruder_colour = "" filtered out
    assert 'extruder_colour' not in all_metadata['metadata']
    assert len(all_metadata['thumbnails']) == TEST_THUMBNAILS
    for key in all_metadata['metadata'].keys():
        assert key in connect_metadata_keys
    pybgcode.close(thumb_f)

    # write thumbnails to png files
    thumcnt = 0
    for thumb in thumbnails:
        with open("thumb" + str(thumcnt) + "." +
                  get_thumbnail_extension(thumb["meta"].format), "wb") as out:
            out.write(thumb["bytes"])
            thumcnt = thumcnt + 1

    in_f = pybgcode.open(TEST_BGCODE, "rb")
    out_f = pybgcode.open(TEST_REVERSE_GCODE, "w")

    res = pybgcode.from_binary_to_ascii(in_f, out_f, True)
    assert res == EResult.Success

    pybgcode.close(out_f)

    assert filecmp.cmp(TEST_GCODE, TEST_REVERSE_GCODE, shallow=False)


if __name__ == "__main__":
    pytest.main()
