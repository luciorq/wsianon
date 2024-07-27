# wsianon: Whole Slide Images Anonymization Library

This project provides a Python wrapper for the EMPAIA WSI Anon C library,
enabling the anonymization of Whole Slide Imaging (WSI) data. The library
supports various WSI formats and offers functionalities to anonymize and
retrieve metadata from WSI files.

## Main Features

- Anonymize WSI files by modifying metadata.
- Retrieve metadata from WSI files.
- Support for multiple WSI formats including SVS, NDPI, and more.


## How to install

Directly from GitHub:

```bash
python -m pip install git+https://github.com/luciorq/wsianon.git
```

<!--
```bash
git clone https://github.com/luciorq/wsianon.git
cd wsianon
python -m pip install .
```
-->

```python
import wsianon

# Anonymize a WSI file
wsianon.anonymize_wsi("path/to/input.svs|ndpi|mrsx", new_label_name="anonymized_wsi")
```

## Acknowledgements

This project is based on the EMPAIA WSI Anon C library. Special thanks to the contributors of the original project.

## Development

For development instructions,
refer to the documentation [docs/DEVELOPMENT.md](./docs/DEVELOPMENT.md).

