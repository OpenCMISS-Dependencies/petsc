import PETSc.package

class Configure(PETSc.package.NewPackage):
  def __init__(self, framework):
    PETSc.package.NewPackage.__init__(self, framework)
    self.download          = ['http://ftp.mcs.anl.gov/pub/petsc/externalpackages/petsc4py-418f86b7e1b8.tar.gz']
    self.functions         = []
    self.includes          = []
    self.liblist           = []
    self.complex           = 1
    return

  def setupDependencies(self, framework):
    PETSc.package.NewPackage.setupDependencies(self, framework)
    self.petscconfigure  = framework.require('PETSc.Configure',self)
    self.numpy           = framework.require('PETSc.packages.Numpy',self)
    self.setCompilers    = framework.require('config.setCompilers',self)
    self.sharedLibraries = framework.require('PETSc.utilities.sharedLibraries', self)
    return

  def Install(self):
    import os
    pp = os.path.join(self.installDir,'lib','python*','site-packages')
    if self.setCompilers.isDarwin():
      apple = 'You may need to\n (csh/tcsh) setenv MACOSX_DEPLOYMENT_TARGET 10.X\n (sh/bash) MACOSX_DEPLOYMENT_TARGET=10.X; export MACOSX_DEPLOYMENT_TARGET\nbefore running make on PETSc'
    else:
      apple = ''
    self.logClearRemoveDirectory()
    self.logResetRemoveDirectory()
    archflags = ""
    if self.setCompilers.isDarwin():
      if self.types.sizes['known-sizeof-void-p'] == 32:
        archflags = "ARCHFLAGS=\'-arch i386\' "
      else:
        archflags = "ARCHFLAGS=\'-arch x86_64\' "
    self.addMakeRule('petsc4py','', \
                       ['@echo "*** Building petsc4py ***"',\
                          '@(MPICC=${PCC} && export MPICC && cd '+self.packageDir+' && \\\n\
           python setup.py clean --all && \\\n\
           '+archflags+'python setup.py install --install-lib='+os.path.join(self.installDir,'lib')+') > ${PETSC_ARCH}/conf/petsc4py.log 2>&1 || \\\n\
             (echo "**************************ERROR*************************************" && \\\n\
             echo "Error building petsc4py. Check ${PETSC_ARCH}/conf/petsc4py.log" && \\\n\
             echo "********************************************************************" && \\\n\
             exit 1)',\
                          '@echo "====================================="',\
                          '@echo "To use petsc4py, add '+os.path.join(self.petscconfigure.installdir,'lib')+' to PYTHONPATH"',\
                          '@echo "====================================="'])

    return self.installDir

  def configureLibrary(self):
    if not self.sharedLibraries.useShared:
        raise RuntimeError('petsc4py requires PETSc be built with shared libraries; rerun with --with-shared-libraries')
    self.checkDownload(1)
    if self.setCompilers.isDarwin():
      # The name of the Python library on Apple is Python which does not end in the expected .dylib
      # Thus see if the python library in the standard locations points to the Python version
      import sys
      import os
      prefix = sys.exec_prefix
      if os.path.isfile(os.path.join(prefix,'Python')):
        for i in ['/usr/lib/libpython.dylib','/usr/lib/libpython'+sys.version[:3]+'.dylib','/opt/local/lib/libpython.dylib','/opt/local/lib/libpython'+sys.version[:3]+'.dylib']:
          if os.path.realpath(i) == os.path.join(prefix,'Python'):
            self.addDefine('PYTHON_LIB','"'+os.path.join(i)+'"')
            return
        raise RuntimeError('realpath of /usr/lib/libpython.dylib ('+os.path.realpath('/usr/lib/libpython.dylib')+') does not point to the expected Python library path ('+os.path.join(prefix,'Python')+') for current Python;\n')
      elif os.path.isfile(os.path.join(prefix,'lib','libpython.dylib')):
        self.addDefine('PYTHON_LIB','"'+os.path.join(prefix,'lib','libpython.dylib')+'"')
      elif os.path.isfile(os.path.join(prefix,'lib','libpython'+sys.version[:3]+'.dylib')):
        self.addDefine('PYTHON_LIB','"'+os.path.join(prefix,'lib','libpython'+sys.version[:3]+'.dylib')+'"')
      else:
        raise RuntimeError('Unable to find Python dynamic library at prefix '+prefix)

  def alternateConfigureLibrary(self):
    self.addMakeRule('petsc4py','')

