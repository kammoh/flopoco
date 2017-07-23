cd TestResults/tmp
echo "-------------------------------------------" >> ../$1/report
echo "./flopoco $*"
echo "./flopoco $*" >> ../$1/report
echo "./flopoco $*" >> ../$1/messages
../../flopoco $* > temp 2>&1
if grep 'gtkwave' temp > /dev/null; then
	echo -n 'VHDL generated' >> ../$1/report
	nvc=$(grep 'nvc  -a' temp)
	echo $nvc >> ../$1/report
	if $nvc >> ../$1/messages 2>&1; then
			message='nvc simulation succeeded'
	else
			echo $nvc
			message='nvc simulation ERROR'
	fi
else
	message = 'VHDL not generated'
fi
echo -n $message >> ../$1/report
echo  $message
