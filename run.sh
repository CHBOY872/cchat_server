#!/bin/sh

cd src
make all
cd ..
mv src/main .
chmod +x main