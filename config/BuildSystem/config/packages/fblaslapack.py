import config.package

class Configure(config.package.Package):
  def __init__(self, framework):
    config.package.Package.__init__(self, framework)
    self.download         = ['http://ftp.mcs.anl.gov/pub/petsc/externalpackages/fblaslapack-3.4.2.tar.gz']
    self.double           = 0
    self.downloadonWindows= 1

  def setupDependencies(self, framework):
    config.package.Package.setupDependencies(self, framework)
    return


  def Install(self):
    import os

    if self.defaultPrecision == '__float128':
      raise RuntimeError('Cannot build fblaslapack with __float128; use --download-f2cblaslapack instead')

    if not hasattr(self.compilers, 'FC'):
      raise RuntimeError('Cannot request fblaslapack without Fortran compiler, use --download-f2cblaslapack intead')

    self.setCompilers.pushLanguage('FC')
    if config.setCompilers.Configure.isNAG(self.setCompilers.getLinker()):
      raise RuntimeError('Cannot compile fortran blaslapack with NAG compiler')
    self.setCompilers.popLanguage()

    libdir = self.libDir
    confdir = self.confDir
    blasDir = self.packageDir

    g = open(os.path.join(blasDir,'tmpmakefile'),'w')
    f = open(os.path.join(blasDir,'makefile'),'r')
    line = f.readline()
    while line:
      if line.startswith('CC  '):
        cc = self.compilers.CC
        line = 'CC = '+cc+'\n'
      if line.startswith('COPTFLAGS '):
        self.setCompilers.pushLanguage('C')
        line = 'COPTFLAGS  = '+self.setCompilers.getCompilerFlags()
        noopt = self.checkNoOptFlag()
        self.setCompilers.popLanguage()
      if line.startswith('CNOOPT'):
        self.setCompilers.pushLanguage('C')
        line = 'CNOOPT = '+noopt+ ' '+self.getSharedFlag(self.setCompilers.getCompilerFlags())+' '+self.getPointerSizeFlag(self.setCompilers.getCompilerFlags())+' '+self.getWindowsNonOptFlags(self.setCompilers.getCompilerFlags())
        self.setCompilers.popLanguage()
      if line.startswith('FC  '):
        fc = self.compilers.FC
        if fc.find('f90') >= 0 or fc.find('f95') >=0:
          import commands
          output  = commands.getoutput(fc+' -v')
          if output.find('IBM') >= 0:
            fc = os.path.join(os.path.dirname(fc),'xlf')
            self.framework.log.write('Using IBM f90 compiler, switching to xlf for compiling BLAS/LAPACK\n')
        line = 'FC = '+fc+'\n'
      if line.startswith('FOPTFLAGS '):
        self.setCompilers.pushLanguage('FC')
        line = 'FOPTFLAGS  = '+self.setCompilers.getCompilerFlags().replace('-Mfree','')+'\n'
        noopt = self.checkNoOptFlag()
        self.setCompilers.popLanguage()
      if line.startswith('FNOOPT'):
        self.setCompilers.pushLanguage('FC')
        line = 'FNOOPT = '+noopt+' '+self.getSharedFlag(self.setCompilers.getCompilerFlags())+' '+self.getPointerSizeFlag(self.setCompilers.getCompilerFlags())+' '+self.getWindowsNonOptFlags(self.setCompilers.getCompilerFlags())+'\n'
        self.setCompilers.popLanguage()
      if line.startswith('AR  '):
        line = 'AR      = '+self.setCompilers.AR+'\n'
      if line.startswith('AR_FLAGS  '):
        line = 'AR_FLAGS      = '+self.setCompilers.AR_FLAGS+'\n'
      if line.startswith('LIB_SUFFIX '):
        line = 'LIB_SUFFIX = '+self.setCompilers.AR_LIB_SUFFIX+'\n'
      if line.startswith('RANLIB  '):
        line = 'RANLIB = '+self.setCompilers.RANLIB+'\n'
      if line.startswith('RM  '):
        line = 'RM = '+self.programs.RM+'\n'

      if line.startswith('include'):
        line = '\n'
      g.write(line) 
      line = f.readline()
    f.close()
    g.close()

    if not self.installNeeded('tmpmakefile'): return self.installDir

    try:
      self.logPrintBox('Compiling FBLASLAPACK; this may take several minutes')
      output,err,ret  = config.base.Configure.executeShellCommand('cd '+blasDir+' && make -f tmpmakefile cleanblaslapck cleanlib && make -f tmpmakefile', timeout=2500, log = self.framework.log)
    except RuntimeError, e:
      raise RuntimeError('Error running make on '+blasDir+': '+str(e))
    try:
      output,err,ret  = config.base.Configure.executeShellCommand('cd '+blasDir+' && mv -f libfblas.'+self.setCompilers.AR_LIB_SUFFIX+' libflapack.'+self.setCompilers.AR_LIB_SUFFIX+' '+ libdir, timeout=30, log = self.framework.log)
    except RuntimeError, e:
      raise RuntimeError('Error moving '+blasDir+' libraries: '+str(e))
    try:
      output,err,ret  = config.base.Configure.executeShellCommand('cd '+blasDir+' && cp -f tmpmakefile '+os.path.join(self.confDir, self.name), timeout=30, log = self.framework.log)
    except RuntimeError, e:
      pass
    return libdir


