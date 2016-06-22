echo "-------------------------------------------" >> report
echo "./flopoco $*" >> report
./flopoco $* > temp 2>&1
if grep 'gtkwave' temp > /dev/null; then
	echo -n 'VHDL generated | ' >> report
	ghdl=$(grep 'ghdl -a' temp)
	if $ghdl >> message 2>&1; then	
		echo -n 'GHDL -a SUCCESS | ' >> report
	else
		echo -n 'GHDL -a ERROR | ' >> report
	fi
	ghdl=$(grep 'ghdl -e' temp)
	if $ghdl >> message 2>&1; then
		echo -n 'GHDL -e SUCCESS | ' >> report
	else
		echo -n 'GHDL -e ERROR | ' >> report
	fi

	ghdl=$(grep 'ghdl -r' temp)
	$ghdl > ghdl 2>&1
	nbError=$(grep -c error ghdl)
	normalNbError=1
	if grep '0 error(s) encoutered' ghdl > /dev/null; then
		if [ $nbError -eq $normalNbError ]; then
			echo 'GHDL -r SUCCESS' >> report
		else
			echo 'GHDL -r ERROR' >> report
			cat ghdl > message
		fi
	fi
else
	echo 'VHDL not generated' >> report
fi