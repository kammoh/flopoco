if make > message 2>&1 ; then
	echo 'Compilation SUCCESS' > report
	exit 0
else
	echo 'Compilation FAILED' > report
	exit 1
fi
