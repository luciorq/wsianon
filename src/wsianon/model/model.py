import ctypes
from enum import Enum
from typing import Optional, List

class Vendor(Enum):
    APERIO = 0
    HAMAMATSU = 1
    MIRAX = 2
    VENTANA = 3
    PHILIPS_ISYNTAX = 4
    PHILIPS_TIFF = 5
    UNKNOWN = 6
    INVALID = 7

class MetadataAttribute(ctypes.Structure):
    _fields_ = [
        ("key", ctypes.c_char_p),
        ("value", ctypes.c_char_p)
    ]

class Metadata(ctypes.Structure):
    _fields_ = [
        ("metadataAttributes", ctypes.POINTER(MetadataAttribute)),
        ("length", ctypes.c_size_t)
    ]

class WSIData(ctypes.Structure):
    _fields_ = [
        ("format", ctypes.c_int8),
        ("filename", ctypes.c_char_p),
        ("metadata", ctypes.POINTER(Metadata))
    ]