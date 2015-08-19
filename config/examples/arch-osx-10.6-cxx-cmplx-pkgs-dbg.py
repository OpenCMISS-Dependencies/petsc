#!/usr/bin/env python

configure_options = [
  '--with-cc=gcc',
  '--with-fc=gfortran', # http://hpc.sourceforge.net/
  '--with-cxx=g++',

  '--with-clanguage=cxx',
  '--with-scalar-type=complex',

  #'-download-fblaslapack=1',
  '--download-mpich=1',
  '--download-cmake=1',
  '--download-metis=1',
  '--download-parmetis=1',
  '--download-ptscotch=1',
  '--download-triangle=1',
  '--download-fftw=1',
  #'--download-superlu=1',
  #'--download-superlu_dist=1',
  '--download-scalapack=1',
  '--download-mumps=1',
  '--download-hdf5',
  '--download-petsc4py=1',
  '--download-mpi4py=1',
  # no with-cxx-dialect=C++11 support '--download-elemental=1',
  #'--download-sundials=1',
  #'--download-hypre=1',
  #'--download-suitesparse=1',
  #'--download-chaco=1',
  #'--download-spai=1',

  ]

if __name__ == '__main__':
  import sys,os
  sys.path.insert(0,os.path.abspath('config'))
  import configure
  configure.petsc_configure(configure_options)
