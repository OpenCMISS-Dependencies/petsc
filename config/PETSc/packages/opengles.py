import PETSc.package
import os

class Configure(PETSc.package.NewPackage):
  def __init__(self, framework):
    PETSc.package.NewPackage.__init__(self, framework)
    self.functions         = []
    self.includes          = []
    self.liblist           = [[]]
    self.complex           = 1   # 0 means cannot use complex
    self.lookforbydefault  = 0 
    self.double            = 0   # 1 means requires double precision 
    self.requires32bitint  = 0;  # 1 means that the package will not work with 64 bit integers
    self.worksonWindows    = 0  # 1 means that package can be used on Microsof Windows
    return

  def setupDependencies(self, framework):
    PETSc.package.NewPackage.setupDependencies(self, framework)
    return

  def configureLibrary(self):
    self.addDefine('HAVE_OPENGL', 1)
    self.addDefine('HAVE_OPENGLES', 1)
