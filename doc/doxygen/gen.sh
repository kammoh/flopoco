#! /bin/sh

echo "INPUT =\\ " > activeops.txt
sed " 1 d ; $ d ; /#.*/d ; s/[[:space:]]//g ; /^$/d ; s/^/\.\.\/\.\.\\// ; s/$/.hpp \\\/" ../../ActiveOperators.txt >> activeops.txt
doxygen	Doxyfile
# rm activeops.txt
