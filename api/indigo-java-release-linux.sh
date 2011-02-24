#!/bin/sh

name=$1

if [ -z $name ]; then
  echo "specify name";
  exit;
fi

rm -rf ./$name 

mkdir $name

cp LICENSE.GPL $name/
cp java/dist/indigo-java.jar $name/
cp renderer/java/dist/indigo-renderer-java.jar $name/
cp ../common/jna/jna.jar $name/

zip -r -9 $name.zip $name
