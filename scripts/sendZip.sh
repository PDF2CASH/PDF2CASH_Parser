#!/bin/bash

apt get install zip
apt get install curl
cd ..
zip -r  ./parser.zip ./build
curl -F 'file=@./parser.zip' https://pdf2cash-cloudupdater.herokuapp.com/api/system/parser/
