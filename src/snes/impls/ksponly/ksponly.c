
#include <petsc-private/snesimpl.h>

#undef __FUNCT__
#define __FUNCT__ "SNESSolve_KSPONLY"
static PetscErrorCode SNESSolve_KSPONLY(SNES snes)
{
  PetscErrorCode     ierr;
  PetscInt           lits;
  Vec                Y,X,F;
  KSPConvergedReason kspreason;

  PetscFunctionBegin;
  snes->numFailures            = 0;
  snes->numLinearSolveFailures = 0;
  snes->reason                 = SNES_CONVERGED_ITERATING;
  snes->iter                   = 0;
  snes->norm                   = 0.0;

  X = snes->vec_sol;
  F = snes->vec_func;
  Y = snes->vec_sol_update;

  ierr = SNESComputeFunction(snes,X,F);CHKERRQ(ierr);
  if (snes->domainerror) {
    snes->reason = SNES_DIVERGED_FUNCTION_DOMAIN;
    PetscFunctionReturn(0);
  }
  if (snes->numbermonitors) {
    PetscReal fnorm;
    ierr = VecNorm(F,NORM_2,&fnorm);CHKERRQ(ierr);
    ierr = SNESMonitor(snes,0,fnorm);CHKERRQ(ierr);
  }

  /* Call general purpose update function */
  if (snes->ops->update) {
    ierr = (*snes->ops->update)(snes, 0);CHKERRQ(ierr);
  }

  /* Solve J Y = F, where J is Jacobian matrix */
  ierr = SNESComputeJacobian(snes,X,snes->jacobian,snes->jacobian_pre);CHKERRQ(ierr);
  ierr = KSPSetOperators(snes->ksp,snes->jacobian,snes->jacobian_pre);CHKERRQ(ierr);
  ierr = KSPSolve(snes->ksp,F,Y);CHKERRQ(ierr);
  ierr = KSPGetConvergedReason(snes->ksp,&kspreason);CHKERRQ(ierr);
  if (kspreason < 0 && ++snes->numLinearSolveFailures >= snes->maxLinearSolveFailures) {
    ierr         = PetscInfo2(snes,"iter=%D, number linear solve failures %D greater than current SNES allowed, stopping solve\n",snes->iter,snes->numLinearSolveFailures);CHKERRQ(ierr);
    snes->reason = SNES_DIVERGED_LINEAR_SOLVE;
  } else snes->reason = SNES_CONVERGED_ITS;

  ierr              = KSPGetIterationNumber(snes->ksp,&lits);CHKERRQ(ierr);
  snes->linear_its += lits;
  ierr              = PetscInfo2(snes,"iter=%D, linear solve iterations=%D\n",snes->iter,lits);CHKERRQ(ierr);
  snes->iter++;

  /* Take the computed step. */
  ierr = VecAXPY(X,-1.0,Y);CHKERRQ(ierr);
  if (snes->numbermonitors) {
    PetscReal fnorm;
    ierr = SNESComputeFunction(snes,X,F);CHKERRQ(ierr);
    ierr = VecNorm(F,NORM_2,&fnorm);CHKERRQ(ierr);
    ierr = SNESMonitor(snes,1,fnorm);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESSetUp_KSPONLY"
static PetscErrorCode SNESSetUp_KSPONLY(SNES snes)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = SNESSetUpMatrices(snes);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESDestroy_KSPONLY"
static PetscErrorCode SNESDestroy_KSPONLY(SNES snes)
{

  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

/* -------------------------------------------------------------------------- */
/*MC
      SNESKSPONLY - Nonlinear solver that only performs one Newton step and does not compute any norms.
      The main purpose of this solver is to solve linear problems using the SNES interface, without
      any additional overhead in the form of vector operations.

   Level: beginner

.seealso:  SNESCreate(), SNES, SNESSetType(), SNESNEWTONLS, SNESNEWTONTR
M*/
#undef __FUNCT__
#define __FUNCT__ "SNESCreate_KSPONLY"
PETSC_EXTERN PetscErrorCode SNESCreate_KSPONLY(SNES snes)
{

  PetscFunctionBegin;
  snes->ops->setup          = SNESSetUp_KSPONLY;
  snes->ops->solve          = SNESSolve_KSPONLY;
  snes->ops->destroy        = SNESDestroy_KSPONLY;
  snes->ops->setfromoptions = 0;
  snes->ops->view           = 0;
  snes->ops->reset          = 0;

  snes->usesksp = PETSC_TRUE;
  snes->usespc  = PETSC_FALSE;

  snes->data = 0;
  PetscFunctionReturn(0);
}
