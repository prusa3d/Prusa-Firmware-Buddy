# OpenPrintTag
This repository contains specification, documentation and utility scripts for the [OpenPrintTag](https://openprinttag.org) format.

**This is a "raw" repository, you can access the compiled documentation on [specs.openprinttag.org](https://specs.openprinttag.org)**

(or use generate_docs.sh to generate a website into the docs folder)

## Directory structure
* `data`: Machine-readable specification data (field & enum definitions, ...)
* `docs_src`: Source code for the [specs.openprinttag.org](https://specs.openprinttag.org) website
* `tests`: Tests
* `utils`: Reference implementation for the format in Python

## Generating documentation
To generate documentation (to the `docs` directory), run:
```python
pip3 install -r requirements.txt
sh generate_docs.sh
```

Then, to view it, you can:
```
cd docs
python3 -m http.server
```
and open your browser on `127.0.0.1:8000`
