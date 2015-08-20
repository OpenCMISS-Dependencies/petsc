#!/usr/bin/env python

# Import the os module, for the os.walk function
import os
import re
from sys import stdout
from collections import OrderedDict
 
# Set the directory you want to start from
rootDir = os.getcwd()
datafiles = os.path.join(rootDir,"share","petsc","datafiles") 
 
# Expressions
re_binaries = re.compile("ex(\d+)(f?): ex\d+f?.o\s*chkopts") 
re_tests = re.compile("-@\$\{MPIEXEC\} -n (\d+) ./ex(?P<nr>\d+)f? ([^>]*)> ex(?P=nr)_?(\d+?).tmp 2>&1;")
# re_tests = re.compile("-@\$\{MPIEXEC\} -n (\d+) ./ex(?P<nr>\d+)f? ([^>|]*)(?:-@if[^>|]+)> ex(?P=nr)_?(\d+?).tmp 2>&1;")
 
def getExeName(base, nr):
    return "ex_" + base + "_" + nr

def getTestName(base, nr, nproc, subnr):
    sub = "_" + subnr if subnr else ""    
    return "test_" + base + "_" + nr + "_np" + nproc + sub 

def processArgs(args):
    args = args.replace("\n", "").replace("\\", "").replace("\t", "")
    # Remove |grep and -@if [MATLAB stuff
    m = re.match("(.*)(?:\||-@).*", args, re.MULTILINE)
    if m:
        #print "Replacing '" + args + "' with '" + m.group(1) + "'"
        args = m.group(1)
    # Insert the correct data path
    args = args.replace("${DATAFILESPATH}",datafiles)
    return args

folders_with_tests = []
allexecs = []
alltests = []
missing_execs = 0
for dirName, subdirList, fileList in os.walk(rootDir):
    m = re.match("(.*)src/(.*)/examples/tests", dirName)
    if m:
        # Get relative folder to working directory
        folder = m.group(0).replace(m.group(1), "")
        if folder not in folders_with_tests:
            makefile = m.group(0) + "/makefile"
            # print folder
         
            cml = open(os.path.join(folder,"CMakeLists.txt", 'w'))
            #cml = stdout
            base = m.group(2).replace("/", "_")
            # cml.write("set(BASENAME " + basename + ")\n\r")
            
            # Read makefile
            print "Reading makefile " + makefile
            mf = open(makefile, 'r')
            mfcont = mf.read()
            mf.close()
            
            tests_found = 0
            # Write executable list
            testbins = re_binaries.findall(mfcont, re.MULTILINE)
            for match in testbins:
                # print match
                exname = getExeName(base, match[0])
                ext = 'F' if match[1] == 'f' else 'c'
                cml.write("add_executable({} ex{}.{})\n".format(exname,match[0],ext))
                cml.write("target_link_libraries({} petsc)\n".format(exname))
                allexecs.append(exname)
            
            # Write test list
            tests = re_tests.findall(mfcont, re.MULTILINE)
            for match in tests:
                # print match
                nproc = match[0]
                exname = getExeName(base, match[1])
                testname = getTestName(base, match[1], nproc, match[3])
                if exname in allexecs:
                    args = processArgs(match[2]);
                
                    cml.write("ADDTEST({} {} {} {})\n".format(testname,nproc,exname,args))
                    alltests.append(testname)
                else:
                    missing_execs += 1
                    print "Binary {} has not been created, skipping test {}".format(exname,testname)
                    
    
            if len(alltests) > 0:
                folders_with_tests.append(folder)
                
            cml.close()
        
for fo in folders_with_tests:
    stdout.write("add_subdirectory("+fo+")\n")
stdout.write("Conversion done. Created {} executables and {} tests. Missing {} executables for further tests\n".format(len(allexecs),len(alltests),missing_execs))
