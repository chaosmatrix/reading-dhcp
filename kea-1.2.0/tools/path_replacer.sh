#!/bin/sh
# Copyright (C) 2014-2016 Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# This script replaces /usr/local, ${prefix}/var and other automake/autoconf
# variables with their actual content.
#
# Invocation:
#
# ./path_replacer.sh input-file.in output-file
#
# This script is initially used to generate configuration files, but it is
# generic and can be used to generate any text files.
#

prefix=/usr/local
sysconfdir=${prefix}/etc
localstatedir=${prefix}/var

echo "Replacing \@prefix\@ with ${prefix}"
echo "Replacing \@sysconfdir\@ with ${sysconfdir}"
echo "Replacing \@localstatedir\@ with ${localstatedir}"

echo "Input file: $1"
echo "Output file: $2"

sed -e "s+\@localstatedir\@+${localstatedir}+g; s+\@prefix\@+${prefix}+g; s+\@sysconfdir\@+${sysconfdir}+g" $1 > $2
