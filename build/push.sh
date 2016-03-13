#!/bin/bash

# mildly dangerous purge of old static content
rm -rf ../about
rm -rf ../css
rm -rf ../docs
rm -rf ../jekyll
rm -rf ../js
rm -rf ../downloads.html
rm -rf ../feed.xml
rm -rf ../index.html

# push new static content
cp -r _site/* ../
