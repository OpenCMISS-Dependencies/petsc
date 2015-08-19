from __future__ import generators
import user
import config.base
import os

class Configure(config.base.Configure):
  def __init__(self, framework):
    config.base.Configure.__init__(self, framework)
    self.headerPrefix = ''
    self.substPrefix  = ''
    return

  def __str__(self):
    return ''

  def setupHelp(self, help):
    import nargs
    return

  def setupDependencies(self, framework):
    config.base.Configure.setupDependencies(self, framework)
    self.arch           = framework.require('PETSc.options.arch', self)
    self.scalartypes    = framework.require('PETSc.options.scalarTypes', self)
    self.indextypes     = framework.require('PETSc.options.indexTypes', self)
    self.datafilespath  = framework.require('PETSc.options.dataFilesPath', self)
    self.compilers      = framework.require('config.compilers', self)
    self.mpi            = framework.require('config.packages.MPI', self)
    self.elemental      = framework.require('config.packages.elemental', self)
    self.x              = framework.require('config.packages.X', self)
    self.fortrancpp     = framework.require('PETSc.options.fortranCPP', self)
    return

  def configureRegression(self):
    '''Output a file listing the jobs that should be run by the PETSc buildtest'''
    jobs  = []    # Jobs can be run always
    rjobs = []    # Jobs can only be run with real numbers; i.e. NOT  complex
    ejobs = []    # Jobs that require an external package install (also cannot work with complex)
    if self.mpi.usingMPIUni:
      jobs.append('C_X_MPIUni')
      if hasattr(self.compilers, 'FC'):
        jobs.append('Fortran_MPIUni')
    else:
      jobs.append('C')
      if hasattr(self.compilers, 'CXX'):
        rjobs.append('Cxx')
      if self.x.found:
        jobs.append('C_X')
      if hasattr(self.compilers, 'FC') and self.fortrancpp.fortranDatatypes:
        jobs.append('F90_DataTypes')
      elif hasattr(self.compilers, 'FC'):
        jobs.append('Fortran')
        if self.compilers.fortranIsF90:
          rjobs.append('F90')
          if self.scalartypes.scalartype.lower() == 'complex':
            rjobs.append('F90_Complex')
          else:
            rjobs.append('F90_NoComplex')
        if self.compilers.fortranIsF2003:
          rjobs.append('F2003')
        if self.scalartypes.scalartype.lower() == 'complex':
          rjobs.append('Fortran_Complex')
        else:
          rjobs.append('Fortran_NoComplex')
      if self.scalartypes.scalartype.lower() == 'complex':
        rjobs.append('C_Complex')
      else:
        rjobs.append('C_NoComplex')
        if self.datafilespath.datafilespath and self.scalartypes.precision == 'double' and self.indextypes.integerSize == 32:
          rjobs.append('DATAFILESPATH')
          if hasattr(self.compilers, 'CXX'):
            rjobs.append('Cxx_DATAFILESPATH')
      # add jobs for each external package BUGBUGBUG may be run before all packages
      # Note: do these tests only for non-complex builds
      if self.scalartypes.scalartype.lower() != 'complex':
        for i in self.framework.packages:
          if not i.name.upper() in ['SOWING','C2HTML','BLASLAPACK','MPI','SCALAPACK','PTHREAD','CUDA','THRUST','VALGRIND','NUMDIFF','FBLASLAPACK','MAKE','MPICH','MATLABENGINE','HWLOC']:
            ejobs.append(i.name.upper())
          # horrible python here
          if i.name.upper() == 'MOAB':
            for j in self.framework.packages:
              if j.name.upper() == 'HDF5':
                ejobs.append('MOAB_HDF5')
      else:
        for i in self.framework.packages:
          if i.name.upper() == 'FFTW':
            jobs.append('FFTW_COMPLEX')

    self.addMakeMacro('TEST_RUNS',' '.join(jobs)+' '+' '.join(ejobs)+' '+' '.join(rjobs))
    return


  def configure(self):
    self.executeTest(self.configureRegression)
    return
