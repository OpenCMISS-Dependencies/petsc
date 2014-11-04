
/*
  Used for testing AIJ matrix with all zeros.
*/

static char help[] = "Used for Solving a linear system where the matrix has all zeros.\n\n";
/*
 Example: mpiexec -n <np> ./ex35 -dof 2 -mat_view -check_final_residual
 */

#include <petscdm.h>
#include <petscdmda.h>
#include <petscksp.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  PetscErrorCode ierr;
  KSP            ksp;
  PC             pc;
  Vec            x,b;
  DM             da;
  Mat            A;
  PetscInt       dof=1;
  PetscBool      flg;
  PetscScalar    zero=0.0;

  PetscInitialize(&argc,&argv,(char*)0,help);
  ierr = PetscOptionsGetInt(NULL,"-dof",&dof,NULL);CHKERRQ(ierr);

  ierr = DMDACreate(PETSC_COMM_WORLD,&da);CHKERRQ(ierr);
  ierr = DMDASetDim(da,3);CHKERRQ(ierr);
  ierr = DMDASetBoundaryType(da,DM_BOUNDARY_NONE,DM_BOUNDARY_NONE,DM_BOUNDARY_NONE);CHKERRQ(ierr);
  ierr = DMDASetStencilType(da,DMDA_STENCIL_STAR);CHKERRQ(ierr);
  ierr = DMDASetSizes(da,3,3,3);CHKERRQ(ierr);
  ierr = DMDASetNumProcs(da,PETSC_DECIDE,PETSC_DECIDE,PETSC_DECIDE);CHKERRQ(ierr);
  ierr = DMDASetDof(da,dof);CHKERRQ(ierr);
  ierr = DMDASetStencilWidth(da,1);CHKERRQ(ierr);
  ierr = DMDASetOwnershipRanges(da,NULL,NULL,NULL);CHKERRQ(ierr);
  ierr = DMSetFromOptions(da);CHKERRQ(ierr);
  ierr = DMSetUp(da);CHKERRQ(ierr);

  ierr = DMCreateGlobalVector(da,&x);CHKERRQ(ierr);
  ierr = DMCreateGlobalVector(da,&b);CHKERRQ(ierr);
  ierr = DMSetMatType(da,MATAIJ);CHKERRQ(ierr);
  ierr = DMCreateMatrix(da,&A);CHKERRQ(ierr);
  ierr = VecSet(b,zero);CHKERRQ(ierr);

  /* Test sbaij matrix */
  flg  = PETSC_FALSE;
  ierr = PetscOptionsGetBool(NULL,"-test_sbaij",&flg,NULL);CHKERRQ(ierr);
  if (flg) {
    Mat sA;
    ierr = MatSetOption(A,MAT_SYMMETRIC,PETSC_TRUE);CHKERRQ(ierr);
    ierr = MatConvert(A,MATSBAIJ,MAT_INITIAL_MATRIX,&sA);CHKERRQ(ierr);
    ierr = MatDestroy(&A);CHKERRQ(ierr);
    A    = sA;
  }

  ierr = KSPCreate(PETSC_COMM_WORLD,&ksp);CHKERRQ(ierr);
  ierr = KSPSetFromOptions(ksp);CHKERRQ(ierr);
  ierr = KSPSetOperators(ksp,A,A);CHKERRQ(ierr);
  ierr = KSPGetPC(ksp,&pc);CHKERRQ(ierr);
  ierr = PCSetDM(pc,(DM)da);CHKERRQ(ierr);

  ierr = KSPSolve(ksp,b,x);CHKERRQ(ierr);

  /* check final residual */
  flg  = PETSC_FALSE;
  ierr = PetscOptionsGetBool(NULL, "-check_final_residual", &flg,NULL);CHKERRQ(ierr);
  if (flg) {
    Vec       b1;
    PetscReal norm;
    ierr = KSPGetSolution(ksp,&x);CHKERRQ(ierr);
    ierr = VecDuplicate(b,&b1);CHKERRQ(ierr);
    ierr = MatMult(A,x,b1);CHKERRQ(ierr);
    ierr = VecAXPY(b1,-1.0,b);CHKERRQ(ierr);
    ierr = VecNorm(b1,NORM_2,&norm);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD,"Final residual %g\n",norm);CHKERRQ(ierr);
    ierr = VecDestroy(&b1);CHKERRQ(ierr);
  }

  ierr = KSPDestroy(&ksp);CHKERRQ(ierr);
  ierr = VecDestroy(&x);CHKERRQ(ierr);
  ierr = VecDestroy(&b);CHKERRQ(ierr);
  ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = DMDestroy(&da);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}

