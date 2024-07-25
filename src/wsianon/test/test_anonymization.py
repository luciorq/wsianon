import os
import pathlib
import shutil
import threading
import time
import pytest
import openslide
import tiffslide

from ..wsianon import get_wsi_data, anonymize_wsi
from ..model.model import Vendor

lock = threading.Lock()

@pytest.fixture(scope='session', autouse=True)
def cleanup():
    temporary_files = []
    def add_filename(filename):
        temporary_files.append(filename)

    yield add_filename

    for filename in set(temporary_files):
        remove_file(filename=filename)


def remove_file(filename):
    with lock:
        if filename.endswith(".mrxs"):
            mrxs_path = filename[:len(filename)-5]
            shutil.rmtree(mrxs_path)
        os.remove(filename)


@pytest.fixture(autouse=True)
def wait_between_tests():
    yield
    time.sleep(1)


def wait_until_exists(filename: str, max_wait_in_sec: int):
    while(max_wait_in_sec > 0):
        if pathlib.Path(filename).exists():
            return True
        time.sleep(1)
        max_wait_in_sec -= 1
    return False

@pytest.mark.parametrize(
    "wsi_filename, vendor",
    [
        ("/data/Aperio/CMU-1.svs", Vendor.APERIO),
        ("/data/Aperio/aperio_gt450_v1.0.1.svs", Vendor.APERIO),
        ("/data/Aperio/aperio_at2_v12.0.11.svs", Vendor.APERIO),
        ("/data/Aperio/aperio_at2_v12.0.0.svs", Vendor.APERIO),
        ("/data/Hamamatsu/OS-1.ndpi", Vendor.HAMAMATSU),
        ("/data/Hamamatsu/Hamamatsu_greater_4gb.ndpi", Vendor.HAMAMATSU),
        ("/data/Ventana/OS-2.bif", Vendor.VENTANA),
        ("/data/Ventana/dp600.tif", Vendor.VENTANA),
        ("/data/Philips iSyntax/4399.isyntax", Vendor.PHILIPS_ISYNTAX),
        ("/non_existing_file.txt", Vendor.INVALID),
    ],
)
def test_format_get_wsi_data(wsi_filename, vendor):
    wsi_data = get_wsi_data(wsi_filename)
    assert Vendor(wsi_data.format) == vendor

@pytest.mark.parametrize(
    "wsi_filepath, original_filename, new_anonyimized_name, file_extension",
    [
        ("/data/Aperio/", "CMU-1", "anon-aperio", "svs"),
    ],
)
def test_anonymize_aperio_format_tiffslide(cleanup, wsi_filepath, original_filename, new_anonyimized_name, file_extension):
    result_filename = pathlib.Path(wsi_filepath).joinpath(f"{new_anonyimized_name}.{file_extension}")
    if result_filename.exists():
        remove_file(str(result_filename.absolute()))

    wsi_filename = str(pathlib.Path(wsi_filepath).joinpath(f"{original_filename}.{file_extension}").absolute())
    result = anonymize_wsi(wsi_filename, new_anonyimized_name)
    assert result != -1

    assert wait_until_exists(str(result_filename), 5)

    try:
        slide = tiffslide.TiffSlide(result_filename)
        associated_images = slide.associated_images
        assert "label" not in associated_images

        if "Aperio" in wsi_filepath:
            for property in ["Filename", "User", "Time", "Date", "Slide", "Barcode", "Rack", "ScanScope ID"]:
                if property == "Time":
                    assert("00:00:00" == slide.properties[f"aperio.{property}"])
                elif property == "Date":
                    assert("01/01/00" == slide.properties[f"aperio.{property}"])
                elif f"aperio.{property}" in slide.properties.keys():
                    if property == "Slide":
                        assert("0" == slide.properties[f"aperio.{property}"])
                    else:
                        assert all(c=='X' for c in slide.properties[f"aperio.{property}"])
                else:
                    pass
        slide.close()
    except tiffslide.TiffFileError as e:
        assert False

    cleanup(str(result_filename.absolute()))

@pytest.mark.parametrize(
    "wsi_filepath, original_filename, new_anonyimized_name, file_extension",
    [
        ("/data/Hamamatsu/", "Hamamatsu_greater_4gb", "anon-hamamatsu", "ndpi"),
    ],
)
def test_anonymize_large_files_openslide(cleanup, wsi_filepath, original_filename, new_anonyimized_name, file_extension):
    result_filename = pathlib.Path(wsi_filepath).joinpath(f"{new_anonyimized_name}.{file_extension}")
    if result_filename.exists():
        remove_file(str(result_filename.absolute()))

    wsi_filename = str(pathlib.Path(wsi_filepath).joinpath(f"{original_filename}.{file_extension}").absolute())
    result = anonymize_wsi(wsi_filename, new_anonyimized_name)
    assert result != -1

    assert wait_until_exists(str(result_filename), 5)

    with openslide.OpenSlide(str(result_filename)) as slide:
        assert "label" not in slide.associated_images

        if "Hamamatsu" in wsi_filepath:
            for property in ["DateTime"]:
                assert("1900:01:01 00:00:00" == slide.properties[f"tiff.{property}"])

    cleanup(str(result_filename.absolute()))


@pytest.mark.parametrize(
    "wsi_filepath, original_filename, new_anonyimized_name, file_extension",
    [
        ("/data/Hamamatsu/", "OS-1", "anon-hamamatsu2", "ndpi"),
        ("/data/Ventana/", "OS-2", "anon-ventana", "bif"),
        ("/data/Ventana/", "dp600", "anon-ventana2", "tif"),
    ],
)
def test_anonymize_file_format_openslide(cleanup, wsi_filepath, original_filename, new_anonyimized_name, file_extension):
    result_filename = pathlib.Path(wsi_filepath).joinpath(f"{new_anonyimized_name}.{file_extension}")
    if result_filename.exists():
        remove_file(str(result_filename.absolute()))

    wsi_filename = str(pathlib.Path(wsi_filepath).joinpath(f"{original_filename}.{file_extension}").absolute())
    result = anonymize_wsi(wsi_filename, new_anonyimized_name)
    assert result != -1

    assert wait_until_exists(str(result_filename), 5)

    with openslide.OpenSlide(str(result_filename)) as slide:
        assert "label" not in slide.associated_images

        if "MIRAX" in wsi_filepath:
            assert "macro" not in slide.associated_images
            for property in ["SLIDE_NAME", "PROJECT_NAME", "SLIDE_CREATIONDATETIME"]:
                if property != "SLIDE_CREATIONDATETIME":
                    assert all(c == "X" for c in slide.properties[f"mirax.GENERAL.{property}"])
                else:
                    assert("01/01/00 00:00:00" == slide.properties[f"mirax.GENERAL.{property}"])
            assert all(c == "0" for c in slide.properties[f"mirax.GENERAL.SLIDE_ID"])

        if "Ventana" in wsi_filepath:
            for property in ["UnitNumber", "UserName", "BuildDate"]:
                assert all(c == " " for c in slide.properties[f"ventana.{property}"])

        if "Hamamatsu" in wsi_filepath:
            for property in ["DateTime"]:
                assert("1900:01:01 00:00:00" == slide.properties[f"tiff.{property}"])

    cleanup(str(result_filename.absolute()))


@pytest.mark.parametrize(
    "wsi_filepath, original_filename, new_anonyimized_name, file_extension",
    [
        ("/data/Aperio/", "CMU-1", "anon-aperio5", "svs"),
        ("/data/Aperio/", "aperio_gt450_v1.0.1" , "anon-aperio6", "svs"),
        ("/data/Aperio/", "aperio_at2_v12.0.11", "anon-aperio7", "svs"),
        ("/data/Aperio/", "aperio_at2_v12.0.0", "anon-aperio8", "svs"),
    ],
)
def test_anonymize_file_format_only_label(cleanup, wsi_filepath, original_filename, new_anonyimized_name, file_extension):
    result_filename = pathlib.Path(wsi_filepath).joinpath(f"{new_anonyimized_name}.{file_extension}")
    if result_filename.exists():
        remove_file(str(result_filename.absolute()))

    wsi_filename = str(pathlib.Path(wsi_filepath).joinpath(f"{original_filename}.{file_extension}").absolute())
    result = anonymize_wsi(filename=wsi_filename, new_label_name=new_anonyimized_name, keep_macro_image=True, disable_unlinking=False, do_inplace=False)
    assert result != -1

    assert wait_until_exists(str(result_filename), 5)

    with openslide.OpenSlide(str(result_filename)) as slide:
        associated_images = slide.associated_images
        assert "label" not in associated_images
        assert "macro" in associated_images

    cleanup(str(result_filename.absolute()))

@pytest.mark.parametrize(
    "wsi_filepath, original_filename, new_anonyimized_name, file_extension",
    [
        ("/data/Hamamatsu/", "OS-1", "anon-hamamatsu3", "ndpi"),
        ("/data/Hamamatsu/", "Hamamatsu_greater_4gb", "anon-hamamatsu4", "ndpi"),
    ],
)
def test_anonymize_file_format_hamamatsu(cleanup, wsi_filepath, original_filename, new_anonyimized_name, file_extension):
    result_filename = pathlib.Path(wsi_filepath).joinpath(f"{new_anonyimized_name}.{file_extension}")
    if result_filename.exists():
        remove_file(str(result_filename.absolute()))

    wsi_filename = str(pathlib.Path(wsi_filepath).joinpath(f"{original_filename}.{file_extension}").absolute())
    result = anonymize_wsi(wsi_filename, new_anonyimized_name, keep_macro_image=True)
    assert result != -1

    assert wait_until_exists(str(result_filename), 5)

    with openslide.OpenSlide(str(result_filename)) as slide:
        assert "label" not in slide.associated_images
        assert "macro" not in slide.associated_images

    cleanup(str(result_filename.absolute()))

# TODO: remove comments when Philips folder was created and 'test.tiff' file was added
# @pytest.mark.parametrize(
#     "wsi_filepath, original_filename, new_anonyimized_name, file_extension",
#     [
#         ("/data/Philips/", "test", "anon-philips", "tiff"),
#     ],
# )
# def test_anonymize_file_only_metadata(cleanup, wsi_filepath, original_filename, new_anonyimized_name, file_extension):
#     result_filename = pathlib.Path(wsi_filepath).joinpath(f"{new_anonyimized_name}.{file_extension}")
#     if result_filename.exists():
#         remove_file(str(result_filename.absolute()))

#     wsi_filename = str(pathlib.Path(wsi_filepath).joinpath(f"{original_filename}.{file_extension}").absolute())
#     result = anonymize_wsi(wsi_filename, new_anonyimized_name)
#     assert result != -1

#     assert wait_until_exists(str(result_filename), 5)

#     with openslide.OpenSlide(str(result_filename)) as slide:
#         if "Philips_TIFF" in wsi_filepath:
#             for property in ["DICOM_DEVICE_SERIAL_NUMBER", "PIM_DP_UFS_BARCODE", "PIM_DP_SOURCE_FILE"]:
#                 assert all(c == "X" for c in slide.properties[f"philips.{property}"])
#             assert "19000101000000.000000" in slide.properties["philips.DICOM_ACQUISITION_DATETIME"]

#     cleanup(str(result_filename.absolute()))

# # TODO: both tests are not working at the moment
# # TODO: rename Philips iSyntax into Philips after placing Philips TIFF and iSyntax into Philips folder
# @pytest.mark.parametrize(
#     "wsi_filepath, original_filename, new_anonyimized_name, file_extension",
#     [
#         ("/data/Philips iSyntax/", "4399", "anon-philips", "isyntax"),
#         ("/data/MIRAX/", "Mirax2.2-1", "anon-mirax2", "mrxs"),
#     ],
# )
# def test_anonymize_file_format_basic(cleanup, wsi_filepath, original_filename, new_anonyimized_name, file_extension):
#     result_filename = pathlib.Path(wsi_filepath).joinpath(f"{new_anonyimized_name}.{file_extension}")
#     if result_filename.exists():
#         remove_file(str(result_filename.absolute()))

#     wsi_filename = str(pathlib.Path(wsi_filepath).joinpath(f"{original_filename}.{file_extension}").absolute())
#     result = anonymize_wsi(wsi_filename, new_anonyimized_name)
#     assert result != -1

#     assert wait_until_exists(str(result_filename), 5)

#     # TODO: add some ordinary checks here?

#     cleanup(str(result_filename.absolute()))