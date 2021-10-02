#ADI Python Documentation

This file explains how the ADI Python docs are generated via an automated Sphinx build to
readthedocs.io.

**NOTE:**  We are using autoapi instead of autodoc extension in sphinx to automatically
crawl the source tree and generate docs because autodoc actually imports all the python,
so it can't run unless all the dependencies are met (i.e., needs full adi environment to build).
Autoapi just inspects the source, so it works better.

## Setup
The Sphinx project contained in this folder was set up following these instructions:
https://docs.readthedocs.io/en/stable/intro/getting-started-with-sphinx.html.  To build
locally, you need to install these three python packages:

```bash
pip install sphinx
pip install sphinx_rtd_theme
pip install sphinx-autoapi
```

# Building
```bash
make clean
make html
```