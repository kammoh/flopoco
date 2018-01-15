mkdir TestResults/$1 > /dev/null
mkdir TestResults/tmp > /dev/null 2>&1
echo -n "" >> TestResults/$1/report
echo -n "" >> TestResults/$1/messages
echo "Testing Operator : $1" >> TestResults/report
echo "Testing Operator : $1"
