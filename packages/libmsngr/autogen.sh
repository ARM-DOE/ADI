#!/bin/sh
set -e
mkdir -p m4
autoreconf --install --force --verbose
rm -rf autom4te.cache
