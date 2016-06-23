echo "-------------------------------------------" >> tests/$1/report
echo "./flopoco $*" >> tests/$1/report
./flopoco $* > temp 2>&1
if grep 'gtkwave' temp > /dev/null; then
	echo -n 'VHDL generated | ' >> tests/$1/report
	ghdl=$(grep 'ghdl -a' temp)
	if $ghdl >> tests/$1/message 2>&1; then	
		echo -n 'GHDL -a SUCCESS | ' >> tests/$1/report
	else
		echo -n 'GHDL -a ERROR | ' >> tests/$1/report
	fi
	ghdl=$(grep 'ghdl -e' temp)
	if $ghdl >> tests/$1/message 2>&1; then
		echo -n 'GHDL -e SUCCESS | ' >> tests/$1/report
	else
		echo -n 'GHDL -e ERROR | ' >> tests/$1/report
	fi

	ghdl=$(grep 'ghdl -r' temp)
	$ghdl > ghdl 2>&1
	nbError=$(grep -c error ghdl)
	normalNbError=1
	if grep '0 error(s) encoutered' ghdl > /dev/null; then
		if [ $nbError -eq $normalNbError ]; then
			echo 'GHDL -r SUCCESS' >> tests/$1/report
		else
			echo 'GHDL -r ERROR' >> tests/$1/report
			cat ghdl > tests/$1/message
		fi
	else
		echo 'GHDL -r ERROR' >> tests/$1/report
		cat ghdl > tests/$1/message
	fi
else
	echo 'VHDL not generated' >> tests/$1/report
fi