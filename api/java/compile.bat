@echo off
mkdir dist
cd src
javac com/gga/indigo/*.java
jar cvf ../dist/indigo-java.jar com/gga/indigo/*.class
cd ..