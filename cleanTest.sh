rm flopoco.vhdl flopoco.o e~testbench_* testbench_* TestBench_* temp ghdl work-obj93.cf > /dev/null 2>&1
nbTests=$(grep -c flopoco report)
nbErrors=$(grep -c ERROR report)
nbSuccess=$(grep -c SUCCESS report)
nbSuccess=$((nbSuccess-1))
nbVHDL=$(grep -c "VHDL generated" report)
rateError=$(((nbErrors*100)/nbTests))
rateSuccess=$(((nbSuccess*100)/nbTests))
rateVHDL=$(((nbVHDL*100)/nbTests))
echo "VHDL : $rateVHDL% generated"
echo "Error : $rateError%"
echo "Success : $rateSuccess%"
echo "See report for details"