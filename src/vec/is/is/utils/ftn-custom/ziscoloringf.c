#include <petsc-private/fortranimpl.h>
#include <petscis.h>
#include <petscviewer.h>

#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define iscoloringview_        ISCOLORINGVIEW
#define iscoloringcreate_      ISCOLORINGCREATE
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define iscoloringview_        iscoloringview
#define iscoloringcreate_      iscoloringcreate
#endif

PETSC_EXTERN void PETSC_STDCALL iscoloringview_(ISColoring *iscoloring,PetscViewer *viewer,PetscErrorCode *ierr)
{
  PetscViewer v;
  PetscPatchDefaultViewers_Fortran(viewer,v);
  *ierr = ISColoringView(*iscoloring,v);
}



PETSC_EXTERN void PETSC_STDCALL iscoloringcreate_(MPI_Comm *comm,PetscInt *n,PetscInt *ncolors,PetscInt *colors,ISColoring *iscoloring,PetscErrorCode *ierr)
{
  ISColoringValue *color;
  PetscInt        i;

  /* copies the colors[] array since that is kept by the ISColoring that is created */
  *ierr = PetscMalloc1((*n+1),&color);if (*ierr) return;
  for (i=0; i<(*n); i++) {
    if (colors[i] > IS_COLORING_MAX) {
      *ierr = PetscError(PETSC_COMM_SELF,__LINE__,"ISColoringCreate_Fortran",__FILE__,PETSC_ERR_PLIB,PETSC_ERROR_INITIAL,"Color too large");
      return;
    }
    if (colors[i] < 0) {
      *ierr = PetscError(PETSC_COMM_SELF,__LINE__,"ISColoringCreate_Fortran",__FILE__,PETSC_ERR_PLIB,PETSC_ERROR_INITIAL,"Color cannot be negative");
      return;
    }
    color[i] = (ISColoringValue)colors[i];
  }
  *ierr = ISColoringCreate(MPI_Comm_f2c(*(MPI_Fint*)&*comm),*n,*ncolors,color,iscoloring);
}

