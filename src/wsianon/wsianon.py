import ctypes
import threading
import glob
import pathlib

# from typing import Optional

from wsianon.model import WSIData

lock = threading.Lock()

""" def _load_library() -> ctypes.CDLL:
    system = platform.system()
    lib_name = {
        'Linux': 'libwsianon.so',
        'Darwin': 'libwsianon.dylib',
        'Windows': 'libwsianon.dll'
    }.get(system)

    if not lib_name:
        raise ModuleNotFoundError(
            f"Unsupported operating system: {system}. Supported systems are Linux, macOS, and Windows."
        )

    try:
        if system == 'Windows':
            return ctypes.WinDLL(lib_name)
        else:
            return ctypes.cdll.LoadLibrary(lib_name)
    except FileNotFoundError:
        raise ModuleNotFoundError(
            f"Could not locate {lib_name}. Please ensure the shared library is correctly installed."
        )
 """

def _load_library() -> ctypes.CDLL:
    current_file = pathlib.Path(__file__)
    libwsianon_path = glob.glob(str(
        pathlib.Path(current_file.absolute().parent / "libwsianon*")
    ))[0]
    return ctypes.CDLL(libwsianon_path)


_wsi_anonymizer = _load_library()

def get_wsi_data(filename: str) -> WSIData:
    """
    Retrieve all necessary WSI data from the slide.

    Args:
        filename (str): The path to the WSI file.

    Returns:
        WSIData: The WSI data structure.
    """
    _wsi_anonymizer.get_wsi_data.argtypes = [ctypes.c_char_p]
    _wsi_anonymizer.get_wsi_data.restype = ctypes.c_void_p

    c_filename = filename.encode('utf-8')
    wsi_data = WSIData.from_address(
        _wsi_anonymizer.get_wsi_data(ctypes.c_char_p(c_filename))
    )
    return wsi_data

def anonymize_wsi(
    filename: str,
    new_label_name: str,
    keep_macro_image: bool = False,
    disable_unlinking: bool = False,
    do_inplace: bool = False
) -> int:
    """
    Perform anonymization on the slide.

    Args:
        filename (str): The path to the WSI file.
        new_label_name (str): The new label name.
        keep_macro_image (bool, optional): Whether to keep the macro image. Defaults to False.
        disable_unlinking (bool, optional): Whether to disable unlinking. Defaults to False.
        do_inplace (bool, optional): Whether to perform the operation in place. Defaults to False.

    Returns:
        int: Result of the anonymization process.
    """
    _wsi_anonymizer.anonymize_wsi.argtypes = [
        ctypes.c_char_p, ctypes.c_char_p,
        ctypes.c_bool, ctypes.c_bool, ctypes.c_bool
    ]
    _wsi_anonymizer.anonymize_wsi.restype = ctypes.c_void_p

    c_filename = filename.encode('utf-8')
    c_new_label_name = new_label_name.encode('utf-8')

    with lock:
        result = _wsi_anonymizer.anonymize_wsi(
            c_filename,
            c_new_label_name,
            keep_macro_image,
            disable_unlinking,
            do_inplace
        )

    return result

"""
CLI script
"""

def main() -> int:
    import argparse
    parser = argparse.ArgumentParser(
        description='Anonymize Whole Slide Images'
    )
    parser.add_argument(
        '--input', type = str, required = True,
        help='the path to the WSI file'
    )
    parser.add_argument(
        '--label', type = str, required = True,
        help='the new label name'
    )
    parser.add_argument(
       "-m", "--macro", action = "store_true",
       help = "keep the macro image"
    )
    args = parser.parse_args()
    return_value = anonymize_wsi(
        filename = args.input,
        new_label_name = args.label,
        keep_macro_image = args.macro
    )
    return return_value
