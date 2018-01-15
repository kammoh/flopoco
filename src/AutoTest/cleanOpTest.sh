rm -R TestResults/tmp
nbTests=$(grep -c './flopoco' TestResults/$1/report)
nbErrors=$(grep -c ERROR TestResults/$1/report)
nbVHDL=$(grep -c "VHDL generated"  TestResults/$1/report)
nbSuccess=$((nbVHDL-nbErrors))
rateError=$(((nbErrors*100)/nbTests))
rateSuccess=$(((nbSuccess*100)/nbTests))
rateVHDL=$(((nbVHDL*100)/nbTests))
echo "VHDL : $rateVHDL% generated" >> TestResults/report
echo "Simulation: $rateError%   failures" >> TestResults/report
echo "            $rateSuccess% success" >> TestResults/report
echo "See report in TestResults/$1 for details" >> TestResults/report
echo "VHDL : $rateVHDL% generated"
echo "Simulation success rate:   $rateSuccess% "
echo "See report in TestResults/$1 for details"
