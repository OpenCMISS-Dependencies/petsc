import PETSc.package

class Configure(PETSc.package.NewPackage):
  def __init__(self, framework):
    PETSc.package.NewPackage.__init__(self, framework)
    self.download          = ['http://www.cmake.org/files/v2.8/cmake-2.8.12.2.tar.gz']
    self.download_sol      = ['http://www.cmake.org/files/v2.8/cmake-2.8.10.2.tar.gz']
    self.complex           = 1
    self.double            = 0
    self.requires32bitint  = 0
    self.downloadonWindows = 1

  def setupHelp(self, help):
    import nargs
    help.addArgument('PETSc', '-with-cmake=<prog>', nargs.Arg(None, 'cmake', 'Specify cmake'))
    help.addArgument('PETSc', '-download-cmake=<no,yes,filename>', nargs.ArgDownload(None, 0, 'Download and install cmake'))
    return

  def checkDownload(self,requireDownload = 1):
    import config.base
    if config.setCompilers.Configure.isSolaris():
      self.download         = self.download_sol
    return config.package.Package.checkDownload(self, requireDownload)

  def Install(self):
    import os
    args = ['--prefix='+self.installDir]
    args = ' '.join(args)
    fd = file(os.path.join(self.packageDir,'cmake.args'), 'w')
    fd.write(args)
    fd.close()
    if self.installNeeded('cmake.args'):
      try:
        self.logPrintBox('Configuring CMake; this may take several minutes')
        output,err,ret  = PETSc.package.NewPackage.executeShellCommand('cd '+self.packageDir+' && ./configure '+args, timeout=900, log = self.framework.log)
      except RuntimeError, e:
        raise RuntimeError('Error running configure on cmake: '+str(e))
      try:
        self.logPrintBox('Compiling CMake; this may take several minutes')
        output,err,ret  = PETSc.package.NewPackage.executeShellCommand('cd '+self.packageDir+' && '+self.make.make_jnp+' && '+self.make.make+' install && '+self.make.make+' clean', timeout=2500, log = self.framework.log)
      except RuntimeError, e:
        raise RuntimeError('Error running make; make install on cmake: '+str(e))
      self.postInstall(output+err,'cmake.args')
    self.binDir = os.path.join(self.installDir, 'bin')
    self.cmake = os.path.join(self.binDir, 'cmake')
    self.addMakeMacro('CMAKE',self.cmake)
    return self.installDir

  def alternateConfigureLibrary(self):
    self.checkDownload(1)

  def configure(self):
    '''Determine whether the cmake exist or not'''

    if (not self.framework.argDB['with-cmake']):
      self.framework.logPrint("Not checking cmake on user request\n")
      return

    if (self.framework.argDB['download-cmake']):
      PETSc.package.NewPackage.configure(self)
    elif self.getExecutable(self.framework.argDB['with-cmake'], getFullPath = 1,resultName='cmake'):
      self.found = 1
    return
