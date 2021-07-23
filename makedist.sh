#!/bin/bash
set -x
name=linux
version=6.8.0
tarballs=$name-$version-`date +%Y%m%d`.tar.xz

git archive --format=tar.xz --prefix=$name-$version/ -o $tarballs HEAD

if mountpoint /data/ >/dev/null 2>&1;then
    cp $tarballs /data/sources
fi
