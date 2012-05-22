#!/bin/sh

mkdir -p aux_scripts m4
libtoolize -c -i
aclocal -I m4 --install
autoheader
automake -a -c
autoconf

