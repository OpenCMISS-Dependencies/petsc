
#include <petsc-private/snesimpl.h>       /*I   "petsc-private/snesimpl.h"   I*/
#include <petscdm.h>
#include <petscblaslapack.h>

#undef __FUNCT__
#define __FUNCT__ "SNESMonitorSolution"
/*@C
   SNESMonitorSolution - Monitors progress of the SNES solvers by calling
   VecView() for the approximate solution at each iteration.

   Collective on SNES

   Input Parameters:
+  snes - the SNES context
.  its - iteration number
.  fgnorm - 2-norm of residual
-  dummy - either a viewer or NULL

   Level: intermediate

.keywords: SNES, nonlinear, vector, monitor, view

.seealso: SNESMonitorSet(), SNESMonitorDefault(), VecView()
@*/
PetscErrorCode  SNESMonitorSolution(SNES snes,PetscInt its,PetscReal fgnorm,void *dummy)
{
  PetscErrorCode ierr;
  Vec            x;
  PetscViewer    viewer = (PetscViewer) dummy;

  PetscFunctionBegin;
  ierr = SNESGetSolution(snes,&x);CHKERRQ(ierr);
  if (!viewer) {
    MPI_Comm comm;
    ierr   = PetscObjectGetComm((PetscObject)snes,&comm);CHKERRQ(ierr);
    viewer = PETSC_VIEWER_DRAW_(comm);
  }
  ierr = VecView(x,viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESMonitorResidual"
/*@C
   SNESMonitorResidual - Monitors progress of the SNES solvers by calling
   VecView() for the residual at each iteration.

   Collective on SNES

   Input Parameters:
+  snes - the SNES context
.  its - iteration number
.  fgnorm - 2-norm of residual
-  dummy - either a viewer or NULL

   Level: intermediate

.keywords: SNES, nonlinear, vector, monitor, view

.seealso: SNESMonitorSet(), SNESMonitorDefault(), VecView()
@*/
PetscErrorCode  SNESMonitorResidual(SNES snes,PetscInt its,PetscReal fgnorm,void *dummy)
{
  PetscErrorCode ierr;
  Vec            x;
  PetscViewer    viewer = (PetscViewer) dummy;

  PetscFunctionBegin;
  ierr = SNESGetFunction(snes,&x,0,0);CHKERRQ(ierr);
  if (!viewer) {
    MPI_Comm comm;
    ierr   = PetscObjectGetComm((PetscObject)snes,&comm);CHKERRQ(ierr);
    viewer = PETSC_VIEWER_DRAW_(comm);
  }
  ierr = VecView(x,viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESMonitorSolutionUpdate"
/*@C
   SNESMonitorSolutionUpdate - Monitors progress of the SNES solvers by calling
   VecView() for the UPDATE to the solution at each iteration.

   Collective on SNES

   Input Parameters:
+  snes - the SNES context
.  its - iteration number
.  fgnorm - 2-norm of residual
-  dummy - either a viewer or NULL

   Level: intermediate

.keywords: SNES, nonlinear, vector, monitor, view

.seealso: SNESMonitorSet(), SNESMonitorDefault(), VecView()
@*/
PetscErrorCode  SNESMonitorSolutionUpdate(SNES snes,PetscInt its,PetscReal fgnorm,void *dummy)
{
  PetscErrorCode ierr;
  Vec            x;
  PetscViewer    viewer = (PetscViewer) dummy;

  PetscFunctionBegin;
  ierr = SNESGetSolutionUpdate(snes,&x);CHKERRQ(ierr);
  if (!viewer) {
    MPI_Comm comm;
    ierr   = PetscObjectGetComm((PetscObject)snes,&comm);CHKERRQ(ierr);
    viewer = PETSC_VIEWER_DRAW_(comm);
  }
  ierr = VecView(x,viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESMonitorDefault"
/*@C
   SNESMonitorDefault - Monitors progress of the SNES solvers (default).

   Collective on SNES

   Input Parameters:
+  snes - the SNES context
.  its - iteration number
.  fgnorm - 2-norm of residual
-  dummy - unused context

   Notes:
   This routine prints the residual norm at each iteration.

   Level: intermediate

.keywords: SNES, nonlinear, default, monitor, norm

.seealso: SNESMonitorSet(), SNESMonitorSolution()
@*/
PetscErrorCode  SNESMonitorDefault(SNES snes,PetscInt its,PetscReal fgnorm,void *dummy)
{
  PetscErrorCode ierr;
  PetscViewer    viewer = dummy ? (PetscViewer) dummy : PETSC_VIEWER_STDOUT_(PetscObjectComm((PetscObject)snes));

  PetscFunctionBegin;
  ierr = PetscViewerASCIIAddTab(viewer,((PetscObject)snes)->tablevel);CHKERRQ(ierr);
  ierr = PetscViewerASCIIPrintf(viewer,"%3D SNES Function norm %14.12e \n",its,(double)fgnorm);CHKERRQ(ierr);
  ierr = PetscViewerASCIISubtractTab(viewer,((PetscObject)snes)->tablevel);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESMonitorJacUpdateSpectrum"
PetscErrorCode SNESMonitorJacUpdateSpectrum(SNES snes,PetscInt it,PetscReal fnorm,void *ctx)
{
#if defined(PETSC_MISSING_LAPACK_GEEV)
  SETERRQ(PetscObjectComm((PetscObject)snes),PETSC_ERR_SUP,"GEEV - Lapack routine is unavailable\nNot able to provide eigen values.");
#elif defined(PETSC_HAVE_ESSL)
  SETERRQ(PetscObjectComm((PetscObject)snes),PETSC_ERR_SUP,"GEEV - No support for ESSL Lapack Routines");
#else
  Vec            X;
  Mat            J,dJ,dJdense;
  PetscErrorCode ierr;
  PetscErrorCode (*func)(SNES,Vec,Mat,Mat,void*);
  PetscInt       n,i;
  PetscBLASInt   nb,lwork;
  PetscReal      *eigr,*eigi;
  PetscScalar    *work;
  PetscScalar    *a;

  PetscFunctionBegin;
  if (it == 0) PetscFunctionReturn(0);
  /* create the difference between the current update and the current jacobian */
  ierr = SNESGetSolution(snes,&X);CHKERRQ(ierr);
  ierr = SNESGetJacobian(snes,NULL,&J,&func,NULL);CHKERRQ(ierr);
  ierr = MatDuplicate(J,MAT_COPY_VALUES,&dJ);CHKERRQ(ierr);
  ierr = SNESComputeJacobian(snes,X,dJ,dJ);CHKERRQ(ierr);
  ierr = MatAXPY(dJ,-1.0,J,SAME_NONZERO_PATTERN);CHKERRQ(ierr);

  /* compute the spectrum directly */
  ierr  = MatConvert(dJ,MATSEQDENSE,MAT_INITIAL_MATRIX,&dJdense);CHKERRQ(ierr);
  ierr  = MatGetSize(dJ,&n,NULL);CHKERRQ(ierr);
  ierr  = PetscBLASIntCast(n,&nb);CHKERRQ(ierr);
  lwork = 3*nb;
  ierr  = PetscMalloc1(n,&eigr);CHKERRQ(ierr);
  ierr  = PetscMalloc1(n,&eigi);CHKERRQ(ierr);
  ierr  = PetscMalloc1(lwork,&work);CHKERRQ(ierr);
  ierr  = MatDenseGetArray(dJdense,&a);CHKERRQ(ierr);
#if !defined(PETSC_USE_COMPLEX)
  {
    PetscBLASInt lierr;
    ierr = PetscFPTrapPush(PETSC_FP_TRAP_OFF);CHKERRQ(ierr);
    PetscStackCallBLAS("LAPACKgeev",LAPACKgeev_("N","N",&nb,a,&nb,eigr,eigi,NULL,&nb,NULL,&nb,work,&lwork,&lierr));
    if (lierr) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"geev() error %d",lierr);
    ierr = PetscFPTrapPop();CHKERRQ(ierr);
  }
#else
  SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Not coded for complex");
#endif
  PetscPrintf(PetscObjectComm((PetscObject)snes),"Eigenvalues of J_%d - J_%d:\n",it,it-1);CHKERRQ(ierr);
  for (i=0;i<n;i++) {
    PetscPrintf(PetscObjectComm((PetscObject)snes),"%5d: %20.5g + %20.5gi\n",i,(double)eigr[i],(double)eigi[i]);CHKERRQ(ierr);
  }
  ierr = MatDenseRestoreArray(dJdense,&a);CHKERRQ(ierr);
  ierr = MatDestroy(&dJ);CHKERRQ(ierr);
  ierr = MatDestroy(&dJdense);CHKERRQ(ierr);
  ierr = PetscFree(eigr);CHKERRQ(ierr);
  ierr = PetscFree(eigi);CHKERRQ(ierr);
  ierr = PetscFree(work);CHKERRQ(ierr);
  PetscFunctionReturn(0);
#endif
}

#undef __FUNCT__
#define __FUNCT__ "SNESMonitorRange_Private"
PetscErrorCode  SNESMonitorRange_Private(SNES snes,PetscInt it,PetscReal *per)
{
  PetscErrorCode ierr;
  Vec            resid;
  PetscReal      rmax,pwork;
  PetscInt       i,n,N;
  PetscScalar    *r;

  PetscFunctionBegin;
  ierr  = SNESGetFunction(snes,&resid,0,0);CHKERRQ(ierr);
  ierr  = VecNorm(resid,NORM_INFINITY,&rmax);CHKERRQ(ierr);
  ierr  = VecGetLocalSize(resid,&n);CHKERRQ(ierr);
  ierr  = VecGetSize(resid,&N);CHKERRQ(ierr);
  ierr  = VecGetArray(resid,&r);CHKERRQ(ierr);
  pwork = 0.0;
  for (i=0; i<n; i++) {
    pwork += (PetscAbsScalar(r[i]) > .20*rmax);
  }
  ierr = MPI_Allreduce(&pwork,per,1,MPIU_REAL,MPIU_SUM,PetscObjectComm((PetscObject)snes));CHKERRQ(ierr);
  ierr = VecRestoreArray(resid,&r);CHKERRQ(ierr);
  *per = *per/N;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESMonitorRange"
/*@C
   SNESMonitorRange - Prints the percentage of residual elements that are more then 10 percent of the maximum value.

   Collective on SNES

   Input Parameters:
+  snes   - iterative context
.  it    - iteration number
.  rnorm - 2-norm (preconditioned) residual value (may be estimated).
-  dummy - unused monitor context

   Options Database Key:
.  -snes_monitor_range - Activates SNESMonitorRange()

   Level: intermediate

.keywords: SNES, default, monitor, residual

.seealso: SNESMonitorSet(), SNESMonitorDefault(), SNESMonitorLGCreate()
@*/
PetscErrorCode  SNESMonitorRange(SNES snes,PetscInt it,PetscReal rnorm,void *dummy)
{
  PetscErrorCode ierr;
  PetscReal      perc,rel;
  PetscViewer    viewer = dummy ? (PetscViewer) dummy : PETSC_VIEWER_STDOUT_(PetscObjectComm((PetscObject)snes));
  /* should be in a MonitorRangeContext */
  static PetscReal prev;

  PetscFunctionBegin;
  if (!it) prev = rnorm;
  ierr = SNESMonitorRange_Private(snes,it,&perc);CHKERRQ(ierr);

  rel  = (prev - rnorm)/prev;
  prev = rnorm;
  ierr = PetscViewerASCIIAddTab(viewer,((PetscObject)snes)->tablevel);CHKERRQ(ierr);
  ierr = PetscViewerASCIIPrintf(viewer,"%3D SNES preconditioned resid norm %14.12e Percent values above 20 percent of maximum %5.2f relative decrease %5.2e ratio %5.2e \n",it,(double)rnorm,(double)(100.0*perc),(double)rel,(double)(rel/perc));CHKERRQ(ierr);
  ierr = PetscViewerASCIISubtractTab(viewer,((PetscObject)snes)->tablevel);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

typedef struct {
  PetscViewer viewer;
  PetscReal   *history;
} SNESMonitorRatioContext;

#undef __FUNCT__
#define __FUNCT__ "SNESMonitorRatio"
/*@C
   SNESMonitorRatio - Monitors progress of the SNES solvers by printing the ratio
   of residual norm at each iteration to the previous.

   Collective on SNES

   Input Parameters:
+  snes - the SNES context
.  its - iteration number
.  fgnorm - 2-norm of residual (or gradient)
-  dummy -  context of monitor

   Level: intermediate

.keywords: SNES, nonlinear, monitor, norm

.seealso: SNESMonitorSet(), SNESMonitorSolution()
@*/
PetscErrorCode  SNESMonitorRatio(SNES snes,PetscInt its,PetscReal fgnorm,void *dummy)
{
  PetscErrorCode          ierr;
  PetscInt                len;
  PetscReal               *history;
  SNESMonitorRatioContext *ctx = (SNESMonitorRatioContext*)dummy;

  PetscFunctionBegin;
  ierr = SNESGetConvergenceHistory(snes,&history,NULL,&len);CHKERRQ(ierr);
  ierr = PetscViewerASCIIAddTab(ctx->viewer,((PetscObject)snes)->tablevel);CHKERRQ(ierr);
  if (!its || !history || its > len) {
    ierr = PetscViewerASCIIPrintf(ctx->viewer,"%3D SNES Function norm %14.12e \n",its,(double)fgnorm);CHKERRQ(ierr);
  } else {
    PetscReal ratio = fgnorm/history[its-1];
    ierr = PetscViewerASCIIPrintf(ctx->viewer,"%3D SNES Function norm %14.12e %14.12e \n",its,(double)fgnorm,(double)ratio);CHKERRQ(ierr);
  }
  ierr = PetscViewerASCIISubtractTab(ctx->viewer,((PetscObject)snes)->tablevel);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
   If the we set the history monitor space then we must destroy it
*/
#undef __FUNCT__
#define __FUNCT__ "SNESMonitorRatioDestroy"
PetscErrorCode SNESMonitorRatioDestroy(void **ct)
{
  PetscErrorCode          ierr;
  SNESMonitorRatioContext *ctx = *(SNESMonitorRatioContext**)ct;

  PetscFunctionBegin;
  ierr = PetscFree(ctx->history);CHKERRQ(ierr);
  ierr = PetscViewerDestroy(&ctx->viewer);CHKERRQ(ierr);
  ierr = PetscFree(ctx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESMonitorSetRatio"
/*@C
   SNESMonitorSetRatio - Sets SNES to use a monitor that prints the
   ratio of the function norm at each iteration.

   Collective on SNES

   Input Parameters:
+   snes - the SNES context
-   viewer - ASCII viewer to print output

   Level: intermediate

.keywords: SNES, nonlinear, monitor, norm

.seealso: SNESMonitorSet(), SNESMonitorSolution(), SNESMonitorDefault()
@*/
PetscErrorCode  SNESMonitorSetRatio(SNES snes,PetscViewer viewer)
{
  PetscErrorCode          ierr;
  SNESMonitorRatioContext *ctx;
  PetscReal               *history;

  PetscFunctionBegin;
  if (!viewer) {
    ierr = PetscViewerASCIIOpen(PetscObjectComm((PetscObject)snes),"stdout",&viewer);CHKERRQ(ierr);
    ierr = PetscObjectReference((PetscObject)viewer);CHKERRQ(ierr);
  }
  ierr = PetscNewLog(snes,&ctx);CHKERRQ(ierr);
  ierr = SNESGetConvergenceHistory(snes,&history,NULL,NULL);CHKERRQ(ierr);
  if (!history) {
    ierr = PetscMalloc1(100,&ctx->history);CHKERRQ(ierr);
    ierr = SNESSetConvergenceHistory(snes,ctx->history,0,100,PETSC_TRUE);CHKERRQ(ierr);
  }
  ctx->viewer = viewer;

  ierr = SNESMonitorSet(snes,SNESMonitorRatio,ctx,SNESMonitorRatioDestroy);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/* ---------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "SNESMonitorDefaultShort"
/*
     Default (short) SNES Monitor, same as SNESMonitorDefault() except
  it prints fewer digits of the residual as the residual gets smaller.
  This is because the later digits are meaningless and are often
  different on different machines; by using this routine different
  machines will usually generate the same output.
*/
PetscErrorCode  SNESMonitorDefaultShort(SNES snes,PetscInt its,PetscReal fgnorm,void *dummy)
{
  PetscErrorCode ierr;
  PetscViewer    viewer = dummy ? (PetscViewer) dummy : PETSC_VIEWER_STDOUT_(PetscObjectComm((PetscObject)snes));

  PetscFunctionBegin;
  ierr = PetscViewerASCIIAddTab(viewer,((PetscObject)snes)->tablevel);CHKERRQ(ierr);
  if (fgnorm > 1.e-9) {
    ierr = PetscViewerASCIIPrintf(viewer,"%3D SNES Function norm %g \n",its,(double)fgnorm);CHKERRQ(ierr);
  } else if (fgnorm > 1.e-11) {
    ierr = PetscViewerASCIIPrintf(viewer,"%3D SNES Function norm %5.3e \n",its,(double)fgnorm);CHKERRQ(ierr);
  } else {
    ierr = PetscViewerASCIIPrintf(viewer,"%3D SNES Function norm < 1.e-11\n",its);CHKERRQ(ierr);
  }
  ierr = PetscViewerASCIISubtractTab(viewer,((PetscObject)snes)->tablevel);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
/* ---------------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "SNESConvergedDefault"
/*@C
   SNESConvergedDefault - Convergence test of the solvers for
   systems of nonlinear equations (default).

   Collective on SNES

   Input Parameters:
+  snes - the SNES context
.  it - the iteration (0 indicates before any Newton steps)
.  xnorm - 2-norm of current iterate
.  snorm - 2-norm of current step
.  fnorm - 2-norm of function at current iterate
-  dummy - unused context

   Output Parameter:
.   reason  - one of
$  SNES_CONVERGED_FNORM_ABS       - (fnorm < abstol),
$  SNES_CONVERGED_SNORM_RELATIVE  - (snorm < stol*xnorm),
$  SNES_CONVERGED_FNORM_RELATIVE  - (fnorm < rtol*fnorm0),
$  SNES_DIVERGED_FUNCTION_COUNT   - (nfct > maxf),
$  SNES_DIVERGED_FNORM_NAN        - (fnorm == NaN),
$  SNES_CONVERGED_ITERATING       - (otherwise),

   where
+    maxf - maximum number of function evaluations,
            set with SNESSetTolerances()
.    nfct - number of function evaluations,
.    abstol - absolute function norm tolerance,
            set with SNESSetTolerances()
-    rtol - relative function norm tolerance, set with SNESSetTolerances()

   Level: intermediate

.keywords: SNES, nonlinear, default, converged, convergence

.seealso: SNESSetConvergenceTest()
@*/
PetscErrorCode  SNESConvergedDefault(SNES snes,PetscInt it,PetscReal xnorm,PetscReal snorm,PetscReal fnorm,SNESConvergedReason *reason,void *dummy)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(snes,SNES_CLASSID,1);
  PetscValidPointer(reason,6);

  *reason = SNES_CONVERGED_ITERATING;

  if (!it) {
    /* set parameter for default relative tolerance convergence test */
    snes->ttol = fnorm*snes->rtol;
  }
  if (PetscIsInfOrNanReal(fnorm)) {
    ierr    = PetscInfo(snes,"Failed to converged, function norm is NaN\n");CHKERRQ(ierr);
    *reason = SNES_DIVERGED_FNORM_NAN;
  } else if (fnorm < snes->abstol) {
    ierr    = PetscInfo2(snes,"Converged due to function norm %14.12e < %14.12e\n",(double)fnorm,(double)snes->abstol);CHKERRQ(ierr);
    *reason = SNES_CONVERGED_FNORM_ABS;
  } else if (snes->nfuncs >= snes->max_funcs) {
    ierr    = PetscInfo2(snes,"Exceeded maximum number of function evaluations: %D > %D\n",snes->nfuncs,snes->max_funcs);CHKERRQ(ierr);
    *reason = SNES_DIVERGED_FUNCTION_COUNT;
  }

  if (it && !*reason) {
    if (fnorm <= snes->ttol) {
      ierr    = PetscInfo2(snes,"Converged due to function norm %14.12e < %14.12e (relative tolerance)\n",(double)fnorm,(double)snes->ttol);CHKERRQ(ierr);
      *reason = SNES_CONVERGED_FNORM_RELATIVE;
    } else if (snorm < snes->stol*xnorm) {
      ierr    = PetscInfo3(snes,"Converged due to small update length: %14.12e < %14.12e * %14.12e\n",(double)snorm,(double)snes->stol,(double)xnorm);CHKERRQ(ierr);
      *reason = SNES_CONVERGED_SNORM_RELATIVE;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESConvergedSkip"
/*@C
   SNESConvergedSkip - Convergence test for SNES that NEVER returns as
   converged, UNLESS the maximum number of iteration have been reached.

   Logically Collective on SNES

   Input Parameters:
+  snes - the SNES context
.  it - the iteration (0 indicates before any Newton steps)
.  xnorm - 2-norm of current iterate
.  snorm - 2-norm of current step
.  fnorm - 2-norm of function at current iterate
-  dummy - unused context

   Output Parameter:
.   reason  - SNES_CONVERGED_ITERATING, SNES_CONVERGED_ITS, or SNES_DIVERGED_FNORM_NAN

   Notes:
   Convergence is then declared after a fixed number of iterations have been used.

   Level: advanced

.keywords: SNES, nonlinear, skip, converged, convergence

.seealso: SNESSetConvergenceTest()
@*/
PetscErrorCode  SNESConvergedSkip(SNES snes,PetscInt it,PetscReal xnorm,PetscReal snorm,PetscReal fnorm,SNESConvergedReason *reason,void *dummy)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(snes,SNES_CLASSID,1);
  PetscValidPointer(reason,6);

  *reason = SNES_CONVERGED_ITERATING;

  if (fnorm != fnorm) {
    ierr    = PetscInfo(snes,"Failed to converged, function norm is NaN\n");CHKERRQ(ierr);
    *reason = SNES_DIVERGED_FNORM_NAN;
  } else if (it == snes->max_its) {
    *reason = SNES_CONVERGED_ITS;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESSetWorkVecs"
/*@C
  SNESSetWorkVecs - Gets a number of work vectors.

  Input Parameters:
. snes  - the SNES context
. nw - number of work vectors to allocate

   Level: developer

   Developers Note: This is PETSC_EXTERN because it may be used by user written plugin SNES implementations

@*/
PetscErrorCode SNESSetWorkVecs(SNES snes,PetscInt nw)
{
  DM             dm;
  Vec            v;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (snes->work) {ierr = VecDestroyVecs(snes->nwork,&snes->work);CHKERRQ(ierr);}
  snes->nwork = nw;

  ierr = SNESGetDM(snes, &dm);CHKERRQ(ierr);
  ierr = DMGetGlobalVector(dm, &v);CHKERRQ(ierr);
  ierr = VecDuplicateVecs(v,snes->nwork,&snes->work);CHKERRQ(ierr);
  ierr = DMRestoreGlobalVector(dm, &v);CHKERRQ(ierr);
  ierr = PetscLogObjectParents(snes,nw,snes->work);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
