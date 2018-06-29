#! /bin/sh
HEADPATH="\.\.\/\.\.\/src\/"
echo "INPUT =\\ " > activeops.txt
sed "s/^/$HEADPATH/ ; s/$/.hpp \\\/" ../../src/Active.txt >> activeops.txt
doxygen	Doxyfile
rm activeops.txt
