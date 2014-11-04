from __future__ import generators
import user
import config.base
import config.package
from sourceDatabase import SourceDB
import os

class Configure(config.package.Package):
  '''FIX: This has not yet been converted to the package style'''
  def __init__(self, framework):
    config.package.Package.__init__(self, framework)
    self.argDB             = framework.argDB
    self.found             = 0
    self.f2c               = 0  # indicates either the f2c BLAS/LAPACK are used (with or without Fortran compiler) or there is no Fortran compiler (and system BLAS/LAPACK is used)
    self.mkl               = 0  # indicates BLAS/LAPACK library used is Intel MKL
    self.missingRoutines   = []
    self.separateBlas      = 1
    self.defaultPrecision  = 'double'

  def setupDependencies(self, framework):
    config.package.Package.setupDependencies(self, framework)
    self.libraries     = framework.require('config.libraries', None)
    self.compilers     = framework.require('config.compilers', None)
    self.compilerFlags = framework.require('config.compilerFlags', self)
    self.setCompilers  = framework.require('config.setCompilers', self)
    self.f2cblaslapack = framework.require('config.packages.f2cblaslapack', self)
    self.fblaslapack   = framework.require('config.packages.fblaslapack', self)
    return


  def __str__(self):
    return 'BLAS/LAPACK: '+self.libraries.toString(self.lib)+'\n'

  def setupHelp(self, help):
    import nargs
    help.addArgument('BLAS/LAPACK', '-with-blas-lapack-dir=<dir>',                nargs.ArgDir(None, None, 'Indicate the directory containing BLAS and LAPACK libraries'))
    help.addArgument('BLAS/LAPACK', '-with-blas-lapack-lib=<libraries: e.g. [/Users/..../liblapack.a,libblas.a,...]>',nargs.ArgLibrary(None, None, 'Indicate the library containing BLAS and LAPACK'))
    help.addArgument('BLAS/LAPACK', '-with-blas-lib=<libraries: e.g. [/Users/..../libblas.a,...]>',    nargs.ArgLibrary(None, None, 'Indicate the library(s) containing BLAS'))
    help.addArgument('BLAS/LAPACK', '-with-lapack-lib=<libraries: e.g. [/Users/..../liblapack.a,...]>',nargs.ArgLibrary(None, None, 'Indicate the library(s) containing LAPACK'))
    help.addArgument('BLAS/LAPACK', '-known-64-bit-blas-indices=<bool>', nargs.ArgBool(None, 0, 'Indicate if using 64 bit integer BLAS'))
    return

  def getPrefix(self):
    if self.defaultPrecision == 'single': return 's'
    if self.defaultPrecision == 'double': return 'd'
    if self.defaultPrecision == 'quad': return 'q'
    if self.defaultPrecision == '__float128': return 'q'
    return 'Unknown precision'

  def getOtherLibs(self, foundBlas = None, blasLibrary = None, separateBlas = None):
    if foundBlas is None:
      foundBlas = self.foundBlas
    if blasLibrary is None:
      blasLibrary = self.blasLibrary
    if separateBlas is None:
      separateBlas = self.separateBlas
    otherLibs = []
    if foundBlas:
      if separateBlas:
        otherLibs += blasLibrary
    if self.useCompatibilityLibs:
      otherLibs += self.compilers.flibs
    return otherLibs

  def checkBlas(self, blasLibrary, otherLibs, fortranMangle, routineIn = 'dot'):
    '''This checks the given library for the routine, dot by default'''
    oldLibs = self.compilers.LIBS
    prototype = ''
    call      = ''
    routine   = self.mangleBlas(routineIn)
    if fortranMangle=='stdcall':
      if routine=='ddot':
        prototype = 'double __stdcall DDOT(int*,double*,int*,double*,int*);'
        call      = 'DDOT(0,0,0,0,0);'
    found   = self.libraries.check(blasLibrary, routine, otherLibs = otherLibs, fortranMangle = fortranMangle, prototype = prototype, call = call)
    self.compilers.LIBS = oldLibs
    return found

  def checkLapack(self, lapackLibrary, otherLibs, fortranMangle, routinesIn = ['getrs', 'geev']):
    oldLibs = self.compilers.LIBS
    routines = list(routinesIn)
    found   = 1
    prototypes = ['','']
    calls      = ['','']
    routines   = map(self.mangleBlas, routines)

    if fortranMangle=='stdcall':
      if routines == ['dgetrs','dgeev']:
        prototypes = ['void __stdcall DGETRS(char*,int,int*,int*,double*,int*,int*,double*,int*,int*);',
                      'void __stdcall DGEEV(char*,int,char*,int,int*,double*,int*,double*,double*,double*,int*,double*,int*,double*,int*,int*);']
        calls      = ['DGETRS(0,0,0,0,0,0,0,0,0,0);',
                      'DGEEV(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);']
    for routine, prototype, call in zip(routines, prototypes, calls):
      found = found and self.libraries.check(lapackLibrary, routine, otherLibs = otherLibs, fortranMangle = fortranMangle, prototype = prototype, call = call)
      if not found: break
    self.compilers.LIBS = oldLibs
    return found

  def checkLib(self, lapackLibrary, blasLibrary = None):
    '''Checking for BLAS and LAPACK symbols'''

    #check for BLASLAPACK_STDCALL calling convention!!!!

    if blasLibrary is None:
      self.separateBlas = 0
      blasLibrary       = lapackLibrary
    else:
      self.separateBlas = 1
    if not isinstance(lapackLibrary, list): lapackLibrary = [lapackLibrary]
    if not isinstance(blasLibrary,   list): blasLibrary   = [blasLibrary]
    foundBlas   = 0
    foundLapack = 0
    self.f2c    = 0
    mangleFunc = self.compilers.fortranMangling
    foundBlas = self.checkBlas(blasLibrary, self.getOtherLibs(foundBlas, blasLibrary), mangleFunc,'dot')
    if foundBlas:
      foundLapack = self.checkLapack(lapackLibrary, self.getOtherLibs(foundBlas, blasLibrary), mangleFunc)
      if foundLapack:
        self.mangling = self.compilers.fortranMangling
      self.framework.logPrint('Found Fortran mangling on BLAS/LAPACK which is '+self.compilers.fortranMangling)
    else:
      self.framework.logPrint('Checking for no name mangling on BLAS/LAPACK')
      foundBlas = self.checkBlas(blasLibrary, self.getOtherLibs(foundBlas, blasLibrary), 0, 'dot')
      if foundBlas:
        foundLapack = self.checkLapack(lapackLibrary, self.getOtherLibs(foundBlas, blasLibrary), 0, ['getrs', 'geev'])
      if foundBlas and foundLapack:
        self.framework.logPrint('Found no name mangling on BLAS/LAPACK')
        self.mangling = 'unchanged'
        self.f2c = 1
      else:
        self.framework.logPrint('Checking for underscore name mangling on BLAS/LAPACK')
        foundBlas = self.checkBlas(blasLibrary, self.getOtherLibs(foundBlas, blasLibrary), 0, 'dot_')
        if foundBlas:
          foundLapack = self.checkLapack(lapackLibrary, self.getOtherLibs(foundBlas, blasLibrary), 0, ['getrs_', 'geev_'])
        if foundBlas and foundLapack:
          self.framework.logPrint('Found underscore name mangling on BLAS/LAPACK')
          self.mangling = 'underscore'
          self.f2c = 1
    return (foundBlas, foundLapack)

  def generateGuesses(self):
    # check that user has used the options properly
    if self.f2cblaslapack.found:
      self.f2c = 1
      libDir = self.f2cblaslapack.libDir
      f2cLibs = [os.path.join(libDir,'libf2cblas.a')]
      if self.libraries.math:
        f2cLibs = f2cLibs+self.libraries.math
      yield ('f2cblaslapack', f2cLibs, os.path.join(libDir,'libf2clapack.a'), 0)
      raise RuntimeError('--download-f2cblaslapack libraries cannot be used')
    if self.fblaslapack.found:
      self.f2c = 0
      libDir = self.fblaslapack.libDir
      yield ('fblaslapack', os.path.join(libDir,'libfblas.a'), os.path.join(libDir,'libflapack.a'), 1)
      raise RuntimeError('--download-fblaslapack libraries cannot be used')
    if 'with-blas-lib' in self.framework.argDB and not 'with-lapack-lib' in self.framework.argDB:
      raise RuntimeError('If you use the --with-blas-lib=<lib> you must also use --with-lapack-lib=<lib> option')
    if not 'with-blas-lib' in self.framework.argDB and 'with-lapack-lib' in self.framework.argDB:
      raise RuntimeError('If you use the --with-lapack-lib=<lib> you must also use --with-blas-lib=<lib> option')
    if 'with-blas-lib' in self.framework.argDB and 'with-blas-lapack-dir' in self.framework.argDB:
      raise RuntimeError('You cannot set both the library containing BLAS with --with-blas-lib=<lib>\nand the directory to search with --with-blas-lapack-dir=<dir>')
    if 'with-blas-lapack-lib' in self.framework.argDB and 'with-blas-lapack-dir' in self.framework.argDB:
      raise RuntimeError('You cannot set both the library containing BLAS/LAPACK with --with-blas-lapack-lib=<lib>\nand the directory to search with --with-blas-lapack-dir=<dir>')

    # Try specified BLASLAPACK library
    if 'with-blas-lapack-lib' in self.framework.argDB:
      yield ('User specified BLAS/LAPACK library', None, self.framework.argDB['with-blas-lapack-lib'], 1)
      raise RuntimeError('You set a value for --with-blas-lapack-lib=<lib>, but '+str(self.framework.argDB['with-blas-lapack-lib'])+' cannot be used\n')
    # Try specified BLAS and LAPACK libraries
    if 'with-blas-lib' in self.framework.argDB and 'with-lapack-lib' in self.framework.argDB:
      yield ('User specified BLAS and LAPACK libraries', self.framework.argDB['with-blas-lib'], self.framework.argDB['with-lapack-lib'], 1)
      raise RuntimeError('You set a value for --with-blas-lib=<lib> and --with-lapack-lib=<lib>, but '+str(self.framework.argDB['with-blas-lib'])+' and '+str(self.framework.argDB['with-lapack-lib'])+' cannot be used\n')
    # Try specified installation root
    if 'with-blas-lapack-dir' in self.framework.argDB:
      dir = self.framework.argDB['with-blas-lapack-dir']
      # error if package-dir is in externalpackages
      if os.path.realpath(dir).find(os.path.realpath(self.externalPackagesDir)) >=0:
        fakeExternalPackagesDir = dir.replace(os.path.realpath(dir).replace(os.path.realpath(self.externalPackagesDir),''),'')
        raise RuntimeError('Bad option: '+'--with-blas-lapack-dir='+self.framework.argDB['with-blas-lapack-dir']+'\n'+
                           fakeExternalPackagesDir+' is reserved for --download-package scratch space. \n'+
                           'Do not install software in this location nor use software in this directory.')
      if not (len(dir) > 2 and dir[1] == ':') :
        dir = os.path.abspath(dir)
      self.framework.log.write('Looking for BLAS/LAPACK in user specified directory: '+dir+'\n')
      self.framework.log.write('Files and directorys in that directory:\n'+str(os.listdir(dir))+'\n')

      # Look for Multi-Threaded MKL for MKL_Pardiso
      if self.framework.argDB['with-mkl_pardiso'] or 'with-mkl_pardiso-dir' in self.framework.argDB or 'with-mkl_pardiso-lib' in self.framework.argDB:
        self.logPrintBox('BLASLAPACK: Looking for Multithreaded MKL for Pardiso')
        for libdir in [os.path.join('lib','64'),os.path.join('lib','ia64'),os.path.join('lib','em64t'),os.path.join('lib','intel64'),'64','ia64','em64t','intel64',
                       os.path.join('lib','32'),os.path.join('lib','ia32'),'32','ia32','']:
          if not os.path.exists(os.path.join(dir,libdir)):
            self.framework.logPrint('MLK Path not found.. skipping: '+os.path.join(dir,libdir))
          else:
            yield ('User specified MKL-Pardiso Intel-Linux64', None, [os.path.join(dir,libdir,'libmkl_intel_lp64.a'),'mkl_core','mkl_intel_thread','iomp5','dl','pthread','m'],1)
            yield ('User specified MKL-Pardiso GNU-Linux64', None, [os.path.join(dir,libdir,'libmkl_intel_lp64.a'),'mkl_core','mkl_gnu_thread','gomp','dl','pthread','m'],1)
            yield ('User specified MKL-Pardiso Intel-Linux32', None, [os.path.join(dir,libdir,'libmkl_intel.a'),'mkl_core','mkl_intel_thread','iomp5','dl','pthread','m'],1)
            yield ('User specified MKL-Pardiso GNU-Linux32', None, [os.path.join(dir,libdir,'libmkl_intel.a'),'mkl_core','mkl_gnu_thread','gomp','dl','pthread','m'],1)
        return

      yield ('User specified installation root (HPUX)', os.path.join(dir, 'libveclib.a'),  os.path.join(dir, 'liblapack.a'), 1)
      f2cLibs = [os.path.join(dir,'libf2cblas.a')]
      if self.libraries.math:
        f2cLibs = f2cLibs+self.libraries.math
      yield ('User specified installation root (F2C)', f2cLibs, os.path.join(dir, 'libf2clapack.a'), 1)
      yield ('User specified installation root', os.path.join(dir, 'libfblas.a'),   os.path.join(dir, 'libflapack.a'), 1)
      # Check MATLAB [ILP64] MKL
      yield ('User specified MATLAB [ILP64] MKL Linux lib dir', None, [os.path.join(dir,'bin','glnxa64','mkl.so'), os.path.join(dir,'sys','os','glnxa64','libiomp5.so'), 'pthread'], 1)
      # Some new MKL 11/12 variations
      for libdir in [os.path.join('lib','32'),os.path.join('lib','ia32'),'32','ia32','']:
        if not os.path.exists(os.path.join(dir,libdir)):
          self.framework.logPrint('MLK Path not found.. skipping: '+os.path.join(dir,libdir))
        else:
          yield ('User specified MKL11/12 Linux32', None, [os.path.join(dir,libdir,'libmkl_intel.a'),'mkl_sequential','mkl_core','pthread','-lm'],1)
      for libdir in [os.path.join('lib','64'),os.path.join('lib','ia64'),os.path.join('lib','em64t'),os.path.join('lib','intel64'),'64','ia64','em64t','intel64','']:
        if not os.path.exists(os.path.join(dir,libdir)):
          self.framework.logPrint('MLK Path not found.. skipping: '+os.path.join(dir,libdir))
        else:
          yield ('User specified MKL11/12 Linux64', None, [os.path.join(dir,libdir,'libmkl_intel_lp64.a'),'mkl_sequential','mkl_core','pthread','-lm'],1)
      # Older Linux MKL checks
      yield ('User specified MKL Linux lib dir', None, [os.path.join(dir, 'libmkl_lapack.a'), 'mkl', 'guide', 'pthread'], 1)
      for libdir in ['32','64','em64t']:
        yield ('User specified MKL Linux installation root', None, [os.path.join(dir,'lib',libdir,'libmkl_lapack.a'),'mkl', 'guide', 'pthread'], 1)
      yield ('User specified MKL Linux-x86 lib dir', None, [os.path.join(dir, 'libmkl_lapack.a'), 'libmkl_def.a', 'guide', 'pthread'], 1)
      yield ('User specified MKL Linux-x86 lib dir', None, [os.path.join(dir, 'libmkl_lapack.a'), 'libmkl_def.a', 'guide', 'vml','pthread'], 1)
      yield ('User specified MKL Linux-ia64 lib dir', None, [os.path.join(dir, 'libmkl_lapack.a'), 'libmkl_ipf.a', 'guide', 'pthread'], 1)
      yield ('User specified MKL Linux-em64t lib dir', None, [os.path.join(dir, 'libmkl_lapack.a'), 'libmkl_em64t.a', 'guide', 'pthread'], 1)
      yield ('User specified MKL Linux-x86 installation root', None, [os.path.join(dir,'lib','32','libmkl_lapack.a'),'libmkl_def.a', 'guide', 'pthread'], 1)
      yield ('User specified MKL Linux-x86 installation root', None, [os.path.join(dir,'lib','32','libmkl_lapack.a'),'libmkl_def.a', 'guide', 'vml','pthread'], 1)
      yield ('User specified MKL Linux-ia64 installation root', None, [os.path.join(dir,'lib','64','libmkl_lapack.a'),'libmkl_ipf.a', 'guide', 'pthread'], 1)
      yield ('User specified MKL Linux-em64t installation root', None, [os.path.join(dir,'lib','em64t','libmkl_lapack.a'),'libmkl_em64t.a', 'guide', 'pthread'], 1)
      # Mac MKL check
      yield ('User specified MKL Mac-x86 lib dir', None, [os.path.join(dir, 'libmkl_lapack.a'), 'libmkl_ia32.a', 'guide'], 1)
      yield ('User specified MKL Max-x86 installation root', None, [os.path.join(dir,'Libraries','32','libmkl_lapack.a'),'libmkl_ia32.a', 'guide'], 1)
      yield ('User specified MKL Max-x86 installation root', None, [os.path.join(dir,'lib','32','libmkl_lapack.a'),'libmkl_ia32.a', 'guide'], 1)
      yield ('User specified MKL Mac-em64t lib dir', None, [os.path.join(dir, 'libmkl_lapack.a'), 'libmkl_intel_lp64.a', 'guide'], 1)
      yield ('User specified MKL Max-em64t installation root', None, [os.path.join(dir,'Libraries','32','libmkl_lapack.a'),'libmkl_intel_lp64.a', 'guide'], 1)
      yield ('User specified MKL Max-em64t installation root', None, [os.path.join(dir,'lib','32','libmkl_lapack.a'),'libmkl_intel_lp64.a', 'guide'], 1)
      # Check MKL on windows
      yield ('User specified MKL Windows lib dir', None, [os.path.join(dir, 'mkl_c_dll.lib')], 1)
      yield ('User specified stdcall MKL Windows lib dir', None, [os.path.join(dir, 'mkl_s_dll.lib')], 1)
      yield ('User specified ia64/em64t MKL Windows lib dir', None, [os.path.join(dir, 'mkl_dll.lib')], 1)
      yield ('User specified MKL10-32 Windows lib dir', None, [os.path.join(dir, 'mkl_intel_c_dll.lib'),'mkl_intel_thread_dll.lib','mkl_core_dll.lib','libiomp5md.lib'], 1)
      yield ('User specified MKL10-32 Windows stdcall lib dir', None, [os.path.join(dir, 'mkl_intel_s_dll.lib'),'mkl_intel_thread_dll.lib','mkl_core_dll.lib','libiomp5md.lib'], 1)
      yield ('User specified MKL10-64 Windows lib dir', None, [os.path.join(dir, 'mkl_intel_lp64_dll.lib'),'mkl_intel_thread_dll.lib','mkl_core_dll.lib','libiomp5md.lib'], 1)
      mkldir = os.path.join(dir, 'ia32', 'lib')
      yield ('User specified MKL Windows installation root', None, [os.path.join(mkldir, 'mkl_c_dll.lib')], 1)
      yield ('User specified stdcall MKL Windows installation root', None, [os.path.join(mkldir, 'mkl_s_dll.lib')], 1)
      yield ('User specified MKL10-32 Windows installation root', None, [os.path.join(mkldir, 'mkl_intel_c_dll.lib'),'mkl_intel_thread_dll.lib','mkl_core_dll.lib','libiomp5md.lib'], 1)
      yield ('User specified MKL10-32 Windows stdcall installation root', None, [os.path.join(mkldir, 'mkl_intel_s_dll.lib'),'mkl_intel_thread_dll.lib','mkl_core_dll.lib','libiomp5md.lib'], 1)
      mkldir = os.path.join(dir, 'em64t', 'lib')
      yield ('User specified MKL10-64 Windows installation root', None, [os.path.join(mkldir, 'mkl_intel_lp64_dll.lib'),'mkl_intel_thread_dll.lib','mkl_core_dll.lib','libiomp5md.lib'], 1)
      yield ('User specified em64t MKL Windows installation root', None, [os.path.join(mkldir, 'mkl_dll.lib')], 1)
      mkldir = os.path.join(dir, 'ia64', 'lib')
      yield ('User specified ia64 MKL Windows installation root', None, [os.path.join(mkldir, 'mkl_dll.lib')], 1)
      yield ('User specified MKL10-64 Windows installation root', None, [os.path.join(mkldir, 'mkl_intel_lp64_dll.lib'),'mkl_intel_thread_dll.lib','mkl_core_dll.lib','libiomp5md.lib'], 1)
      # Check AMD ACML libraries
      yield ('User specified AMD ACML lib dir', None, os.path.join(dir,'lib','libacml.a'), 1)
      yield ('User specified AMD ACML lib dir', None, [os.path.join(dir,'lib','libacml.a'), os.path.join(dir,'lib','libacml_mv.a')], 1)
      yield ('User specified AMD ACML lib dir', None, os.path.join(dir,'lib','libacml_mp.a'), 1)
      yield ('User specified AMD ACML lib dir', None, [os.path.join(dir,'lib','libacml_mp.a'), os.path.join(dir,'lib','libacml_mv.a')], 1)
      # Search for atlas
      yield ('User specified ATLAS Linux installation root', [os.path.join(dir, 'libcblas.a'),os.path.join(dir, 'libf77blas.a'), os.path.join(dir, 'libatlas.a')],  [os.path.join(dir, 'liblapack.a')], 1)
      yield ('User specified ATLAS Linux installation root', [os.path.join(dir, 'libf77blas.a'), os.path.join(dir, 'libatlas.a')],  [os.path.join(dir, 'liblapack.a')], 1)
      # Search for liblapack.a and libblas.a after the implementations with more specific name to avoid
      # finding these in /usr/lib despite using -L<blas-lapack-dir> while attempting to get a different library.
      yield ('User specified installation root', os.path.join(dir, 'libblas.a'),    os.path.join(dir, 'liblapack.a'), 1)
      raise RuntimeError('You set a value for --with-blas-lapack-dir=<dir>, but '+self.framework.argDB['with-blas-lapack-dir']+' cannot be used\n')
    # IRIX locations
    yield ('IRIX Mathematics library', None, 'libcomplib.sgimath.a', 1)
    yield ('Another IRIX Mathematics library', None, 'libscs.a', 1)
    yield ('Compaq/Alpha Mathematics library', None, 'libcxml.a', 1)
    # IBM ESSL locations
    yield ('IBM ESSL Mathematics library', None, 'libessl.a', 1)
    yield ('IBM ESSL Mathematics library for Blue Gene', None, 'libesslbg.a', 2)
    # Portland group compiler blas and lapack
    if 'PGI' in os.environ and config.setCompilers.Configure.isPGI(self.setCompilers.CC):
      dir = os.path.join(os.environ['PGI'],'linux86','5.1','lib')
      yield ('User specified installation root', os.path.join(dir, 'libblas.a'), os.path.join(dir, 'liblapack.a'), 1)
      dir = os.path.join(os.environ['PGI'],'linux86','5.0','lib')
      yield ('User specified installation root', os.path.join(dir, 'libblas.a'), os.path.join(dir, 'liblapack.a'), 1)
      dir = os.path.join(os.environ['PGI'],'linux86','lib')
      yield ('User specified installation root', os.path.join(dir, 'libblas.a'), os.path.join(dir, 'liblapack.a'), 1)
    # Try compiler defaults
    yield ('Default compiler libraries', '', '', 1)
    yield ('Default compiler locations', 'libblas.a', 'liblapack.a', 1)
    yield ('HPUX', 'libveclib.a', 'liblapack.a', 1)
    # /usr/local/lib
    dir = os.path.join('/usr','local','lib')
    yield ('Default compiler locations /usr/local/lib', os.path.join(dir,'libblas.a'), os.path.join(dir,'liblapack.a'), 1)
    yield ('Default Atlas location',['libcblas.a','libf77blas.a','libatlas.a'],  ['liblapack.a'], 1)
    yield ('Default Atlas location',['libf77blas.a','libatlas.a'],  ['liblapack.a'], 1)
    yield ('Default compiler locations with G77', None, ['liblapack.a', 'libblas.a','libg2c.a'], 1)
    yield ('Default compiler locations with gfortran', None, ['liblapack.a', 'libblas.a','libgfortran.a'], 1)
    # Try MacOSX location
    dir = os.path.join('/Library', 'Frameworks', 'Intel_MKL.framework','Libraries','32')
    yield ('MacOSX with Intel MKL', None, [os.path.join(dir,'libmkl_lapack.a'),'libmkl_ia32.a','libguide.a'], 1)
    yield ('MacOSX BLAS/LAPACK library', None, os.path.join('/System', 'Library', 'Frameworks', 'vecLib.framework', 'vecLib'), 1)
    # Sun locations
    yield ('Sun sunperf BLAS/LAPACK library', None, ['libsunperf.a','libsunmath.a','libm.a'], 1)
    yield ('Sun sunperf BLAS/LAPACK library', None, ['libsunperf.a','libF77.a','libM77.a','libsunmath.a','libm.a'], 1)
    yield ('Sun sunperf BLAS/LAPACK library', None, ['libsunperf.a','libfui.a','libfsu.a','libsunmath.a','libm.a'], 1)
    # Try Microsoft Windows location
    for MKL_Version in [os.path.join('MKL','9.0'),os.path.join('MKL','8.1.1'),os.path.join('MKL','8.1'),os.path.join('MKL','8.0.1'),os.path.join('MKL','8.0'),'MKL72','MKL70','MKL61','MKL']:
      mklpath = os.path.join('/cygdrive', 'c', 'Program Files', 'Intel', MKL_Version)
      if not os.path.exists(mklpath):
        self.framework.logPrint('MLK Path not found.. skipping: '+mklpath)
      else:
        mkldir = os.path.join(mklpath, 'ia32', 'lib')
        yield ('Microsoft Windows, Intel MKL library', None, os.path.join(mkldir,'mkl_c_dll.lib'), 1)
        yield ('Microsoft Windows, Intel MKL stdcall library', None, os.path.join(mkldir,'mkl_s_dll.lib'), 1)
        mkldir = os.path.join(mklpath, 'em64t', 'lib')
        yield ('Microsoft Windows, em64t Intel MKL library', None, os.path.join(mkldir,'mkl_dll.lib'), 1)
        mkldir = os.path.join(mklpath, 'ia64', 'lib')
        yield ('Microsoft Windows, ia64 Intel MKL library', None, os.path.join(mkldir,'mkl_dll.lib'), 1)
    return

  def configureLibrary(self):
    self.functionalBlasLapack = []
    self.foundBlas   = 0
    self.foundLapack = 0
    for (name, blasLibrary, lapackLibrary, self.useCompatibilityLibs) in self.generateGuesses():
      self.framework.log.write('================================================================================\n')
      self.framework.log.write('Checking for a functional BLAS and LAPACK in '+name+'\n')
      (foundBlas, foundLapack) = self.executeTest(self.checkLib, [lapackLibrary, blasLibrary])
      if foundBlas:   self.foundBlas   = 1
      if foundLapack: self.foundLapack = 1
      if foundBlas and foundLapack:
        self.functionalBlasLapack.append((name, blasLibrary, lapackLibrary))
        if not self.framework.argDB['with-alternatives']:
          break
    # User chooses one or take first (sort by version)
    if self.foundBlas and self.foundLapack:
      name, self.blasLibrary, self.lapackLibrary = self.functionalBlasLapack[0]
      if not isinstance(self.blasLibrary,   list): self.blasLibrary   = [self.blasLibrary]
      if not isinstance(self.lapackLibrary, list): self.lapackLibrary = [self.lapackLibrary]
      self.lib = []
      if self.lapackLibrary[0]: self.lib.extend(self.lapackLibrary)
      if self.blasLibrary[0]:   self.lib.extend(self.blasLibrary)
      self.dlib = self.lib[:]
      if self.useCompatibilityLibs:
        self.dlib.extend(self.compilers.flibs)
      self.framework.packages.append(self)
    else:
      if not self.foundBlas:
        # check for split blas/blas-dev packages
        import glob
        blib = glob.glob('/usr/lib/libblas.*')
        if blib != [] and not (os.path.isfile('/usr/lib/libblas.so') or os.path.isfile('/usr/lib/libblas.a')):
          raise RuntimeError('Incomplete BLAS install; Perhaps blas package is installed - but blas-dev/blas-devel is required?')
        if hasattr(self.compilers, 'FC') and (self.defaultPrecision != '__float128') : pkg = 'fblaslapack'
        else: pkg = 'f2cblaslapack'
        raise RuntimeError('Could not find a functional BLAS. Run with --with-blas-lib=<lib> to indicate the library containing BLAS.\n Or --download-'+pkg+'=1 to have one automatically downloaded and installed\n')
      if not self.foundLapack:
        # check for split blas/blas-dev packages
        import glob
        llib = glob.glob('/usr/lib/liblapack.*')
        if llib != [] and not (os.path.isfile('/usr/lib/liblapack.so') or os.path.isfile('/usr/lib/liblapack.a')):
          raise RuntimeError('Incomplete LAPACK install; Perhaps lapack package is installed - but lapack-dev/lapack-devel is required?')
        if hasattr(self.compilers, 'FC') and (self.defaultPrecision != '__float128') : pkg = 'fblaslapack'
        else: pkg = 'f2cblaslapack'
        raise RuntimeError('Could not find a functional LAPACK. Run with --with-lapack-lib=<lib> to indicate the library containing LAPACK.\n Or --download-'+pkg+'=1 to have one automatically downloaded and installed\n')

    #  allow user to dictate which blas/lapack mangling to use (some blas/lapack libraries, like on Apple, provide several)
    if 'known-blaslapack-mangling' in self.argDB:
      self.mangling = self.argDB['known-blaslapack-mangling']

    if self.mangling == 'underscore':
        self.addDefine('BLASLAPACK_UNDERSCORE', 1)
    elif self.mangling == 'caps':
        self.addDefine('BLASLAPACK_CAPS', 1)
    elif self.mangling == 'stdcall':
        self.addDefine('BLASLAPACK_STDCALL', 1)
    self.found = 1
    return

  def checkESSL(self):
    '''Check for the IBM ESSL library'''
    if self.libraries.check(self.lapackLibrary, 'iessl'):
      self.addDefine('HAVE_ESSL',1)
    return

  def checkMKL(self):
    '''Check for Intel MKL library'''
    if self.libraries.check(self.lapackLibrary, 'mkl_set_num_threads'):
      self.mkl = 1
    return

  def checkPESSL(self):
    '''Check for the IBM PESSL library - and error out - if used instead of ESSL'''
    if self.libraries.check(self.lapackLibrary, 'ipessl'):
      raise RuntimeError('Cannot use PESSL instead of ESSL!')
    return

  def mangleBlas(self, baseName):
    prefix = self.getPrefix()
    if self.f2c and self.mangling == 'underscore':
      return prefix+baseName+'_'
    else:
      return prefix+baseName

  def checkMissing(self):
    '''Check for missing LAPACK routines'''
    if self.foundLapack:
      mangleFunc = hasattr(self.compilers, 'FC') and not self.f2c
    routines = ['trsen','gerfs','gges','tgsen','gesvd','getrf','getrs','geev','gelss','syev','syevx','sygv','sygvx','potrf','potrs','stebz','pttrf','pttrs','stein','orgqr','geqrf','gesv','hseqr','steqr']
    oldLibs = self.compilers.LIBS
    found, missing = self.libraries.checkClassify(self.lapackLibrary, map(self.mangleBlas,routines), otherLibs = self.getOtherLibs(), fortranMangle = mangleFunc)
    for baseName in routines:
      if self.mangleBlas(baseName) in missing:
        self.missingRoutines.append(baseName)
        self.addDefine('MISSING_LAPACK_'+baseName.upper(), 1)
    self.compilers.LIBS = oldLibs

  def checklsame(self):
    ''' Do the BLAS/LAPACK libraries have a valid lsame() function with correction binding. Lion and xcode 4.2 do not'''
    routine = 'lsame';
    if self.f2c:
      if self.mangling == 'underscore':
        routine = routine + '_'
    else:
      routine = self.compilers.mangleFortranFunction(routine)
    if not self.libraries.check(self.dlib,routine,fortranMangle = 0):
      self.addDefine('MISSING_LAPACK_'+routine, 1)

  def checkForRoutine(self,routine):
    ''' used by other packages to see if a BLAS routine is available
        This is not really correct because other packages do not (usually) know about f2cblasLapack'''
    if self.f2c:
      if self.mangling == 'underscore':
        return self.libraries.check(self.dlib,routine+'_')
      else:
        return self.libraries.check(self.dlib,routine)
    else:
      return self.libraries.check(self.dlib,routine,fortranMangle = hasattr(self.compilers, 'FC'))

  def check64BitBLASIndices(self):
    '''Check for and use 64bit integer blas'''
    if 'known-64-bit-blas-indices' in self.argDB:
      if int(self.argDB['known-64-bit-blas-indices']):
        self.addDefine('HAVE_64BIT_BLAS_INDICES', 1)
    return

  def configure(self):
    self.executeTest(self.configureLibrary)
    self.executeTest(self.check64BitBLASIndices)
    self.executeTest(self.checkESSL)
    self.executeTest(self.checkPESSL)
    self.executeTest(self.checkMKL)
    self.executeTest(self.checkMissing)
    self.executeTest(self.checklsame)
    if self.framework.argDB['with-shared-libraries']:
      symbol = 'dgeev'
      if self.f2c:
        if self.mangling == 'underscore': symbol = 'dgeev_'
      elif hasattr(self.compilers, 'FC'):
        symbol = self.compilers.mangleFortranFunction(symbol)
      if not self.setCompilers.checkIntoShared(symbol,self.lapackLibrary+self.getOtherLibs()):
        raise RuntimeError('The BLAS/LAPACK libraries '+self.libraries.toStringNoDupes(self.lapackLibrary+self.getOtherLibs())+'\ncannot used with a shared library\nEither run ./configure with --with-shared-libraries=0 or use a different BLAS/LAPACK library');
    return

