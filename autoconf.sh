#!/bin/sh
aclocal
libtoolize -f -c
autoheader
autoconf
automake -a
