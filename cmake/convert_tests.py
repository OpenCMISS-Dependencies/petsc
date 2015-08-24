#!/usr/bin/env python

# Import the os module, for the os.walk function
import os
import re
import sys
from collections import OrderedDict
 
global rootDir, numtotaltests, log, debug, binaries, current_folder, missing_sources, missing_execs, alltests, mfcont

# Set the directory you want to start from
rootDir = os.getcwd()
datafiles = os.path.join(rootDir, "share", "petsc", "datafiles") 
 
# Expressions
# Finds the whole text for a test
re_findtest = re.compile(r"^(runex[^:]*):\s*\n((?:(?:.|\n)(?!^runex|TESTEXAMPLES))*)", re.MULTILINE)
# Finds the test case names in the TESTEXAMPLES_XX lists
re_testcases = re.compile(r"(runex(?:[^ ]*))", re.MULTILINE)
# Finds each actual test call
re_tests = re.compile(r"(?:-@)?\$\{MPIEXEC\} -n (\d+) ./ex(?P<nr>\d+f?) ([^>]*)>>? ex(?P=nr)_?((?:\w+)?)\.tmp 2>&1(?:(?:[^o]*)output/([^.]*)\.out)?", re.MULTILINE)

re_binaries = re.compile(r"ex(?P<nr>\d+f?):\s+ex(?P=nr).o\s*chkopts", re.MULTILINE)

re_testnames = re.compile(r"^(runex(?:([^_f:]*)[^:]*)):", re.MULTILINE)
# re_getnumbers = re.compile(r"ex(\d+f?).PETSc", re.MULTILINE)

re_vars = re.compile(r"^([A-Z]*)[ \t]*=[ \t]*(.*)\n", re.MULTILINE)
re_loops = re.compile(r"for (\w+) in \$\{(\w+)\}; do", re.MULTILINE)
re_loops_direct = re.compile(r"for (\w+) in ([^$][^;]*); do", re.MULTILINE)


# re_test = re.compile(r"(?:-@)?\$\{MPIEXEC\} -n (\d+) ./ex(?P<nr>\d+f?) ([^>]*)>>? ex(?P=nr)_?((?:\d+)?)\.tmp 2>&1", re.MULTILINE)
# print re_loops_direct.search("""""")
# m = re_findtest.findall("""""")
# for ma in m:
#    print ma
# print len(m)
# sys.exit(0)

# This is the type of tests that should be extracted - just trying to build everything would require
# every possible 3rd-party software to be plugged-in!
find_examples_types = ["C", "C_X", "FORTRAN", "SUITESPARSE", "MUMPS", "SUPERLU_DIST",
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
    
    # Stubbornly have the tests use the 32 bit variants
    args = args.replace("${PETSC_INDEX_SIZE}", "32")
    args = args.replace("${PETSC_SCALAR_SIZE}", "64")
    args = args.replace("${PETSC_DIR}", rootDir)
    
    return args

def writeExecutableCMake(name, exname):
    # Find executable instruction in makefile
    m = re.search("^ex"+name+": *(.+)chkopts", mfcont, re.MULTILINE)
    # Find all sources (most are one, but some are two or more source files!)
    m = re.findall(" *([^.]*).o",m.group(1))
    sources = []
    for name in m:
        ext = 'F' if name.endswith('f') else 'c'
        src = "{}.{}".format(name,ext)
        fullsrc = os.path.join(current_folder, src)
        if os.path.isfile(fullsrc):
            sources.append(src)
        else:
            missing_sources.append(fullsrc)
    if len(sources) > 0:
        cml.write("add_executable({} {})\n".format(exname, " ".join(sources)))
        cml.write("target_link_libraries({} petsc)\n".format(exname))
        binaries.append(exname)
        return 1
    else:
        log.write("Skipping binary {}, some source files were not found.\n".format(exname, src))
        return 0

def parseTests(base,teststr,vars,forvars,curdir):
    # Write test list
    #print teststr
    tests = re_tests.findall(teststr)
    for match in tests:
        #print match
        localcount = 1
        
        nproc = match[0]
        exname = getExeName(base, match[1])
        if not exname in binaries:
            created = writeExecutableCMake(match[1], exname)
            if not created:
                missing_execs += 1
                continue
        
        fulltestname = getTestName(base, match[1], nproc, match[3])
        while fulltestname in alltests:
            fulltestname = (getTestName(base, match[1], nproc, match[3]) + "_v{}").format(localcount)
            localcount += 1
        
        args = processArgs(match[2], forvars);
        
        # Check if there's an output file to compare to
        outfile = os.path.join('output',"{}.out".format(match[4]))
        if not os.path.isfile(os.path.join(rootDir,curdir,outfile)):
            outfile = "FALSE"
        
        # Check if we have a loop for variables!
        close = 0
        testnameloopextra = ''
        for forvar in forvars:
            if re.search("\$\$\{?" + forvar, args):
                loopvals = vars[forvars[forvar]]
                cml.write("foreach({} {})\n".format(forvar, loopvals))
                # Replace loop variables by CMake-style variables
                args = args.replace("$$" + forvar, "${" + forvar + "}")  # case for $$var
                args = args.replace("$${" + forvar + "}", "${" + forvar + "}")  # case for $${var} - if the first match did not work, those are the cases
                close += 1
                testnameloopextra += "_" + forvar + "-${" + forvar + "}"
        
        cml.write("ADDTEST({}{} {} {} {} \"{}\")\n".format(fulltestname, testnameloopextra, nproc, exname, outfile, args))
        alltests.append(fulltestname)
        if not debug:
            log.write("Added test {}. nProcs={}, args={}\n"
                      .format(fulltestname, nproc, args))
        
        # Close loops if any!    
        for num in range(close):
            cml.write("endforeach()\n")
        #log.write("Binary {} has not been created, skipping test {}\n".format(exname, testname))

debug = 0
#debug = 1

folders_with_tests = []
donefolders = []
binaries = []
alltests = []
skipped_tests = []
missing_execs = 0
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
        # Get relative current_folder to working directory
        current_folder = m.group(0).replace(m.group(1), "")
        
        if debug and m.group(0) != "/home/dwirtz/software/opencmiss/src/dependencies/petsc/src/vec/vec/examples/tutorials":
            continue
        
        if current_folder not in donefolders:
            makefile = os.path.join(m.group(0), "makefile")
            
            if not os.path.isfile(makefile):
                log.write("Skipping current_folder {}, no 'makefile' found".format(m.group(0)))
                continue
         
            if debug:
                cml = sys.stdout
            else:
                # cml = open(os.devnull, 'w')
                cml = open(os.path.join(current_folder, "CMakeLists.txt"), 'w')
            
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
                    # vars[match.group(1)] = filter(None,match.group(2).split(" "))
                    vars[match.group(1)] = match.group(2)
            
            # print vars
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
            log.write("Found variables: {}\nFound loop variables: {}\n".format(vars, forvars))
            
            # Sort out which tests to actually create
            # This is determined by the TESTEXAMPLES_XX variables at the bottom of every makefile.
            # We only convert the examples for "find_examples_types" (see above)
            selected_tests = []
            for type in find_examples_types:
                # Eclipse: TESTEXAMPLES_[^=]*=(.*\\\R)*.*\R
                expr = "^TESTEXAMPLES_" + type + "\s*=\s*((?:.*\\\\\n)*)(.*)\n"
                match = re.findall(expr, mfcont, re.MULTILINE)
                if match:
                    selected_tests.extend(re_testcases.findall(match[0][0] + match[0][1]))
            
            if len(selected_tests) > 0:
                log.write("Test selection: {}".format(selected_tests))
                folders_with_tests.append(current_folder)
                # Find tests
                tests_raw = re_findtest.finditer(mfcont)
                for test_raw in tests_raw:
                    if test_raw.group(1) in selected_tests:
                        log.write("Parsing test {}\n".format(test_raw.group(1)))
                        parseTests(base,test_raw.group(2),vars,forvars,current_folder)
                    else:
                        log.write("Skipping test {}\n".format(test_raw.group(1)))
                        skipped_tests.append("{} -> {}".format(current_folder,test_raw.group(1)))
            if not debug:
                cml.close()
        donefolders.append(current_folder)
        
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
Skipped: {} tests not listed in {}
""".format(len(binaries), len(alltests), missing_execs,
            len(missing_sources), len(skipped_tests), ",".join(find_examples_types))
log.writelines(summary)        
log.close()
if not debug:
    sys.stdout.write(summary + "Complete log at {}\n".format(logfile))
