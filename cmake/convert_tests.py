#!/usr/bin/env python

# Import the os module, for the os.walk function
import os
import re
import sys
from collections import OrderedDict
 
# Set the directory you want to start from
rootDir = os.getcwd()
datafiles = os.path.join(rootDir, "share", "petsc", "datafiles") 
 
# Expressions
re_binaries = re.compile("ex(?P<nr>\d+f?):\s+ex(?P=nr).o\s*chkopts") 
re_tests = re.compile("-@\$\{MPIEXEC\} -n (\d+) ./ex(?P<nr>\d+f?) ([^>]*)> ex(?P=nr)_?((?:\d+)?).tmp 2>&1;")
re_getnumbers = re.compile("ex(\d+f?).PETSc")
# This is the type of tests that should be extracted - just trying to build everything would require
# every possible 3rd-party software to be plugged-in!
find_examples_types = ["C", "C_X", "FORTRAN", "SUITESPARSE", "MUMPS", "PASTIX", "PARMETIS", "SUPERLU", "PTSCOTCH", "F2003", "F90"]
 
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
        # print "Replacing '" + args + "' with '" + m.group(1) + "'"
        args = m.group(1)
    # Insert the correct data path
    args = args.replace("${DATAFILESPATH}", datafiles)
    return args

debug = 0
#debug = 1
folders_with_tests = []
allexecs = []
alltests = []
missing_execs = 0
notmarked = 0
missing_sources = []
if debug:
    log = sys.stdout
else:
    if len(sys.argv) > 1 and os.path.isdir(sys.argv[1]):
        outdir = sys.argv[1]
    else:
        outdir = os.path.realpath(__file__)
    logfile = os.path.join(outdir, "conversion.log")    
    log = open(logfile, 'w')
for dirName, subdirList, fileList in os.walk(rootDir):
    m = re.match("(.*)src/(.*)/examples/tests", dirName)
    if m:
        # Get relative folder to working directory
        folder = m.group(0).replace(m.group(1), "")
        if folder not in folders_with_tests:
            makefile = m.group(0) + "/makefile"
         
            if debug:
                cml = sys.stdout
            else:
                #cml = open(os.devnull, 'w')
                cml = open(os.path.join(folder, "CMakeLists.txt"), 'w')
            
            base = m.group(2).replace("/", "_")
            
            # Read makefile
            log.write("Processing {}\n".format(makefile))
            mf = open(makefile, 'r')
            mfcont = mf.read()
            mf.close()
            
            # Sort out which tests/binaries to actually create
            # This is determined by the TESTEXAMPLES_XX variables at the bottom of every makefile.
            # We only convert the examples for "find_examples_types" (see above)
            selected_test_main_numbers = []
            for type in find_examples_types:
                # Eclipse: TESTEXAMPLES_[^=]*=(.*\\\R)*.*\R
                expr = ".*TESTEXAMPLES_" + type + "\s*=\s*((?:.*\\\\\n)*)(.*)\n"
                match = re.findall(expr, mfcont, re.MULTILINE)
                if match:
                    selected_test_main_numbers.extend(re_getnumbers.findall(match[0][0]+match[0][1]))
            
            #print selected_test_main_numbers
            # Write executable list
            testbins = re_binaries.findall(mfcont, re.MULTILINE)
            for match in testbins:
                #print "match (nr) = "+match
                if match in selected_test_main_numbers:
                    ext = 'F' if match.endswith('f') else 'c'
                    exname = getExeName(base, match)
                    src = "ex{}.{}".format(match, ext)
                    fullsrc = os.path.join(folder, src)
                    if os.path.isfile(fullsrc):
                        cml.write("add_executable({} {})\n".format(exname, src))
                        cml.write("target_link_libraries({} petsc)\n".format(exname))
                        allexecs.append(exname)
                    else:
                        log.write("Skipping binary {}, source {} not found.\n".format(exname, src))
                        missing_sources.append(fullsrc)
                else:
                    log.write("Skipping binary/test {}, not listed in TESTEXAMPLES section.\n".format(match))
                    notmarked += 1
            
            # Write test list
            tests = re_tests.findall(mfcont, re.MULTILINE)
            localcount = 1
            for match in tests:
                if match[1] in selected_test_main_numbers:
                    nproc = match[0]
                    exname = getExeName(base, match[1])
                    testname = getTestName(base, match[1], nproc, match[3])
                    while testname in alltests:
                        testname = (getTestName(base, match[1], nproc, match[3]) + "_v{}").format(localcount)
                        localcount += 1
                    if exname in allexecs:
                        args = processArgs(match[2]);
                        
                        cml.write("ADDTEST({} {} {} \"{}\")\n".format(testname, nproc, exname, args))
                        alltests.append(testname)
                    else:
                        missing_execs += 1
                        log.write("Binary {} has not been created, skipping test {}\n".format(exname, testname))
    
            if len(alltests) > 0:
                folders_with_tests.append(folder)
                
            if not debug:
                cml.close()
            
            #sys.exit(0)
        
# Create an inclusion file that only includes those subfolders that actually have tests created.
includefile = open(os.path.join(outdir,"IncludeTestFolders.cmake"),'w')
includefile.write("#@@@ THIS FILE HAS BEEN GENERATED BY convert_tests.py @@@".format(rootDir))    
for fo in folders_with_tests:
    includefile.write("add_subdirectory({})\n".format(fo))        
includefile.close() 
        
for src in missing_sources:
    log.write("Missing {}\n".format(src))
summary = """
    Conversion done.
    Created {} executables and {} tests.
    Missing: {} executables for detected tests, {} source files
    Skipped compilation of {} executables not listed in {}
    """.format(len(allexecs), len(alltests), missing_execs,
            len(missing_sources), notmarked, ",".join(find_examples_types))
log.writelines(summary)        
log.close()
sys.stdout.write(summary+"Complete log at {}\n".format(logfile))