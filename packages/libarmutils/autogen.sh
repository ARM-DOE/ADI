#!/bin/sh
set -e
autoreconf --install --force --verbose
rm -rf autom4te.cache
