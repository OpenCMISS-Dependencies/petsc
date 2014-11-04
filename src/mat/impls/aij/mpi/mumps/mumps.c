
/*
    Provides an interface to the MUMPS sparse solver
*/

#include <../src/mat/impls/aij/mpi/mpiaij.h> /*I  "petscmat.h"  I*/
#include <../src/mat/impls/sbaij/mpi/mpisbaij.h>

EXTERN_C_BEGIN
#if defined(PETSC_USE_COMPLEX)
#if defined(PETSC_USE_REAL_SINGLE)
#include <cmumps_c.h>
#else
#include <zmumps_c.h>
#endif
#else
#if defined(PETSC_USE_REAL_SINGLE)
#include <smumps_c.h>
#else
#include <dmumps_c.h>
#endif
#endif
EXTERN_C_END
#define JOB_INIT -1
#define JOB_FACTSYMBOLIC 1
#define JOB_FACTNUMERIC 2
#define JOB_SOLVE 3
#define JOB_END -2

/* calls to MUMPS */
#if defined(PETSC_USE_COMPLEX)
#if defined(PETSC_USE_REAL_SINGLE)
#define PetscMUMPS_c cmumps_c
#else
#define PetscMUMPS_c zmumps_c
#endif
#else
#if defined(PETSC_USE_REAL_SINGLE)
#define PetscMUMPS_c smumps_c
#else
#define PetscMUMPS_c dmumps_c
#endif
#endif


/* macros s.t. indices match MUMPS documentation */
#define ICNTL(I) icntl[(I)-1]
#define CNTL(I) cntl[(I)-1]
#define INFOG(I) infog[(I)-1]
#define INFO(I) info[(I)-1]
#define RINFOG(I) rinfog[(I)-1]
#define RINFO(I) rinfo[(I)-1]

typedef struct {
#if defined(PETSC_USE_COMPLEX)
#if defined(PETSC_USE_REAL_SINGLE)
  CMUMPS_STRUC_C id;
#else
  ZMUMPS_STRUC_C id;
#endif
#else
#if defined(PETSC_USE_REAL_SINGLE)
  SMUMPS_STRUC_C id;
#else
  DMUMPS_STRUC_C id;
#endif
#endif

  MatStructure matstruc;
  PetscMPIInt  myid,size;
  PetscInt     *irn,*jcn,nz,sym;
  PetscScalar  *val;
  MPI_Comm     comm_mumps;
  VecScatter   scat_rhs, scat_sol;
  PetscBool    isAIJ,CleanUpMUMPS;
  Vec          b_seq,x_seq;
  PetscInt     ICNTL9_pre;   /* check if ICNTL(9) is changed from previous MatSolve */

  PetscErrorCode (*Destroy)(Mat);
  PetscErrorCode (*ConvertToTriples)(Mat, int, MatReuse, int*, int**, int**, PetscScalar**);
} Mat_MUMPS;

extern PetscErrorCode MatDuplicate_MUMPS(Mat,MatDuplicateOption,Mat*);


/* MatConvertToTriples_A_B */
/*convert Petsc matrix to triples: row[nz], col[nz], val[nz] */
/*
  input:
    A       - matrix in aij,baij or sbaij (bs=1) format
    shift   - 0: C style output triple; 1: Fortran style output triple.
    reuse   - MAT_INITIAL_MATRIX: spaces are allocated and values are set for the triple
              MAT_REUSE_MATRIX:   only the values in v array are updated
  output:
    nnz     - dim of r, c, and v (number of local nonzero entries of A)
    r, c, v - row and col index, matrix values (matrix triples)

  The returned values r, c, and sometimes v are obtained in a single PetscMalloc(). Then in MatDestroy_MUMPS() it is
  freed with PetscFree((mumps->irn);  This is not ideal code, the fact that v is ONLY sometimes part of mumps->irn means
  that the PetscMalloc() cannot easily be replaced with a PetscMalloc3(). 

 */

#undef __FUNCT__
#define __FUNCT__ "MatConvertToTriples_seqaij_seqaij"
PetscErrorCode MatConvertToTriples_seqaij_seqaij(Mat A,int shift,MatReuse reuse,int *nnz,int **r, int **c, PetscScalar **v)
{
  const PetscInt *ai,*aj,*ajj,M=A->rmap->n;
  PetscInt       nz,rnz,i,j;
  PetscErrorCode ierr;
  PetscInt       *row,*col;
  Mat_SeqAIJ     *aa=(Mat_SeqAIJ*)A->data;

  PetscFunctionBegin;
  *v=aa->a;
  if (reuse == MAT_INITIAL_MATRIX) {
    nz   = aa->nz;
    ai   = aa->i;
    aj   = aa->j;
    *nnz = nz;
    ierr = PetscMalloc1(2*nz, &row);CHKERRQ(ierr);
    col  = row + nz;

    nz = 0;
    for (i=0; i<M; i++) {
      rnz = ai[i+1] - ai[i];
      ajj = aj + ai[i];
      for (j=0; j<rnz; j++) {
        row[nz] = i+shift; col[nz++] = ajj[j] + shift;
      }
    }
    *r = row; *c = col;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatConvertToTriples_seqbaij_seqaij"
PetscErrorCode MatConvertToTriples_seqbaij_seqaij(Mat A,int shift,MatReuse reuse,int *nnz,int **r, int **c, PetscScalar **v)
{
  Mat_SeqBAIJ    *aa=(Mat_SeqBAIJ*)A->data;
  const PetscInt *ai,*aj,*ajj,bs2 = aa->bs2;
  PetscInt       bs,M,nz,idx=0,rnz,i,j,k,m;
  PetscErrorCode ierr;
  PetscInt       *row,*col;

  PetscFunctionBegin;
  ierr = MatGetBlockSize(A,&bs);CHKERRQ(ierr);
  M = A->rmap->N/bs;
  *v = aa->a;
  if (reuse == MAT_INITIAL_MATRIX) {
    ai   = aa->i; aj = aa->j;
    nz   = bs2*aa->nz;
    *nnz = nz;
    ierr = PetscMalloc1(2*nz, &row);CHKERRQ(ierr);
    col  = row + nz;

    for (i=0; i<M; i++) {
      ajj = aj + ai[i];
      rnz = ai[i+1] - ai[i];
      for (k=0; k<rnz; k++) {
        for (j=0; j<bs; j++) {
          for (m=0; m<bs; m++) {
            row[idx]   = i*bs + m + shift;
            col[idx++] = bs*(ajj[k]) + j + shift;
          }
        }
      }
    }
    *r = row; *c = col;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatConvertToTriples_seqsbaij_seqsbaij"
PetscErrorCode MatConvertToTriples_seqsbaij_seqsbaij(Mat A,int shift,MatReuse reuse,int *nnz,int **r, int **c, PetscScalar **v)
{
  const PetscInt *ai, *aj,*ajj,M=A->rmap->n;
  PetscInt       nz,rnz,i,j;
  PetscErrorCode ierr;
  PetscInt       *row,*col;
  Mat_SeqSBAIJ   *aa=(Mat_SeqSBAIJ*)A->data;

  PetscFunctionBegin;
  *v = aa->a;
  if (reuse == MAT_INITIAL_MATRIX) {
    nz   = aa->nz;
    ai   = aa->i;
    aj   = aa->j;
    *v   = aa->a;
    *nnz = nz;
    ierr = PetscMalloc1(2*nz, &row);CHKERRQ(ierr);
    col  = row + nz;

    nz = 0;
    for (i=0; i<M; i++) {
      rnz = ai[i+1] - ai[i];
      ajj = aj + ai[i];
      for (j=0; j<rnz; j++) {
        row[nz] = i+shift; col[nz++] = ajj[j] + shift;
      }
    }
    *r = row; *c = col;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatConvertToTriples_seqaij_seqsbaij"
PetscErrorCode MatConvertToTriples_seqaij_seqsbaij(Mat A,int shift,MatReuse reuse,int *nnz,int **r, int **c, PetscScalar **v)
{
  const PetscInt    *ai,*aj,*ajj,*adiag,M=A->rmap->n;
  PetscInt          nz,rnz,i,j;
  const PetscScalar *av,*v1;
  PetscScalar       *val;
  PetscErrorCode    ierr;
  PetscInt          *row,*col;
  Mat_SeqSBAIJ      *aa=(Mat_SeqSBAIJ*)A->data;

  PetscFunctionBegin;
  ai   =aa->i; aj=aa->j;av=aa->a;
  adiag=aa->diag;
  if (reuse == MAT_INITIAL_MATRIX) {
    nz   = M + (aa->nz-M)/2;
    *nnz = nz;
    ierr = PetscMalloc((2*nz*sizeof(PetscInt)+nz*sizeof(PetscScalar)), &row);CHKERRQ(ierr);
    col  = row + nz;
    val  = (PetscScalar*)(col + nz);

    nz = 0;
    for (i=0; i<M; i++) {
      rnz = ai[i+1] - adiag[i];
      ajj = aj + adiag[i];
      v1  = av + adiag[i];
      for (j=0; j<rnz; j++) {
        row[nz] = i+shift; col[nz] = ajj[j] + shift; val[nz++] = v1[j];
      }
    }
    *r = row; *c = col; *v = val;
  } else {
    nz = 0; val = *v;
    for (i=0; i <M; i++) {
      rnz = ai[i+1] - adiag[i];
      ajj = aj + adiag[i];
      v1  = av + adiag[i];
      for (j=0; j<rnz; j++) {
        val[nz++] = v1[j];
      }
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatConvertToTriples_mpisbaij_mpisbaij"
PetscErrorCode MatConvertToTriples_mpisbaij_mpisbaij(Mat A,int shift,MatReuse reuse,int *nnz,int **r, int **c, PetscScalar **v)
{
  const PetscInt    *ai, *aj, *bi, *bj,*garray,m=A->rmap->n,*ajj,*bjj;
  PetscErrorCode    ierr;
  PetscInt          rstart,nz,i,j,jj,irow,countA,countB;
  PetscInt          *row,*col;
  const PetscScalar *av, *bv,*v1,*v2;
  PetscScalar       *val;
  Mat_MPISBAIJ      *mat = (Mat_MPISBAIJ*)A->data;
  Mat_SeqSBAIJ      *aa  = (Mat_SeqSBAIJ*)(mat->A)->data;
  Mat_SeqBAIJ       *bb  = (Mat_SeqBAIJ*)(mat->B)->data;

  PetscFunctionBegin;
  ai=aa->i; aj=aa->j; bi=bb->i; bj=bb->j; rstart= A->rmap->rstart;
  av=aa->a; bv=bb->a;

  garray = mat->garray;

  if (reuse == MAT_INITIAL_MATRIX) {
    nz   = aa->nz + bb->nz;
    *nnz = nz;
    ierr = PetscMalloc((2*nz*sizeof(PetscInt)+nz*sizeof(PetscScalar)), &row);CHKERRQ(ierr);
    col  = row + nz;
    val  = (PetscScalar*)(col + nz);

    *r = row; *c = col; *v = val;
  } else {
    row = *r; col = *c; val = *v;
  }

  jj = 0; irow = rstart;
  for (i=0; i<m; i++) {
    ajj    = aj + ai[i];                 /* ptr to the beginning of this row */
    countA = ai[i+1] - ai[i];
    countB = bi[i+1] - bi[i];
    bjj    = bj + bi[i];
    v1     = av + ai[i];
    v2     = bv + bi[i];

    /* A-part */
    for (j=0; j<countA; j++) {
      if (reuse == MAT_INITIAL_MATRIX) {
        row[jj] = irow + shift; col[jj] = rstart + ajj[j] + shift;
      }
      val[jj++] = v1[j];
    }

    /* B-part */
    for (j=0; j < countB; j++) {
      if (reuse == MAT_INITIAL_MATRIX) {
        row[jj] = irow + shift; col[jj] = garray[bjj[j]] + shift;
      }
      val[jj++] = v2[j];
    }
    irow++;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatConvertToTriples_mpiaij_mpiaij"
PetscErrorCode MatConvertToTriples_mpiaij_mpiaij(Mat A,int shift,MatReuse reuse,int *nnz,int **r, int **c, PetscScalar **v)
{
  const PetscInt    *ai, *aj, *bi, *bj,*garray,m=A->rmap->n,*ajj,*bjj;
  PetscErrorCode    ierr;
  PetscInt          rstart,nz,i,j,jj,irow,countA,countB;
  PetscInt          *row,*col;
  const PetscScalar *av, *bv,*v1,*v2;
  PetscScalar       *val;
  Mat_MPIAIJ        *mat = (Mat_MPIAIJ*)A->data;
  Mat_SeqAIJ        *aa  = (Mat_SeqAIJ*)(mat->A)->data;
  Mat_SeqAIJ        *bb  = (Mat_SeqAIJ*)(mat->B)->data;

  PetscFunctionBegin;
  ai=aa->i; aj=aa->j; bi=bb->i; bj=bb->j; rstart= A->rmap->rstart;
  av=aa->a; bv=bb->a;

  garray = mat->garray;

  if (reuse == MAT_INITIAL_MATRIX) {
    nz   = aa->nz + bb->nz;
    *nnz = nz;
    ierr = PetscMalloc((2*nz*sizeof(PetscInt)+nz*sizeof(PetscScalar)), &row);CHKERRQ(ierr);
    col  = row + nz;
    val  = (PetscScalar*)(col + nz);

    *r = row; *c = col; *v = val;
  } else {
    row = *r; col = *c; val = *v;
  }

  jj = 0; irow = rstart;
  for (i=0; i<m; i++) {
    ajj    = aj + ai[i];                 /* ptr to the beginning of this row */
    countA = ai[i+1] - ai[i];
    countB = bi[i+1] - bi[i];
    bjj    = bj + bi[i];
    v1     = av + ai[i];
    v2     = bv + bi[i];

    /* A-part */
    for (j=0; j<countA; j++) {
      if (reuse == MAT_INITIAL_MATRIX) {
        row[jj] = irow + shift; col[jj] = rstart + ajj[j] + shift;
      }
      val[jj++] = v1[j];
    }

    /* B-part */
    for (j=0; j < countB; j++) {
      if (reuse == MAT_INITIAL_MATRIX) {
        row[jj] = irow + shift; col[jj] = garray[bjj[j]] + shift;
      }
      val[jj++] = v2[j];
    }
    irow++;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatConvertToTriples_mpibaij_mpiaij"
PetscErrorCode MatConvertToTriples_mpibaij_mpiaij(Mat A,int shift,MatReuse reuse,int *nnz,int **r, int **c, PetscScalar **v)
{
  Mat_MPIBAIJ       *mat    = (Mat_MPIBAIJ*)A->data;
  Mat_SeqBAIJ       *aa     = (Mat_SeqBAIJ*)(mat->A)->data;
  Mat_SeqBAIJ       *bb     = (Mat_SeqBAIJ*)(mat->B)->data;
  const PetscInt    *ai     = aa->i, *bi = bb->i, *aj = aa->j, *bj = bb->j,*ajj, *bjj;
  const PetscInt    *garray = mat->garray,mbs=mat->mbs,rstart=A->rmap->rstart;
  const PetscInt    bs2=mat->bs2;
  PetscErrorCode    ierr;
  PetscInt          bs,nz,i,j,k,n,jj,irow,countA,countB,idx;
  PetscInt          *row,*col;
  const PetscScalar *av=aa->a, *bv=bb->a,*v1,*v2;
  PetscScalar       *val;

  PetscFunctionBegin;
  ierr = MatGetBlockSize(A,&bs);CHKERRQ(ierr);
  if (reuse == MAT_INITIAL_MATRIX) {
    nz   = bs2*(aa->nz + bb->nz);
    *nnz = nz;
    ierr = PetscMalloc((2*nz*sizeof(PetscInt)+nz*sizeof(PetscScalar)), &row);CHKERRQ(ierr);
    col  = row + nz;
    val  = (PetscScalar*)(col + nz);

    *r = row; *c = col; *v = val;
  } else {
    row = *r; col = *c; val = *v;
  }

  jj = 0; irow = rstart;
  for (i=0; i<mbs; i++) {
    countA = ai[i+1] - ai[i];
    countB = bi[i+1] - bi[i];
    ajj    = aj + ai[i];
    bjj    = bj + bi[i];
    v1     = av + bs2*ai[i];
    v2     = bv + bs2*bi[i];

    idx = 0;
    /* A-part */
    for (k=0; k<countA; k++) {
      for (j=0; j<bs; j++) {
        for (n=0; n<bs; n++) {
          if (reuse == MAT_INITIAL_MATRIX) {
            row[jj] = irow + n + shift;
            col[jj] = rstart + bs*ajj[k] + j + shift;
          }
          val[jj++] = v1[idx++];
        }
      }
    }

    idx = 0;
    /* B-part */
    for (k=0; k<countB; k++) {
      for (j=0; j<bs; j++) {
        for (n=0; n<bs; n++) {
          if (reuse == MAT_INITIAL_MATRIX) {
            row[jj] = irow + n + shift;
            col[jj] = bs*garray[bjj[k]] + j + shift;
          }
          val[jj++] = v2[idx++];
        }
      }
    }
    irow += bs;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatConvertToTriples_mpiaij_mpisbaij"
PetscErrorCode MatConvertToTriples_mpiaij_mpisbaij(Mat A,int shift,MatReuse reuse,int *nnz,int **r, int **c, PetscScalar **v)
{
  const PetscInt    *ai, *aj,*adiag, *bi, *bj,*garray,m=A->rmap->n,*ajj,*bjj;
  PetscErrorCode    ierr;
  PetscInt          rstart,nz,nza,nzb,i,j,jj,irow,countA,countB;
  PetscInt          *row,*col;
  const PetscScalar *av, *bv,*v1,*v2;
  PetscScalar       *val;
  Mat_MPIAIJ        *mat =  (Mat_MPIAIJ*)A->data;
  Mat_SeqAIJ        *aa  =(Mat_SeqAIJ*)(mat->A)->data;
  Mat_SeqAIJ        *bb  =(Mat_SeqAIJ*)(mat->B)->data;

  PetscFunctionBegin;
  ai=aa->i; aj=aa->j; adiag=aa->diag;
  bi=bb->i; bj=bb->j; garray = mat->garray;
  av=aa->a; bv=bb->a;

  rstart = A->rmap->rstart;

  if (reuse == MAT_INITIAL_MATRIX) {
    nza = 0;    /* num of upper triangular entries in mat->A, including diagonals */
    nzb = 0;    /* num of upper triangular entries in mat->B */
    for (i=0; i<m; i++) {
      nza   += (ai[i+1] - adiag[i]);
      countB = bi[i+1] - bi[i];
      bjj    = bj + bi[i];
      for (j=0; j<countB; j++) {
        if (garray[bjj[j]] > rstart) nzb++;
      }
    }

    nz   = nza + nzb; /* total nz of upper triangular part of mat */
    *nnz = nz;
    ierr = PetscMalloc((2*nz*sizeof(PetscInt)+nz*sizeof(PetscScalar)), &row);CHKERRQ(ierr);
    col  = row + nz;
    val  = (PetscScalar*)(col + nz);

    *r = row; *c = col; *v = val;
  } else {
    row = *r; col = *c; val = *v;
  }

  jj = 0; irow = rstart;
  for (i=0; i<m; i++) {
    ajj    = aj + adiag[i];                 /* ptr to the beginning of the diagonal of this row */
    v1     = av + adiag[i];
    countA = ai[i+1] - adiag[i];
    countB = bi[i+1] - bi[i];
    bjj    = bj + bi[i];
    v2     = bv + bi[i];

    /* A-part */
    for (j=0; j<countA; j++) {
      if (reuse == MAT_INITIAL_MATRIX) {
        row[jj] = irow + shift; col[jj] = rstart + ajj[j] + shift;
      }
      val[jj++] = v1[j];
    }

    /* B-part */
    for (j=0; j < countB; j++) {
      if (garray[bjj[j]] > rstart) {
        if (reuse == MAT_INITIAL_MATRIX) {
          row[jj] = irow + shift; col[jj] = garray[bjj[j]] + shift;
        }
        val[jj++] = v2[j];
      }
    }
    irow++;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDestroy_MUMPS"
PetscErrorCode MatDestroy_MUMPS(Mat A)
{
  Mat_MUMPS      *mumps=(Mat_MUMPS*)A->spptr;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (mumps->CleanUpMUMPS) {
    /* Terminate instance, deallocate memories */
    ierr = PetscFree2(mumps->id.sol_loc,mumps->id.isol_loc);CHKERRQ(ierr);
    ierr = VecScatterDestroy(&mumps->scat_rhs);CHKERRQ(ierr);
    ierr = VecDestroy(&mumps->b_seq);CHKERRQ(ierr);
    ierr = VecScatterDestroy(&mumps->scat_sol);CHKERRQ(ierr);
    ierr = VecDestroy(&mumps->x_seq);CHKERRQ(ierr);
    ierr = PetscFree(mumps->id.perm_in);CHKERRQ(ierr);
    ierr = PetscFree(mumps->irn);CHKERRQ(ierr);

    mumps->id.job = JOB_END;
    PetscMUMPS_c(&mumps->id);
    ierr = MPI_Comm_free(&(mumps->comm_mumps));CHKERRQ(ierr);
  }
  if (mumps->Destroy) {
    ierr = (mumps->Destroy)(A);CHKERRQ(ierr);
  }
  ierr = PetscFree(A->spptr);CHKERRQ(ierr);

  /* clear composed functions */
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatFactorGetSolverPackage_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatMumpsSetIcntl_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatMumpsGetIcntl_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatMumpsSetCntl_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatMumpsGetCntl_C",NULL);CHKERRQ(ierr);

  ierr = PetscObjectComposeFunction((PetscObject)A,"MatMumpsGetInfo_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatMumpsGetInfog_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatMumpsGetRinfo_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatMumpsGetRinfog_C",NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSolve_MUMPS"
PetscErrorCode MatSolve_MUMPS(Mat A,Vec b,Vec x)
{
  Mat_MUMPS        *mumps=(Mat_MUMPS*)A->spptr;
  PetscScalar      *array;
  Vec              b_seq;
  IS               is_iden,is_petsc;
  PetscErrorCode   ierr;
  PetscInt         i;
  static PetscBool cite1 = PETSC_FALSE,cite2 = PETSC_FALSE;

  PetscFunctionBegin;
  ierr = PetscCitationsRegister("@article{MUMPS01,\n  author = {P.~R. Amestoy and I.~S. Duff and J.-Y. L'Excellent and J. Koster},\n  title = {A fully asynchronous multifrontal solver using distributed dynamic scheduling},\n  journal = {SIAM Journal on Matrix Analysis and Applications},\n  volume = {23},\n  number = {1},\n  pages = {15--41},\n  year = {2001}\n}\n",&cite1);CHKERRQ(ierr);
  ierr = PetscCitationsRegister("@article{MUMPS02,\n  author = {P.~R. Amestoy and A. Guermouche and J.-Y. L'Excellent and S. Pralet},\n  title = {Hybrid scheduling for the parallel solution of linear systems},\n  journal = {Parallel Computing},\n  volume = {32},\n  number = {2},\n  pages = {136--156},\n  year = {2006}\n}\n",&cite2);CHKERRQ(ierr);
  mumps->id.nrhs = 1;
  b_seq          = mumps->b_seq;
  if (mumps->size > 1) {
    /* MUMPS only supports centralized rhs. Scatter b into a seqential rhs vector */
    ierr = VecScatterBegin(mumps->scat_rhs,b,b_seq,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(mumps->scat_rhs,b,b_seq,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    if (!mumps->myid) {ierr = VecGetArray(b_seq,&array);CHKERRQ(ierr);}
  } else {  /* size == 1 */
    ierr = VecCopy(b,x);CHKERRQ(ierr);
    ierr = VecGetArray(x,&array);CHKERRQ(ierr);
  }
  if (!mumps->myid) { /* define rhs on the host */
    mumps->id.nrhs = 1;
#if defined(PETSC_USE_COMPLEX)
#if defined(PETSC_USE_REAL_SINGLE)
    mumps->id.rhs = (mumps_complex*)array;
#else
    mumps->id.rhs = (mumps_double_complex*)array;
#endif
#else
    mumps->id.rhs = array;
#endif
  }

  /* solve phase */
  /*-------------*/
  mumps->id.job = JOB_SOLVE;
  PetscMUMPS_c(&mumps->id);
  if (mumps->id.INFOG(1) < 0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error reported by MUMPS in solve phase: INFOG(1)=%d\n",mumps->id.INFOG(1));

  if (mumps->size > 1) { /* convert mumps distributed solution to petsc mpi x */
    if (mumps->scat_sol && mumps->ICNTL9_pre != mumps->id.ICNTL(9)) {
      /* when id.ICNTL(9) changes, the contents of lsol_loc may change (not its size, lsol_loc), recreates scat_sol */
      ierr = VecScatterDestroy(&mumps->scat_sol);CHKERRQ(ierr);
    }
    if (!mumps->scat_sol) { /* create scatter scat_sol */
      ierr = ISCreateStride(PETSC_COMM_SELF,mumps->id.lsol_loc,0,1,&is_iden);CHKERRQ(ierr); /* from */
      for (i=0; i<mumps->id.lsol_loc; i++) {
        mumps->id.isol_loc[i] -= 1; /* change Fortran style to C style */
      }
      ierr = ISCreateGeneral(PETSC_COMM_SELF,mumps->id.lsol_loc,mumps->id.isol_loc,PETSC_COPY_VALUES,&is_petsc);CHKERRQ(ierr);  /* to */
      ierr = VecScatterCreate(mumps->x_seq,is_iden,x,is_petsc,&mumps->scat_sol);CHKERRQ(ierr);
      ierr = ISDestroy(&is_iden);CHKERRQ(ierr);
      ierr = ISDestroy(&is_petsc);CHKERRQ(ierr);

      mumps->ICNTL9_pre = mumps->id.ICNTL(9); /* save current value of id.ICNTL(9) */
    }

    ierr = VecScatterBegin(mumps->scat_sol,mumps->x_seq,x,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(mumps->scat_sol,mumps->x_seq,x,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSolveTranspose_MUMPS"
PetscErrorCode MatSolveTranspose_MUMPS(Mat A,Vec b,Vec x)
{
  Mat_MUMPS      *mumps=(Mat_MUMPS*)A->spptr;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  mumps->id.ICNTL(9) = 0;

  ierr = MatSolve_MUMPS(A,b,x);CHKERRQ(ierr);

  mumps->id.ICNTL(9) = 1;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMatSolve_MUMPS"
PetscErrorCode MatMatSolve_MUMPS(Mat A,Mat B,Mat X)
{
  PetscErrorCode ierr;
  PetscBool      flg;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompareAny((PetscObject)B,&flg,MATSEQDENSE,MATMPIDENSE,NULL);CHKERRQ(ierr);
  if (!flg) SETERRQ(PetscObjectComm((PetscObject)A),PETSC_ERR_ARG_WRONG,"Matrix B must be MATDENSE matrix");
  ierr = PetscObjectTypeCompareAny((PetscObject)X,&flg,MATSEQDENSE,MATMPIDENSE,NULL);CHKERRQ(ierr);
  if (!flg) SETERRQ(PetscObjectComm((PetscObject)A),PETSC_ERR_ARG_WRONG,"Matrix X must be MATDENSE matrix");
  SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"MatMatSolve_MUMPS() is not implemented yet");
  PetscFunctionReturn(0);
}

#if !defined(PETSC_USE_COMPLEX)
/*
  input:
   F:        numeric factor
  output:
   nneg:     total number of negative pivots
   nzero:    0
   npos:     (global dimension of F) - nneg
*/

#undef __FUNCT__
#define __FUNCT__ "MatGetInertia_SBAIJMUMPS"
PetscErrorCode MatGetInertia_SBAIJMUMPS(Mat F,int *nneg,int *nzero,int *npos)
{
  Mat_MUMPS      *mumps =(Mat_MUMPS*)F->spptr;
  PetscErrorCode ierr;
  PetscMPIInt    size;

  PetscFunctionBegin;
  ierr = MPI_Comm_size(PetscObjectComm((PetscObject)F),&size);CHKERRQ(ierr);
  /* MUMPS 4.3.1 calls ScaLAPACK when ICNTL(13)=0 (default), which does not offer the possibility to compute the inertia of a dense matrix. Set ICNTL(13)=1 to skip ScaLAPACK */
  if (size > 1 && mumps->id.ICNTL(13) != 1) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"ICNTL(13)=%d. -mat_mumps_icntl_13 must be set as 1 for correct global matrix inertia\n",mumps->id.INFOG(13));

  if (nneg) *nneg = mumps->id.INFOG(12); 
  if (nzero || npos) {
    if (mumps->id.ICNTL(24) != 1) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"-mat_mumps_icntl_24 must be set as 1 for null pivot row detection");
    if (nzero) *nzero = mumps->id.INFOG(28);
    if (npos) *npos   = F->rmap->N - (mumps->id.INFOG(12) + mumps->id.INFOG(28));
  }
  PetscFunctionReturn(0);
}
#endif /* !defined(PETSC_USE_COMPLEX) */

#undef __FUNCT__
#define __FUNCT__ "MatFactorNumeric_MUMPS"
PetscErrorCode MatFactorNumeric_MUMPS(Mat F,Mat A,const MatFactorInfo *info)
{
  Mat_MUMPS      *mumps =(Mat_MUMPS*)(F)->spptr;
  PetscErrorCode ierr;
  Mat            F_diag;
  PetscBool      isMPIAIJ;

  PetscFunctionBegin;
  ierr = (*mumps->ConvertToTriples)(A, 1, MAT_REUSE_MATRIX, &mumps->nz, &mumps->irn, &mumps->jcn, &mumps->val);CHKERRQ(ierr);

  /* numerical factorization phase */
  /*-------------------------------*/
  mumps->id.job = JOB_FACTNUMERIC;
  if (!mumps->id.ICNTL(18)) {
    if (!mumps->myid) {
#if defined(PETSC_USE_COMPLEX)
#if defined(PETSC_USE_REAL_SINGLE)
      mumps->id.a = (mumps_complex*)mumps->val;
#else
      mumps->id.a = (mumps_double_complex*)mumps->val;
#endif
#else
      mumps->id.a = mumps->val;
#endif
    }
  } else {
#if defined(PETSC_USE_COMPLEX)
#if defined(PETSC_USE_REAL_SINGLE)
    mumps->id.a_loc = (mumps_complex*)mumps->val;
#else
    mumps->id.a_loc = (mumps_double_complex*)mumps->val;
#endif
#else
    mumps->id.a_loc = mumps->val;
#endif
  }
  PetscMUMPS_c(&mumps->id);
  if (mumps->id.INFOG(1) < 0) {
    if (mumps->id.INFO(1) == -13) {
      if (mumps->id.INFO(2) < 0) {
        SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error reported by MUMPS in numerical factorization phase: Cannot allocate required memory %d megabytes\n",-mumps->id.INFO(2));
      } else {
        SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error reported by MUMPS in numerical factorization phase: Cannot allocate required memory %d bytes\n",mumps->id.INFO(2));
      }
    } else SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error reported by MUMPS in numerical factorization phase: INFO(1)=%d, INFO(2)=%d\n",mumps->id.INFO(1),mumps->id.INFO(2));
  }
  if (!mumps->myid && mumps->id.ICNTL(16) > 0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"  mumps->id.ICNTL(16):=%d\n",mumps->id.INFOG(16));
  
  (F)->assembled      = PETSC_TRUE;
  mumps->matstruc     = SAME_NONZERO_PATTERN;
  mumps->CleanUpMUMPS = PETSC_TRUE;

  if (mumps->size > 1) {
    PetscInt    lsol_loc;
    PetscScalar *sol_loc;

    ierr = PetscObjectTypeCompare((PetscObject)A,MATMPIAIJ,&isMPIAIJ);CHKERRQ(ierr);
    if (isMPIAIJ) F_diag = ((Mat_MPIAIJ*)(F)->data)->A;
    else F_diag = ((Mat_MPISBAIJ*)(F)->data)->A;
    F_diag->assembled = PETSC_TRUE;

    /* distributed solution; Create x_seq=sol_loc for repeated use */
    if (mumps->x_seq) {
      ierr = VecScatterDestroy(&mumps->scat_sol);CHKERRQ(ierr);
      ierr = PetscFree2(mumps->id.sol_loc,mumps->id.isol_loc);CHKERRQ(ierr);
      ierr = VecDestroy(&mumps->x_seq);CHKERRQ(ierr);
    }
    lsol_loc = mumps->id.INFO(23); /* length of sol_loc */
    ierr = PetscMalloc2(lsol_loc,&sol_loc,lsol_loc,&mumps->id.isol_loc);CHKERRQ(ierr);
    mumps->id.lsol_loc = lsol_loc;
#if defined(PETSC_USE_COMPLEX)
#if defined(PETSC_USE_REAL_SINGLE)
    mumps->id.sol_loc = (mumps_complex*)sol_loc;
#else
    mumps->id.sol_loc = (mumps_double_complex*)sol_loc;
#endif
#else
    mumps->id.sol_loc = sol_loc;
#endif
    ierr = VecCreateSeqWithArray(PETSC_COMM_SELF,1,lsol_loc,sol_loc,&mumps->x_seq);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/* Sets MUMPS options from the options database */
#undef __FUNCT__
#define __FUNCT__ "PetscSetMUMPSFromOptions"
PetscErrorCode PetscSetMUMPSFromOptions(Mat F, Mat A)
{
  Mat_MUMPS      *mumps = (Mat_MUMPS*)F->spptr;
  PetscErrorCode ierr;
  PetscInt       icntl;
  PetscBool      flg;

  PetscFunctionBegin;
  ierr = PetscOptionsBegin(PetscObjectComm((PetscObject)A),((PetscObject)A)->prefix,"MUMPS Options","Mat");CHKERRQ(ierr);
  ierr = PetscOptionsInt("-mat_mumps_icntl_1","ICNTL(1): output stream for error messages","None",mumps->id.ICNTL(1),&icntl,&flg);CHKERRQ(ierr);
  if (flg) mumps->id.ICNTL(1) = icntl;
  ierr = PetscOptionsInt("-mat_mumps_icntl_2","ICNTL(2): output stream for diagnostic printing, statistics, and warning","None",mumps->id.ICNTL(2),&icntl,&flg);CHKERRQ(ierr);
  if (flg) mumps->id.ICNTL(2) = icntl;
  ierr = PetscOptionsInt("-mat_mumps_icntl_3","ICNTL(3): output stream for global information, collected on the host","None",mumps->id.ICNTL(3),&icntl,&flg);CHKERRQ(ierr);
  if (flg) mumps->id.ICNTL(3) = icntl;

  ierr = PetscOptionsInt("-mat_mumps_icntl_4","ICNTL(4): level of printing (0 to 4)","None",mumps->id.ICNTL(4),&icntl,&flg);CHKERRQ(ierr);
  if (flg) mumps->id.ICNTL(4) = icntl;
  if (mumps->id.ICNTL(4) || PetscLogPrintInfo) mumps->id.ICNTL(3) = 6; /* resume MUMPS default id.ICNTL(3) = 6 */

  ierr = PetscOptionsInt("-mat_mumps_icntl_6","ICNTL(6): permuting and/or scaling the matrix (0 to 7)","None",mumps->id.ICNTL(6),&icntl,&flg);CHKERRQ(ierr);
  if (flg) mumps->id.ICNTL(6) = icntl;

  ierr = PetscOptionsInt("-mat_mumps_icntl_7","ICNTL(7): matrix ordering (0 to 7). 3=Scotch, 4=PORD, 5=Metis","None",mumps->id.ICNTL(7),&icntl,&flg);CHKERRQ(ierr);
  if (flg) {
    if (icntl== 1 && mumps->size > 1) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"pivot order be set by the user in PERM_IN -- not supported by the PETSc/MUMPS interface\n");
    else mumps->id.ICNTL(7) = icntl;
  }

  ierr = PetscOptionsInt("-mat_mumps_icntl_8","ICNTL(8): scaling strategy (-2 to 8 or 77)","None",mumps->id.ICNTL(8),&mumps->id.ICNTL(8),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-mat_mumps_icntl_10","ICNTL(10): max num of refinements","None",mumps->id.ICNTL(10),&mumps->id.ICNTL(10),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-mat_mumps_icntl_11","ICNTL(11): statistics related to the linear system solved (via -ksp_view)","None",mumps->id.ICNTL(11),&mumps->id.ICNTL(11),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-mat_mumps_icntl_12","ICNTL(12): efficiency control: defines the ordering strategy with scaling constraints (0 to 3)","None",mumps->id.ICNTL(12),&mumps->id.ICNTL(12),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-mat_mumps_icntl_13","ICNTL(13): efficiency control: with or without ScaLAPACK","None",mumps->id.ICNTL(13),&mumps->id.ICNTL(13),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-mat_mumps_icntl_14","ICNTL(14): percentage of estimated workspace increase","None",mumps->id.ICNTL(14),&mumps->id.ICNTL(14),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-mat_mumps_icntl_19","ICNTL(19): Schur complement","None",mumps->id.ICNTL(19),&mumps->id.ICNTL(19),NULL);CHKERRQ(ierr);

  ierr = PetscOptionsInt("-mat_mumps_icntl_22","ICNTL(22): in-core/out-of-core facility (0 or 1)","None",mumps->id.ICNTL(22),&mumps->id.ICNTL(22),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-mat_mumps_icntl_23","ICNTL(23): max size of the working memory (MB) that can allocate per processor","None",mumps->id.ICNTL(23),&mumps->id.ICNTL(23),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-mat_mumps_icntl_24","ICNTL(24): detection of null pivot rows (0 or 1)","None",mumps->id.ICNTL(24),&mumps->id.ICNTL(24),NULL);CHKERRQ(ierr);
  if (mumps->id.ICNTL(24)) {
    mumps->id.ICNTL(13) = 1; /* turn-off ScaLAPACK to help with the correct detection of null pivots */
  }

  ierr = PetscOptionsInt("-mat_mumps_icntl_25","ICNTL(25): computation of a null space basis","None",mumps->id.ICNTL(25),&mumps->id.ICNTL(25),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-mat_mumps_icntl_26","ICNTL(26): Schur options for right-hand side or solution vector","None",mumps->id.ICNTL(26),&mumps->id.ICNTL(26),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-mat_mumps_icntl_27","ICNTL(27): experimental parameter","None",mumps->id.ICNTL(27),&mumps->id.ICNTL(27),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-mat_mumps_icntl_28","ICNTL(28): use 1 for sequential analysis and ictnl(7) ordering, or 2 for parallel analysis and ictnl(29) ordering","None",mumps->id.ICNTL(28),&mumps->id.ICNTL(28),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-mat_mumps_icntl_29","ICNTL(29): parallel ordering 1 = ptscotch 2 = parmetis","None",mumps->id.ICNTL(29),&mumps->id.ICNTL(29),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-mat_mumps_icntl_30","ICNTL(30): compute user-specified set of entries in inv(A)","None",mumps->id.ICNTL(30),&mumps->id.ICNTL(30),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-mat_mumps_icntl_31","ICNTL(31): factors can be discarded in the solve phase","None",mumps->id.ICNTL(31),&mumps->id.ICNTL(31),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-mat_mumps_icntl_33","ICNTL(33): compute determinant","None",mumps->id.ICNTL(33),&mumps->id.ICNTL(33),NULL);CHKERRQ(ierr);

  ierr = PetscOptionsReal("-mat_mumps_cntl_1","CNTL(1): relative pivoting threshold","None",mumps->id.CNTL(1),&mumps->id.CNTL(1),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-mat_mumps_cntl_2","CNTL(2): stopping criterion of refinement","None",mumps->id.CNTL(2),&mumps->id.CNTL(2),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-mat_mumps_cntl_3","CNTL(3): absolute pivoting threshold","None",mumps->id.CNTL(3),&mumps->id.CNTL(3),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-mat_mumps_cntl_4","CNTL(4): value for static pivoting","None",mumps->id.CNTL(4),&mumps->id.CNTL(4),NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-mat_mumps_cntl_5","CNTL(5): fixation for null pivots","None",mumps->id.CNTL(5),&mumps->id.CNTL(5),NULL);CHKERRQ(ierr);

  ierr = PetscOptionsString("-mat_mumps_ooc_tmpdir", "out of core directory", "None", mumps->id.ooc_tmpdir, mumps->id.ooc_tmpdir, 256, NULL);
  PetscOptionsEnd();
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscInitializeMUMPS"
PetscErrorCode PetscInitializeMUMPS(Mat A,Mat_MUMPS *mumps)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)A), &mumps->myid);
  ierr = MPI_Comm_size(PetscObjectComm((PetscObject)A),&mumps->size);CHKERRQ(ierr);
  ierr = MPI_Comm_dup(PetscObjectComm((PetscObject)A),&(mumps->comm_mumps));CHKERRQ(ierr);

  mumps->id.comm_fortran = MPI_Comm_c2f(mumps->comm_mumps);

  mumps->id.job = JOB_INIT;
  mumps->id.par = 1;  /* host participates factorizaton and solve */
  mumps->id.sym = mumps->sym;
  PetscMUMPS_c(&mumps->id);

  mumps->CleanUpMUMPS = PETSC_FALSE;
  mumps->scat_rhs     = NULL;
  mumps->scat_sol     = NULL;

  /* set PETSc-MUMPS default options - override MUMPS default */
  mumps->id.ICNTL(3) = 0;
  mumps->id.ICNTL(4) = 0;
  if (mumps->size == 1) {
    mumps->id.ICNTL(18) = 0;   /* centralized assembled matrix input */
  } else {
    mumps->id.ICNTL(18) = 3;   /* distributed assembled matrix input */
    mumps->id.ICNTL(21) = 1;   /* distributed solution */
  }
  PetscFunctionReturn(0);
}

/* Note Petsc r(=c) permutation is used when mumps->id.ICNTL(7)==1 with centralized assembled matrix input; otherwise r and c are ignored */
#undef __FUNCT__
#define __FUNCT__ "MatLUFactorSymbolic_AIJMUMPS"
PetscErrorCode MatLUFactorSymbolic_AIJMUMPS(Mat F,Mat A,IS r,IS c,const MatFactorInfo *info)
{
  Mat_MUMPS      *mumps = (Mat_MUMPS*)F->spptr;
  PetscErrorCode ierr;
  Vec            b;
  IS             is_iden;
  const PetscInt M = A->rmap->N;

  PetscFunctionBegin;
  mumps->matstruc = DIFFERENT_NONZERO_PATTERN;

  /* Set MUMPS options from the options database */
  ierr = PetscSetMUMPSFromOptions(F,A);CHKERRQ(ierr);

  ierr = (*mumps->ConvertToTriples)(A, 1, MAT_INITIAL_MATRIX, &mumps->nz, &mumps->irn, &mumps->jcn, &mumps->val);CHKERRQ(ierr);

  /* analysis phase */
  /*----------------*/
  mumps->id.job = JOB_FACTSYMBOLIC;
  mumps->id.n   = M;
  switch (mumps->id.ICNTL(18)) {
  case 0:  /* centralized assembled matrix input */
    if (!mumps->myid) {
      mumps->id.nz =mumps->nz; mumps->id.irn=mumps->irn; mumps->id.jcn=mumps->jcn;
      if (mumps->id.ICNTL(6)>1) {
#if defined(PETSC_USE_COMPLEX)
#if defined(PETSC_USE_REAL_SINGLE)
        mumps->id.a = (mumps_complex*)mumps->val;
#else
        mumps->id.a = (mumps_double_complex*)mumps->val;
#endif
#else
        mumps->id.a = mumps->val;
#endif
      }
      if (mumps->id.ICNTL(7) == 1) { /* use user-provide matrix ordering - assuming r = c ordering */
        /*
        PetscBool      flag;
        ierr = ISEqual(r,c,&flag);CHKERRQ(ierr);
        if (!flag) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_USER,"row_perm != col_perm");
        ierr = ISView(r,PETSC_VIEWER_STDOUT_SELF);
         */
        if (!mumps->myid) {
          const PetscInt *idx;
          PetscInt       i,*perm_in;

          ierr = PetscMalloc1(M,&perm_in);CHKERRQ(ierr);
          ierr = ISGetIndices(r,&idx);CHKERRQ(ierr);

          mumps->id.perm_in = perm_in;
          for (i=0; i<M; i++) perm_in[i] = idx[i]+1; /* perm_in[]: start from 1, not 0! */
          ierr = ISRestoreIndices(r,&idx);CHKERRQ(ierr);
        }
      }
    }
    break;
  case 3:  /* distributed assembled matrix input (size>1) */
    mumps->id.nz_loc = mumps->nz;
    mumps->id.irn_loc=mumps->irn; mumps->id.jcn_loc=mumps->jcn;
    if (mumps->id.ICNTL(6)>1) {
#if defined(PETSC_USE_COMPLEX)
#if defined(PETSC_USE_REAL_SINGLE)
      mumps->id.a_loc = (mumps_complex*)mumps->val;
#else
      mumps->id.a_loc = (mumps_double_complex*)mumps->val;
#endif
#else
      mumps->id.a_loc = mumps->val;
#endif
    }
    /* MUMPS only supports centralized rhs. Create scatter scat_rhs for repeated use in MatSolve() */
    if (!mumps->myid) {
      ierr = VecCreateSeq(PETSC_COMM_SELF,A->cmap->N,&mumps->b_seq);CHKERRQ(ierr);
      ierr = ISCreateStride(PETSC_COMM_SELF,A->cmap->N,0,1,&is_iden);CHKERRQ(ierr);
    } else {
      ierr = VecCreateSeq(PETSC_COMM_SELF,0,&mumps->b_seq);CHKERRQ(ierr);
      ierr = ISCreateStride(PETSC_COMM_SELF,0,0,1,&is_iden);CHKERRQ(ierr);
    }
    ierr = MatGetVecs(A,NULL,&b);CHKERRQ(ierr);
    ierr = VecScatterCreate(b,is_iden,mumps->b_seq,is_iden,&mumps->scat_rhs);CHKERRQ(ierr);
    ierr = ISDestroy(&is_iden);CHKERRQ(ierr);
    ierr = VecDestroy(&b);CHKERRQ(ierr);
    break;
  }
  PetscMUMPS_c(&mumps->id);
  if (mumps->id.INFOG(1) < 0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error reported by MUMPS in analysis phase: INFOG(1)=%d\n",mumps->id.INFOG(1));

  F->ops->lufactornumeric = MatFactorNumeric_MUMPS;
  F->ops->solve           = MatSolve_MUMPS;
  F->ops->solvetranspose  = MatSolveTranspose_MUMPS;
  F->ops->matsolve        = 0;  /* use MatMatSolve_Basic() until mumps supports distributed rhs */
  PetscFunctionReturn(0);
}

/* Note the Petsc r and c permutations are ignored */
#undef __FUNCT__
#define __FUNCT__ "MatLUFactorSymbolic_BAIJMUMPS"
PetscErrorCode MatLUFactorSymbolic_BAIJMUMPS(Mat F,Mat A,IS r,IS c,const MatFactorInfo *info)
{
  Mat_MUMPS      *mumps = (Mat_MUMPS*)F->spptr;
  PetscErrorCode ierr;
  Vec            b;
  IS             is_iden;
  const PetscInt M = A->rmap->N;

  PetscFunctionBegin;
  mumps->matstruc = DIFFERENT_NONZERO_PATTERN;

  /* Set MUMPS options from the options database */
  ierr = PetscSetMUMPSFromOptions(F,A);CHKERRQ(ierr);

  ierr = (*mumps->ConvertToTriples)(A, 1, MAT_INITIAL_MATRIX, &mumps->nz, &mumps->irn, &mumps->jcn, &mumps->val);CHKERRQ(ierr);

  /* analysis phase */
  /*----------------*/
  mumps->id.job = JOB_FACTSYMBOLIC;
  mumps->id.n   = M;
  switch (mumps->id.ICNTL(18)) {
  case 0:  /* centralized assembled matrix input */
    if (!mumps->myid) {
      mumps->id.nz =mumps->nz; mumps->id.irn=mumps->irn; mumps->id.jcn=mumps->jcn;
      if (mumps->id.ICNTL(6)>1) {
#if defined(PETSC_USE_COMPLEX)
#if defined(PETSC_USE_REAL_SINGLE)
        mumps->id.a = (mumps_complex*)mumps->val;
#else
        mumps->id.a = (mumps_double_complex*)mumps->val;
#endif
#else
        mumps->id.a = mumps->val;
#endif
      }
    }
    break;
  case 3:  /* distributed assembled matrix input (size>1) */
    mumps->id.nz_loc = mumps->nz;
    mumps->id.irn_loc=mumps->irn; mumps->id.jcn_loc=mumps->jcn;
    if (mumps->id.ICNTL(6)>1) {
#if defined(PETSC_USE_COMPLEX)
#if defined(PETSC_USE_REAL_SINGLE)
      mumps->id.a_loc = (mumps_complex*)mumps->val;
#else
      mumps->id.a_loc = (mumps_double_complex*)mumps->val;
#endif
#else
      mumps->id.a_loc = mumps->val;
#endif
    }
    /* MUMPS only supports centralized rhs. Create scatter scat_rhs for repeated use in MatSolve() */
    if (!mumps->myid) {
      ierr = VecCreateSeq(PETSC_COMM_SELF,A->cmap->N,&mumps->b_seq);CHKERRQ(ierr);
      ierr = ISCreateStride(PETSC_COMM_SELF,A->cmap->N,0,1,&is_iden);CHKERRQ(ierr);
    } else {
      ierr = VecCreateSeq(PETSC_COMM_SELF,0,&mumps->b_seq);CHKERRQ(ierr);
      ierr = ISCreateStride(PETSC_COMM_SELF,0,0,1,&is_iden);CHKERRQ(ierr);
    }
    ierr = MatGetVecs(A,NULL,&b);CHKERRQ(ierr);
    ierr = VecScatterCreate(b,is_iden,mumps->b_seq,is_iden,&mumps->scat_rhs);CHKERRQ(ierr);
    ierr = ISDestroy(&is_iden);CHKERRQ(ierr);
    ierr = VecDestroy(&b);CHKERRQ(ierr);
    break;
  }
  PetscMUMPS_c(&mumps->id);
  if (mumps->id.INFOG(1) < 0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error reported by MUMPS in analysis phase: INFOG(1)=%d\n",mumps->id.INFOG(1));

  F->ops->lufactornumeric = MatFactorNumeric_MUMPS;
  F->ops->solve           = MatSolve_MUMPS;
  F->ops->solvetranspose  = MatSolveTranspose_MUMPS;
  PetscFunctionReturn(0);
}

/* Note the Petsc r permutation and factor info are ignored */
#undef __FUNCT__
#define __FUNCT__ "MatCholeskyFactorSymbolic_MUMPS"
PetscErrorCode MatCholeskyFactorSymbolic_MUMPS(Mat F,Mat A,IS r,const MatFactorInfo *info)
{
  Mat_MUMPS      *mumps = (Mat_MUMPS*)F->spptr;
  PetscErrorCode ierr;
  Vec            b;
  IS             is_iden;
  const PetscInt M = A->rmap->N;

  PetscFunctionBegin;
  mumps->matstruc = DIFFERENT_NONZERO_PATTERN;

  /* Set MUMPS options from the options database */
  ierr = PetscSetMUMPSFromOptions(F,A);CHKERRQ(ierr);

  ierr = (*mumps->ConvertToTriples)(A, 1, MAT_INITIAL_MATRIX, &mumps->nz, &mumps->irn, &mumps->jcn, &mumps->val);CHKERRQ(ierr);

  /* analysis phase */
  /*----------------*/
  mumps->id.job = JOB_FACTSYMBOLIC;
  mumps->id.n   = M;
  switch (mumps->id.ICNTL(18)) {
  case 0:  /* centralized assembled matrix input */
    if (!mumps->myid) {
      mumps->id.nz =mumps->nz; mumps->id.irn=mumps->irn; mumps->id.jcn=mumps->jcn;
      if (mumps->id.ICNTL(6)>1) {
#if defined(PETSC_USE_COMPLEX)
#if defined(PETSC_USE_REAL_SINGLE)
        mumps->id.a = (mumps_complex*)mumps->val;
#else
        mumps->id.a = (mumps_double_complex*)mumps->val;
#endif
#else
        mumps->id.a = mumps->val;
#endif
      }
    }
    break;
  case 3:  /* distributed assembled matrix input (size>1) */
    mumps->id.nz_loc = mumps->nz;
    mumps->id.irn_loc=mumps->irn; mumps->id.jcn_loc=mumps->jcn;
    if (mumps->id.ICNTL(6)>1) {
#if defined(PETSC_USE_COMPLEX)
#if defined(PETSC_USE_REAL_SINGLE)
      mumps->id.a_loc = (mumps_complex*)mumps->val;
#else
      mumps->id.a_loc = (mumps_double_complex*)mumps->val;
#endif
#else
      mumps->id.a_loc = mumps->val;
#endif
    }
    /* MUMPS only supports centralized rhs. Create scatter scat_rhs for repeated use in MatSolve() */
    if (!mumps->myid) {
      ierr = VecCreateSeq(PETSC_COMM_SELF,A->cmap->N,&mumps->b_seq);CHKERRQ(ierr);
      ierr = ISCreateStride(PETSC_COMM_SELF,A->cmap->N,0,1,&is_iden);CHKERRQ(ierr);
    } else {
      ierr = VecCreateSeq(PETSC_COMM_SELF,0,&mumps->b_seq);CHKERRQ(ierr);
      ierr = ISCreateStride(PETSC_COMM_SELF,0,0,1,&is_iden);CHKERRQ(ierr);
    }
    ierr = MatGetVecs(A,NULL,&b);CHKERRQ(ierr);
    ierr = VecScatterCreate(b,is_iden,mumps->b_seq,is_iden,&mumps->scat_rhs);CHKERRQ(ierr);
    ierr = ISDestroy(&is_iden);CHKERRQ(ierr);
    ierr = VecDestroy(&b);CHKERRQ(ierr);
    break;
  }
  PetscMUMPS_c(&mumps->id);
  if (mumps->id.INFOG(1) < 0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error reported by MUMPS in analysis phase: INFOG(1)=%d\n",mumps->id.INFOG(1));

  F->ops->choleskyfactornumeric = MatFactorNumeric_MUMPS;
  F->ops->solve                 = MatSolve_MUMPS;
  F->ops->solvetranspose        = MatSolve_MUMPS;
  F->ops->matsolve              = 0; /* use MatMatSolve_Basic() until mumps supports distributed rhs */
#if !defined(PETSC_USE_COMPLEX)
  F->ops->getinertia = MatGetInertia_SBAIJMUMPS;
#else
  F->ops->getinertia = NULL;
#endif
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatView_MUMPS"
PetscErrorCode MatView_MUMPS(Mat A,PetscViewer viewer)
{
  PetscErrorCode    ierr;
  PetscBool         iascii;
  PetscViewerFormat format;
  Mat_MUMPS         *mumps=(Mat_MUMPS*)A->spptr;

  PetscFunctionBegin;
  /* check if matrix is mumps type */
  if (A->ops->solve != MatSolve_MUMPS) PetscFunctionReturn(0);

  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  if (iascii) {
    ierr = PetscViewerGetFormat(viewer,&format);CHKERRQ(ierr);
    if (format == PETSC_VIEWER_ASCII_INFO) {
      ierr = PetscViewerASCIIPrintf(viewer,"MUMPS run parameters:\n");CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  SYM (matrix type):                   %d \n",mumps->id.sym);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  PAR (host participation):            %d \n",mumps->id.par);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(1) (output for error):         %d \n",mumps->id.ICNTL(1));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(2) (output of diagnostic msg): %d \n",mumps->id.ICNTL(2));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(3) (output for global info):   %d \n",mumps->id.ICNTL(3));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(4) (level of printing):        %d \n",mumps->id.ICNTL(4));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(5) (input mat struct):         %d \n",mumps->id.ICNTL(5));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(6) (matrix prescaling):        %d \n",mumps->id.ICNTL(6));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(7) (sequentia matrix ordering):%d \n",mumps->id.ICNTL(7));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(8) (scalling strategy):        %d \n",mumps->id.ICNTL(8));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(10) (max num of refinements):  %d \n",mumps->id.ICNTL(10));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(11) (error analysis):          %d \n",mumps->id.ICNTL(11));CHKERRQ(ierr);
      if (mumps->id.ICNTL(11)>0) {
        ierr = PetscViewerASCIIPrintf(viewer,"    RINFOG(4) (inf norm of input mat):        %g\n",mumps->id.RINFOG(4));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"    RINFOG(5) (inf norm of solution):         %g\n",mumps->id.RINFOG(5));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"    RINFOG(6) (inf norm of residual):         %g\n",mumps->id.RINFOG(6));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"    RINFOG(7),RINFOG(8) (backward error est): %g, %g\n",mumps->id.RINFOG(7),mumps->id.RINFOG(8));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"    RINFOG(9) (error estimate):               %g \n",mumps->id.RINFOG(9));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"    RINFOG(10),RINFOG(11)(condition numbers): %g, %g\n",mumps->id.RINFOG(10),mumps->id.RINFOG(11));CHKERRQ(ierr);
      }
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(12) (efficiency control):                         %d \n",mumps->id.ICNTL(12));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(13) (efficiency control):                         %d \n",mumps->id.ICNTL(13));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(14) (percentage of estimated workspace increase): %d \n",mumps->id.ICNTL(14));CHKERRQ(ierr);
      /* ICNTL(15-17) not used */
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(18) (input mat struct):                           %d \n",mumps->id.ICNTL(18));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(19) (Shur complement info):                       %d \n",mumps->id.ICNTL(19));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(20) (rhs sparse pattern):                         %d \n",mumps->id.ICNTL(20));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(21) (somumpstion struct):                            %d \n",mumps->id.ICNTL(21));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(22) (in-core/out-of-core facility):               %d \n",mumps->id.ICNTL(22));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(23) (max size of memory can be allocated locally):%d \n",mumps->id.ICNTL(23));CHKERRQ(ierr);

      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(24) (detection of null pivot rows):               %d \n",mumps->id.ICNTL(24));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(25) (computation of a null space basis):          %d \n",mumps->id.ICNTL(25));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(26) (Schur options for rhs or solution):          %d \n",mumps->id.ICNTL(26));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(27) (experimental parameter):                     %d \n",mumps->id.ICNTL(27));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(28) (use parallel or sequential ordering):        %d \n",mumps->id.ICNTL(28));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(29) (parallel ordering):                          %d \n",mumps->id.ICNTL(29));CHKERRQ(ierr);

      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(30) (user-specified set of entries in inv(A)):    %d \n",mumps->id.ICNTL(30));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(31) (factors is discarded in the solve phase):    %d \n",mumps->id.ICNTL(31));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  ICNTL(33) (compute determinant):                        %d \n",mumps->id.ICNTL(33));CHKERRQ(ierr);

      ierr = PetscViewerASCIIPrintf(viewer,"  CNTL(1) (relative pivoting threshold):      %g \n",mumps->id.CNTL(1));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  CNTL(2) (stopping criterion of refinement): %g \n",mumps->id.CNTL(2));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  CNTL(3) (absomumpste pivoting threshold):      %g \n",mumps->id.CNTL(3));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  CNTL(4) (vamumpse of static pivoting):         %g \n",mumps->id.CNTL(4));CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,"  CNTL(5) (fixation for null pivots):         %g \n",mumps->id.CNTL(5));CHKERRQ(ierr);

      /* infomation local to each processor */
      ierr = PetscViewerASCIIPrintf(viewer, "  RINFO(1) (local estimated flops for the elimination after analysis): \n");CHKERRQ(ierr);
      ierr = PetscViewerASCIISynchronizedAllow(viewer,PETSC_TRUE);CHKERRQ(ierr);
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"    [%d] %g \n",mumps->myid,mumps->id.RINFO(1));CHKERRQ(ierr);
      ierr = PetscViewerFlush(viewer);
      ierr = PetscViewerASCIIPrintf(viewer, "  RINFO(2) (local estimated flops for the assembly after factorization): \n");CHKERRQ(ierr);
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"    [%d]  %g \n",mumps->myid,mumps->id.RINFO(2));CHKERRQ(ierr);
      ierr = PetscViewerFlush(viewer);
      ierr = PetscViewerASCIIPrintf(viewer, "  RINFO(3) (local estimated flops for the elimination after factorization): \n");CHKERRQ(ierr);
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"    [%d]  %g \n",mumps->myid,mumps->id.RINFO(3));CHKERRQ(ierr);
      ierr = PetscViewerFlush(viewer);

      ierr = PetscViewerASCIIPrintf(viewer, "  INFO(15) (estimated size of (in MB) MUMPS internal data for running numerical factorization): \n");CHKERRQ(ierr);
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"  [%d] %d \n",mumps->myid,mumps->id.INFO(15));CHKERRQ(ierr);
      ierr = PetscViewerFlush(viewer);

      ierr = PetscViewerASCIIPrintf(viewer, "  INFO(16) (size of (in MB) MUMPS internal data used during numerical factorization): \n");CHKERRQ(ierr);
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"    [%d] %d \n",mumps->myid,mumps->id.INFO(16));CHKERRQ(ierr);
      ierr = PetscViewerFlush(viewer);

      ierr = PetscViewerASCIIPrintf(viewer, "  INFO(23) (num of pivots eliminated on this processor after factorization): \n");CHKERRQ(ierr);
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"    [%d] %d \n",mumps->myid,mumps->id.INFO(23));CHKERRQ(ierr);
      ierr = PetscViewerFlush(viewer);
      ierr = PetscViewerASCIISynchronizedAllow(viewer,PETSC_FALSE);CHKERRQ(ierr);

      if (!mumps->myid) { /* information from the host */
        ierr = PetscViewerASCIIPrintf(viewer,"  RINFOG(1) (global estimated flops for the elimination after analysis): %g \n",mumps->id.RINFOG(1));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  RINFOG(2) (global estimated flops for the assembly after factorization): %g \n",mumps->id.RINFOG(2));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  RINFOG(3) (global estimated flops for the elimination after factorization): %g \n",mumps->id.RINFOG(3));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  (RINFOG(12) RINFOG(13))*2^INFOG(34) (determinant): (%g,%g)*(2^%d)\n",mumps->id.RINFOG(12),mumps->id.RINFOG(13),mumps->id.INFOG(34));CHKERRQ(ierr);

        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(3) (estimated real workspace for factors on all processors after analysis): %d \n",mumps->id.INFOG(3));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(4) (estimated integer workspace for factors on all processors after analysis): %d \n",mumps->id.INFOG(4));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(5) (estimated maximum front size in the complete tree): %d \n",mumps->id.INFOG(5));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(6) (number of nodes in the complete tree): %d \n",mumps->id.INFOG(6));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(7) (ordering option effectively use after analysis): %d \n",mumps->id.INFOG(7));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(8) (structural symmetry in percent of the permuted matrix after analysis): %d \n",mumps->id.INFOG(8));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(9) (total real/complex workspace to store the matrix factors after factorization): %d \n",mumps->id.INFOG(9));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(10) (total integer space store the matrix factors after factorization): %d \n",mumps->id.INFOG(10));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(11) (order of largest frontal matrix after factorization): %d \n",mumps->id.INFOG(11));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(12) (number of off-diagonal pivots): %d \n",mumps->id.INFOG(12));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(13) (number of delayed pivots after factorization): %d \n",mumps->id.INFOG(13));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(14) (number of memory compress after factorization): %d \n",mumps->id.INFOG(14));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(15) (number of steps of iterative refinement after solution): %d \n",mumps->id.INFOG(15));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(16) (estimated size (in MB) of all MUMPS internal data for factorization after analysis: value on the most memory consuming processor): %d \n",mumps->id.INFOG(16));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(17) (estimated size of all MUMPS internal data for factorization after analysis: sum over all processors): %d \n",mumps->id.INFOG(17));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(18) (size of all MUMPS internal data allocated during factorization: value on the most memory consuming processor): %d \n",mumps->id.INFOG(18));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(19) (size of all MUMPS internal data allocated during factorization: sum over all processors): %d \n",mumps->id.INFOG(19));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(20) (estimated number of entries in the factors): %d \n",mumps->id.INFOG(20));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(21) (size in MB of memory effectively used during factorization - value on the most memory consuming processor): %d \n",mumps->id.INFOG(21));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(22) (size in MB of memory effectively used during factorization - sum over all processors): %d \n",mumps->id.INFOG(22));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(23) (after analysis: value of ICNTL(6) effectively used): %d \n",mumps->id.INFOG(23));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(24) (after analysis: value of ICNTL(12) effectively used): %d \n",mumps->id.INFOG(24));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(25) (after factorization: number of pivots modified by static pivoting): %d \n",mumps->id.INFOG(25));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(28) (after factorization: number of null pivots encountered): %d\n",mumps->id.INFOG(28));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(29) (after factorization: effective number of entries in the factors (sum over all processors)): %d\n",mumps->id.INFOG(29));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(30, 31) (after solution: size in Mbytes of memory used during solution phase): %d, %d\n",mumps->id.INFOG(30),mumps->id.INFOG(31));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(32) (after analysis: type of analysis done): %d\n",mumps->id.INFOG(32));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(33) (value used for ICNTL(8)): %d\n",mumps->id.INFOG(33));CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(viewer,"  INFOG(34) (exponent of the determinant if determinant is requested): %d\n",mumps->id.INFOG(34));CHKERRQ(ierr);
      }
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetInfo_MUMPS"
PetscErrorCode MatGetInfo_MUMPS(Mat A,MatInfoType flag,MatInfo *info)
{
  Mat_MUMPS *mumps =(Mat_MUMPS*)A->spptr;

  PetscFunctionBegin;
  info->block_size        = 1.0;
  info->nz_allocated      = mumps->id.INFOG(20);
  info->nz_used           = mumps->id.INFOG(20);
  info->nz_unneeded       = 0.0;
  info->assemblies        = 0.0;
  info->mallocs           = 0.0;
  info->memory            = 0.0;
  info->fill_ratio_given  = 0;
  info->fill_ratio_needed = 0;
  info->factor_mallocs    = 0;
  PetscFunctionReturn(0);
}

/* -------------------------------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "MatMumpsSetIcntl_MUMPS"
PetscErrorCode MatMumpsSetIcntl_MUMPS(Mat F,PetscInt icntl,PetscInt ival)
{
  Mat_MUMPS *mumps =(Mat_MUMPS*)F->spptr;

  PetscFunctionBegin;
  mumps->id.ICNTL(icntl) = ival;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMumpsGetIcntl_MUMPS"
PetscErrorCode MatMumpsGetIcntl_MUMPS(Mat F,PetscInt icntl,PetscInt *ival)
{
  Mat_MUMPS *mumps =(Mat_MUMPS*)F->spptr;

  PetscFunctionBegin;
  *ival = mumps->id.ICNTL(icntl);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMumpsSetIcntl"
/*@
  MatMumpsSetIcntl - Set MUMPS parameter ICNTL()

   Logically Collective on Mat

   Input Parameters:
+  F - the factored matrix obtained by calling MatGetFactor() from PETSc-MUMPS interface
.  icntl - index of MUMPS parameter array ICNTL()
-  ival - value of MUMPS ICNTL(icntl)

  Options Database:
.   -mat_mumps_icntl_<icntl> <ival>

   Level: beginner

   References: MUMPS Users' Guide

.seealso: MatGetFactor()
@*/
PetscErrorCode MatMumpsSetIcntl(Mat F,PetscInt icntl,PetscInt ival)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidLogicalCollectiveInt(F,icntl,2);
  PetscValidLogicalCollectiveInt(F,ival,3);
  ierr = PetscTryMethod(F,"MatMumpsSetIcntl_C",(Mat,PetscInt,PetscInt),(F,icntl,ival));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMumpsGetIcntl"
/*@
  MatMumpsGetIcntl - Get MUMPS parameter ICNTL()

   Logically Collective on Mat

   Input Parameters:
+  F - the factored matrix obtained by calling MatGetFactor() from PETSc-MUMPS interface
-  icntl - index of MUMPS parameter array ICNTL()

  Output Parameter:
.  ival - value of MUMPS ICNTL(icntl)

   Level: beginner

   References: MUMPS Users' Guide

.seealso: MatGetFactor()
@*/
PetscErrorCode MatMumpsGetIcntl(Mat F,PetscInt icntl,PetscInt *ival)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidLogicalCollectiveInt(F,icntl,2);
  PetscValidIntPointer(ival,3);
  ierr = PetscTryMethod(F,"MatMumpsGetIcntl_C",(Mat,PetscInt,PetscInt*),(F,icntl,ival));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/* -------------------------------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "MatMumpsSetCntl_MUMPS"
PetscErrorCode MatMumpsSetCntl_MUMPS(Mat F,PetscInt icntl,PetscReal val)
{
  Mat_MUMPS *mumps =(Mat_MUMPS*)F->spptr;

  PetscFunctionBegin;
  mumps->id.CNTL(icntl) = val;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMumpsGetCntl_MUMPS"
PetscErrorCode MatMumpsGetCntl_MUMPS(Mat F,PetscInt icntl,PetscReal *val)
{
  Mat_MUMPS *mumps =(Mat_MUMPS*)F->spptr;

  PetscFunctionBegin;
  *val = mumps->id.CNTL(icntl);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMumpsSetCntl"
/*@
  MatMumpsSetCntl - Set MUMPS parameter CNTL()

   Logically Collective on Mat

   Input Parameters:
+  F - the factored matrix obtained by calling MatGetFactor() from PETSc-MUMPS interface
.  icntl - index of MUMPS parameter array CNTL()
-  val - value of MUMPS CNTL(icntl)

  Options Database:
.   -mat_mumps_cntl_<icntl> <val>

   Level: beginner

   References: MUMPS Users' Guide

.seealso: MatGetFactor()
@*/
PetscErrorCode MatMumpsSetCntl(Mat F,PetscInt icntl,PetscReal val)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidLogicalCollectiveInt(F,icntl,2);
  PetscValidLogicalCollectiveReal(F,val,3);
  ierr = PetscTryMethod(F,"MatMumpsSetCntl_C",(Mat,PetscInt,PetscReal),(F,icntl,val));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMumpsGetCntl"
/*@
  MatMumpsGetCntl - Get MUMPS parameter CNTL()

   Logically Collective on Mat

   Input Parameters:
+  F - the factored matrix obtained by calling MatGetFactor() from PETSc-MUMPS interface
-  icntl - index of MUMPS parameter array CNTL()

  Output Parameter:
.  val - value of MUMPS CNTL(icntl)

   Level: beginner

   References: MUMPS Users' Guide

.seealso: MatGetFactor()
@*/
PetscErrorCode MatMumpsGetCntl(Mat F,PetscInt icntl,PetscReal *val)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidLogicalCollectiveInt(F,icntl,2);
  PetscValidRealPointer(val,3);
  ierr = PetscTryMethod(F,"MatMumpsGetCntl_C",(Mat,PetscInt,PetscReal*),(F,icntl,val));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMumpsGetInfo_MUMPS"
PetscErrorCode MatMumpsGetInfo_MUMPS(Mat F,PetscInt icntl,PetscInt *info)
{
  Mat_MUMPS *mumps =(Mat_MUMPS*)F->spptr;

  PetscFunctionBegin;
  *info = mumps->id.INFO(icntl);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMumpsGetInfog_MUMPS"
PetscErrorCode MatMumpsGetInfog_MUMPS(Mat F,PetscInt icntl,PetscInt *infog)
{
  Mat_MUMPS *mumps =(Mat_MUMPS*)F->spptr;

  PetscFunctionBegin;
  *infog = mumps->id.INFOG(icntl);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMumpsGetRinfo_MUMPS"
PetscErrorCode MatMumpsGetRinfo_MUMPS(Mat F,PetscInt icntl,PetscReal *rinfo)
{
  Mat_MUMPS *mumps =(Mat_MUMPS*)F->spptr;

  PetscFunctionBegin;
  *rinfo = mumps->id.RINFO(icntl);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMumpsGetRinfog_MUMPS"
PetscErrorCode MatMumpsGetRinfog_MUMPS(Mat F,PetscInt icntl,PetscReal *rinfog)
{
  Mat_MUMPS *mumps =(Mat_MUMPS*)F->spptr;

  PetscFunctionBegin;
  *rinfog = mumps->id.RINFOG(icntl); 
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMumpsGetInfo"
/*@
  MatMumpsGetInfo - Get MUMPS parameter INFO()

   Logically Collective on Mat

   Input Parameters:
+  F - the factored matrix obtained by calling MatGetFactor() from PETSc-MUMPS interface
-  icntl - index of MUMPS parameter array INFO()

  Output Parameter:
.  ival - value of MUMPS INFO(icntl)

   Level: beginner

   References: MUMPS Users' Guide

.seealso: MatGetFactor()
@*/
PetscErrorCode MatMumpsGetInfo(Mat F,PetscInt icntl,PetscInt *ival)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidIntPointer(ival,3);
  ierr = PetscTryMethod(F,"MatMumpsGetInfo_C",(Mat,PetscInt,PetscInt*),(F,icntl,ival));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMumpsGetInfog"
/*@
  MatMumpsGetInfog - Get MUMPS parameter INFOG()

   Logically Collective on Mat

   Input Parameters:
+  F - the factored matrix obtained by calling MatGetFactor() from PETSc-MUMPS interface
-  icntl - index of MUMPS parameter array INFOG()

  Output Parameter:
.  ival - value of MUMPS INFOG(icntl)

   Level: beginner

   References: MUMPS Users' Guide

.seealso: MatGetFactor()
@*/
PetscErrorCode MatMumpsGetInfog(Mat F,PetscInt icntl,PetscInt *ival)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidIntPointer(ival,3);
  ierr = PetscTryMethod(F,"MatMumpsGetInfog_C",(Mat,PetscInt,PetscInt*),(F,icntl,ival));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMumpsGetRinfo"
/*@
  MatMumpsGetRinfo - Get MUMPS parameter RINFO()

   Logically Collective on Mat

   Input Parameters:
+  F - the factored matrix obtained by calling MatGetFactor() from PETSc-MUMPS interface
-  icntl - index of MUMPS parameter array RINFO()

  Output Parameter:
.  val - value of MUMPS RINFO(icntl)

   Level: beginner

   References: MUMPS Users' Guide

.seealso: MatGetFactor()
@*/
PetscErrorCode MatMumpsGetRinfo(Mat F,PetscInt icntl,PetscReal *val)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidRealPointer(val,3);
  ierr = PetscTryMethod(F,"MatMumpsGetRinfo_C",(Mat,PetscInt,PetscReal*),(F,icntl,val));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMumpsGetRinfog"
/*@
  MatMumpsGetRinfog - Get MUMPS parameter RINFOG()

   Logically Collective on Mat

   Input Parameters:
+  F - the factored matrix obtained by calling MatGetFactor() from PETSc-MUMPS interface
-  icntl - index of MUMPS parameter array RINFOG()

  Output Parameter:
.  val - value of MUMPS RINFOG(icntl)

   Level: beginner

   References: MUMPS Users' Guide

.seealso: MatGetFactor()
@*/
PetscErrorCode MatMumpsGetRinfog(Mat F,PetscInt icntl,PetscReal *val)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidRealPointer(val,3);
  ierr = PetscTryMethod(F,"MatMumpsGetRinfog_C",(Mat,PetscInt,PetscReal*),(F,icntl,val));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*MC
  MATSOLVERMUMPS -  A matrix type providing direct solvers (LU and Cholesky) for
  distributed and sequential matrices via the external package MUMPS.

  Works with MATAIJ and MATSBAIJ matrices

  Options Database Keys:
+ -mat_mumps_icntl_4 <0,...,4> - print level
. -mat_mumps_icntl_6 <0,...,7> - matrix prescaling options (see MUMPS User's Guide)
. -mat_mumps_icntl_7 <0,...,7> - matrix orderings (see MUMPS User's Guidec)
. -mat_mumps_icntl_9 <1,2> - A or A^T x=b to be solved: 1 denotes A, 2 denotes A^T
. -mat_mumps_icntl_10 <n> - maximum number of iterative refinements
. -mat_mumps_icntl_11 <n> - error analysis, a positive value returns statistics during -ksp_view
. -mat_mumps_icntl_12 <n> - efficiency control (see MUMPS User's Guide)
. -mat_mumps_icntl_13 <n> - efficiency control (see MUMPS User's Guide)
. -mat_mumps_icntl_14 <n> - efficiency control (see MUMPS User's Guide)
. -mat_mumps_icntl_15 <n> - efficiency control (see MUMPS User's Guide)
. -mat_mumps_cntl_1 <delta> - relative pivoting threshold
. -mat_mumps_cntl_2 <tol> - stopping criterion for refinement
- -mat_mumps_cntl_3 <adelta> - absolute pivoting threshold

  Level: beginner

.seealso: PCFactorSetMatSolverPackage(), MatSolverPackage

M*/

#undef __FUNCT__
#define __FUNCT__ "MatFactorGetSolverPackage_mumps"
static PetscErrorCode MatFactorGetSolverPackage_mumps(Mat A,const MatSolverPackage *type)
{
  PetscFunctionBegin;
  *type = MATSOLVERMUMPS;
  PetscFunctionReturn(0);
}

/* MatGetFactor for Seq and MPI AIJ matrices */
#undef __FUNCT__
#define __FUNCT__ "MatGetFactor_aij_mumps"
PETSC_EXTERN PetscErrorCode MatGetFactor_aij_mumps(Mat A,MatFactorType ftype,Mat *F)
{
  Mat            B;
  PetscErrorCode ierr;
  Mat_MUMPS      *mumps;
  PetscBool      isSeqAIJ;

  PetscFunctionBegin;
  /* Create the factorization matrix */
  ierr = PetscObjectTypeCompare((PetscObject)A,MATSEQAIJ,&isSeqAIJ);CHKERRQ(ierr);
  ierr = MatCreate(PetscObjectComm((PetscObject)A),&B);CHKERRQ(ierr);
  ierr = MatSetSizes(B,A->rmap->n,A->cmap->n,A->rmap->N,A->cmap->N);CHKERRQ(ierr);
  ierr = MatSetType(B,((PetscObject)A)->type_name);CHKERRQ(ierr);
  if (isSeqAIJ) {
    ierr = MatSeqAIJSetPreallocation(B,0,NULL);CHKERRQ(ierr);
  } else {
    ierr = MatMPIAIJSetPreallocation(B,0,NULL,0,NULL);CHKERRQ(ierr);
  }

  ierr = PetscNewLog(B,&mumps);CHKERRQ(ierr);

  B->ops->view    = MatView_MUMPS;
  B->ops->getinfo = MatGetInfo_MUMPS;

  ierr = PetscObjectComposeFunction((PetscObject)B,"MatFactorGetSolverPackage_C",MatFactorGetSolverPackage_mumps);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsSetIcntl_C",MatMumpsSetIcntl_MUMPS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsGetIcntl_C",MatMumpsGetIcntl_MUMPS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsSetCntl_C",MatMumpsSetCntl_MUMPS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsGetCntl_C",MatMumpsGetCntl_MUMPS);CHKERRQ(ierr);

  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsGetInfo_C",MatMumpsGetInfo_MUMPS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsGetInfog_C",MatMumpsGetInfog_MUMPS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsGetRinfo_C",MatMumpsGetRinfo_MUMPS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsGetRinfog_C",MatMumpsGetRinfog_MUMPS);CHKERRQ(ierr);
  if (ftype == MAT_FACTOR_LU) {
    B->ops->lufactorsymbolic = MatLUFactorSymbolic_AIJMUMPS;
    B->factortype            = MAT_FACTOR_LU;
    if (isSeqAIJ) mumps->ConvertToTriples = MatConvertToTriples_seqaij_seqaij;
    else mumps->ConvertToTriples = MatConvertToTriples_mpiaij_mpiaij;
    mumps->sym = 0;
  } else {
    B->ops->choleskyfactorsymbolic = MatCholeskyFactorSymbolic_MUMPS;
    B->factortype                  = MAT_FACTOR_CHOLESKY;
    if (isSeqAIJ) mumps->ConvertToTriples = MatConvertToTriples_seqaij_seqsbaij;
    else mumps->ConvertToTriples = MatConvertToTriples_mpiaij_mpisbaij;
    if (A->spd_set && A->spd) mumps->sym = 1;
    else                      mumps->sym = 2;
  }

  mumps->isAIJ    = PETSC_TRUE;
  mumps->Destroy  = B->ops->destroy;
  B->ops->destroy = MatDestroy_MUMPS;
  B->spptr        = (void*)mumps;

  ierr = PetscInitializeMUMPS(A,mumps);CHKERRQ(ierr);

  *F = B;
  PetscFunctionReturn(0);
}

/* MatGetFactor for Seq and MPI SBAIJ matrices */
#undef __FUNCT__
#define __FUNCT__ "MatGetFactor_sbaij_mumps"
PETSC_EXTERN PetscErrorCode MatGetFactor_sbaij_mumps(Mat A,MatFactorType ftype,Mat *F)
{
  Mat            B;
  PetscErrorCode ierr;
  Mat_MUMPS      *mumps;
  PetscBool      isSeqSBAIJ;

  PetscFunctionBegin;
  if (ftype != MAT_FACTOR_CHOLESKY) SETERRQ(PetscObjectComm((PetscObject)A),PETSC_ERR_SUP,"Cannot use PETSc SBAIJ matrices with MUMPS LU, use AIJ matrix");
  if (A->rmap->bs > 1) SETERRQ(PetscObjectComm((PetscObject)A),PETSC_ERR_SUP,"Cannot use PETSc SBAIJ matrices with block size > 1 with MUMPS Cholesky, use AIJ matrix instead");
  ierr = PetscObjectTypeCompare((PetscObject)A,MATSEQSBAIJ,&isSeqSBAIJ);CHKERRQ(ierr);
  /* Create the factorization matrix */
  ierr = MatCreate(PetscObjectComm((PetscObject)A),&B);CHKERRQ(ierr);
  ierr = MatSetSizes(B,A->rmap->n,A->cmap->n,A->rmap->N,A->cmap->N);CHKERRQ(ierr);
  ierr = MatSetType(B,((PetscObject)A)->type_name);CHKERRQ(ierr);
  ierr = PetscNewLog(B,&mumps);CHKERRQ(ierr);
  if (isSeqSBAIJ) {
    ierr = MatSeqSBAIJSetPreallocation(B,1,0,NULL);CHKERRQ(ierr);

    mumps->ConvertToTriples = MatConvertToTriples_seqsbaij_seqsbaij;
  } else {
    ierr = MatMPISBAIJSetPreallocation(B,1,0,NULL,0,NULL);CHKERRQ(ierr);

    mumps->ConvertToTriples = MatConvertToTriples_mpisbaij_mpisbaij;
  }

  B->ops->choleskyfactorsymbolic = MatCholeskyFactorSymbolic_MUMPS;
  B->ops->view                   = MatView_MUMPS;

  ierr = PetscObjectComposeFunction((PetscObject)B,"MatFactorGetSolverPackage_C",MatFactorGetSolverPackage_mumps);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsSetIcntl_C",MatMumpsSetIcntl_MUMPS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsGetIcntl_C",MatMumpsGetIcntl_MUMPS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsSetCntl_C",MatMumpsSetCntl_MUMPS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsGetCntl_C",MatMumpsGetCntl_MUMPS);CHKERRQ(ierr);

  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsGetInfo_C",MatMumpsGetInfo_MUMPS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsGetInfog_C",MatMumpsGetInfog_MUMPS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsGetRinfo_C",MatMumpsGetRinfo_MUMPS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsGetRinfog_C",MatMumpsGetRinfog_MUMPS);CHKERRQ(ierr);

  B->factortype = MAT_FACTOR_CHOLESKY;
  if (A->spd_set && A->spd) mumps->sym = 1;
  else                      mumps->sym = 2;

  mumps->isAIJ    = PETSC_FALSE;
  mumps->Destroy  = B->ops->destroy;
  B->ops->destroy = MatDestroy_MUMPS;
  B->spptr        = (void*)mumps;

  ierr = PetscInitializeMUMPS(A,mumps);CHKERRQ(ierr);

  *F = B;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetFactor_baij_mumps"
PETSC_EXTERN PetscErrorCode MatGetFactor_baij_mumps(Mat A,MatFactorType ftype,Mat *F)
{
  Mat            B;
  PetscErrorCode ierr;
  Mat_MUMPS      *mumps;
  PetscBool      isSeqBAIJ;

  PetscFunctionBegin;
  /* Create the factorization matrix */
  ierr = PetscObjectTypeCompare((PetscObject)A,MATSEQBAIJ,&isSeqBAIJ);CHKERRQ(ierr);
  ierr = MatCreate(PetscObjectComm((PetscObject)A),&B);CHKERRQ(ierr);
  ierr = MatSetSizes(B,A->rmap->n,A->cmap->n,A->rmap->N,A->cmap->N);CHKERRQ(ierr);
  ierr = MatSetType(B,((PetscObject)A)->type_name);CHKERRQ(ierr);
  if (isSeqBAIJ) {
    ierr = MatSeqBAIJSetPreallocation(B,A->rmap->bs,0,NULL);CHKERRQ(ierr);
  } else {
    ierr = MatMPIBAIJSetPreallocation(B,A->rmap->bs,0,NULL,0,NULL);CHKERRQ(ierr);
  }

  ierr = PetscNewLog(B,&mumps);CHKERRQ(ierr);
  if (ftype == MAT_FACTOR_LU) {
    B->ops->lufactorsymbolic = MatLUFactorSymbolic_BAIJMUMPS;
    B->factortype            = MAT_FACTOR_LU;
    if (isSeqBAIJ) mumps->ConvertToTriples = MatConvertToTriples_seqbaij_seqaij;
    else mumps->ConvertToTriples = MatConvertToTriples_mpibaij_mpiaij;
    mumps->sym = 0;
  } else SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Cannot use PETSc BAIJ matrices with MUMPS Cholesky, use SBAIJ or AIJ matrix instead\n");

  B->ops->view = MatView_MUMPS;

  ierr = PetscObjectComposeFunction((PetscObject)B,"MatFactorGetSolverPackage_C",MatFactorGetSolverPackage_mumps);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsSetIcntl_C",MatMumpsSetIcntl_MUMPS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsGetIcntl_C",MatMumpsGetIcntl_MUMPS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsSetCntl_C",MatMumpsSetCntl_MUMPS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsGetCntl_C",MatMumpsGetCntl_MUMPS);CHKERRQ(ierr);

  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsGetInfo_C",MatMumpsGetInfo_MUMPS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsGetInfog_C",MatMumpsGetInfog_MUMPS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsGetRinfo_C",MatMumpsGetRinfo_MUMPS);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMumpsGetRinfog_C",MatMumpsGetRinfog_MUMPS);CHKERRQ(ierr);

  mumps->isAIJ    = PETSC_TRUE;
  mumps->Destroy  = B->ops->destroy;
  B->ops->destroy = MatDestroy_MUMPS;
  B->spptr        = (void*)mumps;

  ierr = PetscInitializeMUMPS(A,mumps);CHKERRQ(ierr);

  *F = B;
  PetscFunctionReturn(0);
}
