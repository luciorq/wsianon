from setuptools import Extension, setup
#import os
#import platform
#import shutil
import glob

# Dynamically generate the list of .c files in the include directory
source_files = glob.glob('include/*.c')
files_to_remove = [
    'include/console-app.c', 'include/js-file.c',
    'include/wsi-anonymizer-wasm.c'
]
for f in files_to_remove:
    source_files.remove(f)

header_files = glob.glob('include/*.h')

setup(
    name = "wsianon",
    ext_modules=[
        Extension(
            name="wsianon.libwsianon",
            sources=source_files,
            language="c",
            dependencies=header_files,
            include_dirs=["src", "include"],
        ),
    ],
    entry_points = {
        'console_scripts': ['wsianon=wsianon:main']
    }
)
