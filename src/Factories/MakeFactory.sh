#! /bin/sh
#Such Inception

cd $1
sed "s/.*/	DEF(\0)\;/" Interfaced.txt > headers.txt
# elt -> DEF(elt);
sed "s/.*/			\0::registerFactory()\;/" Interfaced.txt > registration.txt

cat head.txt headers.txt middle.txt registration.txt tail.txt > ../Factories.cpp

rm headers.txt registration.txt
