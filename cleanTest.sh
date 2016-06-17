rm flopoco.vhdl flopoco.o e~testbench_* testbench_* TestBench_* temp ghdl work-obj93.cf > /dev/null 2>&1
nbTests=$(grep -c flopoco report)
nbErrors=$(grep -c ERROR report)
nbSuccess=$(grep -c SUCCESS report)
nbSuccess=$((nbSuccess-1))
rateError=$((nbErrors/$nbTests*100))
rateSuccess=$((nbSuccess/$nbTests*100))
echo "Error : $rateError%"
echo "Success : $rateSuccess%"
echo "See report for details"