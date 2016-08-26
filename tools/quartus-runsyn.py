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
#    parser.add_argument('-i', '--implement', action='store_true', help='Go all the way to implementation (default stops after synthesis)')
    parser.add_argument('-f', '--file', help='File name (default flopoco.vhdl)')
    parser.add_argument('-e', '--entity', help='Entity name (default is last entity of the VHDL file)')
    parser.add_argument('-t', '--target', help='Target name (default is StratixV)')

    options=parser.parse_args()

    if (options.file==None):
        filename = "flopoco.vhdl"
    else:
        filename = options.file

    if (options.entity==None):
        entity = get_last_entity(filename)
    else:
        entityname=options.entity
        
    if (options.target==None):
        target = "stratixV"
    else:
        target=options.target
        

    filename_abs = os.path.abspath(filename)
    os.system("quartus_map --source="+filename + " --family="+target + " " + entity)
    os.system("quartus_fit " + entity)


