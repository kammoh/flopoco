rm tests/tmp/*
nbTests=$(grep -c flopoco tests/$1/report)
nbErrors=$(grep -c ERROR tests/$1/report)
nbSuccess=$(grep -c SUCCESS tests/$1/report)
nbSuccess=$((nbSuccess))
nbVHDL=$(grep -c "VHDL generated" tests/$1/report)
rateError=$(((nbErrors*100)/nbTests))
rateSuccess=$(((nbSuccess*100)/nbTests))
rateVHDL=$(((nbVHDL*100)/nbTests))
echo "VHDL : $rateVHDL% generated"
echo "Error : $rateError%"
echo "Success : $rateSuccess%"
echo "See report for details"