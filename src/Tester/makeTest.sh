if make > TestResults/makeMessages 2>&1 ; then
	echo 'Compilation SUCCESS'
	rm -r TestResults/* > /dev/null 2>&1
	mkdir TestResults > /dev/null 2>&1
	mkdir TestResults/tmp > /dev/null 2>&1
	exit 0
else
	echo 'Compilation FAILED'
	exit 1
fi
