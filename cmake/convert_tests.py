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
re_binaries = re.compile(r"ex(?P<nr>\d+f?):\s+ex(?P=nr).o\s*chkopts", re.MULTILINE)
re_tests = re.compile(r"(?:-@)?\$\{MPIEXEC\} -n (\d+) ./ex(?P<nr>\d+f?) ([^>]*)>>? ex(?P=nr)_?((?:\d+)?)\.tmp 2>&1", re.MULTILINE)
re_getnumbers = re.compile(r"ex(\d+f?).PETSc", re.MULTILINE)
re_vars = re.compile(r"^([A-Z]*)[ \t]*=[ \t]*(.*)\n", re.MULTILINE)
re_loops = re.compile(r"for (\w+) in \$\{(\w+)\}; do", re.MULTILINE)
re_loops_direct = re.compile(r"for (\w+) in ([^$][^;]*); do", re.MULTILINE)

#re_test = re.compile(r"(?:-@)?\$\{MPIEXEC\} -n (\d+) ./ex(?P<nr>\d+f?) ([^>]*)>>? ex(?P=nr)_?((?:\d+)?)\.tmp 2>&1", re.MULTILINE)
#print re_loops_direct.search("""""")
#print re_tests.findall("""""")
#sys.exit(0)

# This is the type of tests that should be extracted - just trying to build everything would require
# every possible 3rd-party software to be plugged-in!
find_examples_types = ["C", "C_X", "FORTRAN", "SUITESPARSE", "MUMPS",
                       "PASTIX", "PARMETIS", "SUPERLU", "PTSCOTCH", "F2003", "F90", "DATAFILESPATH"]
 
def getExeName(base, nr):
    return "run_" + base + "_" + nr

def getTestName(base, nr, nproc, subnr):
    sub = "_" + subnr if subnr else ""    
    return base + "_" + nr + "_np" + nproc + sub 

def processArgs(args, loopvars):
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
        outdir = os.path.dirname(os.path.realpath(__file__))
    logfile = os.path.join(outdir, "conversion.log")    
    log = open(logfile, 'w')
for dirName, subdirList, fileList in os.walk(rootDir):
    m = re.match(r"(.*)src/(.*)/examples/(tests|tutorials)", dirName)
    if m:
        # Get relative folder to working directory
        folder = m.group(0).replace(m.group(1), "")
        
        if debug and m.group(0) != "/home/dwirtz/software/opencmiss/src/dependencies/petsc/src/mat/examples/tests":
            continue
        
        if folder not in folders_with_tests:
            makefile = os.path.join(m.group(0), "makefile")
            
            if not os.path.isfile(makefile):
                log.write("Skipping folder {}, no 'makefile' found".format(m.group(0)))
                continue
         
            if debug:
                cml = sys.stdout
            else:
                # cml = open(os.devnull, 'w')
                cml = open(os.path.join(folder, "CMakeLists.txt"), 'w')
            
            base = m.group(2).replace("/", "_") + "_" + m.group(3)
            
            # Read makefile
            log.write("Processing {}\n".format(makefile))
            mf = open(makefile, 'r')
            mfcont = mf.read()
            mf.close()
            
            # Read variables & loop variables
            variter = re_vars.finditer(mfcont)
            vars = dict()
            for match in variter:
                if match.group(2):
                    #vars[match.group(1)] = filter(None,match.group(2).split(" "))
                    vars[match.group(1)] = match.group(2)
            
            #print vars
            # Read loop variables
            forvars = dict()
            variter = re_loops.finditer(mfcont)
            for match in variter:
                forvars[match.group(1)] = match.group(2)
            varall = re_loops_direct.findall(mfcont)
            for pos in range(len(varall)):
                tmp_varname = "DIRECTLY_SPECIFIED_{}".format(pos)
                vars[tmp_varname] = varall[pos][1] 
                forvars[varall[pos][0]] = tmp_varname 
            log.write("Found variables: {}\nFound loop variables: {}\n".format(vars,forvars))
            
            
            # Sort out which tests/binaries to actually create
            # This is determined by the TESTEXAMPLES_XX variables at the bottom of every makefile.
            # We only convert the examples for "find_examples_types" (see above)
            selected_test_main_numbers = []
            for type in find_examples_types:
                # Eclipse: TESTEXAMPLES_[^=]*=(.*\\\R)*.*\R
                expr = ".*TESTEXAMPLES_" + type + "\s*=\s*((?:.*\\\\\n)*)(.*)\n"
                match = re.findall(expr, mfcont, re.MULTILINE)
                if match:
                    selected_test_main_numbers.extend(re_getnumbers.findall(match[0][0] + match[0][1]))
            
            # print selected_test_main_numbers
            # Write executable list
            testbins = re_binaries.findall(mfcont)
            for match in testbins:
                # print "match (nr) = "+match
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
            tests = re_tests.findall(mfcont)
            for match in tests:
                localcount = 1
                if match[1] in selected_test_main_numbers:
                    nproc = match[0]
                    exname = getExeName(base, match[1])
                    testname = getTestName(base, match[1], nproc, match[3])
                    while testname in alltests:
                        testname = (getTestName(base, match[1], nproc, match[3]) + "_v{}").format(localcount)
                        localcount += 1
                    if exname in allexecs:
                        args = processArgs(match[2],forvars);
                        
                        # Check if we have a loop for variables!
                        close = 0
                        testnameloopextra = ''
                        for forvar in forvars:
                            if re.search("\$\$\{?"+forvar, args):
                                loopvals = vars[forvars[forvar]]
                                cml.write("foreach({} in {})\n".format(forvar, loopvals))
                                # Replace loop variables by CMake-style variables
                                args = args.replace("$$"+forvar,"${"+forvar+"}") # case for $$var
                                args = args.replace("$${"+forvar+"}","${"+forvar+"}") # case for $${var} - if the first match did not work, those are the cases
                                close += 1
                                testnameloopextra += "_"+forvar+"-${"+forvar+"}"
                        
                        cml.write("ADDTEST({}{} {} {} \"{}\")\n".format(testname, testnameloopextra, nproc, exname, args))
                        alltests.append(testname)
                        if not debug:
                            log.write("Added test {}. nProcs={}, args={}\n"
                                      .format(testname, nproc, args))
                        
                        # Close loops if any!    
                        for num in range(close):
                            cml.write("endforeach()\n")
                            
                    else:
                        missing_execs += 1
                        log.write("Binary {} has not been created, skipping test {}\n".format(exname, testname))
    
            if len(alltests) > 0:
                folders_with_tests.append(folder)
                
            if not debug:
                cml.close()
            
            # sys.exit(0)
        
# Create an inclusion file that only includes those subfolders that actually have tests created.
if debug:
    includefile = sys.stdout
else:
    includefile = open(os.path.join(outdir, "IncludeTestFolders.cmake"), 'w')
includefile.write("#@@@ THIS FILE HAS BEEN GENERATED BY convert_tests.py @@@\n".format(rootDir))    
for fo in folders_with_tests:
    includefile.write("add_subdirectory({})\n".format(fo))        
if not debug:
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
if not debug:
    sys.stdout.write(summary + "Complete log at {}\n".format(logfile))
