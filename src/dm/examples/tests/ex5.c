
/* This file created by Peter Mell   6/30/95 */

static char help[] = "Solves the one dimensional heat equation.\n\n";

#include <petscdm.h>
#include <petscdmda.h>
#include <petscdraw.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  PetscMPIInt    rank,size;
  PetscErrorCode ierr;
  PetscInt       M = 14,time_steps = 1000,w=1,s=1,localsize,j,i,mybase,myend;
  DM             da;
  PetscViewer    viewer;
  PetscDraw      draw;
  Vec            local,global,copy;
  PetscScalar    *localptr,*copyptr;
  PetscReal      h,k;

  ierr = PetscInitialize(&argc,&argv,(char*)0,help);CHKERRQ(ierr);

  ierr = PetscOptionsGetInt(NULL,"-M",&M,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,"-time",&time_steps,NULL);CHKERRQ(ierr);

  /* Set up the array */
  ierr = DMDACreate1d(PETSC_COMM_WORLD,DM_BOUNDARY_NONE,M,w,s,NULL,&da);CHKERRQ(ierr);
  ierr = DMCreateGlobalVector(da,&global);CHKERRQ(ierr);
  ierr = DMCreateLocalVector(da,&local);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(PETSC_COMM_WORLD,&rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size);CHKERRQ(ierr);

  /* Make copy of local array for doing updates */
  ierr = VecDuplicate(local,&copy);CHKERRQ(ierr);

  /* Set Up Display to Show Heat Graph */
  ierr = PetscViewerDrawOpen(PETSC_COMM_WORLD,0,"",80,480,500,160,&viewer);CHKERRQ(ierr);
  ierr = PetscViewerDrawGetDraw(viewer,0,&draw);CHKERRQ(ierr);
  ierr = PetscDrawSetDoubleBuffer(draw);CHKERRQ(ierr);

  /* determine starting point of each processor */
  ierr = VecGetOwnershipRange(global,&mybase,&myend);CHKERRQ(ierr);

  /* Initialize the Array */
  ierr = VecGetLocalSize (local,&localsize);CHKERRQ(ierr);
  ierr = VecGetArray (local,&localptr);CHKERRQ(ierr);
  ierr = VecGetArray (copy,&copyptr);CHKERRQ(ierr);

  localptr[0] = copyptr[0] = 0.0;

  localptr[localsize-1] = copyptr[localsize-1] = 1.0;
  for (i=1; i<localsize-1; i++) {
    j = (i-1) + mybase;

    localptr[i] = PetscSinReal((PETSC_PI*j*6)/((PetscReal)M) + 1.2 * PetscSinReal((PETSC_PI*j*2)/((PetscReal)M))) * 4+4;
  }

  ierr = VecRestoreArray(local,&localptr);CHKERRQ(ierr);
  ierr = VecRestoreArray(copy,&copyptr);CHKERRQ(ierr);
  ierr = DMLocalToGlobalBegin(da,local,INSERT_VALUES,global);CHKERRQ(ierr);
  ierr = DMLocalToGlobalEnd(da,local,INSERT_VALUES,global);CHKERRQ(ierr);

  /* Assign Parameters */
  h= 1.0/M;
  k= h*h/2.2;

  for (j=0; j<time_steps; j++) {

    /* Global to Local */
    ierr = DMGlobalToLocalBegin(da,global,INSERT_VALUES,local);CHKERRQ(ierr);
    ierr = DMGlobalToLocalEnd(da,global,INSERT_VALUES,local);CHKERRQ(ierr);

    /*Extract local array */
    ierr = VecGetArray(local,&localptr);CHKERRQ(ierr);
    ierr = VecGetArray (copy,&copyptr);CHKERRQ(ierr);

    /* Update Locally - Make array of new values */
    /* Note: I don't do anything for the first and last entry */
    for (i=1; i< localsize-1; i++) {
      copyptr[i] = localptr[i] + (k/(h*h)) * (localptr[i+1]-2.0*localptr[i]+localptr[i-1]);
    }

    ierr = VecRestoreArray(copy,&copyptr);CHKERRQ(ierr);
    ierr = VecRestoreArray(local,&localptr);CHKERRQ(ierr);

    /* Local to Global */
    ierr = DMLocalToGlobalBegin(da,copy,INSERT_VALUES,global);CHKERRQ(ierr);
    ierr = DMLocalToGlobalEnd(da,copy,INSERT_VALUES,global);CHKERRQ(ierr);

    /* View Wave */
    ierr = VecView(global,viewer);CHKERRQ(ierr);

  }

  ierr = PetscViewerDestroy(&viewer);CHKERRQ(ierr);
  ierr = VecDestroy(&copy);CHKERRQ(ierr);
  ierr = VecDestroy(&local);CHKERRQ(ierr);
  ierr = VecDestroy(&global);CHKERRQ(ierr);
  ierr = DMDestroy(&da);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}




