##
################################################################################
##             Compilation and Result Generation script for Vivado
## This tool is part of  FloPoCo
## Author:  Florent de Dinechin, 2015
## All rights reserved
################################################################################
import os
import sys
import re
import string
import subprocess
import argparse

def usage():
    print "Usage: \nvivado-runsyn\n" 
    sys.exit()

    
def get_last_entity(filename):
    vhdl=open(filename).read()
    endss = [match.end() for match in re.finditer("entity", vhdl)] # list of endpoints of match of "entity"
    last_entity_name_start = endss[-2] +1 # skip the space
    i = last_entity_name_start
    while(vhdl[i]!=" "):
        i=i+1
    last_entity_name_end = i
    entityname=vhdl[last_entity_name_start:last_entity_name_end]
    return entityname



#/* main */
if __name__ == '__main__':


    parser = argparse.ArgumentParser(description='This is an helper script for FloPoCo that launches Xilinx Vivado and extracts resource consumption and critical path information')
    parser.add_argument('-i', '--implement', action='store_true', help='Go all the way to implementation (default stops after synthesis)')
    parser.add_argument('-f', '--file', help='File name (default flopoco.vhdl)')
    parser.add_argument('-e', '--entity', help='Entity name (default is last entity of the VHDL file)')

    options=parser.parse_args()

    if (options.implement==True):
        synthesis_only=False
    else:
        synthesis_only=True

    if (options.file==None):
        filename = "flopoco.vhdl"
    else:
        filename = options.file

    if (options.entity==None):
        entity = get_last_entity(filename)
    else:
        entityname=options.entity
        

    workdir="/tmp/vivado_runsyn_files"
    project_name = "test_" + entity
    tcl_script_name = os.path.join(workdir, project_name+".tcl")
    filename_abs = os.path.abspath(filename)
    os.system("rm -R "+workdir)
    os.mkdir(workdir)
    os.chdir(workdir)


    # First futile attempt to get a timing report

    xdc_file_name="/tmp/clock.xdc" # created by FloPoCo.

    tclscriptfile = open( tcl_script_name,"w")
    tclscriptfile.write("# Synthesis of " + entity + "\n")
    tclscriptfile.write("# Generated by FloPoCo's vivado-runsyn.py utility\n")
    tclscriptfile.write("create_project " + project_name + " -part xc7z020clg484-1\n")
    tclscriptfile.write("set_property board_part em.avnet.com:zed:part0:1.3 [current_project]\n")
    tclscriptfile.write("add_files -norecurse " + filename_abs + "\n")
    tclscriptfile.write("read_xdc " + xdc_file_name + "\n")
    tclscriptfile.write("update_compile_order -fileset sources_1\n")
    tclscriptfile.write("update_compile_order -fileset sim_1\n")

    if synthesis_only:
        result_name = "synth_1"
    else:
        result_name = "impl_1"
        
    tclscriptfile.write("launch_runs " + result_name + "\n")
    tclscriptfile.write("wait_on_run " + result_name + "\n")
    tclscriptfile.write("open_run " + result_name + " -name " + result_name + "\n")
#    tclscriptfile.write("report_timing_summary -delay_type max -report_unconstrained -check_timing_verbose -max_paths 10 -input_pins -file " + os.path.join(workdir, project_name+"_timing_report.txt") + " \n")

    # Timing report
    timing_report_file = workdir + "/" + project_name + ".runs/" + result_name + "/" + entity + "_timing_"
    if synthesis_only:
        timing_report_file +="synth.rpt"
    else:
        timing_report_file +="placed.rpt"

    tclscriptfile.write("report_timing -file " + timing_report_file + " \n")
        
    tclscriptfile.close()

    vivado_command = ("vivado -mode batch -source " + tcl_script_name)
    print vivado_command
    os.system(vivado_command)

    # Reporting
    utilization_report_file = workdir + "/" + project_name + ".runs/" + result_name + "/" + entity + "_utilization_"
    if synthesis_only:
        utilization_report_file +="synth.rpt"
    else:
        utilization_report_file +="placed.rpt"

    print("cat " + utilization_report_file)
    os.system("cat " + utilization_report_file)
    print("cat " + timing_report_file)
    os.system("cat " + timing_report_file)

    
    
    
#    p = subprocess.Popen(vivado_command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, close_fds=True)
