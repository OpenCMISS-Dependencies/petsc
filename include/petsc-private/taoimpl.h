#ifndef __TAO_IMPL_H
#define __TAO_IMPL_H

#include <petsctaolinesearch.h>
#include <petsc-private/petscimpl.h>
#include <petscksp.h>

typedef struct _TaoOps *TaoOps;

struct _TaoOps {
  /* Methods set by application */
    PetscErrorCode (*computeobjective)(Tao, Vec, PetscReal*, void*);
    PetscErrorCode (*computeobjectiveandgradient)(Tao, Vec, PetscReal*, Vec, void*);
    PetscErrorCode (*computegradient)(Tao, Vec, Vec, void*);
    PetscErrorCode (*computehessian)(Tao, Vec, Mat, Mat,  void*);
    PetscErrorCode (*computeseparableobjective)(Tao, Vec, Vec, void*);
    PetscErrorCode (*computeconstraints)(Tao, Vec, Vec, void*);
    PetscErrorCode (*computeinequalityconstraints)(Tao, Vec, Vec, void*);
    PetscErrorCode (*computeequalityconstraints)(Tao, Vec, Vec, void*);
    PetscErrorCode (*computejacobian)(Tao, Vec, Mat, Mat,  void*);
    PetscErrorCode (*computejacobianstate)(Tao, Vec, Mat, Mat, Mat,  void*);
    PetscErrorCode (*computejacobiandesign)(Tao, Vec, Mat, void*);
    PetscErrorCode (*computejacobianinequality)(Tao, Vec, Mat, Mat,  void*);
    PetscErrorCode (*computejacobianequality)(Tao, Vec, Mat, Mat,  void*);
    PetscErrorCode (*computebounds)(Tao, Vec, Vec, void*);

    PetscErrorCode (*convergencetest)(Tao,void*);
    PetscErrorCode (*convergencedestroy)(void*);

  /* Methods set by solver */
    PetscErrorCode (*computedual)(Tao, Vec, Vec);
    PetscErrorCode (*setup)(Tao);
    PetscErrorCode (*solve)(Tao);
    PetscErrorCode (*view)(Tao, PetscViewer);
    PetscErrorCode (*setfromoptions)(Tao);
    PetscErrorCode (*destroy)(Tao);
};

#define MAXTAOMONITORS 10

struct _p_Tao {
    PETSCHEADER(struct _TaoOps);
    void *user;
    void *user_objP;
    void *user_objgradP;
    void *user_gradP;
    void *user_hessP;
    void *user_sepobjP;
    void *user_conP;
    void *user_con_equalityP;
    void *user_con_inequalityP;
    void *user_jacP;
    void *user_jac_equalityP;
    void *user_jac_inequalityP;
    void *user_jac_stateP;
    void *user_jac_designP;
    void *user_boundsP;

    PetscErrorCode (*monitor[MAXTAOMONITORS])(Tao,void*);
    PetscErrorCode (*monitordestroy[MAXTAOMONITORS])(void**);
    void *monitorcontext[MAXTAOMONITORS];
    PetscInt numbermonitors;
    void *cnvP;
    TaoConvergedReason reason;

    PetscBool setupcalled;
    void *data;

    Vec solution;
    Vec gradient;
    Vec stepdirection;
    Vec XL;
    Vec XU;
    Vec IL;
    Vec IU;
    Vec DI;
    Vec DE;
    Mat hessian;
    Mat hessian_pre;
    Vec sep_objective;
    Vec constraints;
    Vec constraints_equality;
    Vec constraints_inequality;
    Mat jacobian;
    Mat jacobian_pre;
    Mat jacobian_inequality;
    Mat jacobian_inequality_pre;
    Mat jacobian_equality;
    Mat jacobian_equality_pre;
    Mat jacobian_state;
    Mat jacobian_state_inv;
    Mat jacobian_design;
    Mat jacobian_state_pre;
    Mat jacobian_design_pre;
    IS state_is;
    IS design_is;
    PetscReal step;
    PetscReal residual;
    PetscReal gnorm0;
    PetscReal cnorm;
    PetscReal cnorm0;
    PetscReal fc;


    PetscInt  max_it;
    PetscInt  max_funcs;
    PetscInt  max_constraints;
    PetscInt  nfuncs;
    PetscInt  ngrads;
    PetscInt  nfuncgrads;
    PetscInt  nhess;
    PetscInt  niter;
    PetscInt  nconstraints;
    PetscInt  niconstraints;
    PetscInt  neconstraints;
    PetscInt  njac;
    PetscInt  njac_equality;
    PetscInt  njac_inequality;
    PetscInt  njac_state;
    PetscInt  njac_design;

    PetscInt  ksp_its;


    TaoLineSearch linesearch;
    PetscBool lsflag; /* goes up when line search fails */
    KSP ksp;
    PetscReal trust0; /* initial trust region radius */
    PetscReal trust;  /* Current trust region */

    PetscReal fatol;
    PetscReal frtol;
    PetscReal gatol;
    PetscReal grtol;
    PetscReal gttol;
    PetscReal catol;
    PetscReal crtol;
    PetscReal xtol;
    PetscReal steptol;
    PetscReal fmin;

    PetscBool printreason;
    PetscBool viewsolution;
    PetscBool viewgradient;
    PetscBool viewconstraints;
    PetscBool viewhessian;
    PetscBool viewjacobian;

    TaoSubsetType subset_type;
    PetscInt      hist_max;/* Number of iteration histories to keep */
    PetscReal     *hist_obj; /* obj value at each iteration */
    PetscReal     *hist_resid; /* residual at each iteration */
    PetscReal     *hist_cnorm; /* constraint norm at each iteration */
    PetscInt      hist_len;
    PetscBool     hist_reset;
};

extern PetscLogEvent Tao_Solve, Tao_ObjectiveEval, Tao_ObjGradientEval, Tao_GradientEval, Tao_HessianEval, Tao_ConstraintsEval, Tao_JacobianEval;

#define TaoLogHistory(tao,obj,resid,cnorm) \
  { if (tao->hist_max > tao->hist_len) \
      { if (tao->hist_obj) tao->hist_obj[tao->hist_len]=obj;\
        if (tao->hist_resid) tao->hist_resid[tao->hist_len]=resid;\
        if (tao->hist_cnorm) tao->hist_cnorm[tao->hist_len]=cnorm;} \
    tao->hist_len++;\
  }

#endif

PETSC_INTERN PetscErrorCode TaoVecGetSubVec(Vec, IS, TaoSubsetType, PetscReal, Vec*);
PETSC_INTERN PetscErrorCode TaoMatGetSubMat(Mat, IS, Vec, TaoSubsetType, Mat*);
