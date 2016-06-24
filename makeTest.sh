if make > message 2>&1 ; then
	echo 'Compilation SUCCESS'
	rm -r tests/*
	mkdir tests/tmp
	exit 0
else
	echo 'Compilation FAILED'
	exit 1
fi
