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
python -m pip install --upgrade 'git+https://github.com/luciorq/wsianon.git'
```

```python
import wsianon

# Anonymize a WSI file
wsianon.anonymize_wsi("path/to/input.svs|ndpi|mrsx", new_label_name="anonymized_wsi")
```

From the command line

```bash
python -m wsianon --input 'path/to/input.svs|ndpi|mrsx' --label "anonymized_wsi"
```

Or simply if installed in the right path

```bash
wsianon --input 'path/to/input.svs|ndpi|mrsx' --label "anonymized_wsi"
```

## Acknowledgements

This project is based on the EMPAIA WSI Anon C library. Special thanks to the contributors of the original project.

## Development

For development instructions,
refer to the documentation [docs/DEVELOPMENT.md](./docs/DEVELOPMENT.md).

```bash
pixi init;
pixi add python pip;
pixi run python -m pip install -e ".[examples,docs,tests]";
```