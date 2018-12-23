#!/bin/bash
if [ "$TRAVIS_OS_NAME" == "linux" ]
then
	pip install --user cpp-coveralls gcovr
fi

# Build libgit2
mkdir -p $HOME/deproot
wget https://github.com/libgit2/libgit2/archive/v0.26.8.tar.gz
tar -xzf v0.26.8.tar.gz
mkdir -p libgit2-build && cd libgit2-build
cmake -DCMAKE_INSTALL_PREFIX=$HOME/deproot --build=. ../libgit2-0.26.8
make && make install
cd .. ; rm -rf libgit2-build libgit2-0.26.8

