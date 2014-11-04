#include <petsc-private/fortranimpl.h>
#include <petscpc.h>

#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define pcshellsetapply_           PCSHELLSETAPPLY
#define pcshellsetapplyrichardson_ PCSHELLSETAPPLYRICHARDSON
#define pcshellsetapplytranspose_  PCSHELLSETAPPLYTRANSPOSE
#define pcshellsetsetup_           PCSHELLSETSETUP
#define pcshellsetdestroy_         PCSHELLSETDESTROY
#define pcshellsetname_            PCSHELLSETNAME
#define pcshellgetname_            PCSHELLGETNAME
#define pcshellsetcontext_         PCSHELLSETCONTEXT
#define pcshellgetcontext_         PCSHELLGETCONTEXT
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define pcshellsetapply_           pcshellsetapply
#define pcshellsetapplyrichardson_ pcshellsetapplyrichardson
#define pcshellsetapplytranspose_  pcshellsetapplytranspose
#define pcshellsetsetup_           pcshellsetsetup
#define pcshellsetdestroy_         pcshellsetdestroy
#define pcshellsetname_            pcshellsetname
#define pcshellgetname_            pcshellgetname
#define pcshellsetcontext_         pcshellsetcontext
#define pcshellgetcontext_         pcshellgetcontext
#endif

/* These are not extern C because they are passed into non-extern C user level functions */
static PetscErrorCode ourshellapply(PC pc,Vec x,Vec y)
{
  PetscErrorCode ierr = 0;
  (*(void (PETSC_STDCALL *)(PC*,Vec*,Vec*,PetscErrorCode*))(((PetscObject)pc)->fortran_func_pointers[0]))(&pc,&x,&y,&ierr);CHKERRQ(ierr);
  return 0;
}

static PetscErrorCode ourshellapplyctx(PC pc,Vec x,Vec y)
{
  PetscErrorCode ierr = 0;
  void           *ctx;
  ierr = PCShellGetContext(pc,&ctx);CHKERRQ(ierr);
  (*(void (PETSC_STDCALL *)(PC*,void*,Vec*,Vec*,PetscErrorCode*))(((PetscObject)pc)->fortran_func_pointers[0]))(&pc,ctx,&x,&y,&ierr);CHKERRQ(ierr);
  return 0;
}

static PetscErrorCode ourapplyrichardson(PC pc,Vec x,Vec y,Vec w,PetscReal rtol,PetscReal abstol,PetscReal dtol,PetscInt m,PetscBool guesszero,PetscInt *outits,PCRichardsonConvergedReason *reason)
{
  PetscErrorCode ierr = 0;
  (*(void (PETSC_STDCALL *)(PC*,Vec*,Vec*,Vec*,PetscReal*,PetscReal*,PetscReal*,PetscInt*,PetscBool *,PetscInt*,PCRichardsonConvergedReason*,PetscErrorCode*))(((PetscObject)pc)->fortran_func_pointers[1]))(&pc,&x,&y,&w,&rtol,&abstol,&dtol,&m,&guesszero,outits,reason,&ierr);CHKERRQ(ierr);
  return 0;
}

static PetscErrorCode ourshellapplytranspose(PC pc,Vec x,Vec y)
{
  PetscErrorCode ierr = 0;
  (*(void (PETSC_STDCALL *)(void*,Vec*,Vec*,PetscErrorCode*))(((PetscObject)pc)->fortran_func_pointers[2]))(&pc,&x,&y,&ierr);CHKERRQ(ierr);
  return 0;
}

static PetscErrorCode ourshellsetup(PC pc)
{
  PetscErrorCode ierr = 0;
  (*(void (PETSC_STDCALL *)(PC*,PetscErrorCode*))(((PetscObject)pc)->fortran_func_pointers[3]))(&pc,&ierr);CHKERRQ(ierr);
  return 0;
}

static PetscErrorCode ourshellsetupctx(PC pc)
{
  PetscErrorCode ierr = 0;
  void           *ctx;
  ierr = PCShellGetContext(pc,&ctx);CHKERRQ(ierr);
  (*(void (PETSC_STDCALL *)(PC*,void*,PetscErrorCode*))(((PetscObject)pc)->fortran_func_pointers[3]))(&pc,ctx,&ierr);CHKERRQ(ierr);
  return 0;
}

static PetscErrorCode ourshelldestroy(PC pc)
{
  PetscErrorCode ierr = 0;
  (*(void (PETSC_STDCALL *)(void*,PetscErrorCode*))(((PetscObject)pc)->fortran_func_pointers[4]))(&pc,&ierr);CHKERRQ(ierr);
  return 0;
}

PETSC_EXTERN void PETSC_STDCALL pcshellgetcontext_(PC *pc,void **ctx,PetscErrorCode *ierr)
{
  *ierr = PCShellGetContext(*pc,ctx);
}

PETSC_EXTERN void PETSC_STDCALL pcshellsetapply_(PC *pc,void (PETSC_STDCALL *apply)(void*,Vec*,Vec*,PetscErrorCode*),PetscErrorCode *ierr)
{
  PetscObjectAllocateFortranPointers(*pc,5);
  ((PetscObject)*pc)->fortran_func_pointers[0] = (PetscVoidFunction)apply;

  *ierr = PCShellSetApply(*pc,ourshellapply);
}

PETSC_EXTERN void PETSC_STDCALL pcshellsetapplyctx_(PC *pc,void (PETSC_STDCALL *apply)(void*,void*,Vec*,Vec*,PetscErrorCode*),PetscErrorCode *ierr)
{
  PetscObjectAllocateFortranPointers(*pc,5);
  ((PetscObject)*pc)->fortran_func_pointers[0] = (PetscVoidFunction)apply;

  *ierr = PCShellSetApply(*pc,ourshellapplyctx);
}

PETSC_EXTERN void PETSC_STDCALL pcshellsetapplyrichardson_(PC *pc,void (PETSC_STDCALL *apply)(void*,Vec*,Vec*,Vec*,PetscReal*,PetscReal*,PetscReal*,PetscInt*,PetscBool*,PetscInt*,PCRichardsonConvergedReason*,PetscErrorCode*),PetscErrorCode *ierr)
{
  PetscObjectAllocateFortranPointers(*pc,5);
  ((PetscObject)*pc)->fortran_func_pointers[1] = (PetscVoidFunction)apply;
  *ierr = PCShellSetApplyRichardson(*pc,ourapplyrichardson);
}

PETSC_EXTERN void PETSC_STDCALL pcshellsetapplytranspose_(PC *pc,void (PETSC_STDCALL *applytranspose)(void*,Vec *,Vec *,PetscErrorCode*), PetscErrorCode *ierr)
{
  PetscObjectAllocateFortranPointers(*pc,5);
  ((PetscObject)*pc)->fortran_func_pointers[2] = (PetscVoidFunction)applytranspose;

  *ierr = PCShellSetApplyTranspose(*pc,ourshellapplytranspose);
}

PETSC_EXTERN void PETSC_STDCALL pcshellsetsetupctx_(PC *pc,void (PETSC_STDCALL *setup)(void*,void*,PetscErrorCode*),PetscErrorCode *ierr)
{
  PetscObjectAllocateFortranPointers(*pc,5);
  ((PetscObject)*pc)->fortran_func_pointers[3] = (PetscVoidFunction)setup;

  *ierr = PCShellSetSetUp(*pc,ourshellsetupctx);
}

PETSC_EXTERN void PETSC_STDCALL pcshellsetsetup_(PC *pc,void (PETSC_STDCALL *setup)(void*,PetscErrorCode*),PetscErrorCode *ierr)
{
  PetscObjectAllocateFortranPointers(*pc,5);
  ((PetscObject)*pc)->fortran_func_pointers[3] = (PetscVoidFunction)setup;

  *ierr = PCShellSetSetUp(*pc,ourshellsetup);
}

PETSC_EXTERN void PETSC_STDCALL pcshellsetdestroy_(PC *pc,void (PETSC_STDCALL *setup)(void*,PetscErrorCode*),PetscErrorCode *ierr)
{
  PetscObjectAllocateFortranPointers(*pc,5);
  ((PetscObject)*pc)->fortran_func_pointers[4] = (PetscVoidFunction)setup;

  *ierr = PCShellSetDestroy(*pc,ourshelldestroy);
}

PETSC_EXTERN void PETSC_STDCALL pcshellsetname_(PC *pc,CHAR name PETSC_MIXED_LEN(len), PetscErrorCode *ierr PETSC_END_LEN(len))
{
  char *c;
  FIXCHAR(name,len,c);
  *ierr = PCShellSetName(*pc,c);
  FREECHAR(name,c);
}

PETSC_EXTERN void PETSC_STDCALL pcshellgetname_(PC *pc,CHAR name PETSC_MIXED_LEN(len), PetscErrorCode *ierr PETSC_END_LEN(len))
{
  const char *c;

  *ierr = PCShellGetName(*pc,&c);if (*ierr) return;
  *ierr = PetscStrncpy(name,c,len);
}

/* -----------------------------------------------------------------*/

