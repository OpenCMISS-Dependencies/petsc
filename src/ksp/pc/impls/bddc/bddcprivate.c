#include <../src/ksp/pc/impls/bddc/bddc.h>
#include <../src/ksp/pc/impls/bddc/bddcprivate.h>
#include <petscblaslapack.h>

#undef __FUNCT__
#define __FUNCT__ "PCBDDCSetUpSolvers"
PetscErrorCode PCBDDCSetUpSolvers(PC pc)
{
  PC_BDDC*       pcbddc = (PC_BDDC*)pc->data;
  PetscScalar    *coarse_submat_vals;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* Compute matrix after change of basis and extract local submatrices */
  ierr = PCBDDCSetUpLocalMatrices(pc);CHKERRQ(ierr);

  /* Setup local scatters R_to_B and (optionally) R_to_D */
  /* PCBDDCSetUpLocalWorkVectors and PCBDDCSetUpLocalMatrices should be called first! */
  ierr = PCBDDCSetUpLocalScatters(pc);CHKERRQ(ierr);

  /* Setup local solvers ksp_D and ksp_R */
  /* PCBDDCSetUpLocalScatters should be called first! */
  ierr = PCBDDCSetUpLocalSolvers(pc);CHKERRQ(ierr);

  /* Change global null space passed in by the user if change of basis has been requested */
  if (pcbddc->NullSpace && pcbddc->ChangeOfBasisMatrix) {
    ierr = PCBDDCNullSpaceAdaptGlobal(pc);CHKERRQ(ierr);
  }

  /*
     Setup local correction and local part of coarse basis.
     Gives back the dense local part of the coarse matrix in column major ordering
  */
  ierr = PCBDDCSetUpCorrection(pc,&coarse_submat_vals);CHKERRQ(ierr);

  /* Compute total number of coarse nodes and setup coarse solver */
  ierr = PCBDDCSetUpCoarseSolver(pc,coarse_submat_vals);CHKERRQ(ierr);

  /* free */
  ierr = PetscFree(coarse_submat_vals);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCBDDCResetCustomization"
PetscErrorCode PCBDDCResetCustomization(PC pc)
{
  PC_BDDC        *pcbddc = (PC_BDDC*)pc->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PCBDDCGraphResetCSR(pcbddc->mat_graph);CHKERRQ(ierr);
  ierr = ISDestroy(&pcbddc->user_primal_vertices);CHKERRQ(ierr);
  ierr = MatNullSpaceDestroy(&pcbddc->NullSpace);CHKERRQ(ierr);
  ierr = ISDestroy(&pcbddc->NeumannBoundaries);CHKERRQ(ierr);
  ierr = ISDestroy(&pcbddc->NeumannBoundariesLocal);CHKERRQ(ierr);
  ierr = ISDestroy(&pcbddc->DirichletBoundaries);CHKERRQ(ierr);
  ierr = MatNullSpaceDestroy(&pcbddc->onearnullspace);CHKERRQ(ierr);
  ierr = PetscFree(pcbddc->onearnullvecs_state);CHKERRQ(ierr);
  ierr = ISDestroy(&pcbddc->DirichletBoundariesLocal);CHKERRQ(ierr);
  ierr = PCBDDCSetDofsSplitting(pc,0,NULL);CHKERRQ(ierr);
  ierr = PCBDDCSetDofsSplittingLocal(pc,0,NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCBDDCResetTopography"
PetscErrorCode PCBDDCResetTopography(PC pc)
{
  PC_BDDC        *pcbddc = (PC_BDDC*)pc->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatDestroy(&pcbddc->user_ChangeOfBasisMatrix);CHKERRQ(ierr);
  ierr = MatDestroy(&pcbddc->ChangeOfBasisMatrix);CHKERRQ(ierr);
  ierr = MatDestroy(&pcbddc->ConstraintMatrix);CHKERRQ(ierr);
  ierr = PCBDDCGraphReset(pcbddc->mat_graph);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCBDDCResetSolvers"
PetscErrorCode PCBDDCResetSolvers(PC pc)
{
  PC_BDDC        *pcbddc = (PC_BDDC*)pc->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecDestroy(&pcbddc->coarse_vec);CHKERRQ(ierr);
  ierr = VecDestroy(&pcbddc->coarse_rhs);CHKERRQ(ierr);
  ierr = MatDestroy(&pcbddc->coarse_phi_B);CHKERRQ(ierr);
  ierr = MatDestroy(&pcbddc->coarse_phi_D);CHKERRQ(ierr);
  ierr = MatDestroy(&pcbddc->coarse_psi_B);CHKERRQ(ierr);
  ierr = MatDestroy(&pcbddc->coarse_psi_D);CHKERRQ(ierr);
  ierr = VecDestroy(&pcbddc->vec1_P);CHKERRQ(ierr);
  ierr = VecDestroy(&pcbddc->vec1_C);CHKERRQ(ierr);
  ierr = MatDestroy(&pcbddc->local_auxmat1);CHKERRQ(ierr);
  ierr = MatDestroy(&pcbddc->local_auxmat2);CHKERRQ(ierr);
  ierr = VecDestroy(&pcbddc->vec1_R);CHKERRQ(ierr);
  ierr = VecDestroy(&pcbddc->vec2_R);CHKERRQ(ierr);
  ierr = ISDestroy(&pcbddc->is_R_local);CHKERRQ(ierr);
  ierr = VecScatterDestroy(&pcbddc->R_to_B);CHKERRQ(ierr);
  ierr = VecScatterDestroy(&pcbddc->R_to_D);CHKERRQ(ierr);
  ierr = VecScatterDestroy(&pcbddc->coarse_loc_to_glob);CHKERRQ(ierr);
  ierr = KSPDestroy(&pcbddc->ksp_D);CHKERRQ(ierr);
  ierr = KSPDestroy(&pcbddc->ksp_R);CHKERRQ(ierr);
  ierr = KSPDestroy(&pcbddc->coarse_ksp);CHKERRQ(ierr);
  ierr = MatDestroy(&pcbddc->local_mat);CHKERRQ(ierr);
  ierr = PetscFree(pcbddc->primal_indices_local_idxs);CHKERRQ(ierr);
  ierr = PetscFree(pcbddc->global_primal_indices);CHKERRQ(ierr);
  ierr = ISDestroy(&pcbddc->coarse_subassembling);CHKERRQ(ierr);
  ierr = ISDestroy(&pcbddc->coarse_subassembling_init);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCBDDCSetUpLocalWorkVectors"
PetscErrorCode PCBDDCSetUpLocalWorkVectors(PC pc)
{
  PC_BDDC        *pcbddc = (PC_BDDC*)pc->data;
  PC_IS          *pcis = (PC_IS*)pc->data;
  VecType        impVecType;
  PetscInt       n_constraints,n_R,old_size;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!pcbddc->ConstraintMatrix) {
    SETERRQ(PetscObjectComm((PetscObject)pc),PETSC_ERR_PLIB,"BDDC Constraint matrix has not been created");
  }
  /* get sizes */
  n_constraints = pcbddc->local_primal_size - pcbddc->n_actual_vertices;
  n_R = pcis->n-pcbddc->n_actual_vertices;
  ierr = VecGetType(pcis->vec1_N,&impVecType);CHKERRQ(ierr);
  /* local work vectors (try to avoid unneeded work)*/
  /* R nodes */
  old_size = -1;
  if (pcbddc->vec1_R) {
    ierr = VecGetSize(pcbddc->vec1_R,&old_size);CHKERRQ(ierr);
  }
  if (n_R != old_size) {
    ierr = VecDestroy(&pcbddc->vec1_R);CHKERRQ(ierr);
    ierr = VecDestroy(&pcbddc->vec2_R);CHKERRQ(ierr);
    ierr = VecCreate(PetscObjectComm((PetscObject)pcis->vec1_N),&pcbddc->vec1_R);CHKERRQ(ierr);
    ierr = VecSetSizes(pcbddc->vec1_R,PETSC_DECIDE,n_R);CHKERRQ(ierr);
    ierr = VecSetType(pcbddc->vec1_R,impVecType);CHKERRQ(ierr);
    ierr = VecDuplicate(pcbddc->vec1_R,&pcbddc->vec2_R);CHKERRQ(ierr);
  }
  /* local primal dofs */
  old_size = -1;
  if (pcbddc->vec1_P) {
    ierr = VecGetSize(pcbddc->vec1_P,&old_size);CHKERRQ(ierr);
  }
  if (pcbddc->local_primal_size != old_size) {
    ierr = VecDestroy(&pcbddc->vec1_P);CHKERRQ(ierr);
    ierr = VecCreate(PetscObjectComm((PetscObject)pcis->vec1_N),&pcbddc->vec1_P);CHKERRQ(ierr);
    ierr = VecSetSizes(pcbddc->vec1_P,PETSC_DECIDE,pcbddc->local_primal_size);CHKERRQ(ierr);
    ierr = VecSetType(pcbddc->vec1_P,impVecType);CHKERRQ(ierr);
  }
  /* local explicit constraints */
  old_size = -1;
  if (pcbddc->vec1_C) {
    ierr = VecGetSize(pcbddc->vec1_C,&old_size);CHKERRQ(ierr);
  }
  if (n_constraints && n_constraints != old_size) {
    ierr = VecDestroy(&pcbddc->vec1_C);CHKERRQ(ierr);
    ierr = VecCreate(PetscObjectComm((PetscObject)pcis->vec1_N),&pcbddc->vec1_C);CHKERRQ(ierr);
    ierr = VecSetSizes(pcbddc->vec1_C,PETSC_DECIDE,n_constraints);CHKERRQ(ierr);
    ierr = VecSetType(pcbddc->vec1_C,impVecType);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCBDDCSetUpCorrection"
PetscErrorCode PCBDDCSetUpCorrection(PC pc, PetscScalar **coarse_submat_vals_n)
{
  PetscErrorCode         ierr;
  /* pointers to pcis and pcbddc */
  PC_IS*                 pcis = (PC_IS*)pc->data;
  PC_BDDC*               pcbddc = (PC_BDDC*)pc->data;
  /* submatrices of local problem */
  Mat                    A_RV,A_VR,A_VV;
  /* working matrices */
  Mat                    M1,M2,M3,C_CR;
  /* working vectors */
  Vec                    vec1_C,vec2_C,vec1_V,vec2_V;
  /* additional working stuff */
  IS                     is_aux;
  PetscScalar            *coarse_submat_vals; /* TODO: use a PETSc matrix */
  const PetscScalar      *array,*row_cmat_values;
  const PetscInt         *row_cmat_indices,*idx_R_local;
  PetscInt               *idx_V_B,*auxindices;
  PetscInt               n_vertices,n_constraints,size_of_constraint;
  PetscInt               i,j,n_R,n_D,n_B;
  PetscBool              unsymmetric_check;
  /* matrix type (vector type propagated downstream from vec1_C and local matrix type) */
  MatType                impMatType;
  /* some shortcuts to scalars */
  PetscScalar            zero=0.0,one=1.0,m_one=-1.0;
  /* for debugging purposes */
  PetscReal              *coarsefunctions_errors,*constraints_errors;

  PetscFunctionBegin;
  /* get number of vertices (corners plus constraints with change of basis)
     pcbddc->n_actual_vertices stores the actual number of vertices, pcbddc->n_vertices the number of corners computed */
  n_vertices = pcbddc->n_actual_vertices;
  n_constraints = pcbddc->local_primal_size-n_vertices;
  /* Set Non-overlapping dimensions */
  n_B = pcis->n_B; n_D = pcis->n - n_B;
  n_R = pcis->n-n_vertices;

  /* Set types for local objects needed by BDDC precondtioner */
  impMatType = MATSEQDENSE;

  /* Allocating some extra storage just to be safe */
  ierr = PetscMalloc (pcis->n*sizeof(PetscInt),&auxindices);CHKERRQ(ierr);
  for (i=0;i<pcis->n;i++) auxindices[i]=i;

  /* vertices in boundary numbering */
  ierr = PetscMalloc1(n_vertices,&idx_V_B);CHKERRQ(ierr);
  ierr = ISGlobalToLocalMappingApply(pcbddc->BtoNmap,IS_GTOLM_DROP,n_vertices,pcbddc->primal_indices_local_idxs,&i,idx_V_B);CHKERRQ(ierr);
  if (i != n_vertices) {
    SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_PLIB,"Error in boundary numbering for BDDC vertices! %d != %d\n",n_vertices,i);
  }

  /* Precompute stuffs needed for preprocessing and application of BDDC*/
  if (n_constraints) {
    /* see if we can save some allocations */
    if (pcbddc->local_auxmat2) {
      PetscInt on_R,on_constraints;
      ierr = MatGetSize(pcbddc->local_auxmat2,&on_R,&on_constraints);CHKERRQ(ierr);
      if (on_R != n_R || on_constraints != n_constraints) {
        ierr = MatDestroy(&pcbddc->local_auxmat2);CHKERRQ(ierr);
        ierr = MatDestroy(&pcbddc->local_auxmat1);CHKERRQ(ierr);
      }
    }
    /* work vectors */
    ierr = VecDuplicate(pcbddc->vec1_C,&vec1_C);CHKERRQ(ierr);
    ierr = VecDuplicate(pcbddc->vec1_C,&vec2_C);CHKERRQ(ierr);
    /* auxiliary matrices */
    if (!pcbddc->local_auxmat2) {
      ierr = MatCreate(PETSC_COMM_SELF,&pcbddc->local_auxmat2);CHKERRQ(ierr);
      ierr = MatSetSizes(pcbddc->local_auxmat2,n_R,n_constraints,PETSC_DECIDE,PETSC_DECIDE);CHKERRQ(ierr);
      ierr = MatSetType(pcbddc->local_auxmat2,impMatType);CHKERRQ(ierr);
      ierr = MatSetUp(pcbddc->local_auxmat2);CHKERRQ(ierr);
    }

    /* Extract constraints on R nodes: C_{CR}  */
    ierr = ISCreateStride(PETSC_COMM_SELF,n_constraints,n_vertices,1,&is_aux);CHKERRQ(ierr);
    ierr = MatGetSubMatrix(pcbddc->ConstraintMatrix,is_aux,pcbddc->is_R_local,MAT_INITIAL_MATRIX,&C_CR);CHKERRQ(ierr);
    ierr = ISDestroy(&is_aux);CHKERRQ(ierr);

    /* Assemble local_auxmat2 = - A_{RR}^{-1} C^T_{CR} needed by BDDC application */
    for (i=0;i<n_constraints;i++) {
      ierr = VecSet(pcbddc->vec1_R,zero);CHKERRQ(ierr);
      /* Get row of constraint matrix in R numbering */
      ierr = MatGetRow(C_CR,i,&size_of_constraint,&row_cmat_indices,&row_cmat_values);CHKERRQ(ierr);
      ierr = VecSetValues(pcbddc->vec1_R,size_of_constraint,row_cmat_indices,row_cmat_values,INSERT_VALUES);CHKERRQ(ierr);
      ierr = MatRestoreRow(C_CR,i,&size_of_constraint,&row_cmat_indices,&row_cmat_values);CHKERRQ(ierr);
      ierr = VecAssemblyBegin(pcbddc->vec1_R);CHKERRQ(ierr);
      ierr = VecAssemblyEnd(pcbddc->vec1_R);CHKERRQ(ierr);
      /* Solve for row of constraint matrix in R numbering */
      ierr = KSPSolve(pcbddc->ksp_R,pcbddc->vec1_R,pcbddc->vec2_R);CHKERRQ(ierr);
      /* Set values in local_auxmat2 */
      ierr = VecGetArrayRead(pcbddc->vec2_R,&array);CHKERRQ(ierr);
      ierr = MatSetValues(pcbddc->local_auxmat2,n_R,auxindices,1,&i,array,INSERT_VALUES);CHKERRQ(ierr);
      ierr = VecRestoreArrayRead(pcbddc->vec2_R,&array);CHKERRQ(ierr);
    }
    ierr = MatAssemblyBegin(pcbddc->local_auxmat2,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd(pcbddc->local_auxmat2,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatScale(pcbddc->local_auxmat2,m_one);CHKERRQ(ierr);

    /* Assemble explicitly M1 = ( C_{CR} A_{RR}^{-1} C^T_{CR} )^{-1} needed in preproc  */
    ierr = MatMatMult(C_CR,pcbddc->local_auxmat2,MAT_INITIAL_MATRIX,PETSC_DEFAULT,&M3);CHKERRQ(ierr);
    ierr = MatLUFactor(M3,NULL,NULL,NULL);CHKERRQ(ierr);
    ierr = MatCreate(PETSC_COMM_SELF,&M1);CHKERRQ(ierr);
    ierr = MatSetSizes(M1,n_constraints,n_constraints,n_constraints,n_constraints);CHKERRQ(ierr);
    ierr = MatSetType(M1,impMatType);CHKERRQ(ierr);
    ierr = MatSetUp(M1);CHKERRQ(ierr);
    ierr = MatDuplicate(M1,MAT_DO_NOT_COPY_VALUES,&M2);CHKERRQ(ierr);
    ierr = MatZeroEntries(M2);CHKERRQ(ierr);
    ierr = VecSet(vec1_C,m_one);CHKERRQ(ierr);
    ierr = MatDiagonalSet(M2,vec1_C,INSERT_VALUES);CHKERRQ(ierr);
    ierr = MatMatSolve(M3,M2,M1);CHKERRQ(ierr);
    ierr = MatDestroy(&M2);CHKERRQ(ierr);
    ierr = MatDestroy(&M3);CHKERRQ(ierr);
    /* Assemble local_auxmat1 = M1*C_{CR} needed by BDDC application in KSP and in preproc */
    if (!pcbddc->local_auxmat1) {
      ierr = MatMatMult(M1,C_CR,MAT_INITIAL_MATRIX,PETSC_DEFAULT,&pcbddc->local_auxmat1);CHKERRQ(ierr);
    } else {
      ierr = MatMatMult(M1,C_CR,MAT_REUSE_MATRIX,PETSC_DEFAULT,&pcbddc->local_auxmat1);CHKERRQ(ierr);
    }
  }

  /* Get submatrices from subdomain matrix */
  if (n_vertices) {
    PetscInt ibs,mbs;
    PetscBool issbaij;
    Mat newmat;

    ierr = ISComplement(pcbddc->is_R_local,0,pcis->n,&is_aux);CHKERRQ(ierr);
    ierr = MatGetBlockSize(pcbddc->local_mat,&mbs);CHKERRQ(ierr);
    ierr = ISGetBlockSize(pcbddc->is_R_local,&ibs);CHKERRQ(ierr);
    if (ibs != mbs) { /* need to convert to SEQAIJ */
      ierr = MatConvert(pcbddc->local_mat,MATSEQAIJ,MAT_INITIAL_MATRIX,&newmat);CHKERRQ(ierr);
      ierr = MatGetSubMatrix(newmat,pcbddc->is_R_local,is_aux,MAT_INITIAL_MATRIX,&A_RV);CHKERRQ(ierr);
      ierr = MatGetSubMatrix(newmat,is_aux,pcbddc->is_R_local,MAT_INITIAL_MATRIX,&A_VR);CHKERRQ(ierr);
      ierr = MatGetSubMatrix(newmat,is_aux,is_aux,MAT_INITIAL_MATRIX,&A_VV);CHKERRQ(ierr);
      ierr = MatDestroy(&newmat);CHKERRQ(ierr);
    } else {
      /* this is safe */
      ierr = MatGetSubMatrix(pcbddc->local_mat,is_aux,is_aux,MAT_INITIAL_MATRIX,&A_VV);CHKERRQ(ierr);
      ierr = PetscObjectTypeCompare((PetscObject)pcbddc->local_mat,MATSEQSBAIJ,&issbaij);CHKERRQ(ierr);
      if (issbaij) { /* need to convert to BAIJ to get offdiagonal blocks */
        ierr = MatConvert(pcbddc->local_mat,MATSEQBAIJ,MAT_INITIAL_MATRIX,&newmat);CHKERRQ(ierr);
        /* which of the two approaches is faster? */
        /* ierr = MatGetSubMatrix(newmat,pcbddc->is_R_local,is_aux,MAT_INITIAL_MATRIX,&A_RV);CHKERRQ(ierr);
        ierr = MatCreateTranspose(A_RV,&A_VR);CHKERRQ(ierr);*/
        ierr = MatGetSubMatrix(newmat,is_aux,pcbddc->is_R_local,MAT_INITIAL_MATRIX,&A_VR);CHKERRQ(ierr);
        ierr = MatCreateTranspose(A_VR,&A_RV);CHKERRQ(ierr);
        ierr = MatDestroy(&newmat);CHKERRQ(ierr);
      } else {
        ierr = MatGetSubMatrix(pcbddc->local_mat,pcbddc->is_R_local,is_aux,MAT_INITIAL_MATRIX,&A_RV);CHKERRQ(ierr);
        ierr = MatGetSubMatrix(pcbddc->local_mat,is_aux,pcbddc->is_R_local,MAT_INITIAL_MATRIX,&A_VR);CHKERRQ(ierr);
      }
    }
    ierr = MatGetVecs(A_RV,&vec1_V,NULL);CHKERRQ(ierr);
    ierr = VecDuplicate(vec1_V,&vec2_V);CHKERRQ(ierr);
    ierr = ISDestroy(&is_aux);CHKERRQ(ierr);
  }

  /* Matrix of coarse basis functions (local) */
  if (pcbddc->coarse_phi_B) {
    PetscInt on_B,on_primal;
    ierr = MatGetSize(pcbddc->coarse_phi_B,&on_B,&on_primal);CHKERRQ(ierr);
    if (on_B != n_B || on_primal != pcbddc->local_primal_size) {
      ierr = MatDestroy(&pcbddc->coarse_phi_B);CHKERRQ(ierr);
      ierr = MatDestroy(&pcbddc->coarse_psi_B);CHKERRQ(ierr);
    }
  }
  if (pcbddc->coarse_phi_D) {
    PetscInt on_D,on_primal;
    ierr = MatGetSize(pcbddc->coarse_phi_D,&on_D,&on_primal);CHKERRQ(ierr);
    if (on_D != n_D || on_primal != pcbddc->local_primal_size) {
      ierr = MatDestroy(&pcbddc->coarse_phi_D);CHKERRQ(ierr);
      ierr = MatDestroy(&pcbddc->coarse_psi_D);CHKERRQ(ierr);
    }
  }
  if (!pcbddc->coarse_phi_B) {
    ierr = MatCreate(PETSC_COMM_SELF,&pcbddc->coarse_phi_B);CHKERRQ(ierr);
    ierr = MatSetSizes(pcbddc->coarse_phi_B,n_B,pcbddc->local_primal_size,n_B,pcbddc->local_primal_size);CHKERRQ(ierr);
    ierr = MatSetType(pcbddc->coarse_phi_B,impMatType);CHKERRQ(ierr);
    ierr = MatSetUp(pcbddc->coarse_phi_B);CHKERRQ(ierr);
  }
  if ( (pcbddc->switch_static || pcbddc->dbg_flag) && !pcbddc->coarse_phi_D ) {
    ierr = MatCreate(PETSC_COMM_SELF,&pcbddc->coarse_phi_D);CHKERRQ(ierr);
    ierr = MatSetSizes(pcbddc->coarse_phi_D,n_D,pcbddc->local_primal_size,n_D,pcbddc->local_primal_size);CHKERRQ(ierr);
    ierr = MatSetType(pcbddc->coarse_phi_D,impMatType);CHKERRQ(ierr);
    ierr = MatSetUp(pcbddc->coarse_phi_D);CHKERRQ(ierr);
  }

  if (pcbddc->dbg_flag) {
    ierr = ISGetIndices(pcbddc->is_R_local,&idx_R_local);CHKERRQ(ierr);
    ierr = PetscMalloc1(2*pcbddc->local_primal_size,&coarsefunctions_errors);CHKERRQ(ierr);
    ierr = PetscMalloc1(2*pcbddc->local_primal_size,&constraints_errors);CHKERRQ(ierr);
  }
  /* Subdomain contribution (Non-overlapping) to coarse matrix  */
  ierr = PetscMalloc1((pcbddc->local_primal_size)*(pcbddc->local_primal_size),&coarse_submat_vals);CHKERRQ(ierr);

  /* We are now ready to evaluate coarse basis functions and subdomain contribution to coarse problem */

  /* vertices */
  for (i=0;i<n_vertices;i++) {
    /* this should not be needed, but MatMult_BAIJ is broken when using compressed row routines */
    ierr = VecSet(pcbddc->vec1_R,zero);CHKERRQ(ierr); /* TODO: REMOVE IT */
    ierr = VecSet(vec1_V,zero);CHKERRQ(ierr);
    ierr = VecSetValue(vec1_V,i,one,INSERT_VALUES);CHKERRQ(ierr);
    ierr = VecAssemblyBegin(vec1_V);CHKERRQ(ierr);
    ierr = VecAssemblyEnd(vec1_V);CHKERRQ(ierr);
    /* simplified solution of saddle point problem with null rhs on constraints multipliers */
    ierr = MatMult(A_RV,vec1_V,pcbddc->vec1_R);CHKERRQ(ierr);
    ierr = KSPSolve(pcbddc->ksp_R,pcbddc->vec1_R,pcbddc->vec1_R);CHKERRQ(ierr);
    ierr = VecScale(pcbddc->vec1_R,m_one);CHKERRQ(ierr);
    if (n_constraints) {
      ierr = MatMult(pcbddc->local_auxmat1,pcbddc->vec1_R,vec1_C);CHKERRQ(ierr);
      ierr = MatMultAdd(pcbddc->local_auxmat2,vec1_C,pcbddc->vec1_R,pcbddc->vec1_R);CHKERRQ(ierr);
      ierr = VecScale(vec1_C,m_one);CHKERRQ(ierr);
    }
    ierr = MatMult(A_VR,pcbddc->vec1_R,vec2_V);CHKERRQ(ierr);
    ierr = MatMultAdd(A_VV,vec1_V,vec2_V,vec2_V);CHKERRQ(ierr);

    /* Set values in coarse basis function and subdomain part of coarse_mat */
    /* coarse basis functions */
    ierr = VecSet(pcis->vec1_B,zero);CHKERRQ(ierr);
    ierr = VecScatterBegin(pcbddc->R_to_B,pcbddc->vec1_R,pcis->vec1_B,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(pcbddc->R_to_B,pcbddc->vec1_R,pcis->vec1_B,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecGetArrayRead(pcis->vec1_B,&array);CHKERRQ(ierr);
    ierr = MatSetValues(pcbddc->coarse_phi_B,n_B,auxindices,1,&i,array,INSERT_VALUES);CHKERRQ(ierr);
    ierr = VecRestoreArrayRead(pcis->vec1_B,&array);CHKERRQ(ierr);
    ierr = MatSetValue(pcbddc->coarse_phi_B,idx_V_B[i],i,one,INSERT_VALUES);CHKERRQ(ierr);
    if (pcbddc->switch_static || pcbddc->dbg_flag) {
      ierr = VecScatterBegin(pcbddc->R_to_D,pcbddc->vec1_R,pcis->vec1_D,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
      ierr = VecScatterEnd(pcbddc->R_to_D,pcbddc->vec1_R,pcis->vec1_D,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
      ierr = VecGetArrayRead(pcis->vec1_D,&array);CHKERRQ(ierr);
      ierr = MatSetValues(pcbddc->coarse_phi_D,n_D,auxindices,1,&i,array,INSERT_VALUES);CHKERRQ(ierr);
      ierr = VecRestoreArrayRead(pcis->vec1_D,&array);CHKERRQ(ierr);
    }
    /* subdomain contribution to coarse matrix. WARNING -> column major ordering */
    ierr = VecGetArrayRead(vec2_V,&array);CHKERRQ(ierr);
    ierr = PetscMemcpy(&coarse_submat_vals[i*pcbddc->local_primal_size],array,n_vertices*sizeof(PetscScalar));CHKERRQ(ierr);
    ierr = VecRestoreArrayRead(vec2_V,&array);CHKERRQ(ierr);
    if (n_constraints) {
      ierr = VecGetArrayRead(vec1_C,&array);CHKERRQ(ierr);
      ierr = PetscMemcpy(&coarse_submat_vals[i*pcbddc->local_primal_size+n_vertices],array,n_constraints*sizeof(PetscScalar));CHKERRQ(ierr);
      ierr = VecRestoreArrayRead(vec1_C,&array);CHKERRQ(ierr);
    }

    /* check */
    if (pcbddc->dbg_flag) {
      /* assemble subdomain vector on local nodes */
      ierr = VecSet(pcis->vec1_N,zero);CHKERRQ(ierr);
      ierr = VecGetArrayRead(pcbddc->vec1_R,&array);CHKERRQ(ierr);
      ierr = VecSetValues(pcis->vec1_N,n_R,idx_R_local,array,INSERT_VALUES);CHKERRQ(ierr);
      ierr = VecRestoreArrayRead(pcbddc->vec1_R,&array);CHKERRQ(ierr);
      ierr = VecSetValue(pcis->vec1_N,pcbddc->primal_indices_local_idxs[i],one,INSERT_VALUES);CHKERRQ(ierr);
      ierr = VecAssemblyBegin(pcis->vec1_N);CHKERRQ(ierr);
      ierr = VecAssemblyEnd(pcis->vec1_N);CHKERRQ(ierr);
      /* assemble subdomain vector of lagrange multipliers (i.e. primal nodes) */
      ierr = VecSet(pcbddc->vec1_P,zero);CHKERRQ(ierr);
      ierr = VecGetArrayRead(vec2_V,&array);CHKERRQ(ierr);
      ierr = VecSetValues(pcbddc->vec1_P,n_vertices,auxindices,array,INSERT_VALUES);CHKERRQ(ierr);
      ierr = VecRestoreArrayRead(vec2_V,&array);CHKERRQ(ierr);
      if (n_constraints) {
        ierr = VecGetArrayRead(vec1_C,&array);CHKERRQ(ierr);
        ierr = VecSetValues(pcbddc->vec1_P,n_constraints,&auxindices[n_vertices],array,INSERT_VALUES);CHKERRQ(ierr);
        ierr = VecRestoreArrayRead(vec1_C,&array);CHKERRQ(ierr);
      }
      ierr = VecAssemblyBegin(pcbddc->vec1_P);CHKERRQ(ierr);
      ierr = VecAssemblyEnd(pcbddc->vec1_P);CHKERRQ(ierr);
      ierr = VecScale(pcbddc->vec1_P,m_one);CHKERRQ(ierr);
      /* check saddle point solution */
      ierr = MatMult(pcbddc->local_mat,pcis->vec1_N,pcis->vec2_N);CHKERRQ(ierr);
      ierr = MatMultTransposeAdd(pcbddc->ConstraintMatrix,pcbddc->vec1_P,pcis->vec2_N,pcis->vec2_N);CHKERRQ(ierr);
      ierr = VecNorm(pcis->vec2_N,NORM_INFINITY,&coarsefunctions_errors[i]);CHKERRQ(ierr);
      ierr = MatMult(pcbddc->ConstraintMatrix,pcis->vec1_N,pcbddc->vec1_P);CHKERRQ(ierr);
      /* shift by the identity matrix */
      ierr = VecSetValue(pcbddc->vec1_P,i,m_one,ADD_VALUES);CHKERRQ(ierr);
      ierr = VecAssemblyBegin(pcbddc->vec1_P);CHKERRQ(ierr);
      ierr = VecAssemblyEnd(pcbddc->vec1_P);CHKERRQ(ierr);
      ierr = VecNorm(pcbddc->vec1_P,NORM_INFINITY,&constraints_errors[i]);CHKERRQ(ierr);
    }
  }

  /* constraints */
  for (i=0;i<n_constraints;i++) {
    ierr = VecSet(vec2_C,zero);CHKERRQ(ierr);
    ierr = VecSetValue(vec2_C,i,m_one,INSERT_VALUES);CHKERRQ(ierr);
    ierr = VecAssemblyBegin(vec2_C);CHKERRQ(ierr);
    ierr = VecAssemblyEnd(vec2_C);CHKERRQ(ierr);
    /* simplified solution of saddle point problem with null rhs on vertices multipliers */
    ierr = MatMult(M1,vec2_C,vec1_C);CHKERRQ(ierr);
    ierr = MatMult(pcbddc->local_auxmat2,vec1_C,pcbddc->vec1_R);CHKERRQ(ierr);
    ierr = VecScale(vec1_C,m_one);CHKERRQ(ierr);
    if (n_vertices) {
      ierr = MatMult(A_VR,pcbddc->vec1_R,vec2_V);CHKERRQ(ierr);
    }
    /* Set values in coarse basis function and subdomain part of coarse_mat */
    /* coarse basis functions */
    j = i+n_vertices; /* don't touch this! */
    ierr = VecSet(pcis->vec1_B,zero);CHKERRQ(ierr);
    ierr = VecScatterBegin(pcbddc->R_to_B,pcbddc->vec1_R,pcis->vec1_B,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(pcbddc->R_to_B,pcbddc->vec1_R,pcis->vec1_B,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecGetArrayRead(pcis->vec1_B,&array);CHKERRQ(ierr);
    ierr = MatSetValues(pcbddc->coarse_phi_B,n_B,auxindices,1,&j,array,INSERT_VALUES);CHKERRQ(ierr);
    ierr = VecRestoreArrayRead(pcis->vec1_B,&array);CHKERRQ(ierr);
    if (pcbddc->switch_static || pcbddc->dbg_flag) {
      ierr = VecScatterBegin(pcbddc->R_to_D,pcbddc->vec1_R,pcis->vec1_D,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
      ierr = VecScatterEnd(pcbddc->R_to_D,pcbddc->vec1_R,pcis->vec1_D,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
      ierr = VecGetArrayRead(pcis->vec1_D,&array);CHKERRQ(ierr);
      ierr = MatSetValues(pcbddc->coarse_phi_D,n_D,auxindices,1,&j,array,INSERT_VALUES);CHKERRQ(ierr);
      ierr = VecRestoreArrayRead(pcis->vec1_D,&array);CHKERRQ(ierr);
    }
    /* subdomain contribution to coarse matrix. WARNING -> column major ordering */
    if (n_vertices) {
      ierr = VecGetArrayRead(vec2_V,&array);CHKERRQ(ierr);
      ierr = PetscMemcpy(&coarse_submat_vals[j*pcbddc->local_primal_size],array,n_vertices*sizeof(PetscScalar));CHKERRQ(ierr);
      ierr = VecRestoreArrayRead(vec2_V,&array);CHKERRQ(ierr);
    }
    ierr = VecGetArrayRead(vec1_C,&array);CHKERRQ(ierr);
    ierr = PetscMemcpy(&coarse_submat_vals[j*pcbddc->local_primal_size+n_vertices],array,n_constraints*sizeof(PetscScalar));CHKERRQ(ierr);
    ierr = VecRestoreArrayRead(vec1_C,&array);CHKERRQ(ierr);

    if (pcbddc->dbg_flag) {
      /* assemble subdomain vector on nodes */
      ierr = VecSet(pcis->vec1_N,zero);CHKERRQ(ierr);
      ierr = VecGetArrayRead(pcbddc->vec1_R,&array);CHKERRQ(ierr);
      ierr = VecSetValues(pcis->vec1_N,n_R,idx_R_local,array,INSERT_VALUES);CHKERRQ(ierr);
      ierr = VecRestoreArrayRead(pcbddc->vec1_R,&array);CHKERRQ(ierr);
      ierr = VecAssemblyBegin(pcis->vec1_N);CHKERRQ(ierr);
      ierr = VecAssemblyEnd(pcis->vec1_N);CHKERRQ(ierr);
      /* assemble subdomain vector of lagrange multipliers */
      ierr = VecSet(pcbddc->vec1_P,zero);CHKERRQ(ierr);
      if (n_vertices) {
        ierr = VecGetArrayRead(vec2_V,&array);CHKERRQ(ierr);
        ierr = VecSetValues(pcbddc->vec1_P,n_vertices,auxindices,array,INSERT_VALUES);CHKERRQ(ierr);
        ierr = VecRestoreArrayRead(vec2_V,&array);CHKERRQ(ierr);
      }
      ierr = VecGetArrayRead(vec1_C,&array);CHKERRQ(ierr);
      ierr = VecSetValues(pcbddc->vec1_P,n_constraints,&auxindices[n_vertices],array,INSERT_VALUES);CHKERRQ(ierr);
      ierr = VecRestoreArrayRead(vec1_C,&array);CHKERRQ(ierr);
      ierr = VecAssemblyBegin(pcbddc->vec1_P);CHKERRQ(ierr);
      ierr = VecAssemblyEnd(pcbddc->vec1_P);CHKERRQ(ierr);
      ierr = VecScale(pcbddc->vec1_P,m_one);CHKERRQ(ierr);
      /* check saddle point solution */
      ierr = MatMult(pcbddc->local_mat,pcis->vec1_N,pcis->vec2_N);CHKERRQ(ierr);
      ierr = MatMultTransposeAdd(pcbddc->ConstraintMatrix,pcbddc->vec1_P,pcis->vec2_N,pcis->vec2_N);CHKERRQ(ierr);
      ierr = VecNorm(pcis->vec2_N,NORM_INFINITY,&coarsefunctions_errors[j]);CHKERRQ(ierr);
      ierr = MatMult(pcbddc->ConstraintMatrix,pcis->vec1_N,pcbddc->vec1_P);CHKERRQ(ierr);
      /* shift by the identity matrix */
      ierr = VecSetValue(pcbddc->vec1_P,j,m_one,ADD_VALUES);CHKERRQ(ierr);
      ierr = VecAssemblyBegin(pcbddc->vec1_P);CHKERRQ(ierr);
      ierr = VecAssemblyEnd(pcbddc->vec1_P);CHKERRQ(ierr);
      ierr = VecNorm(pcbddc->vec1_P,NORM_INFINITY,&constraints_errors[j]);CHKERRQ(ierr);
    }
  }
  /* call assembling routines for local coarse basis */
  ierr = MatAssemblyBegin(pcbddc->coarse_phi_B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(pcbddc->coarse_phi_B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  if (pcbddc->switch_static || pcbddc->dbg_flag) {
    ierr = MatAssemblyBegin(pcbddc->coarse_phi_D,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd(pcbddc->coarse_phi_D,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  }

  /* compute other basis functions for non-symmetric problems */
  ierr = MatIsSymmetric(pc->pmat,1.e-4,&pcbddc->issym);CHKERRQ(ierr);
  if (!pcbddc->issym) {
    if (!pcbddc->coarse_psi_B) {
      ierr = MatCreate(PETSC_COMM_SELF,&pcbddc->coarse_psi_B);CHKERRQ(ierr);
      ierr = MatSetSizes(pcbddc->coarse_psi_B,n_B,pcbddc->local_primal_size,n_B,pcbddc->local_primal_size);CHKERRQ(ierr);
      ierr = MatSetType(pcbddc->coarse_psi_B,impMatType);CHKERRQ(ierr);
      ierr = MatSetUp(pcbddc->coarse_psi_B);CHKERRQ(ierr);
    }
    if ( (pcbddc->switch_static || pcbddc->dbg_flag) && !pcbddc->coarse_psi_D) {
      ierr = MatCreate(PETSC_COMM_SELF,&pcbddc->coarse_psi_D);CHKERRQ(ierr);
      ierr = MatSetSizes(pcbddc->coarse_psi_D,n_D,pcbddc->local_primal_size,n_D,pcbddc->local_primal_size);CHKERRQ(ierr);
      ierr = MatSetType(pcbddc->coarse_psi_D,impMatType);CHKERRQ(ierr);
      ierr = MatSetUp(pcbddc->coarse_psi_D);CHKERRQ(ierr);
    }
    for (i=0;i<pcbddc->local_primal_size;i++) {
      if (n_constraints) {
        ierr = VecSet(vec1_C,zero);CHKERRQ(ierr);
        for (j=0;j<n_constraints;j++) {
          ierr = VecSetValue(vec1_C,j,coarse_submat_vals[(j+n_vertices)*pcbddc->local_primal_size+i],INSERT_VALUES);CHKERRQ(ierr);
        }
        ierr = VecAssemblyBegin(vec1_C);CHKERRQ(ierr);
        ierr = VecAssemblyEnd(vec1_C);CHKERRQ(ierr);
      }
      if (i<n_vertices) {
        ierr = VecSet(vec1_V,zero);CHKERRQ(ierr);
        ierr = VecSetValue(vec1_V,i,m_one,INSERT_VALUES);CHKERRQ(ierr);
        ierr = VecAssemblyBegin(vec1_V);CHKERRQ(ierr);
        ierr = VecAssemblyEnd(vec1_V);CHKERRQ(ierr);
        ierr = MatMultTranspose(A_VR,vec1_V,pcbddc->vec1_R);CHKERRQ(ierr);
        if (n_constraints) {
          ierr = MatMultTransposeAdd(C_CR,vec1_C,pcbddc->vec1_R,pcbddc->vec1_R);CHKERRQ(ierr);
        }
      } else {
        ierr = MatMultTranspose(C_CR,vec1_C,pcbddc->vec1_R);CHKERRQ(ierr);
      }
      ierr = KSPSolveTranspose(pcbddc->ksp_R,pcbddc->vec1_R,pcbddc->vec1_R);CHKERRQ(ierr);
      ierr = VecSet(pcis->vec1_B,zero);CHKERRQ(ierr);
      ierr = VecScatterBegin(pcbddc->R_to_B,pcbddc->vec1_R,pcis->vec1_B,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
      ierr = VecScatterEnd(pcbddc->R_to_B,pcbddc->vec1_R,pcis->vec1_B,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
      ierr = VecGetArrayRead(pcis->vec1_B,&array);CHKERRQ(ierr);
      ierr = MatSetValues(pcbddc->coarse_psi_B,n_B,auxindices,1,&i,array,INSERT_VALUES);CHKERRQ(ierr);
      ierr = VecRestoreArrayRead(pcis->vec1_B,&array);CHKERRQ(ierr);
      if (i<n_vertices) {
        ierr = MatSetValue(pcbddc->coarse_psi_B,idx_V_B[i],i,one,INSERT_VALUES);CHKERRQ(ierr);
      }
      if (pcbddc->switch_static || pcbddc->dbg_flag) {
        ierr = VecScatterBegin(pcbddc->R_to_D,pcbddc->vec1_R,pcis->vec1_D,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
        ierr = VecScatterEnd(pcbddc->R_to_D,pcbddc->vec1_R,pcis->vec1_D,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
        ierr = VecGetArrayRead(pcis->vec1_D,&array);CHKERRQ(ierr);
        ierr = MatSetValues(pcbddc->coarse_psi_D,n_D,auxindices,1,&i,array,INSERT_VALUES);CHKERRQ(ierr);
        ierr = VecRestoreArrayRead(pcis->vec1_D,&array);CHKERRQ(ierr);
      }

      if (pcbddc->dbg_flag) {
        /* assemble subdomain vector on nodes */
        ierr = VecSet(pcis->vec1_N,zero);CHKERRQ(ierr);
        ierr = VecGetArrayRead(pcbddc->vec1_R,&array);CHKERRQ(ierr);
        ierr = VecSetValues(pcis->vec1_N,n_R,idx_R_local,array,INSERT_VALUES);CHKERRQ(ierr);
        ierr = VecRestoreArrayRead(pcbddc->vec1_R,&array);CHKERRQ(ierr);
        if (i<n_vertices) {
          ierr = VecSetValue(pcis->vec1_N,pcbddc->primal_indices_local_idxs[i],one,INSERT_VALUES);CHKERRQ(ierr);
        }
        ierr = VecAssemblyBegin(pcis->vec1_N);CHKERRQ(ierr);
        ierr = VecAssemblyEnd(pcis->vec1_N);CHKERRQ(ierr);
        /* assemble subdomain vector of lagrange multipliers */
        for (j=0;j<pcbddc->local_primal_size;j++) {
          ierr = VecSetValue(pcbddc->vec1_P,j,-coarse_submat_vals[j*pcbddc->local_primal_size+i],INSERT_VALUES);CHKERRQ(ierr);
        }
        ierr = VecAssemblyBegin(pcbddc->vec1_P);CHKERRQ(ierr);
        ierr = VecAssemblyEnd(pcbddc->vec1_P);CHKERRQ(ierr);
        /* check saddle point solution */
        ierr = MatMultTranspose(pcbddc->local_mat,pcis->vec1_N,pcis->vec2_N);CHKERRQ(ierr);
        ierr = MatMultTransposeAdd(pcbddc->ConstraintMatrix,pcbddc->vec1_P,pcis->vec2_N,pcis->vec2_N);CHKERRQ(ierr);
        ierr = VecNorm(pcis->vec2_N,NORM_INFINITY,&coarsefunctions_errors[i+pcbddc->local_primal_size]);CHKERRQ(ierr);
        ierr = MatMult(pcbddc->ConstraintMatrix,pcis->vec1_N,pcbddc->vec1_P);CHKERRQ(ierr);
        /* shift by the identity matrix */
        ierr = VecSetValue(pcbddc->vec1_P,i,m_one,ADD_VALUES);CHKERRQ(ierr);
        ierr = VecAssemblyBegin(pcbddc->vec1_P);CHKERRQ(ierr);
        ierr = VecAssemblyEnd(pcbddc->vec1_P);CHKERRQ(ierr);
        ierr = VecNorm(pcbddc->vec1_P,NORM_INFINITY,&constraints_errors[i+pcbddc->local_primal_size]);CHKERRQ(ierr);
      }
    }
    ierr = MatAssemblyBegin(pcbddc->coarse_psi_B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd(pcbddc->coarse_psi_B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    if (pcbddc->switch_static || pcbddc->dbg_flag) {
      ierr = MatAssemblyBegin(pcbddc->coarse_psi_D,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
      ierr = MatAssemblyEnd(pcbddc->coarse_psi_D,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    }
    unsymmetric_check = PETSC_TRUE;
  } else { /* take references to already computed coarse basis */
    unsymmetric_check = PETSC_FALSE;
    ierr = PetscObjectReference((PetscObject)pcbddc->coarse_phi_B);CHKERRQ(ierr);
    pcbddc->coarse_psi_B = pcbddc->coarse_phi_B;
    if (pcbddc->coarse_phi_D) {
      ierr = PetscObjectReference((PetscObject)pcbddc->coarse_phi_D);CHKERRQ(ierr);
      pcbddc->coarse_psi_D = pcbddc->coarse_phi_D;
    }
  }
  ierr = PetscFree(idx_V_B);CHKERRQ(ierr);
  /* Checking coarse_sub_mat and coarse basis functios */
  /* Symmetric case     : It should be \Phi^{(j)^T} A^{(j)} \Phi^{(j)}=coarse_sub_mat */
  /* Non-symmetric case : It should be \Psi^{(j)^T} A^{(j)} \Phi^{(j)}=coarse_sub_mat */
  if (pcbddc->dbg_flag) {
    Mat         coarse_sub_mat;
    Mat         AUXMAT,TM1,TM2,TM3,TM4;
    Mat         coarse_phi_D,coarse_phi_B;
    Mat         coarse_psi_D,coarse_psi_B;
    Mat         A_II,A_BB,A_IB,A_BI;
    MatType     checkmattype=MATSEQAIJ;
    PetscReal   real_value;

    ierr = MatConvert(pcis->A_II,checkmattype,MAT_INITIAL_MATRIX,&A_II);CHKERRQ(ierr);
    ierr = MatConvert(pcis->A_IB,checkmattype,MAT_INITIAL_MATRIX,&A_IB);CHKERRQ(ierr);
    ierr = MatConvert(pcis->A_BI,checkmattype,MAT_INITIAL_MATRIX,&A_BI);CHKERRQ(ierr);
    ierr = MatConvert(pcis->A_BB,checkmattype,MAT_INITIAL_MATRIX,&A_BB);CHKERRQ(ierr);
    ierr = MatConvert(pcbddc->coarse_phi_D,checkmattype,MAT_INITIAL_MATRIX,&coarse_phi_D);CHKERRQ(ierr);
    ierr = MatConvert(pcbddc->coarse_phi_B,checkmattype,MAT_INITIAL_MATRIX,&coarse_phi_B);CHKERRQ(ierr);
    if (unsymmetric_check) {
      ierr = MatConvert(pcbddc->coarse_psi_D,checkmattype,MAT_INITIAL_MATRIX,&coarse_psi_D);CHKERRQ(ierr);
      ierr = MatConvert(pcbddc->coarse_psi_B,checkmattype,MAT_INITIAL_MATRIX,&coarse_psi_B);CHKERRQ(ierr);
    }
    ierr = MatCreateSeqDense(PETSC_COMM_SELF,pcbddc->local_primal_size,pcbddc->local_primal_size,coarse_submat_vals,&coarse_sub_mat);CHKERRQ(ierr);

    ierr = PetscViewerASCIIPrintf(pcbddc->dbg_viewer,"--------------------------------------------------\n");CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(pcbddc->dbg_viewer,"Check coarse sub mat and local basis functions\n");CHKERRQ(ierr);
    ierr = PetscViewerFlush(pcbddc->dbg_viewer);CHKERRQ(ierr);
    if (unsymmetric_check) {
      ierr = MatMatMult(A_II,coarse_phi_D,MAT_INITIAL_MATRIX,1.0,&AUXMAT);CHKERRQ(ierr);
      ierr = MatTransposeMatMult(coarse_psi_D,AUXMAT,MAT_INITIAL_MATRIX,1.0,&TM1);CHKERRQ(ierr);
      ierr = MatDestroy(&AUXMAT);CHKERRQ(ierr);
      ierr = MatMatMult(A_BB,coarse_phi_B,MAT_INITIAL_MATRIX,1.0,&AUXMAT);CHKERRQ(ierr);
      ierr = MatTransposeMatMult(coarse_psi_B,AUXMAT,MAT_INITIAL_MATRIX,1.0,&TM2);CHKERRQ(ierr);
      ierr = MatDestroy(&AUXMAT);CHKERRQ(ierr);
      ierr = MatMatMult(A_IB,coarse_phi_B,MAT_INITIAL_MATRIX,1.0,&AUXMAT);CHKERRQ(ierr);
      ierr = MatTransposeMatMult(coarse_psi_D,AUXMAT,MAT_INITIAL_MATRIX,1.0,&TM3);CHKERRQ(ierr);
      ierr = MatDestroy(&AUXMAT);CHKERRQ(ierr);
      ierr = MatMatMult(A_BI,coarse_phi_D,MAT_INITIAL_MATRIX,1.0,&AUXMAT);CHKERRQ(ierr);
      ierr = MatTransposeMatMult(coarse_psi_B,AUXMAT,MAT_INITIAL_MATRIX,1.0,&TM4);CHKERRQ(ierr);
      ierr = MatDestroy(&AUXMAT);CHKERRQ(ierr);
    } else {
      ierr = MatPtAP(A_II,coarse_phi_D,MAT_INITIAL_MATRIX,1.0,&TM1);CHKERRQ(ierr);
      ierr = MatPtAP(A_BB,coarse_phi_B,MAT_INITIAL_MATRIX,1.0,&TM2);CHKERRQ(ierr);
      ierr = MatMatMult(A_IB,coarse_phi_B,MAT_INITIAL_MATRIX,1.0,&AUXMAT);CHKERRQ(ierr);
      ierr = MatTransposeMatMult(coarse_phi_D,AUXMAT,MAT_INITIAL_MATRIX,1.0,&TM3);CHKERRQ(ierr);
      ierr = MatDestroy(&AUXMAT);CHKERRQ(ierr);
      ierr = MatMatMult(A_BI,coarse_phi_D,MAT_INITIAL_MATRIX,1.0,&AUXMAT);CHKERRQ(ierr);
      ierr = MatTransposeMatMult(coarse_phi_B,AUXMAT,MAT_INITIAL_MATRIX,1.0,&TM4);CHKERRQ(ierr);
      ierr = MatDestroy(&AUXMAT);CHKERRQ(ierr);
    }
    ierr = MatAXPY(TM1,one,TM2,DIFFERENT_NONZERO_PATTERN);CHKERRQ(ierr);
    ierr = MatAXPY(TM1,one,TM3,DIFFERENT_NONZERO_PATTERN);CHKERRQ(ierr);
    ierr = MatAXPY(TM1,one,TM4,DIFFERENT_NONZERO_PATTERN);CHKERRQ(ierr);
    ierr = MatConvert(TM1,MATSEQDENSE,MAT_REUSE_MATRIX,&TM1);CHKERRQ(ierr);
    ierr = MatAXPY(TM1,m_one,coarse_sub_mat,DIFFERENT_NONZERO_PATTERN);CHKERRQ(ierr);
    ierr = MatNorm(TM1,NORM_INFINITY,&real_value);CHKERRQ(ierr);
    ierr = PetscViewerASCIISynchronizedAllow(pcbddc->dbg_viewer,PETSC_TRUE);CHKERRQ(ierr);
    ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"----------------------------------\n");CHKERRQ(ierr);
    ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"Subdomain %04d \n",PetscGlobalRank);CHKERRQ(ierr);
    ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"matrix error = % 1.14e\n",real_value);CHKERRQ(ierr);
    ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"coarse functions (phi) errors\n");CHKERRQ(ierr);
    for (i=0;i<pcbddc->local_primal_size;i++) {
      ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"local %02d-th function error = % 1.14e\n",i,coarsefunctions_errors[i]);CHKERRQ(ierr);
    }
    ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"constraints (phi) errors\n");CHKERRQ(ierr);
    for (i=0;i<pcbddc->local_primal_size;i++) {
      ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"local %02d-th function error = % 1.14e\n",i,constraints_errors[i]);CHKERRQ(ierr);
    }
    if (unsymmetric_check) {
      ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"coarse functions (psi) errors\n");CHKERRQ(ierr);
      for (i=pcbddc->local_primal_size;i<2*pcbddc->local_primal_size;i++) {
        ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"local %02d-th function error = % 1.14e\n",i-pcbddc->local_primal_size,coarsefunctions_errors[i]);CHKERRQ(ierr);
      }
      ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"constraints (psi) errors\n");CHKERRQ(ierr);
      for (i=pcbddc->local_primal_size;i<2*pcbddc->local_primal_size;i++) {
        ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"local %02d-th function error = % 1.14e\n",i-pcbddc->local_primal_size,constraints_errors[i]);CHKERRQ(ierr);
      }
    }
    ierr = PetscViewerFlush(pcbddc->dbg_viewer);CHKERRQ(ierr);
    ierr = MatDestroy(&A_II);CHKERRQ(ierr);
    ierr = MatDestroy(&A_BB);CHKERRQ(ierr);
    ierr = MatDestroy(&A_IB);CHKERRQ(ierr);
    ierr = MatDestroy(&A_BI);CHKERRQ(ierr);
    ierr = MatDestroy(&TM1);CHKERRQ(ierr);
    ierr = MatDestroy(&TM2);CHKERRQ(ierr);
    ierr = MatDestroy(&TM3);CHKERRQ(ierr);
    ierr = MatDestroy(&TM4);CHKERRQ(ierr);
    ierr = MatDestroy(&coarse_phi_D);CHKERRQ(ierr);
    ierr = MatDestroy(&coarse_phi_B);CHKERRQ(ierr);
    if (unsymmetric_check) {
      ierr = MatDestroy(&coarse_psi_D);CHKERRQ(ierr);
      ierr = MatDestroy(&coarse_psi_B);CHKERRQ(ierr);
    }
    ierr = MatDestroy(&coarse_sub_mat);CHKERRQ(ierr);
    ierr = ISRestoreIndices(pcbddc->is_R_local,&idx_R_local);CHKERRQ(ierr);
    ierr = PetscFree(coarsefunctions_errors);CHKERRQ(ierr);
    ierr = PetscFree(constraints_errors);CHKERRQ(ierr);
  }
  /* free memory */
  if (n_vertices) {
    ierr = VecDestroy(&vec1_V);CHKERRQ(ierr);
    ierr = VecDestroy(&vec2_V);CHKERRQ(ierr);
    ierr = MatDestroy(&A_RV);CHKERRQ(ierr);
    ierr = MatDestroy(&A_VR);CHKERRQ(ierr);
    ierr = MatDestroy(&A_VV);CHKERRQ(ierr);
  }
  if (n_constraints) {
    ierr = VecDestroy(&vec1_C);CHKERRQ(ierr);
    ierr = VecDestroy(&vec2_C);CHKERRQ(ierr);
    ierr = MatDestroy(&M1);CHKERRQ(ierr);
    ierr = MatDestroy(&C_CR);CHKERRQ(ierr);
  }
  ierr = PetscFree(auxindices);CHKERRQ(ierr);
  /* get back data */
  *coarse_submat_vals_n = coarse_submat_vals;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCBDDCSetUpLocalMatrices"
PetscErrorCode PCBDDCSetUpLocalMatrices(PC pc)
{
  PC_IS*            pcis = (PC_IS*)(pc->data);
  PC_BDDC*          pcbddc = (PC_BDDC*)pc->data;
  Mat_IS*           matis = (Mat_IS*)pc->pmat->data;
  PetscBool         issbaij,isseqaij;
  /* manage repeated solves */
  MatReuse          reuse;
  PetscErrorCode    ierr;

  PetscFunctionBegin;
  if ( (pcbddc->use_change_of_basis && !pcbddc->ChangeOfBasisMatrix) || (pcbddc->user_ChangeOfBasisMatrix && !pcbddc->ChangeOfBasisMatrix) ) {
    SETERRQ(PetscObjectComm((PetscObject)pc),PETSC_ERR_PLIB,"BDDC Change of basis matrix has not been created");
  }
  /* get mat flags */
  reuse = MAT_INITIAL_MATRIX;
  if (pc->setupcalled) {
    if (pc->flag == SAME_NONZERO_PATTERN) {
      reuse = MAT_REUSE_MATRIX;
    } else {
      reuse = MAT_INITIAL_MATRIX;
    }
  }
  if (reuse == MAT_INITIAL_MATRIX) {
    ierr = MatDestroy(&pcis->A_II);CHKERRQ(ierr);
    ierr = MatDestroy(&pcis->A_IB);CHKERRQ(ierr);
    ierr = MatDestroy(&pcis->A_BI);CHKERRQ(ierr);
    ierr = MatDestroy(&pcis->A_BB);CHKERRQ(ierr);
    ierr = MatDestroy(&pcbddc->local_mat);CHKERRQ(ierr);
  }

  /* transform local matrices if needed */
  if (pcbddc->ChangeOfBasisMatrix) {
    Mat         change_mat_all;
    PetscScalar *row_cmat_values;
    PetscInt    *row_cmat_indices;
    PetscInt    *nnz,*is_indices,*temp_indices;
    PetscInt    i,j,k,n_D,n_B;

    /* Get Non-overlapping dimensions */
    n_B = pcis->n_B;
    n_D = pcis->n-n_B;

    /* compute nonzero structure of change of basis on all local nodes */
    ierr = PetscMalloc1(pcis->n,&nnz);CHKERRQ(ierr);
    ierr = ISGetIndices(pcis->is_I_local,(const PetscInt**)&is_indices);CHKERRQ(ierr);
    for (i=0;i<n_D;i++) nnz[is_indices[i]] = 1;
    ierr = ISRestoreIndices(pcis->is_I_local,(const PetscInt**)&is_indices);CHKERRQ(ierr);
    ierr = ISGetIndices(pcis->is_B_local,(const PetscInt**)&is_indices);CHKERRQ(ierr);
    k=1;
    for (i=0;i<n_B;i++) {
      ierr = MatGetRow(pcbddc->ChangeOfBasisMatrix,i,&j,NULL,NULL);CHKERRQ(ierr);
      nnz[is_indices[i]]=j;
      if (k < j) k = j;
      ierr = MatRestoreRow(pcbddc->ChangeOfBasisMatrix,i,&j,NULL,NULL);CHKERRQ(ierr);
    }
    ierr = ISRestoreIndices(pcis->is_B_local,(const PetscInt**)&is_indices);CHKERRQ(ierr);
    /* assemble change of basis matrix on the whole set of local dofs */
    ierr = PetscMalloc1(k,&temp_indices);CHKERRQ(ierr);
    ierr = MatCreate(PETSC_COMM_SELF,&change_mat_all);CHKERRQ(ierr);
    ierr = MatSetSizes(change_mat_all,pcis->n,pcis->n,pcis->n,pcis->n);CHKERRQ(ierr);
    ierr = MatSetType(change_mat_all,MATSEQAIJ);CHKERRQ(ierr);
    ierr = MatSeqAIJSetPreallocation(change_mat_all,0,nnz);CHKERRQ(ierr);
    ierr = ISGetIndices(pcis->is_I_local,(const PetscInt**)&is_indices);CHKERRQ(ierr);
    for (i=0;i<n_D;i++) {
      ierr = MatSetValue(change_mat_all,is_indices[i],is_indices[i],1.0,INSERT_VALUES);CHKERRQ(ierr);
    }
    ierr = ISRestoreIndices(pcis->is_I_local,(const PetscInt**)&is_indices);CHKERRQ(ierr);
    ierr = ISGetIndices(pcis->is_B_local,(const PetscInt**)&is_indices);CHKERRQ(ierr);
    for (i=0;i<n_B;i++) {
      ierr = MatGetRow(pcbddc->ChangeOfBasisMatrix,i,&j,(const PetscInt**)&row_cmat_indices,(const PetscScalar**)&row_cmat_values);CHKERRQ(ierr);
      for (k=0; k<j; k++) temp_indices[k]=is_indices[row_cmat_indices[k]];
      ierr = MatSetValues(change_mat_all,1,&is_indices[i],j,temp_indices,row_cmat_values,INSERT_VALUES);CHKERRQ(ierr);
      ierr = MatRestoreRow(pcbddc->ChangeOfBasisMatrix,i,&j,(const PetscInt**)&row_cmat_indices,(const PetscScalar**)&row_cmat_values);CHKERRQ(ierr);
    }
    ierr = MatAssemblyBegin(change_mat_all,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd(change_mat_all,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    /* TODO: HOW TO WORK WITH BAIJ and SBAIJ and SEQDENSE? */
    ierr = PetscObjectTypeCompare((PetscObject)matis->A,MATSEQBAIJ,&isseqaij);CHKERRQ(ierr);
    if (isseqaij) {
      ierr = MatPtAP(matis->A,change_mat_all,reuse,2.0,&pcbddc->local_mat);CHKERRQ(ierr);
    } else {
      Mat work_mat;
      ierr = MatConvert(matis->A,MATSEQAIJ,MAT_INITIAL_MATRIX,&work_mat);CHKERRQ(ierr);
      ierr = MatPtAP(work_mat,change_mat_all,reuse,2.0,&pcbddc->local_mat);CHKERRQ(ierr);
      ierr = MatDestroy(&work_mat);CHKERRQ(ierr);
    }
    /*
    ierr = PetscViewerSetFormat(PETSC_VIEWER_STDOUT_SELF,PETSC_VIEWER_ASCII_MATLAB);CHKERRQ(ierr);
    ierr = MatView(change_mat_all,(PetscViewer)0);CHKERRQ(ierr);
    */
    ierr = MatDestroy(&change_mat_all);CHKERRQ(ierr);
    ierr = PetscFree(nnz);CHKERRQ(ierr);
    ierr = PetscFree(temp_indices);CHKERRQ(ierr);
  } else {
    /* without change of basis, the local matrix is unchanged */
    if (!pcbddc->local_mat) {
      ierr = PetscObjectReference((PetscObject)matis->A);CHKERRQ(ierr);
      pcbddc->local_mat = matis->A;
    }
  }

  /* get submatrices */
  ierr = MatGetSubMatrix(pcbddc->local_mat,pcis->is_I_local,pcis->is_I_local,reuse,&pcis->A_II);CHKERRQ(ierr);
  ierr = MatGetSubMatrix(pcbddc->local_mat,pcis->is_B_local,pcis->is_B_local,reuse,&pcis->A_BB);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)pcbddc->local_mat,MATSEQSBAIJ,&issbaij);CHKERRQ(ierr);
  if (!issbaij) {
    ierr = MatGetSubMatrix(pcbddc->local_mat,pcis->is_I_local,pcis->is_B_local,reuse,&pcis->A_IB);CHKERRQ(ierr);
    ierr = MatGetSubMatrix(pcbddc->local_mat,pcis->is_B_local,pcis->is_I_local,reuse,&pcis->A_BI);CHKERRQ(ierr);
  } else {
    Mat newmat;
    ierr = MatConvert(pcbddc->local_mat,MATSEQBAIJ,MAT_INITIAL_MATRIX,&newmat);CHKERRQ(ierr);
    ierr = MatGetSubMatrix(newmat,pcis->is_I_local,pcis->is_B_local,reuse,&pcis->A_IB);CHKERRQ(ierr);
    ierr = MatGetSubMatrix(newmat,pcis->is_B_local,pcis->is_I_local,reuse,&pcis->A_BI);CHKERRQ(ierr);
    ierr = MatDestroy(&newmat);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCBDDCSetUpLocalScatters"
PetscErrorCode PCBDDCSetUpLocalScatters(PC pc)
{
  PC_IS*         pcis = (PC_IS*)(pc->data);
  PC_BDDC*       pcbddc = (PC_BDDC*)pc->data;
  IS             is_aux1,is_aux2;
  PetscInt       *aux_array1,*aux_array2,*is_indices,*idx_R_local;
  PetscInt       n_vertices,i,j,n_R,n_D,n_B;
  PetscInt       vbs,bs;
  PetscBT        bitmask;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /*
    No need to setup local scatters if
      - primal space is unchanged
        AND
      - we actually have locally some primal dofs (could not be true in multilevel or for isolated subdomains)
        AND
      - we are not in debugging mode (this is needed since there are Synchronized prints at the end of the subroutine
  */
  if (!pcbddc->new_primal_space_local && pcbddc->local_primal_size && !pcbddc->dbg_flag) {
    PetscFunctionReturn(0);
  }
  /* destroy old objects */
  ierr = ISDestroy(&pcbddc->is_R_local);CHKERRQ(ierr);
  ierr = VecScatterDestroy(&pcbddc->R_to_B);CHKERRQ(ierr);
  ierr = VecScatterDestroy(&pcbddc->R_to_D);CHKERRQ(ierr);
  /* Set Non-overlapping dimensions */
  n_B = pcis->n_B; n_D = pcis->n - n_B;
  n_vertices = pcbddc->n_actual_vertices;
  /* create auxiliary bitmask */
  ierr = PetscBTCreate(pcis->n,&bitmask);CHKERRQ(ierr);
  for (i=0;i<n_vertices;i++) {
    ierr = PetscBTSet(bitmask,pcbddc->primal_indices_local_idxs[i]);CHKERRQ(ierr);
  }

  /* Dohrmann's notation: dofs splitted in R (Remaining: all dofs but the vertices) and V (Vertices) */
  ierr = PetscMalloc1((pcis->n-n_vertices),&idx_R_local);CHKERRQ(ierr);
  for (i=0, n_R=0; i<pcis->n; i++) {
    if (!PetscBTLookup(bitmask,i)) {
      idx_R_local[n_R] = i;
      n_R++;
    }
  }

  /* Block code */
  vbs = 1;
  ierr = MatGetBlockSize(pcbddc->local_mat,&bs);CHKERRQ(ierr);
  if (bs>1 && !(n_vertices%bs)) {
    PetscBool is_blocked = PETSC_TRUE;
    PetscInt  *vary;
    /* Verify if the vertex indices correspond to each element in a block (code taken from sbaij2.c) */
    ierr = PetscMalloc1(pcis->n/bs,&vary);CHKERRQ(ierr);
    ierr = PetscMemzero(vary,pcis->n/bs*sizeof(PetscInt));CHKERRQ(ierr);
    for (i=0; i<n_vertices; i++) vary[pcbddc->primal_indices_local_idxs[i]/bs]++;
    for (i=0; i<n_vertices; i++) {
      if (vary[i]!=0 && vary[i]!=bs) {
        is_blocked = PETSC_FALSE;
        break;
      }
    }
    if (is_blocked) { /* build compressed IS for R nodes (complement of vertices) */
      vbs = bs;
      for (i=0;i<n_R/vbs;i++) {
        idx_R_local[i] = idx_R_local[vbs*i]/vbs;
      }
    }
    ierr = PetscFree(vary);CHKERRQ(ierr);
  }
  ierr = ISCreateBlock(PETSC_COMM_SELF,vbs,n_R/vbs,idx_R_local,PETSC_COPY_VALUES,&pcbddc->is_R_local);CHKERRQ(ierr);
  ierr = PetscFree(idx_R_local);CHKERRQ(ierr);

  /* print some info if requested */
  if (pcbddc->dbg_flag) {
    ierr = PetscViewerASCIIPrintf(pcbddc->dbg_viewer,"--------------------------------------------------\n");CHKERRQ(ierr);
    ierr = PetscViewerFlush(pcbddc->dbg_viewer);CHKERRQ(ierr);
    ierr = PetscViewerASCIISynchronizedAllow(pcbddc->dbg_viewer,PETSC_TRUE);CHKERRQ(ierr);
    ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"Subdomain %04d local dimensions\n",PetscGlobalRank);CHKERRQ(ierr);
    ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"local_size = %d, dirichlet_size = %d, boundary_size = %d\n",pcis->n,n_D,n_B);CHKERRQ(ierr);
    ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"r_size = %d, v_size = %d, constraints = %d, local_primal_size = %d\n",n_R,n_vertices,pcbddc->local_primal_size-n_vertices,pcbddc->local_primal_size);CHKERRQ(ierr);
    ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"pcbddc->n_vertices = %d, pcbddc->n_constraints = %d\n",pcbddc->n_vertices,pcbddc->n_constraints);CHKERRQ(ierr);
    ierr = PetscViewerFlush(pcbddc->dbg_viewer);CHKERRQ(ierr);
  }

  /* VecScatters pcbddc->R_to_B and (optionally) pcbddc->R_to_D */
  ierr = ISGetIndices(pcbddc->is_R_local,(const PetscInt**)&idx_R_local);CHKERRQ(ierr);
  ierr = PetscMalloc1((pcis->n_B-n_vertices),&aux_array1);CHKERRQ(ierr);
  ierr = PetscMalloc1((pcis->n_B-n_vertices),&aux_array2);CHKERRQ(ierr);
  ierr = ISGetIndices(pcis->is_I_local,(const PetscInt**)&is_indices);CHKERRQ(ierr);
  for (i=0; i<n_D; i++) {
    ierr = PetscBTSet(bitmask,is_indices[i]);CHKERRQ(ierr);
  }
  ierr = ISRestoreIndices(pcis->is_I_local,(const PetscInt**)&is_indices);CHKERRQ(ierr);
  for (i=0, j=0; i<n_R; i++) {
    if (!PetscBTLookup(bitmask,idx_R_local[i])) {
      aux_array1[j++] = i;
    }
  }
  ierr = ISCreateGeneral(PETSC_COMM_SELF,j,aux_array1,PETSC_OWN_POINTER,&is_aux1);CHKERRQ(ierr);
  ierr = ISGetIndices(pcis->is_B_local,(const PetscInt**)&is_indices);CHKERRQ(ierr);
  for (i=0, j=0; i<n_B; i++) {
    if (!PetscBTLookup(bitmask,is_indices[i])) {
      aux_array2[j++] = i;
    }
  }
  ierr = ISRestoreIndices(pcis->is_B_local,(const PetscInt**)&is_indices);CHKERRQ(ierr);
  ierr = ISCreateGeneral(PETSC_COMM_SELF,j,aux_array2,PETSC_OWN_POINTER,&is_aux2);CHKERRQ(ierr);
  ierr = VecScatterCreate(pcbddc->vec1_R,is_aux1,pcis->vec1_B,is_aux2,&pcbddc->R_to_B);CHKERRQ(ierr);
  ierr = ISDestroy(&is_aux1);CHKERRQ(ierr);
  ierr = ISDestroy(&is_aux2);CHKERRQ(ierr);

  if (pcbddc->switch_static || pcbddc->dbg_flag) {
    ierr = PetscMalloc1(n_D,&aux_array1);CHKERRQ(ierr);
    for (i=0, j=0; i<n_R; i++) {
      if (PetscBTLookup(bitmask,idx_R_local[i])) {
        aux_array1[j++] = i;
      }
    }
    ierr = ISCreateGeneral(PETSC_COMM_SELF,j,aux_array1,PETSC_OWN_POINTER,&is_aux1);CHKERRQ(ierr);
    ierr = VecScatterCreate(pcbddc->vec1_R,is_aux1,pcis->vec1_D,(IS)0,&pcbddc->R_to_D);CHKERRQ(ierr);
    ierr = ISDestroy(&is_aux1);CHKERRQ(ierr);
  }
  ierr = PetscBTDestroy(&bitmask);CHKERRQ(ierr);
  ierr = ISRestoreIndices(pcbddc->is_R_local,(const PetscInt**)&idx_R_local);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "PCBDDCSetUpLocalSolvers"
PetscErrorCode PCBDDCSetUpLocalSolvers(PC pc)
{
  PC_BDDC        *pcbddc = (PC_BDDC*)pc->data;
  PC_IS          *pcis = (PC_IS*)pc->data;
  PC             pc_temp;
  Mat            A_RR;
  MatReuse       reuse;
  PetscScalar    m_one = -1.0;
  PetscReal      value;
  PetscInt       n_D,n_R,ibs,mbs;
  PetscBool      use_exact,use_exact_reduced,issbaij;
  PetscErrorCode ierr;
  /* prefixes stuff */
  char           dir_prefix[256],neu_prefix[256],str_level[16];
  size_t         len;

  PetscFunctionBegin;

  /* compute prefixes */
  ierr = PetscStrcpy(dir_prefix,"");CHKERRQ(ierr);
  ierr = PetscStrcpy(neu_prefix,"");CHKERRQ(ierr);
  if (!pcbddc->current_level) {
    ierr = PetscStrcpy(dir_prefix,((PetscObject)pc)->prefix);CHKERRQ(ierr);
    ierr = PetscStrcpy(neu_prefix,((PetscObject)pc)->prefix);CHKERRQ(ierr);
    ierr = PetscStrcat(dir_prefix,"pc_bddc_dirichlet_");CHKERRQ(ierr);
    ierr = PetscStrcat(neu_prefix,"pc_bddc_neumann_");CHKERRQ(ierr);
  } else {
    ierr = PetscStrcpy(str_level,"");CHKERRQ(ierr);
    sprintf(str_level,"l%d_",(int)(pcbddc->current_level));
    ierr = PetscStrlen(((PetscObject)pc)->prefix,&len);CHKERRQ(ierr);
    len -= 15; /* remove "pc_bddc_coarse_" */
    if (pcbddc->current_level>1) len -= 3; /* remove "lX_" with X level number */
    if (pcbddc->current_level>10) len -= 1; /* remove another char from level number */
    ierr = PetscStrncpy(dir_prefix,((PetscObject)pc)->prefix,len+1);CHKERRQ(ierr);
    ierr = PetscStrncpy(neu_prefix,((PetscObject)pc)->prefix,len+1);CHKERRQ(ierr);
    ierr = PetscStrcat(dir_prefix,"pc_bddc_dirichlet_");CHKERRQ(ierr);
    ierr = PetscStrcat(neu_prefix,"pc_bddc_neumann_");CHKERRQ(ierr);
    ierr = PetscStrcat(dir_prefix,str_level);CHKERRQ(ierr);
    ierr = PetscStrcat(neu_prefix,str_level);CHKERRQ(ierr);
  }

  /* DIRICHLET PROBLEM */
  /* Matrix for Dirichlet problem is pcis->A_II */
  ierr = ISGetSize(pcis->is_I_local,&n_D);CHKERRQ(ierr);
  if (!pcbddc->ksp_D) { /* create object if not yet build */
    ierr = KSPCreate(PETSC_COMM_SELF,&pcbddc->ksp_D);CHKERRQ(ierr);
    ierr = PetscObjectIncrementTabLevel((PetscObject)pcbddc->ksp_D,(PetscObject)pc,1);CHKERRQ(ierr);
    /* default */
    ierr = KSPSetType(pcbddc->ksp_D,KSPPREONLY);CHKERRQ(ierr);
    ierr = KSPSetOptionsPrefix(pcbddc->ksp_D,dir_prefix);CHKERRQ(ierr);
    ierr = PetscObjectTypeCompare((PetscObject)pcis->A_II,MATSEQSBAIJ,&issbaij);CHKERRQ(ierr);
    ierr = KSPGetPC(pcbddc->ksp_D,&pc_temp);CHKERRQ(ierr);
    if (issbaij) {
      ierr = PCSetType(pc_temp,PCCHOLESKY);CHKERRQ(ierr);
    } else {
      ierr = PCSetType(pc_temp,PCLU);CHKERRQ(ierr);
    }
    /* Allow user's customization */
    ierr = KSPSetFromOptions(pcbddc->ksp_D);CHKERRQ(ierr);
    ierr = PCFactorSetReuseFill(pc_temp,PETSC_TRUE);CHKERRQ(ierr);
  }
  ierr = KSPSetOperators(pcbddc->ksp_D,pcis->A_II,pcis->A_II);CHKERRQ(ierr);
  /* umfpack interface has a bug when matrix dimension is zero. TODO solve from umfpack interface */
  if (!n_D) {
    ierr = KSPGetPC(pcbddc->ksp_D,&pc_temp);CHKERRQ(ierr);
    ierr = PCSetType(pc_temp,PCNONE);CHKERRQ(ierr);
  }
  /* Set Up KSP for Dirichlet problem of BDDC */
  ierr = KSPSetUp(pcbddc->ksp_D);CHKERRQ(ierr);
  /* set ksp_D into pcis data */
  ierr = KSPDestroy(&pcis->ksp_D);CHKERRQ(ierr);
  ierr = PetscObjectReference((PetscObject)pcbddc->ksp_D);CHKERRQ(ierr);
  pcis->ksp_D = pcbddc->ksp_D;

  /* NEUMANN PROBLEM */
  /* Matrix for Neumann problem is A_RR -> we need to create/reuse it at this point */
  ierr = ISGetSize(pcbddc->is_R_local,&n_R);CHKERRQ(ierr);
  if (pcbddc->ksp_R) { /* already created ksp */
    PetscInt nn_R;
    ierr = KSPGetOperators(pcbddc->ksp_R,NULL,&A_RR);CHKERRQ(ierr);
    ierr = PetscObjectReference((PetscObject)A_RR);CHKERRQ(ierr);
    ierr = MatGetSize(A_RR,&nn_R,NULL);CHKERRQ(ierr);
    if (nn_R != n_R) { /* old ksp is not reusable, so reset it */
      ierr = KSPReset(pcbddc->ksp_R);CHKERRQ(ierr);
      ierr = MatDestroy(&A_RR);CHKERRQ(ierr);
      reuse = MAT_INITIAL_MATRIX;
    } else { /* same sizes, but nonzero pattern depend on primal vertices so it can be changed */
      if (pcbddc->new_primal_space_local) { /* we are not sure the matrix will have the same nonzero pattern */
        ierr = MatDestroy(&A_RR);CHKERRQ(ierr);
        reuse = MAT_INITIAL_MATRIX;
      } else { /* safe to reuse the matrix */
        reuse = MAT_REUSE_MATRIX;
      }
    }
    /* last check */
    if (pc->flag == DIFFERENT_NONZERO_PATTERN) {
      ierr = MatDestroy(&A_RR);CHKERRQ(ierr);
      reuse = MAT_INITIAL_MATRIX;
    }
  } else { /* first time, so we need to create the matrix */
    reuse = MAT_INITIAL_MATRIX;
  }
  /* extract A_RR */
  ierr = MatGetBlockSize(pcbddc->local_mat,&mbs);CHKERRQ(ierr);
  ierr = ISGetBlockSize(pcbddc->is_R_local,&ibs);CHKERRQ(ierr);
  if (ibs != mbs) {
    Mat newmat;
    ierr = MatConvert(pcbddc->local_mat,MATSEQAIJ,MAT_INITIAL_MATRIX,&newmat);CHKERRQ(ierr);
    ierr = MatGetSubMatrix(newmat,pcbddc->is_R_local,pcbddc->is_R_local,reuse,&A_RR);CHKERRQ(ierr);
    ierr = MatDestroy(&newmat);CHKERRQ(ierr);
  } else {
    ierr = MatGetSubMatrix(pcbddc->local_mat,pcbddc->is_R_local,pcbddc->is_R_local,reuse,&A_RR);CHKERRQ(ierr);
  }
  if (!pcbddc->ksp_R) { /* create object if not present */
    ierr = KSPCreate(PETSC_COMM_SELF,&pcbddc->ksp_R);CHKERRQ(ierr);
    ierr = PetscObjectIncrementTabLevel((PetscObject)pcbddc->ksp_R,(PetscObject)pc,1);CHKERRQ(ierr);
    /* default */
    ierr = KSPSetType(pcbddc->ksp_R,KSPPREONLY);CHKERRQ(ierr);
    ierr = KSPSetOptionsPrefix(pcbddc->ksp_R,neu_prefix);CHKERRQ(ierr);
    ierr = KSPGetPC(pcbddc->ksp_R,&pc_temp);CHKERRQ(ierr);
    ierr = PetscObjectTypeCompare((PetscObject)A_RR,MATSEQSBAIJ,&issbaij);CHKERRQ(ierr);
    if (issbaij) {
      ierr = PCSetType(pc_temp,PCCHOLESKY);CHKERRQ(ierr);
    } else {
      ierr = PCSetType(pc_temp,PCLU);CHKERRQ(ierr);
    }
    /* Allow user's customization */
    ierr = KSPSetFromOptions(pcbddc->ksp_R);CHKERRQ(ierr);
    ierr = PCFactorSetReuseFill(pc_temp,PETSC_TRUE);CHKERRQ(ierr);
  }
  ierr = KSPSetOperators(pcbddc->ksp_R,A_RR,A_RR);CHKERRQ(ierr);
  /* umfpack interface has a bug when matrix dimension is zero. TODO solve from umfpack interface */
  if (!n_R) {
    ierr = KSPGetPC(pcbddc->ksp_R,&pc_temp);CHKERRQ(ierr);
    ierr = PCSetType(pc_temp,PCNONE);CHKERRQ(ierr);
  }
  /* Set Up KSP for Neumann problem of BDDC */
  ierr = KSPSetUp(pcbddc->ksp_R);CHKERRQ(ierr);

  /* check Dirichlet and Neumann solvers and adapt them if a nullspace correction is needed */
  if (pcbddc->NullSpace || pcbddc->dbg_flag) {
    /* Dirichlet */
    ierr = VecSetRandom(pcis->vec1_D,NULL);CHKERRQ(ierr);
    ierr = MatMult(pcis->A_II,pcis->vec1_D,pcis->vec2_D);CHKERRQ(ierr);
    ierr = KSPSolve(pcbddc->ksp_D,pcis->vec2_D,pcis->vec2_D);CHKERRQ(ierr);
    ierr = VecAXPY(pcis->vec1_D,m_one,pcis->vec2_D);CHKERRQ(ierr);
    ierr = VecNorm(pcis->vec1_D,NORM_INFINITY,&value);CHKERRQ(ierr);
    /* need to be adapted? */
    use_exact = (PetscAbsReal(value) > 1.e-4 ? PETSC_FALSE : PETSC_TRUE);
    ierr = MPI_Allreduce(&use_exact,&use_exact_reduced,1,MPIU_BOOL,MPI_LAND,PetscObjectComm((PetscObject)pc));CHKERRQ(ierr);
    ierr = PCBDDCSetUseExactDirichlet(pc,use_exact_reduced);CHKERRQ(ierr);
    /* print info */
    if (pcbddc->dbg_flag) {
      ierr = PetscViewerFlush(pcbddc->dbg_viewer);CHKERRQ(ierr);
      ierr = PetscViewerASCIISynchronizedAllow(pcbddc->dbg_viewer,PETSC_TRUE);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(pcbddc->dbg_viewer,"--------------------------------------------------\n");CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(pcbddc->dbg_viewer,"Checking solution of Dirichlet and Neumann problems\n");CHKERRQ(ierr);
      ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"Subdomain %04d infinity error for Dirichlet solve (%s) = % 1.14e \n",PetscGlobalRank,((PetscObject)(pcbddc->ksp_D))->prefix,value);CHKERRQ(ierr);
      ierr = PetscViewerFlush(pcbddc->dbg_viewer);CHKERRQ(ierr);
    }
    if (pcbddc->NullSpace && !use_exact_reduced && !pcbddc->switch_static) {
      ierr = PCBDDCNullSpaceAssembleCorrection(pc,pcis->is_I_local);CHKERRQ(ierr);
    }

    /* Neumann */
    ierr = VecSetRandom(pcbddc->vec1_R,NULL);CHKERRQ(ierr);
    ierr = MatMult(A_RR,pcbddc->vec1_R,pcbddc->vec2_R);CHKERRQ(ierr);
    ierr = KSPSolve(pcbddc->ksp_R,pcbddc->vec2_R,pcbddc->vec2_R);CHKERRQ(ierr);
    ierr = VecAXPY(pcbddc->vec1_R,m_one,pcbddc->vec2_R);CHKERRQ(ierr);
    ierr = VecNorm(pcbddc->vec1_R,NORM_INFINITY,&value);CHKERRQ(ierr);
    /* need to be adapted? */
    use_exact = (PetscAbsReal(value) > 1.e-4 ? PETSC_FALSE : PETSC_TRUE);
    ierr = MPI_Allreduce(&use_exact,&use_exact_reduced,1,MPIU_BOOL,MPI_LAND,PetscObjectComm((PetscObject)pc));CHKERRQ(ierr);
    /* print info */
    if (pcbddc->dbg_flag) {
      ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"Subdomain %04d infinity error for Neumann solve (%s) = % 1.14e \n",PetscGlobalRank,((PetscObject)(pcbddc->ksp_R))->prefix,value);CHKERRQ(ierr);
      ierr = PetscViewerFlush(pcbddc->dbg_viewer);CHKERRQ(ierr);
    }
    if (pcbddc->NullSpace && !use_exact_reduced) { /* is it the right logic? */
      ierr = PCBDDCNullSpaceAssembleCorrection(pc,pcbddc->is_R_local);CHKERRQ(ierr);
    }
  }
  /* free Neumann problem's matrix */
  ierr = MatDestroy(&A_RR);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCBDDCSolveSubstructureCorrection"
static PetscErrorCode  PCBDDCSolveSubstructureCorrection(PC pc, Vec rhs, Vec sol, Vec work, PetscBool applytranspose)
{
  PetscErrorCode ierr;
  PC_BDDC*       pcbddc = (PC_BDDC*)(pc->data);

  PetscFunctionBegin;
  if (applytranspose) {
    if (pcbddc->local_auxmat1) {
      ierr = MatMultTranspose(pcbddc->local_auxmat2,rhs,work);CHKERRQ(ierr);
      ierr = MatMultTransposeAdd(pcbddc->local_auxmat1,work,rhs,rhs);CHKERRQ(ierr);
    }
    ierr = KSPSolveTranspose(pcbddc->ksp_R,rhs,sol);CHKERRQ(ierr);
  } else {
    ierr = KSPSolve(pcbddc->ksp_R,rhs,sol);CHKERRQ(ierr);
    if (pcbddc->local_auxmat1) {
      ierr = MatMult(pcbddc->local_auxmat1,sol,work);CHKERRQ(ierr);
      ierr = MatMultAdd(pcbddc->local_auxmat2,work,sol,sol);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

/* parameter apply transpose determines if the interface preconditioner should be applied transposed or not */
#undef __FUNCT__
#define __FUNCT__ "PCBDDCApplyInterfacePreconditioner"
PetscErrorCode  PCBDDCApplyInterfacePreconditioner(PC pc, PetscBool applytranspose)
{
  PetscErrorCode ierr;
  PC_BDDC*        pcbddc = (PC_BDDC*)(pc->data);
  PC_IS*            pcis = (PC_IS*)  (pc->data);
  const PetscScalar zero = 0.0;

  PetscFunctionBegin;
  /* Application of PSI^T or PHI^T (depending on applytranspose, see comment above) */
  if (applytranspose) {
    ierr = MatMultTranspose(pcbddc->coarse_phi_B,pcis->vec1_B,pcbddc->vec1_P);CHKERRQ(ierr);
    if (pcbddc->switch_static) { ierr = MatMultTransposeAdd(pcbddc->coarse_phi_D,pcis->vec1_D,pcbddc->vec1_P,pcbddc->vec1_P);CHKERRQ(ierr); }
  } else {
    ierr = MatMultTranspose(pcbddc->coarse_psi_B,pcis->vec1_B,pcbddc->vec1_P);CHKERRQ(ierr);
    if (pcbddc->switch_static) { ierr = MatMultTransposeAdd(pcbddc->coarse_psi_D,pcis->vec1_D,pcbddc->vec1_P,pcbddc->vec1_P);CHKERRQ(ierr); }
  }
  /* start communications from local primal nodes to rhs of coarse solver */
  ierr = VecSet(pcbddc->coarse_vec,zero);CHKERRQ(ierr);
  ierr = PCBDDCScatterCoarseDataBegin(pc,ADD_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = PCBDDCScatterCoarseDataEnd(pc,ADD_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);

  /* Coarse solution -> rhs and sol updated in PCBDDCScattarCoarseDataBegin/End */
  /* TODO remove null space when doing multilevel */
  if (pcbddc->coarse_ksp) {
    if (applytranspose) {
      ierr = KSPSolveTranspose(pcbddc->coarse_ksp,NULL,NULL);CHKERRQ(ierr);
    } else {
      ierr = KSPSolve(pcbddc->coarse_ksp,NULL,NULL);CHKERRQ(ierr);
    }
  }
  /* start communications from coarse solver solution to local primal nodes */
  ierr = PCBDDCScatterCoarseDataBegin(pc,INSERT_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);

  /* Local solution on R nodes */
  ierr = VecSet(pcbddc->vec1_R,zero);CHKERRQ(ierr);
  ierr = VecScatterBegin(pcbddc->R_to_B,pcis->vec1_B,pcbddc->vec1_R,INSERT_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
  ierr = VecScatterEnd(pcbddc->R_to_B,pcis->vec1_B,pcbddc->vec1_R,INSERT_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
  if (pcbddc->switch_static) {
    ierr = VecScatterBegin(pcbddc->R_to_D,pcis->vec1_D,pcbddc->vec1_R,INSERT_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
    ierr = VecScatterEnd(pcbddc->R_to_D,pcis->vec1_D,pcbddc->vec1_R,INSERT_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
  }
  ierr = PCBDDCSolveSubstructureCorrection(pc,pcbddc->vec1_R,pcbddc->vec2_R,pcbddc->vec1_C,applytranspose);CHKERRQ(ierr);
  ierr = VecSet(pcis->vec1_B,zero);CHKERRQ(ierr);
  ierr = VecScatterBegin(pcbddc->R_to_B,pcbddc->vec2_R,pcis->vec1_B,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(pcbddc->R_to_B,pcbddc->vec2_R,pcis->vec1_B,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  if (pcbddc->switch_static) {
    ierr = VecScatterBegin(pcbddc->R_to_D,pcbddc->vec2_R,pcis->vec1_D,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(pcbddc->R_to_D,pcbddc->vec2_R,pcis->vec1_D,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  }

  /* complete communications from coarse sol to local primal nodes */
  ierr = PCBDDCScatterCoarseDataEnd(pc,INSERT_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);

  /* Sum contributions from two levels */
  if (applytranspose) {
    ierr = MatMultAdd(pcbddc->coarse_psi_B,pcbddc->vec1_P,pcis->vec1_B,pcis->vec1_B);CHKERRQ(ierr);
    if (pcbddc->switch_static) { ierr = MatMultAdd(pcbddc->coarse_psi_D,pcbddc->vec1_P,pcis->vec1_D,pcis->vec1_D);CHKERRQ(ierr); }
  } else {
    ierr = MatMultAdd(pcbddc->coarse_phi_B,pcbddc->vec1_P,pcis->vec1_B,pcis->vec1_B);CHKERRQ(ierr);
    if (pcbddc->switch_static) { ierr = MatMultAdd(pcbddc->coarse_phi_D,pcbddc->vec1_P,pcis->vec1_D,pcis->vec1_D);CHKERRQ(ierr); }
  }
  PetscFunctionReturn(0);
}

/* TODO: the following two function can be optimized using VecPlaceArray whenever possible and using overlap flag */
#undef __FUNCT__
#define __FUNCT__ "PCBDDCScatterCoarseDataBegin"
PetscErrorCode PCBDDCScatterCoarseDataBegin(PC pc,InsertMode imode, ScatterMode smode)
{
  PetscErrorCode ierr;
  PC_BDDC*       pcbddc = (PC_BDDC*)(pc->data);
  PetscScalar    *array,*array2;
  Vec            from,to;

  PetscFunctionBegin;
  if (smode == SCATTER_REVERSE) { /* from global to local -> get data from coarse solution */
    from = pcbddc->coarse_vec;
    to = pcbddc->vec1_P;
    if (pcbddc->coarse_ksp) { /* get array from coarse processes */
      Vec tvec;
      PetscInt lsize;
      ierr = KSPGetSolution(pcbddc->coarse_ksp,&tvec);CHKERRQ(ierr);
      ierr = VecGetLocalSize(tvec,&lsize);CHKERRQ(ierr);
      ierr = VecGetArrayRead(tvec,(const PetscScalar**)&array);CHKERRQ(ierr);
      ierr = VecGetArray(from,&array2);CHKERRQ(ierr);
      ierr = PetscMemcpy(array2,array,lsize*sizeof(PetscScalar));CHKERRQ(ierr);
      ierr = VecRestoreArrayRead(tvec,(const PetscScalar**)&array);CHKERRQ(ierr);
      ierr = VecRestoreArray(from,&array2);CHKERRQ(ierr);
    }
  } else { /* from local to global -> put data in coarse right hand side */
    from = pcbddc->vec1_P;
    to = pcbddc->coarse_vec;
  }
  ierr = VecScatterBegin(pcbddc->coarse_loc_to_glob,from,to,imode,smode);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCBDDCScatterCoarseDataEnd"
PetscErrorCode PCBDDCScatterCoarseDataEnd(PC pc, InsertMode imode, ScatterMode smode)
{
  PetscErrorCode ierr;
  PC_BDDC*       pcbddc = (PC_BDDC*)(pc->data);
  PetscScalar    *array,*array2;
  Vec            from,to;

  PetscFunctionBegin;
  if (smode == SCATTER_REVERSE) { /* from global to local -> get data from coarse solution */
    from = pcbddc->coarse_vec;
    to = pcbddc->vec1_P;
  } else { /* from local to global -> put data in coarse right hand side */
    from = pcbddc->vec1_P;
    to = pcbddc->coarse_vec;
  }
  ierr = VecScatterEnd(pcbddc->coarse_loc_to_glob,from,to,imode,smode);CHKERRQ(ierr);
  if (smode == SCATTER_FORWARD) {
    if (pcbddc->coarse_ksp) { /* get array from coarse processes */
      Vec tvec;
      PetscInt lsize;
      ierr = KSPGetRhs(pcbddc->coarse_ksp,&tvec);CHKERRQ(ierr);
      ierr = VecGetLocalSize(tvec,&lsize);CHKERRQ(ierr);
      ierr = VecGetArrayRead(to,(const PetscScalar**)&array);CHKERRQ(ierr);
      ierr = VecGetArray(tvec,&array2);CHKERRQ(ierr);
      ierr = PetscMemcpy(array2,array,lsize*sizeof(PetscScalar));CHKERRQ(ierr);
      ierr = VecRestoreArrayRead(to,(const PetscScalar**)&array);CHKERRQ(ierr);
      ierr = VecRestoreArray(tvec,&array2);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

/* uncomment for testing purposes */
/* #define PETSC_MISSING_LAPACK_GESVD 1 */
#undef __FUNCT__
#define __FUNCT__ "PCBDDCConstraintsSetUp"
PetscErrorCode PCBDDCConstraintsSetUp(PC pc)
{
  PetscErrorCode    ierr;
  PC_IS*            pcis = (PC_IS*)(pc->data);
  PC_BDDC*          pcbddc = (PC_BDDC*)pc->data;
  Mat_IS*           matis = (Mat_IS*)pc->pmat->data;
  /* constraint and (optionally) change of basis matrix implemented as SeqAIJ */
  MatType           impMatType=MATSEQAIJ;
  /* one and zero */
  PetscScalar       one=1.0,zero=0.0;
  /* space to store constraints and their local indices */
  PetscScalar       *temp_quadrature_constraint;
  PetscInt          *temp_indices,*temp_indices_to_constraint,*temp_indices_to_constraint_B;
  /* iterators */
  PetscInt          i,j,k,total_counts,temp_start_ptr;
  /* stuff to store connected components stored in pcbddc->mat_graph */
  IS                ISForVertices,*ISForFaces,*ISForEdges,*used_IS;
  PetscInt          n_ISForFaces,n_ISForEdges;
  /* near null space stuff */
  MatNullSpace      nearnullsp;
  const Vec         *nearnullvecs;
  Vec               *localnearnullsp;
  PetscBool         nnsp_has_cnst;
  PetscInt          nnsp_size;
  PetscScalar       *array;
  /* BLAS integers */
  PetscBLASInt      lwork,lierr;
  PetscBLASInt      Blas_N,Blas_M,Blas_K,Blas_one=1;
  PetscBLASInt      Blas_LDA,Blas_LDB,Blas_LDC;
  /* LAPACK working arrays for SVD or POD */
  PetscBool         skip_lapack;
  PetscScalar       *work;
  PetscReal         *singular_vals;
#if defined(PETSC_USE_COMPLEX)
  PetscReal         *rwork;
#endif
#if defined(PETSC_MISSING_LAPACK_GESVD)
  PetscBLASInt      Blas_one_2=1;
  PetscScalar       *temp_basis,*correlation_mat;
#else
  PetscBLASInt      dummy_int_1=1,dummy_int_2=1;
  PetscScalar       dummy_scalar_1=0.0,dummy_scalar_2=0.0;
#endif
  /* reuse */
  PetscInt          olocal_primal_size;
  PetscInt          *oprimal_indices_local_idxs;
  /* change of basis */
  PetscInt          *aux_primal_numbering,*aux_primal_minloc,*global_indices;
  PetscBool         boolforchange,qr_needed;
  PetscBT           touched,change_basis,qr_needed_idx;
  /* auxiliary stuff */
  PetscInt          *nnz,*is_indices,*aux_primal_numbering_B;
  /* some quantities */
  PetscInt          n_vertices,total_primal_vertices,valid_constraints;
  PetscInt          size_of_constraint,max_size_of_constraint,max_constraints,temp_constraints;


  PetscFunctionBegin;
  /* Destroy Mat objects computed previously */
  ierr = MatDestroy(&pcbddc->ChangeOfBasisMatrix);CHKERRQ(ierr);
  ierr = MatDestroy(&pcbddc->ConstraintMatrix);CHKERRQ(ierr);
  /* Get index sets for faces, edges and vertices from graph */
  if (!pcbddc->use_faces && !pcbddc->use_edges && !pcbddc->use_vertices) {
    pcbddc->use_vertices = PETSC_TRUE;
  }
  ierr = PCBDDCGraphGetCandidatesIS(pcbddc->mat_graph,pcbddc->use_faces,pcbddc->use_edges,pcbddc->use_vertices,&n_ISForFaces,&ISForFaces,&n_ISForEdges,&ISForEdges,&ISForVertices);
  /* HACKS (the two following code branches) */
  if (!ISForVertices && pcbddc->NullSpace && !pcbddc->user_ChangeOfBasisMatrix) {
    pcbddc->use_change_of_basis = PETSC_TRUE;
    pcbddc->use_change_on_faces = PETSC_FALSE;
  }
  if (pcbddc->NullSpace) {
    /* use_change_of_basis should be consistent among processors */
    PetscBool tbool = pcbddc->use_change_of_basis;
    ierr = MPI_Allreduce(&tbool,&(pcbddc->use_change_of_basis),1,MPIU_BOOL,MPI_LOR,PetscObjectComm((PetscObject)pc));CHKERRQ(ierr);
  }
  /* print some info */
  if (pcbddc->dbg_flag) {
    ierr = PetscViewerASCIISynchronizedAllow(pcbddc->dbg_viewer,PETSC_TRUE);CHKERRQ(ierr);
    ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"--------------------------------------------------------------\n");CHKERRQ(ierr);
    i = 0;
    if (ISForVertices) {
      ierr = ISGetSize(ISForVertices,&i);CHKERRQ(ierr);
    }
    ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"Subdomain %04d got %02d local candidate vertices\n",PetscGlobalRank,i);CHKERRQ(ierr);
    ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"Subdomain %04d got %02d local candidate edges\n",PetscGlobalRank,n_ISForEdges);CHKERRQ(ierr);
    ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"Subdomain %04d got %02d local candidate faces\n",PetscGlobalRank,n_ISForFaces);CHKERRQ(ierr);
    ierr = PetscViewerFlush(pcbddc->dbg_viewer);CHKERRQ(ierr);
  }
  /* check if near null space is attached to global mat */
  ierr = MatGetNearNullSpace(pc->pmat,&nearnullsp);CHKERRQ(ierr);
  if (nearnullsp) {
    ierr = MatNullSpaceGetVecs(nearnullsp,&nnsp_has_cnst,&nnsp_size,&nearnullvecs);CHKERRQ(ierr);
    /* remove any stored info */
    ierr = MatNullSpaceDestroy(&pcbddc->onearnullspace);CHKERRQ(ierr);
    ierr = PetscFree(pcbddc->onearnullvecs_state);CHKERRQ(ierr);
    /* store information for BDDC solver reuse */
    ierr = PetscObjectReference((PetscObject)nearnullsp);CHKERRQ(ierr);
    pcbddc->onearnullspace = nearnullsp;
    ierr = PetscMalloc1(nnsp_size,&pcbddc->onearnullvecs_state);CHKERRQ(ierr);
    for (i=0;i<nnsp_size;i++) {
      ierr = PetscObjectStateGet((PetscObject)nearnullvecs[i],&pcbddc->onearnullvecs_state[i]);CHKERRQ(ierr);
    }
  } else { /* if near null space is not provided BDDC uses constants by default */
    nnsp_size = 0;
    nnsp_has_cnst = PETSC_TRUE;
  }
  /* get max number of constraints on a single cc */
  max_constraints = nnsp_size;
  if (nnsp_has_cnst) max_constraints++;

  /*
       Evaluate maximum storage size needed by the procedure
       - temp_indices will contain start index of each constraint stored as follows
       - temp_indices_to_constraint  [temp_indices[i],...,temp[indices[i+1]-1] will contain the indices (in local numbering) on which the constraint acts
       - temp_indices_to_constraint_B[temp_indices[i],...,temp[indices[i+1]-1] will contain the indices (in boundary numbering) on which the constraint acts
       - temp_quadrature_constraint  [temp_indices[i],...,temp[indices[i+1]-1] will contain the scalars representing the constraint itself
                                                                                                                                                         */
  total_counts = n_ISForFaces+n_ISForEdges;
  total_counts *= max_constraints;
  n_vertices = 0;
  if (ISForVertices) {
    ierr = ISGetSize(ISForVertices,&n_vertices);CHKERRQ(ierr);
  }
  total_counts += n_vertices;
  ierr = PetscMalloc1((total_counts+1),&temp_indices);CHKERRQ(ierr);
  ierr = PetscBTCreate(total_counts,&change_basis);CHKERRQ(ierr);
  total_counts = 0;
  max_size_of_constraint = 0;
  for (i=0;i<n_ISForEdges+n_ISForFaces;i++) {
    if (i<n_ISForEdges) {
      used_IS = &ISForEdges[i];
    } else {
      used_IS = &ISForFaces[i-n_ISForEdges];
    }
    ierr = ISGetSize(*used_IS,&j);CHKERRQ(ierr);
    total_counts += j;
    max_size_of_constraint = PetscMax(j,max_size_of_constraint);
  }
  total_counts *= max_constraints;
  total_counts += n_vertices;
  ierr = PetscMalloc1(total_counts,&temp_quadrature_constraint);CHKERRQ(ierr);
  ierr = PetscMalloc1(total_counts,&temp_indices_to_constraint);CHKERRQ(ierr);
  ierr = PetscMalloc1(total_counts,&temp_indices_to_constraint_B);CHKERRQ(ierr);
  /* get local part of global near null space vectors */
  ierr = PetscMalloc1(nnsp_size,&localnearnullsp);CHKERRQ(ierr);
  for (k=0;k<nnsp_size;k++) {
    ierr = VecDuplicate(pcis->vec1_N,&localnearnullsp[k]);CHKERRQ(ierr);
    ierr = VecScatterBegin(matis->ctx,nearnullvecs[k],localnearnullsp[k],INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(matis->ctx,nearnullvecs[k],localnearnullsp[k],INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  }

  /* whether or not to skip lapack calls */
  skip_lapack = PETSC_TRUE;
  if (n_ISForFaces+n_ISForEdges) skip_lapack = PETSC_FALSE;

  /* First we issue queries to allocate optimal workspace for LAPACKgesvd (or LAPACKsyev if SVD is missing) */
  if (!pcbddc->use_nnsp_true && !skip_lapack) {
    PetscScalar temp_work;
#if defined(PETSC_MISSING_LAPACK_GESVD)
    /* Proper Orthogonal Decomposition (POD) using the snapshot method */
    ierr = PetscMalloc1(max_constraints*max_constraints,&correlation_mat);CHKERRQ(ierr);
    ierr = PetscMalloc1(max_constraints,&singular_vals);CHKERRQ(ierr);
    ierr = PetscMalloc1(max_size_of_constraint*max_constraints,&temp_basis);CHKERRQ(ierr);
#if defined(PETSC_USE_COMPLEX)
    ierr = PetscMalloc1(3*max_constraints,&rwork);CHKERRQ(ierr);
#endif
    /* now we evaluate the optimal workspace using query with lwork=-1 */
    ierr = PetscBLASIntCast(max_constraints,&Blas_N);CHKERRQ(ierr);
    ierr = PetscBLASIntCast(max_constraints,&Blas_LDA);CHKERRQ(ierr);
    lwork = -1;
    ierr = PetscFPTrapPush(PETSC_FP_TRAP_OFF);CHKERRQ(ierr);
#if !defined(PETSC_USE_COMPLEX)
    PetscStackCallBLAS("LAPACKsyev",LAPACKsyev_("V","U",&Blas_N,correlation_mat,&Blas_LDA,singular_vals,&temp_work,&lwork,&lierr));
#else
    PetscStackCallBLAS("LAPACKsyev",LAPACKsyev_("V","U",&Blas_N,correlation_mat,&Blas_LDA,singular_vals,&temp_work,&lwork,rwork,&lierr));
#endif
    ierr = PetscFPTrapPop();CHKERRQ(ierr);
    if (lierr) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error in query to SYEV Lapack routine %d",(int)lierr);
#else /* on missing GESVD */
    /* SVD */
    PetscInt max_n,min_n;
    max_n = max_size_of_constraint;
    min_n = max_constraints;
    if (max_size_of_constraint < max_constraints) {
      min_n = max_size_of_constraint;
      max_n = max_constraints;
    }
    ierr = PetscMalloc1(min_n,&singular_vals);CHKERRQ(ierr);
#if defined(PETSC_USE_COMPLEX)
    ierr = PetscMalloc1(5*min_n,&rwork);CHKERRQ(ierr);
#endif
    /* now we evaluate the optimal workspace using query with lwork=-1 */
    lwork = -1;
    ierr = PetscBLASIntCast(max_n,&Blas_M);CHKERRQ(ierr);
    ierr = PetscBLASIntCast(min_n,&Blas_N);CHKERRQ(ierr);
    ierr = PetscBLASIntCast(max_n,&Blas_LDA);CHKERRQ(ierr);
    ierr = PetscFPTrapPush(PETSC_FP_TRAP_OFF);CHKERRQ(ierr);
#if !defined(PETSC_USE_COMPLEX)
    PetscStackCallBLAS("LAPACKgesvd",LAPACKgesvd_("O","N",&Blas_M,&Blas_N,&temp_quadrature_constraint[0],&Blas_LDA,singular_vals,&dummy_scalar_1,&dummy_int_1,&dummy_scalar_2,&dummy_int_2,&temp_work,&lwork,&lierr));
#else
    PetscStackCallBLAS("LAPACKgesvd",LAPACKgesvd_("O","N",&Blas_M,&Blas_N,&temp_quadrature_constraint[0],&Blas_LDA,singular_vals,&dummy_scalar_1,&dummy_int_1,&dummy_scalar_2,&dummy_int_2,&temp_work,&lwork,rwork,&lierr));
#endif
    ierr = PetscFPTrapPop();CHKERRQ(ierr);
    if (lierr) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error in query to GESVD Lapack routine %d",(int)lierr);
#endif /* on missing GESVD */
    /* Allocate optimal workspace */
    ierr = PetscBLASIntCast((PetscInt)PetscRealPart(temp_work),&lwork);CHKERRQ(ierr);
    ierr = PetscMalloc1((PetscInt)lwork,&work);CHKERRQ(ierr);
  }
  /* Now we can loop on constraining sets */
  total_counts = 0;
  temp_indices[0] = 0;
  /* vertices */
  if (ISForVertices) {
    ierr = ISGetIndices(ISForVertices,(const PetscInt**)&is_indices);CHKERRQ(ierr);
    if (nnsp_has_cnst) { /* consider all vertices */
      ierr = PetscMemcpy(&temp_indices_to_constraint[temp_indices[total_counts]],is_indices,n_vertices*sizeof(PetscInt));CHKERRQ(ierr);
      for (i=0;i<n_vertices;i++) {
        temp_quadrature_constraint[temp_indices[total_counts]]=1.0;
        temp_indices[total_counts+1]=temp_indices[total_counts]+1;
        total_counts++;
      }
    } else { /* consider vertices for which exist at least a localnearnullsp which is not null there */
      PetscBool used_vertex;
      for (i=0;i<n_vertices;i++) {
        used_vertex = PETSC_FALSE;
        k = 0;
        while (!used_vertex && k<nnsp_size) {
          ierr = VecGetArrayRead(localnearnullsp[k],(const PetscScalar**)&array);CHKERRQ(ierr);
          if (PetscAbsScalar(array[is_indices[i]])>0.0) {
            temp_indices_to_constraint[temp_indices[total_counts]]=is_indices[i];
            temp_quadrature_constraint[temp_indices[total_counts]]=1.0;
            temp_indices[total_counts+1]=temp_indices[total_counts]+1;
            total_counts++;
            used_vertex = PETSC_TRUE;
          }
          ierr = VecRestoreArrayRead(localnearnullsp[k],(const PetscScalar**)&array);CHKERRQ(ierr);
          k++;
        }
      }
    }
    ierr = ISRestoreIndices(ISForVertices,(const PetscInt**)&is_indices);CHKERRQ(ierr);
    n_vertices = total_counts;
  }

  /* edges and faces */
  for (i=0;i<n_ISForEdges+n_ISForFaces;i++) {
    if (i<n_ISForEdges) {
      used_IS = &ISForEdges[i];
      boolforchange = pcbddc->use_change_of_basis; /* change or not the basis on the edge */
    } else {
      used_IS = &ISForFaces[i-n_ISForEdges];
      boolforchange = (PetscBool)(pcbddc->use_change_of_basis && pcbddc->use_change_on_faces); /* change or not the basis on the face */
    }
    temp_constraints = 0;          /* zero the number of constraints I have on this conn comp */
    temp_start_ptr = total_counts; /* need to know the starting index of constraints stored */
    ierr = ISGetSize(*used_IS,&size_of_constraint);CHKERRQ(ierr);
    ierr = ISGetIndices(*used_IS,(const PetscInt**)&is_indices);CHKERRQ(ierr);
    /* change of basis should not be performed on local periodic nodes */
    if (pcbddc->mat_graph->mirrors && pcbddc->mat_graph->mirrors[is_indices[0]]) boolforchange = PETSC_FALSE;
    if (nnsp_has_cnst) {
      PetscScalar quad_value;
      temp_constraints++;
      quad_value = (PetscScalar)(1.0/PetscSqrtReal((PetscReal)size_of_constraint));
      ierr = PetscMemcpy(&temp_indices_to_constraint[temp_indices[total_counts]],is_indices,size_of_constraint*sizeof(PetscInt));CHKERRQ(ierr);
      for (j=0;j<size_of_constraint;j++) {
        temp_quadrature_constraint[temp_indices[total_counts]+j]=quad_value;
      }
      temp_indices[total_counts+1]=temp_indices[total_counts]+size_of_constraint;  /* store new starting point */
      total_counts++;
    }
    for (k=0;k<nnsp_size;k++) {
      PetscReal real_value;
      ierr = VecGetArrayRead(localnearnullsp[k],(const PetscScalar**)&array);CHKERRQ(ierr);
      ierr = PetscMemcpy(&temp_indices_to_constraint[temp_indices[total_counts]],is_indices,size_of_constraint*sizeof(PetscInt));CHKERRQ(ierr);
      for (j=0;j<size_of_constraint;j++) {
        temp_quadrature_constraint[temp_indices[total_counts]+j]=array[is_indices[j]];
      }
      ierr = VecRestoreArrayRead(localnearnullsp[k],(const PetscScalar**)&array);CHKERRQ(ierr);
      /* check if array is null on the connected component */
      ierr = PetscBLASIntCast(size_of_constraint,&Blas_N);CHKERRQ(ierr);
      PetscStackCallBLAS("BLASasum",real_value = BLASasum_(&Blas_N,&temp_quadrature_constraint[temp_indices[total_counts]],&Blas_one));
      if (real_value > 0.0) { /* keep indices and values */
        temp_constraints++;
        temp_indices[total_counts+1]=temp_indices[total_counts]+size_of_constraint;  /* store new starting point */
        total_counts++;
      }
    }
    ierr = ISRestoreIndices(*used_IS,(const PetscInt**)&is_indices);CHKERRQ(ierr);
    valid_constraints = temp_constraints;
    /* perform SVD on the constraints if use_nnsp_true has not be requested by the user and there are non-null constraints on the cc */
    if (!pcbddc->use_nnsp_true && temp_constraints) {
      PetscReal tol = 1.0e-8; /* tolerance for retaining eigenmodes */

#if defined(PETSC_MISSING_LAPACK_GESVD)
      /* SVD: Y = U*S*V^H                -> U (eigenvectors of Y*Y^H) = Y*V*(S)^\dag
         POD: Y^H*Y = V*D*V^H, D = S^H*S -> U = Y*V*D^(-1/2)
         -> When PETSC_USE_COMPLEX and PETSC_MISSING_LAPACK_GESVD are defined
            the constraints basis will differ (by a complex factor with absolute value equal to 1)
            from that computed using LAPACKgesvd
         -> This is due to a different computation of eigenvectors in LAPACKheev
         -> The quality of the POD-computed basis will be the same */
      ierr = PetscMemzero(correlation_mat,temp_constraints*temp_constraints*sizeof(PetscScalar));CHKERRQ(ierr);
      /* Store upper triangular part of correlation matrix */
      ierr = PetscBLASIntCast(size_of_constraint,&Blas_N);CHKERRQ(ierr);
      ierr = PetscFPTrapPush(PETSC_FP_TRAP_OFF);CHKERRQ(ierr);
      for (j=0;j<temp_constraints;j++) {
        for (k=0;k<j+1;k++) {
          PetscStackCallBLAS("BLASdot",correlation_mat[j*temp_constraints+k]=BLASdot_(&Blas_N,&temp_quadrature_constraint[temp_indices[temp_start_ptr+k]],&Blas_one,&temp_quadrature_constraint[temp_indices[temp_start_ptr+j]],&Blas_one_2));
        }
      }
      /* compute eigenvalues and eigenvectors of correlation matrix */
      ierr = PetscBLASIntCast(temp_constraints,&Blas_N);CHKERRQ(ierr);
      ierr = PetscBLASIntCast(temp_constraints,&Blas_LDA);CHKERRQ(ierr);
#if !defined(PETSC_USE_COMPLEX)
      PetscStackCallBLAS("LAPACKsyev",LAPACKsyev_("V","U",&Blas_N,correlation_mat,&Blas_LDA,singular_vals,work,&lwork,&lierr));
#else
      PetscStackCallBLAS("LAPACKsyev",LAPACKsyev_("V","U",&Blas_N,correlation_mat,&Blas_LDA,singular_vals,work,&lwork,rwork,&lierr));
#endif
      ierr = PetscFPTrapPop();CHKERRQ(ierr);
      if (lierr) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error in SYEV Lapack routine %d",(int)lierr);
      /* retain eigenvalues greater than tol: note that LAPACKsyev gives eigs in ascending order */
      j=0;
      while (j < temp_constraints && singular_vals[j] < tol) j++;
      total_counts=total_counts-j;
      valid_constraints = temp_constraints-j;
      /* scale and copy POD basis into used quadrature memory */
      ierr = PetscBLASIntCast(size_of_constraint,&Blas_M);CHKERRQ(ierr);
      ierr = PetscBLASIntCast(temp_constraints,&Blas_N);CHKERRQ(ierr);
      ierr = PetscBLASIntCast(temp_constraints,&Blas_K);CHKERRQ(ierr);
      ierr = PetscBLASIntCast(size_of_constraint,&Blas_LDA);CHKERRQ(ierr);
      ierr = PetscBLASIntCast(temp_constraints,&Blas_LDB);CHKERRQ(ierr);
      ierr = PetscBLASIntCast(size_of_constraint,&Blas_LDC);CHKERRQ(ierr);
      if (j<temp_constraints) {
        PetscInt ii;
        for (k=j;k<temp_constraints;k++) singular_vals[k]=1.0/PetscSqrtReal(singular_vals[k]);
        ierr = PetscFPTrapPush(PETSC_FP_TRAP_OFF);CHKERRQ(ierr);
        PetscStackCallBLAS("BLASgemm",BLASgemm_("N","N",&Blas_M,&Blas_N,&Blas_K,&one,&temp_quadrature_constraint[temp_indices[temp_start_ptr]],&Blas_LDA,correlation_mat,&Blas_LDB,&zero,temp_basis,&Blas_LDC));
        ierr = PetscFPTrapPop();CHKERRQ(ierr);
        for (k=0;k<temp_constraints-j;k++) {
          for (ii=0;ii<size_of_constraint;ii++) {
            temp_quadrature_constraint[temp_indices[temp_start_ptr+k]+ii]=singular_vals[temp_constraints-1-k]*temp_basis[(temp_constraints-1-k)*size_of_constraint+ii];
          }
        }
      }
#else  /* on missing GESVD */
      ierr = PetscBLASIntCast(size_of_constraint,&Blas_M);CHKERRQ(ierr);
      ierr = PetscBLASIntCast(temp_constraints,&Blas_N);CHKERRQ(ierr);
      ierr = PetscBLASIntCast(size_of_constraint,&Blas_LDA);CHKERRQ(ierr);
      ierr = PetscFPTrapPush(PETSC_FP_TRAP_OFF);CHKERRQ(ierr);
#if !defined(PETSC_USE_COMPLEX)
      PetscStackCallBLAS("LAPACKgesvd",LAPACKgesvd_("O","N",&Blas_M,&Blas_N,&temp_quadrature_constraint[temp_indices[temp_start_ptr]],&Blas_LDA,singular_vals,&dummy_scalar_1,&dummy_int_1,&dummy_scalar_2,&dummy_int_2,work,&lwork,&lierr));
#else
      PetscStackCallBLAS("LAPACKgesvd",LAPACKgesvd_("O","N",&Blas_M,&Blas_N,&temp_quadrature_constraint[temp_indices[temp_start_ptr]],&Blas_LDA,singular_vals,&dummy_scalar_1,&dummy_int_1,&dummy_scalar_2,&dummy_int_2,work,&lwork,rwork,&lierr));
#endif
      if (lierr) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error in GESVD Lapack routine %d",(int)lierr);
      ierr = PetscFPTrapPop();CHKERRQ(ierr);
      /* retain eigenvalues greater than tol: note that LAPACKgesvd gives eigs in descending order */
      k = temp_constraints;
      if (k > size_of_constraint) k = size_of_constraint;
      j = 0;
      while (j < k && singular_vals[k-j-1] < tol) j++;
      total_counts = total_counts-temp_constraints+k-j;
      valid_constraints = k-j;
#endif /* on missing GESVD */
    }
    /* setting change_of_basis flag is safe now */
    if (boolforchange) {
      for (j=0;j<valid_constraints;j++) {
        PetscBTSet(change_basis,total_counts-j-1);
      }
    }
  }
  /* free index sets of faces, edges and vertices */
  for (i=0;i<n_ISForFaces;i++) {
    ierr = ISDestroy(&ISForFaces[i]);CHKERRQ(ierr);
  }
  ierr = PetscFree(ISForFaces);CHKERRQ(ierr);
  for (i=0;i<n_ISForEdges;i++) {
    ierr = ISDestroy(&ISForEdges[i]);CHKERRQ(ierr);
  }
  ierr = PetscFree(ISForEdges);CHKERRQ(ierr);
  ierr = ISDestroy(&ISForVertices);CHKERRQ(ierr);
  /* map temp_indices_to_constraint in boundary numbering */
  ierr = ISGlobalToLocalMappingApply(pcbddc->BtoNmap,IS_GTOLM_DROP,temp_indices[total_counts],temp_indices_to_constraint,&i,temp_indices_to_constraint_B);CHKERRQ(ierr);
  if (i != temp_indices[total_counts]) {
    SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_SUP,"Error in boundary numbering for constraints indices %d != %d\n",temp_indices[total_counts],i);
  }

  /* free workspace */
  if (!pcbddc->use_nnsp_true && !skip_lapack) {
    ierr = PetscFree(work);CHKERRQ(ierr);
#if defined(PETSC_USE_COMPLEX)
    ierr = PetscFree(rwork);CHKERRQ(ierr);
#endif
    ierr = PetscFree(singular_vals);CHKERRQ(ierr);
#if defined(PETSC_MISSING_LAPACK_GESVD)
    ierr = PetscFree(correlation_mat);CHKERRQ(ierr);
    ierr = PetscFree(temp_basis);CHKERRQ(ierr);
#endif
  }
  for (k=0;k<nnsp_size;k++) {
    ierr = VecDestroy(&localnearnullsp[k]);CHKERRQ(ierr);
  }
  ierr = PetscFree(localnearnullsp);CHKERRQ(ierr);

  /* set quantities in pcbddc data structure and store previous primal size */
  /* n_vertices defines the number of subdomain corners in the primal space */
  /* n_constraints defines the number of averages (they can be point primal dofs if change of basis is requested) */
  olocal_primal_size = pcbddc->local_primal_size;
  pcbddc->local_primal_size = total_counts;
  pcbddc->n_vertices = n_vertices;
  pcbddc->n_constraints = pcbddc->local_primal_size-pcbddc->n_vertices;

  /* Create constraint matrix */
  /* The constraint matrix is used to compute the l2g map of primal dofs */
  /* so we need to set it up properly either with or without change of basis */
  ierr = MatCreate(PETSC_COMM_SELF,&pcbddc->ConstraintMatrix);CHKERRQ(ierr);
  ierr = MatSetType(pcbddc->ConstraintMatrix,impMatType);CHKERRQ(ierr);
  ierr = MatSetSizes(pcbddc->ConstraintMatrix,pcbddc->local_primal_size,pcis->n,pcbddc->local_primal_size,pcis->n);CHKERRQ(ierr);
  /* array to compute a local numbering of constraints : vertices first then constraints */
  ierr = PetscMalloc1(pcbddc->local_primal_size,&aux_primal_numbering);CHKERRQ(ierr);
  /* array to select the proper local node (of minimum index with respect to global ordering) when changing the basis */
  /* note: it should not be needed since IS for faces and edges are already sorted by global ordering when analyzing the graph but... just in case */
  ierr = PetscMalloc1(pcbddc->local_primal_size,&aux_primal_minloc);CHKERRQ(ierr);
  /* auxiliary stuff for basis change */
  ierr = PetscMalloc1(max_size_of_constraint,&global_indices);CHKERRQ(ierr);
  ierr = PetscBTCreate(pcis->n_B,&touched);CHKERRQ(ierr);

  /* find primal_dofs: subdomain corners plus dofs selected as primal after change of basis */
  total_primal_vertices=0;
  for (i=0;i<pcbddc->local_primal_size;i++) {
    size_of_constraint=temp_indices[i+1]-temp_indices[i];
    if (size_of_constraint == 1) {
      ierr = PetscBTSet(touched,temp_indices_to_constraint_B[temp_indices[i]]);CHKERRQ(ierr);
      aux_primal_numbering[total_primal_vertices]=temp_indices_to_constraint[temp_indices[i]];
      aux_primal_minloc[total_primal_vertices]=0;
      total_primal_vertices++;
    } else if (PetscBTLookup(change_basis,i)) { /* Same procedure used in PCBDDCGetPrimalConstraintsLocalIdx */
      PetscInt min_loc,min_index;
      ierr = ISLocalToGlobalMappingApply(pcbddc->mat_graph->l2gmap,size_of_constraint,&temp_indices_to_constraint[temp_indices[i]],global_indices);CHKERRQ(ierr);
      /* find first untouched local node */
      k = 0;
      while (PetscBTLookup(touched,temp_indices_to_constraint_B[temp_indices[i]+k])) k++;
      min_index = global_indices[k];
      min_loc = k;
      /* search the minimum among global nodes already untouched on the cc */
      for (k=1;k<size_of_constraint;k++) {
        /* there can be more than one constraint on a single connected component */
        if (!PetscBTLookup(touched,temp_indices_to_constraint_B[temp_indices[i]+k]) && min_index > global_indices[k]) {
          min_index = global_indices[k];
          min_loc = k;
        }
      }
      ierr = PetscBTSet(touched,temp_indices_to_constraint_B[temp_indices[i]+min_loc]);CHKERRQ(ierr);
      aux_primal_numbering[total_primal_vertices]=temp_indices_to_constraint[temp_indices[i]+min_loc];
      aux_primal_minloc[total_primal_vertices]=min_loc;
      total_primal_vertices++;
    }
  }
  /* determine if a QR strategy is needed for change of basis */
  qr_needed = PETSC_FALSE;
  ierr = PetscBTCreate(pcbddc->local_primal_size,&qr_needed_idx);CHKERRQ(ierr);
  for (i=pcbddc->n_vertices;i<pcbddc->local_primal_size;i++) {
    if (PetscBTLookup(change_basis,i)) {
      size_of_constraint = temp_indices[i+1]-temp_indices[i];
      j = 0;
      for (k=0;k<size_of_constraint;k++) {
        if (PetscBTLookup(touched,temp_indices_to_constraint_B[temp_indices[i]+k])) {
          j++;
        }
      }
      /* found more than one primal dof on the cc */
      if (j > 1) {
        PetscBTSet(qr_needed_idx,i);
        qr_needed = PETSC_TRUE;
      }
    }
  }
  /* free workspace */
  ierr = PetscFree(global_indices);CHKERRQ(ierr);

  /* permute indices in order to have a sorted set of vertices */
  ierr = PetscSortInt(total_primal_vertices,aux_primal_numbering);CHKERRQ(ierr);

  /* nonzero structure of constraint matrix */
  ierr = PetscMalloc1(pcbddc->local_primal_size,&nnz);CHKERRQ(ierr);
  for (i=0;i<total_primal_vertices;i++) nnz[i]=1;
  j=total_primal_vertices;
  for (i=pcbddc->n_vertices;i<pcbddc->local_primal_size;i++) {
    if (!PetscBTLookup(change_basis,i)) {
      nnz[j]=temp_indices[i+1]-temp_indices[i];
      j++;
    }
  }
  ierr = MatSeqAIJSetPreallocation(pcbddc->ConstraintMatrix,0,nnz);CHKERRQ(ierr);
  ierr = PetscFree(nnz);CHKERRQ(ierr);
  /* set values in constraint matrix */
  for (i=0;i<total_primal_vertices;i++) {
    ierr = MatSetValue(pcbddc->ConstraintMatrix,i,aux_primal_numbering[i],1.0,INSERT_VALUES);CHKERRQ(ierr);
  }
  total_counts = total_primal_vertices;
  for (i=pcbddc->n_vertices;i<pcbddc->local_primal_size;i++) {
    if (!PetscBTLookup(change_basis,i)) {
      size_of_constraint=temp_indices[i+1]-temp_indices[i];
      ierr = MatSetValues(pcbddc->ConstraintMatrix,1,&total_counts,size_of_constraint,&temp_indices_to_constraint[temp_indices[i]],&temp_quadrature_constraint[temp_indices[i]],INSERT_VALUES);CHKERRQ(ierr);
      total_counts++;
    }
  }
  /* assembling */
  ierr = MatAssemblyBegin(pcbddc->ConstraintMatrix,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(pcbddc->ConstraintMatrix,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  /*
  ierr = PetscViewerSetFormat(PETSC_VIEWER_STDOUT_SELF,PETSC_VIEWER_ASCII_MATLAB);CHKERRQ(ierr);
  ierr = MatView(pcbddc->ConstraintMatrix,(PetscViewer)0);CHKERRQ(ierr);
  */
  /* Create matrix for change of basis. We don't need it in case pcbddc->use_change_of_basis is FALSE */
  if (pcbddc->use_change_of_basis) {
    /* dual and primal dofs on a single cc */
    PetscInt     dual_dofs,primal_dofs;
    /* iterator on aux_primal_minloc (ordered as read from nearnullspace: vertices, edges and then constraints) */
    PetscInt     primal_counter;
    /* working stuff for GEQRF */
    PetscScalar  *qr_basis,*qr_tau = NULL,*qr_work,lqr_work_t;
    PetscBLASInt lqr_work;
    /* working stuff for UNGQR */
    PetscScalar  *gqr_work,lgqr_work_t;
    PetscBLASInt lgqr_work;
    /* working stuff for TRTRS */
    PetscScalar  *trs_rhs;
    PetscBLASInt Blas_NRHS;
    /* pointers for values insertion into change of basis matrix */
    PetscInt     *start_rows,*start_cols;
    PetscScalar  *start_vals;
    /* working stuff for values insertion */
    PetscBT      is_primal;

    /* change of basis acts on local interfaces -> dimension is n_B x n_B */
    ierr = MatCreate(PETSC_COMM_SELF,&pcbddc->ChangeOfBasisMatrix);CHKERRQ(ierr);
    ierr = MatSetType(pcbddc->ChangeOfBasisMatrix,impMatType);CHKERRQ(ierr);
    ierr = MatSetSizes(pcbddc->ChangeOfBasisMatrix,pcis->n_B,pcis->n_B,pcis->n_B,pcis->n_B);CHKERRQ(ierr);
    /* work arrays */
    ierr = PetscMalloc1(pcis->n_B,&nnz);CHKERRQ(ierr);
    for (i=0;i<pcis->n_B;i++) nnz[i]=1;
    /* nonzeros per row */
    for (i=pcbddc->n_vertices;i<pcbddc->local_primal_size;i++) {
      if (PetscBTLookup(change_basis,i)) {
        size_of_constraint = temp_indices[i+1]-temp_indices[i];
        if (PetscBTLookup(qr_needed_idx,i)) {
          for (j=0;j<size_of_constraint;j++) nnz[temp_indices_to_constraint_B[temp_indices[i]+j]] = size_of_constraint;
        } else {
          for (j=0;j<size_of_constraint;j++) nnz[temp_indices_to_constraint_B[temp_indices[i]+j]] = 2;
          /* get local primal index on the cc */
          j = 0;
          while (!PetscBTLookup(touched,temp_indices_to_constraint_B[temp_indices[i]+j])) j++;
          nnz[temp_indices_to_constraint_B[temp_indices[i]+j]] = size_of_constraint;
        }
      }
    }
    ierr = MatSeqAIJSetPreallocation(pcbddc->ChangeOfBasisMatrix,0,nnz);CHKERRQ(ierr);
    ierr = PetscFree(nnz);CHKERRQ(ierr);
    /* Set initial identity in the matrix */
    for (i=0;i<pcis->n_B;i++) {
      ierr = MatSetValue(pcbddc->ChangeOfBasisMatrix,i,i,1.0,INSERT_VALUES);CHKERRQ(ierr);
    }

    if (pcbddc->dbg_flag) {
      ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"--------------------------------------------------------------\n");CHKERRQ(ierr);
      ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"Checking change of basis computation for subdomain %04d\n",PetscGlobalRank);CHKERRQ(ierr);
    }


    /* Now we loop on the constraints which need a change of basis */
    /*
       Change of basis matrix is evaluated similarly to the FIRST APPROACH in
       Klawonn and Widlund, Dual-primal FETI-DP methods for linear elasticity, (see Sect 6.2.1)

       Basic blocks of change of basis matrix T computed

          - Using the following block transformation if there is only a primal dof on the cc
            (in the example, primal dof is the last one of the edge in LOCAL ordering
             in this code, primal dof is the first one of the edge in GLOBAL ordering)
            | 1        0   ...        0     1 |
            | 0        1   ...        0     1 |
            |              ...                |
            | 0        ...            1     1 |
            | -s_1/s_n ...    -s_{n-1}/-s_n 1 |

          - via QR decomposition of constraints otherwise
    */
    if (qr_needed) {
      /* space to store Q */
      ierr = PetscMalloc1((max_size_of_constraint)*(max_size_of_constraint),&qr_basis);CHKERRQ(ierr);
      /* first we issue queries for optimal work */
      ierr = PetscBLASIntCast(max_size_of_constraint,&Blas_M);CHKERRQ(ierr);
      ierr = PetscBLASIntCast(max_constraints,&Blas_N);CHKERRQ(ierr);
      ierr = PetscBLASIntCast(max_size_of_constraint,&Blas_LDA);CHKERRQ(ierr);
      lqr_work = -1;
      PetscStackCallBLAS("LAPACKgeqrf",LAPACKgeqrf_(&Blas_M,&Blas_N,qr_basis,&Blas_LDA,qr_tau,&lqr_work_t,&lqr_work,&lierr));
      if (lierr) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error in query to GEQRF Lapack routine %d",(int)lierr);
      ierr = PetscBLASIntCast((PetscInt)PetscRealPart(lqr_work_t),&lqr_work);CHKERRQ(ierr);
      ierr = PetscMalloc1((PetscInt)PetscRealPart(lqr_work_t),&qr_work);CHKERRQ(ierr);
      lgqr_work = -1;
      ierr = PetscBLASIntCast(max_size_of_constraint,&Blas_M);CHKERRQ(ierr);
      ierr = PetscBLASIntCast(max_size_of_constraint,&Blas_N);CHKERRQ(ierr);
      ierr = PetscBLASIntCast(max_constraints,&Blas_K);CHKERRQ(ierr);
      ierr = PetscBLASIntCast(max_size_of_constraint,&Blas_LDA);CHKERRQ(ierr);
      if (Blas_K>Blas_M) Blas_K=Blas_M; /* adjust just for computing optimal work */
      PetscStackCallBLAS("LAPACKungqr",LAPACKungqr_(&Blas_M,&Blas_N,&Blas_K,qr_basis,&Blas_LDA,qr_tau,&lgqr_work_t,&lgqr_work,&lierr));
      if (lierr) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error in query to UNGQR Lapack routine %d",(int)lierr);
      ierr = PetscBLASIntCast((PetscInt)PetscRealPart(lgqr_work_t),&lgqr_work);CHKERRQ(ierr);
      ierr = PetscMalloc1((PetscInt)PetscRealPart(lgqr_work_t),&gqr_work);CHKERRQ(ierr);
      /* array to store scaling factors for reflectors */
      ierr = PetscMalloc1(max_constraints,&qr_tau);CHKERRQ(ierr);
      /* array to store rhs and solution of triangular solver */
      ierr = PetscMalloc1(max_constraints*max_constraints,&trs_rhs);CHKERRQ(ierr);
      /* allocating workspace for check */
      if (pcbddc->dbg_flag) {
        ierr = PetscMalloc1(max_size_of_constraint*(max_constraints+max_size_of_constraint),&work);CHKERRQ(ierr);
      }
    }
    /* array to store whether a node is primal or not */
    ierr = PetscBTCreate(pcis->n_B,&is_primal);CHKERRQ(ierr);
    ierr = PetscMalloc1(total_primal_vertices,&aux_primal_numbering_B);CHKERRQ(ierr);
    ierr = ISGlobalToLocalMappingApply(pcbddc->BtoNmap,IS_GTOLM_DROP,total_primal_vertices,aux_primal_numbering,&i,aux_primal_numbering_B);CHKERRQ(ierr);
    if (i != total_primal_vertices) {
      SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_SUP,"Error in boundary numbering for BDDC vertices! %d != %d\n",total_primal_vertices,i);
    }
    for (i=0;i<total_primal_vertices;i++) {
      ierr = PetscBTSet(is_primal,aux_primal_numbering_B[i]);CHKERRQ(ierr);
    }
    ierr = PetscFree(aux_primal_numbering_B);CHKERRQ(ierr);

    /* loop on constraints and see whether or not they need a change of basis and compute it */
    /* -> using implicit ordering contained in temp_indices data */
    total_counts = pcbddc->n_vertices;
    primal_counter = total_counts;
    while (total_counts<pcbddc->local_primal_size) {
      primal_dofs = 1;
      if (PetscBTLookup(change_basis,total_counts)) {
        /* get all constraints with same support: if more then one constraint is present on the cc then surely indices are stored contiguosly */
        while (total_counts+primal_dofs < pcbddc->local_primal_size && temp_indices_to_constraint_B[temp_indices[total_counts]] == temp_indices_to_constraint_B[temp_indices[total_counts+primal_dofs]]) {
          primal_dofs++;
        }
        /* get constraint info */
        size_of_constraint = temp_indices[total_counts+1]-temp_indices[total_counts];
        dual_dofs = size_of_constraint-primal_dofs;

        if (pcbddc->dbg_flag) {
          ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"Constraints %d to %d (incl) need a change of basis (size %d)\n",total_counts,total_counts+primal_dofs-1,size_of_constraint);CHKERRQ(ierr);
        }

        if (primal_dofs > 1) { /* QR */

          /* copy quadrature constraints for change of basis check */
          if (pcbddc->dbg_flag) {
            ierr = PetscMemcpy(work,&temp_quadrature_constraint[temp_indices[total_counts]],size_of_constraint*primal_dofs*sizeof(PetscScalar));CHKERRQ(ierr);
          }
          /* copy temporary constraints into larger work vector (in order to store all columns of Q) */
          ierr = PetscMemcpy(qr_basis,&temp_quadrature_constraint[temp_indices[total_counts]],size_of_constraint*primal_dofs*sizeof(PetscScalar));CHKERRQ(ierr);

          /* compute QR decomposition of constraints */
          ierr = PetscBLASIntCast(size_of_constraint,&Blas_M);CHKERRQ(ierr);
          ierr = PetscBLASIntCast(primal_dofs,&Blas_N);CHKERRQ(ierr);
          ierr = PetscBLASIntCast(size_of_constraint,&Blas_LDA);CHKERRQ(ierr);
          ierr = PetscFPTrapPush(PETSC_FP_TRAP_OFF);CHKERRQ(ierr);
          PetscStackCallBLAS("LAPACKgeqrf",LAPACKgeqrf_(&Blas_M,&Blas_N,qr_basis,&Blas_LDA,qr_tau,qr_work,&lqr_work,&lierr));
          if (lierr) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error in GEQRF Lapack routine %d",(int)lierr);
          ierr = PetscFPTrapPop();CHKERRQ(ierr);

          /* explictly compute R^-T */
          ierr = PetscMemzero(trs_rhs,primal_dofs*primal_dofs*sizeof(*trs_rhs));CHKERRQ(ierr);
          for (j=0;j<primal_dofs;j++) trs_rhs[j*(primal_dofs+1)] = 1.0;
          ierr = PetscBLASIntCast(primal_dofs,&Blas_N);CHKERRQ(ierr);
          ierr = PetscBLASIntCast(primal_dofs,&Blas_NRHS);CHKERRQ(ierr);
          ierr = PetscBLASIntCast(size_of_constraint,&Blas_LDA);CHKERRQ(ierr);
          ierr = PetscBLASIntCast(primal_dofs,&Blas_LDB);CHKERRQ(ierr);
          ierr = PetscFPTrapPush(PETSC_FP_TRAP_OFF);CHKERRQ(ierr);
          PetscStackCallBLAS("LAPACKtrtrs",LAPACKtrtrs_("U","T","N",&Blas_N,&Blas_NRHS,qr_basis,&Blas_LDA,trs_rhs,&Blas_LDB,&lierr));
          if (lierr) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error in TRTRS Lapack routine %d",(int)lierr);
          ierr = PetscFPTrapPop();CHKERRQ(ierr);

          /* explicitly compute all columns of Q (Q = [Q1 | Q2] ) overwriting QR factorization in qr_basis */
          ierr = PetscBLASIntCast(size_of_constraint,&Blas_M);CHKERRQ(ierr);
          ierr = PetscBLASIntCast(size_of_constraint,&Blas_N);CHKERRQ(ierr);
          ierr = PetscBLASIntCast(primal_dofs,&Blas_K);CHKERRQ(ierr);
          ierr = PetscBLASIntCast(size_of_constraint,&Blas_LDA);CHKERRQ(ierr);
          ierr = PetscFPTrapPush(PETSC_FP_TRAP_OFF);CHKERRQ(ierr);
          PetscStackCallBLAS("LAPACKungqr",LAPACKungqr_(&Blas_M,&Blas_N,&Blas_K,qr_basis,&Blas_LDA,qr_tau,gqr_work,&lgqr_work,&lierr));
          if (lierr) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Error in UNGQR Lapack routine %d",(int)lierr);
          ierr = PetscFPTrapPop();CHKERRQ(ierr);

          /* first primal_dofs columns of Q need to be re-scaled in order to be unitary w.r.t constraints
             i.e. C_{pxn}*Q_{nxn} should be equal to [I_pxp | 0_pxd] (see check below)
             where n=size_of_constraint, p=primal_dofs, d=dual_dofs (n=p+d), I and 0 identity and null matrix resp. */
          ierr = PetscBLASIntCast(size_of_constraint,&Blas_M);CHKERRQ(ierr);
          ierr = PetscBLASIntCast(primal_dofs,&Blas_N);CHKERRQ(ierr);
          ierr = PetscBLASIntCast(primal_dofs,&Blas_K);CHKERRQ(ierr);
          ierr = PetscBLASIntCast(size_of_constraint,&Blas_LDA);CHKERRQ(ierr);
          ierr = PetscBLASIntCast(primal_dofs,&Blas_LDB);CHKERRQ(ierr);
          ierr = PetscBLASIntCast(size_of_constraint,&Blas_LDC);CHKERRQ(ierr);
          ierr = PetscFPTrapPush(PETSC_FP_TRAP_OFF);CHKERRQ(ierr);
          PetscStackCallBLAS("BLASgemm",BLASgemm_("N","N",&Blas_M,&Blas_N,&Blas_K,&one,qr_basis,&Blas_LDA,trs_rhs,&Blas_LDB,&zero,&temp_quadrature_constraint[temp_indices[total_counts]],&Blas_LDC));
          ierr = PetscFPTrapPop();CHKERRQ(ierr);
          ierr = PetscMemcpy(qr_basis,&temp_quadrature_constraint[temp_indices[total_counts]],size_of_constraint*primal_dofs*sizeof(PetscScalar));CHKERRQ(ierr);

          /* insert values in change of basis matrix respecting global ordering of new primal dofs */
          start_rows = &temp_indices_to_constraint_B[temp_indices[total_counts]];
          /* insert cols for primal dofs */
          for (j=0;j<primal_dofs;j++) {
            start_vals = &qr_basis[j*size_of_constraint];
            start_cols = &temp_indices_to_constraint_B[temp_indices[total_counts]+aux_primal_minloc[primal_counter+j]];
            ierr = MatSetValues(pcbddc->ChangeOfBasisMatrix,size_of_constraint,start_rows,1,start_cols,start_vals,INSERT_VALUES);CHKERRQ(ierr);
          }
          /* insert cols for dual dofs */
          for (j=0,k=0;j<dual_dofs;k++) {
            if (!PetscBTLookup(is_primal,temp_indices_to_constraint_B[temp_indices[total_counts]+k])) {
              start_vals = &qr_basis[(primal_dofs+j)*size_of_constraint];
              start_cols = &temp_indices_to_constraint_B[temp_indices[total_counts]+k];
              ierr = MatSetValues(pcbddc->ChangeOfBasisMatrix,size_of_constraint,start_rows,1,start_cols,start_vals,INSERT_VALUES);CHKERRQ(ierr);
              j++;
            }
          }

          /* check change of basis */
          if (pcbddc->dbg_flag) {
            PetscInt   ii,jj;
            PetscBool valid_qr=PETSC_TRUE;
            ierr = PetscBLASIntCast(primal_dofs,&Blas_M);CHKERRQ(ierr);
            ierr = PetscBLASIntCast(size_of_constraint,&Blas_N);CHKERRQ(ierr);
            ierr = PetscBLASIntCast(size_of_constraint,&Blas_K);CHKERRQ(ierr);
            ierr = PetscBLASIntCast(size_of_constraint,&Blas_LDA);CHKERRQ(ierr);
            ierr = PetscBLASIntCast(size_of_constraint,&Blas_LDB);CHKERRQ(ierr);
            ierr = PetscBLASIntCast(primal_dofs,&Blas_LDC);CHKERRQ(ierr);
            ierr = PetscFPTrapPush(PETSC_FP_TRAP_OFF);CHKERRQ(ierr);
            PetscStackCallBLAS("BLASgemm",BLASgemm_("T","N",&Blas_M,&Blas_N,&Blas_K,&one,work,&Blas_LDA,qr_basis,&Blas_LDB,&zero,&work[size_of_constraint*primal_dofs],&Blas_LDC));
            ierr = PetscFPTrapPop();CHKERRQ(ierr);
            for (jj=0;jj<size_of_constraint;jj++) {
              for (ii=0;ii<primal_dofs;ii++) {
                if (ii != jj && PetscAbsScalar(work[size_of_constraint*primal_dofs+jj*primal_dofs+ii]) > 1.e-12) valid_qr = PETSC_FALSE;
                if (ii == jj && PetscAbsScalar(work[size_of_constraint*primal_dofs+jj*primal_dofs+ii]-1.0) > 1.e-12) valid_qr = PETSC_FALSE;
              }
            }
            if (!valid_qr) {
              ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"\t-> wrong change of basis!\n");CHKERRQ(ierr);
              for (jj=0;jj<size_of_constraint;jj++) {
                for (ii=0;ii<primal_dofs;ii++) {
                  if (ii != jj && PetscAbsScalar(work[size_of_constraint*primal_dofs+jj*primal_dofs+ii]) > 1.e-12) {
                    PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"\tQr basis function %d is not orthogonal to constraint %d (%1.14e)!\n",jj,ii,PetscAbsScalar(work[size_of_constraint*primal_dofs+jj*primal_dofs+ii]));
                  }
                  if (ii == jj && PetscAbsScalar(work[size_of_constraint*primal_dofs+jj*primal_dofs+ii]-1.0) > 1.e-12) {
                    PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"\tQr basis function %d is not unitary w.r.t constraint %d (%1.14e)!\n",jj,ii,PetscAbsScalar(work[size_of_constraint*primal_dofs+jj*primal_dofs+ii]));
                  }
                }
              }
            } else {
              ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"\t-> right change of basis!\n");CHKERRQ(ierr);
            }
          }
        } else { /* simple transformation block */
          PetscInt row,col;
          PetscScalar val;
          for (j=0;j<size_of_constraint;j++) {
            row = temp_indices_to_constraint_B[temp_indices[total_counts]+j];
            if (!PetscBTLookup(is_primal,row)) {
              col = temp_indices_to_constraint_B[temp_indices[total_counts]+aux_primal_minloc[primal_counter]];
              ierr = MatSetValue(pcbddc->ChangeOfBasisMatrix,row,row,1.0,INSERT_VALUES);CHKERRQ(ierr);
              ierr = MatSetValue(pcbddc->ChangeOfBasisMatrix,row,col,1.0,INSERT_VALUES);CHKERRQ(ierr);
            } else {
              for (k=0;k<size_of_constraint;k++) {
                col = temp_indices_to_constraint_B[temp_indices[total_counts]+k];
                if (row != col) {
                  val = -temp_quadrature_constraint[temp_indices[total_counts]+k]/temp_quadrature_constraint[temp_indices[total_counts]+aux_primal_minloc[primal_counter]];
                } else {
                  val = 1.0;
                }
                ierr = MatSetValue(pcbddc->ChangeOfBasisMatrix,row,col,val,INSERT_VALUES);CHKERRQ(ierr);
              }
            }
          }
          if (pcbddc->dbg_flag) {
            ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"\t-> using standard change of basis\n");CHKERRQ(ierr);
          }
        }
        /* increment primal counter */
        primal_counter += primal_dofs;
      } else {
        if (pcbddc->dbg_flag) {
          ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"Constraint %d does not need a change of basis (size %d)\n",total_counts,temp_indices[total_counts+1]-temp_indices[total_counts]);CHKERRQ(ierr);
        }
      }
      /* increment constraint counter total_counts */
      total_counts += primal_dofs;
    }

    /* free workspace */
    if (qr_needed) {
      if (pcbddc->dbg_flag) {
        ierr = PetscFree(work);CHKERRQ(ierr);
      }
      ierr = PetscFree(trs_rhs);CHKERRQ(ierr);
      ierr = PetscFree(qr_tau);CHKERRQ(ierr);
      ierr = PetscFree(qr_work);CHKERRQ(ierr);
      ierr = PetscFree(gqr_work);CHKERRQ(ierr);
      ierr = PetscFree(qr_basis);CHKERRQ(ierr);
    }
    ierr = PetscBTDestroy(&is_primal);CHKERRQ(ierr);
    /* assembling */
    ierr = MatAssemblyBegin(pcbddc->ChangeOfBasisMatrix,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd(pcbddc->ChangeOfBasisMatrix,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    /*
    ierr = PetscViewerSetFormat(PETSC_VIEWER_STDOUT_SELF,PETSC_VIEWER_ASCII_MATLAB);CHKERRQ(ierr);
    ierr = MatView(pcbddc->ChangeOfBasisMatrix,(PetscViewer)0);CHKERRQ(ierr);
    */
  }
  /* Change of basis as provided by the user in local numbering (internal and boundary) or boundary only */
  if (pcbddc->user_ChangeOfBasisMatrix) {
    PetscInt rows,cols;
    ierr = MatGetSize(pcbddc->user_ChangeOfBasisMatrix,&rows,&cols);CHKERRQ(ierr);
    if (rows == pcis->n && cols == pcis->n) {
      ierr = MatGetSubMatrix(pcbddc->user_ChangeOfBasisMatrix,pcis->is_B_local,pcis->is_B_local,MAT_INITIAL_MATRIX,&pcbddc->ChangeOfBasisMatrix);CHKERRQ(ierr);
    } else {
      ierr = PetscObjectReference((PetscObject)pcbddc->user_ChangeOfBasisMatrix);CHKERRQ(ierr);
      pcbddc->ChangeOfBasisMatrix = pcbddc->user_ChangeOfBasisMatrix;
    }
  }

  /* get indices in local ordering for vertices and constraints */
  if (olocal_primal_size == pcbddc->local_primal_size) { /* if this is true, I need to check if a new primal space has been introduced */
    ierr = PetscMalloc1(olocal_primal_size,&oprimal_indices_local_idxs);CHKERRQ(ierr);
    ierr = PetscMemcpy(oprimal_indices_local_idxs,pcbddc->primal_indices_local_idxs,olocal_primal_size*sizeof(PetscInt));CHKERRQ(ierr);
  }
  ierr = PetscFree(aux_primal_numbering);CHKERRQ(ierr);
  ierr = PetscFree(pcbddc->primal_indices_local_idxs);CHKERRQ(ierr);
  ierr = PetscMalloc1(pcbddc->local_primal_size,&pcbddc->primal_indices_local_idxs);CHKERRQ(ierr);
  ierr = PCBDDCGetPrimalVerticesLocalIdx(pc,&i,&aux_primal_numbering);CHKERRQ(ierr);
  ierr = PetscMemcpy(pcbddc->primal_indices_local_idxs,aux_primal_numbering,i*sizeof(PetscInt));CHKERRQ(ierr);
  ierr = PetscFree(aux_primal_numbering);CHKERRQ(ierr);
  ierr = PCBDDCGetPrimalConstraintsLocalIdx(pc,&j,&aux_primal_numbering);CHKERRQ(ierr);
  ierr = PetscMemcpy(&pcbddc->primal_indices_local_idxs[i],aux_primal_numbering,j*sizeof(PetscInt));CHKERRQ(ierr);
  ierr = PetscFree(aux_primal_numbering);CHKERRQ(ierr);
  /* set quantities in PCBDDC data struct */
  pcbddc->n_actual_vertices = i;
  /* check if a new primal space has been introduced */
  pcbddc->new_primal_space_local = PETSC_TRUE;
  if (olocal_primal_size == pcbddc->local_primal_size) {
    ierr = PetscMemcmp(pcbddc->primal_indices_local_idxs,oprimal_indices_local_idxs,olocal_primal_size,&pcbddc->new_primal_space_local);CHKERRQ(ierr);
    pcbddc->new_primal_space_local = (PetscBool)(!pcbddc->new_primal_space_local);
    ierr = PetscFree(oprimal_indices_local_idxs);CHKERRQ(ierr);
  }
  /* new_primal_space will be used for numbering of coarse dofs, so it should be the same across all subdomains */
  ierr = MPI_Allreduce(&pcbddc->new_primal_space_local,&pcbddc->new_primal_space,1,MPIU_BOOL,MPI_LOR,PetscObjectComm((PetscObject)pc));CHKERRQ(ierr);

  /* flush dbg viewer */
  if (pcbddc->dbg_flag) {
    ierr = PetscViewerFlush(pcbddc->dbg_viewer);CHKERRQ(ierr);
  }

  /* free workspace */
  ierr = PetscBTDestroy(&touched);CHKERRQ(ierr);
  ierr = PetscBTDestroy(&qr_needed_idx);CHKERRQ(ierr);
  ierr = PetscFree(aux_primal_minloc);CHKERRQ(ierr);
  ierr = PetscFree(temp_indices);CHKERRQ(ierr);
  ierr = PetscBTDestroy(&change_basis);CHKERRQ(ierr);
  ierr = PetscFree(temp_indices_to_constraint);CHKERRQ(ierr);
  ierr = PetscFree(temp_indices_to_constraint_B);CHKERRQ(ierr);
  ierr = PetscFree(temp_quadrature_constraint);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCBDDCAnalyzeInterface"
PetscErrorCode PCBDDCAnalyzeInterface(PC pc)
{
  PC_BDDC     *pcbddc = (PC_BDDC*)pc->data;
  PC_IS       *pcis = (PC_IS*)pc->data;
  Mat_IS      *matis  = (Mat_IS*)pc->pmat->data;
  PetscInt    ierr,i,vertex_size;
  PetscViewer viewer=pcbddc->dbg_viewer;

  PetscFunctionBegin;
  /* Reset previously computed graph */
  ierr = PCBDDCGraphReset(pcbddc->mat_graph);CHKERRQ(ierr);
  /* Init local Graph struct */
  ierr = PCBDDCGraphInit(pcbddc->mat_graph,matis->mapping);CHKERRQ(ierr);

  /* Check validity of the csr graph passed in by the user */
  if (pcbddc->mat_graph->nvtxs_csr != pcbddc->mat_graph->nvtxs) {
    ierr = PCBDDCGraphResetCSR(pcbddc->mat_graph);CHKERRQ(ierr);
  }

  /* Set default CSR adjacency of local dofs if not provided by the user with PCBDDCSetLocalAdjacencyGraph */
  if (pcbddc->use_local_adj && (!pcbddc->mat_graph->xadj || !pcbddc->mat_graph->adjncy)) {
    Mat mat_adj;
    const PetscInt *xadj,*adjncy;
    PetscBool flg_row=PETSC_TRUE;

    ierr = MatConvert(matis->A,MATMPIADJ,MAT_INITIAL_MATRIX,&mat_adj);CHKERRQ(ierr);
    ierr = MatGetRowIJ(mat_adj,0,PETSC_TRUE,PETSC_FALSE,&i,&xadj,&adjncy,&flg_row);CHKERRQ(ierr);
    if (!flg_row) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_PLIB,"Error in MatGetRowIJ called in %s\n",__FUNCT__);
    }
    ierr = PCBDDCSetLocalAdjacencyGraph(pc,i,xadj,adjncy,PETSC_COPY_VALUES);CHKERRQ(ierr);
    ierr = MatRestoreRowIJ(mat_adj,0,PETSC_TRUE,PETSC_FALSE,&i,&xadj,&adjncy,&flg_row);CHKERRQ(ierr);
    if (!flg_row) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_PLIB,"Error in MatRestoreRowIJ called in %s\n",__FUNCT__);
    }
    ierr = MatDestroy(&mat_adj);CHKERRQ(ierr);
  }

  /* Set default dofs' splitting if no information has been provided by the user with PCBDDCSetDofsSplitting or PCBDDCSetDofsSplittingLocal */
  vertex_size = 1;
  if (pcbddc->user_provided_isfordofs) {
    if (pcbddc->n_ISForDofs) { /* need to convert from global to local and remove references to global dofs splitting */
      ierr = PetscMalloc1(pcbddc->n_ISForDofs,&pcbddc->ISForDofsLocal);CHKERRQ(ierr);
      for (i=0;i<pcbddc->n_ISForDofs;i++) {
        ierr = PCBDDCGlobalToLocal(matis->ctx,pcis->vec1_global,pcis->vec1_N,pcbddc->ISForDofs[i],&pcbddc->ISForDofsLocal[i]);CHKERRQ(ierr);
        ierr = ISDestroy(&pcbddc->ISForDofs[i]);CHKERRQ(ierr);
      }
      pcbddc->n_ISForDofsLocal = pcbddc->n_ISForDofs;
      pcbddc->n_ISForDofs = 0;
      ierr = PetscFree(pcbddc->ISForDofs);CHKERRQ(ierr);
    }
    /* mat block size as vertex size (used for elasticity with rigid body modes as nearnullspace) */
    ierr = MatGetBlockSize(matis->A,&vertex_size);CHKERRQ(ierr);
  } else {
    if (!pcbddc->n_ISForDofsLocal) { /* field split not present, create it in local ordering */
      ierr = MatGetBlockSize(pc->pmat,&pcbddc->n_ISForDofsLocal);CHKERRQ(ierr);
      ierr = PetscMalloc(pcbddc->n_ISForDofsLocal*sizeof(IS),&pcbddc->ISForDofsLocal);CHKERRQ(ierr);
      for (i=0;i<pcbddc->n_ISForDofsLocal;i++) {
        ierr = ISCreateStride(PetscObjectComm((PetscObject)pc),pcis->n/pcbddc->n_ISForDofsLocal,i,pcbddc->n_ISForDofsLocal,&pcbddc->ISForDofsLocal[i]);CHKERRQ(ierr);
      }
    }
  }

  /* Setup of Graph */
  if (!pcbddc->DirichletBoundariesLocal && pcbddc->DirichletBoundaries) { /* need to convert from global to local */
    ierr = PCBDDCGlobalToLocal(matis->ctx,pcis->vec1_global,pcis->vec1_N,pcbddc->DirichletBoundaries,&pcbddc->DirichletBoundariesLocal);CHKERRQ(ierr);
  }
  if (!pcbddc->NeumannBoundariesLocal && pcbddc->NeumannBoundaries) { /* need to convert from global to local */
    ierr = PCBDDCGlobalToLocal(matis->ctx,pcis->vec1_global,pcis->vec1_N,pcbddc->NeumannBoundaries,&pcbddc->NeumannBoundariesLocal);CHKERRQ(ierr);
  }
  ierr = PCBDDCGraphSetUp(pcbddc->mat_graph,vertex_size,pcbddc->NeumannBoundariesLocal,pcbddc->DirichletBoundariesLocal,pcbddc->n_ISForDofsLocal,pcbddc->ISForDofsLocal,pcbddc->user_primal_vertices);

  /* Graph's connected components analysis */
  ierr = PCBDDCGraphComputeConnectedComponents(pcbddc->mat_graph);CHKERRQ(ierr);

  /* print some info to stdout */
  if (pcbddc->dbg_flag) {
    ierr = PCBDDCGraphASCIIView(pcbddc->mat_graph,pcbddc->dbg_flag,viewer);
  }

  /* mark topography has done */
  pcbddc->recompute_topography = PETSC_FALSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCBDDCGetPrimalVerticesLocalIdx"
PetscErrorCode  PCBDDCGetPrimalVerticesLocalIdx(PC pc, PetscInt *n_vertices, PetscInt **vertices_idx)
{
  PC_BDDC        *pcbddc = (PC_BDDC*)(pc->data);
  PetscInt       *vertices,*row_cmat_indices,n,i,size_of_constraint,local_primal_size;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  n = 0;
  vertices = 0;
  if (pcbddc->ConstraintMatrix) {
    ierr = MatGetSize(pcbddc->ConstraintMatrix,&local_primal_size,&i);CHKERRQ(ierr);
    for (i=0;i<local_primal_size;i++) {
      ierr = MatGetRow(pcbddc->ConstraintMatrix,i,&size_of_constraint,NULL,NULL);CHKERRQ(ierr);
      if (size_of_constraint == 1) n++;
      ierr = MatRestoreRow(pcbddc->ConstraintMatrix,i,&size_of_constraint,NULL,NULL);CHKERRQ(ierr);
    }
    if (vertices_idx) {
      ierr = PetscMalloc1(n,&vertices);CHKERRQ(ierr);
      n = 0;
      for (i=0;i<local_primal_size;i++) {
        ierr = MatGetRow(pcbddc->ConstraintMatrix,i,&size_of_constraint,(const PetscInt**)&row_cmat_indices,NULL);CHKERRQ(ierr);
        if (size_of_constraint == 1) {
          vertices[n++]=row_cmat_indices[0];
        }
        ierr = MatRestoreRow(pcbddc->ConstraintMatrix,i,&size_of_constraint,(const PetscInt**)&row_cmat_indices,NULL);CHKERRQ(ierr);
      }
    }
  }
  *n_vertices = n;
  if (vertices_idx) *vertices_idx = vertices;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCBDDCGetPrimalConstraintsLocalIdx"
PetscErrorCode  PCBDDCGetPrimalConstraintsLocalIdx(PC pc, PetscInt *n_constraints, PetscInt **constraints_idx)
{
  PC_BDDC        *pcbddc = (PC_BDDC*)(pc->data);
  PetscInt       *constraints_index,*row_cmat_indices,*row_cmat_global_indices;
  PetscInt       n,i,j,size_of_constraint,local_primal_size,local_size,max_size_of_constraint,min_index,min_loc;
  PetscBT        touched;
  PetscErrorCode ierr;

    /* This function assumes that the number of local constraints per connected component
       is not greater than the number of nodes defined for the connected component
       (otherwise we will surely have linear dependence between constraints and thus a singular coarse problem) */
  PetscFunctionBegin;
  n = 0;
  constraints_index = 0;
  if (pcbddc->ConstraintMatrix) {
    ierr = MatGetSize(pcbddc->ConstraintMatrix,&local_primal_size,&local_size);CHKERRQ(ierr);
    max_size_of_constraint = 0;
    for (i=0;i<local_primal_size;i++) {
      ierr = MatGetRow(pcbddc->ConstraintMatrix,i,&size_of_constraint,NULL,NULL);CHKERRQ(ierr);
      if (size_of_constraint > 1) {
        n++;
      }
      max_size_of_constraint = PetscMax(size_of_constraint,max_size_of_constraint);
      ierr = MatRestoreRow(pcbddc->ConstraintMatrix,i,&size_of_constraint,NULL,NULL);CHKERRQ(ierr);
    }
    if (constraints_idx) {
      ierr = PetscMalloc1(n,&constraints_index);CHKERRQ(ierr);
      ierr = PetscMalloc1(max_size_of_constraint,&row_cmat_global_indices);CHKERRQ(ierr);
      ierr = PetscBTCreate(local_size,&touched);CHKERRQ(ierr);
      n = 0;
      for (i=0;i<local_primal_size;i++) {
        ierr = MatGetRow(pcbddc->ConstraintMatrix,i,&size_of_constraint,(const PetscInt**)&row_cmat_indices,NULL);CHKERRQ(ierr);
        if (size_of_constraint > 1) {
          ierr = ISLocalToGlobalMappingApply(pcbddc->mat_graph->l2gmap,size_of_constraint,row_cmat_indices,row_cmat_global_indices);CHKERRQ(ierr);
          /* find first untouched local node */
          j = 0;
          while (PetscBTLookup(touched,row_cmat_indices[j])) j++;
          min_index = row_cmat_global_indices[j];
          min_loc = j;
          /* search the minimum among nodes not yet touched on the connected component
             since there can be more than one constraint on a single cc */
          for (j=1;j<size_of_constraint;j++) {
            if (!PetscBTLookup(touched,row_cmat_indices[j]) && min_index > row_cmat_global_indices[j]) {
              min_index = row_cmat_global_indices[j];
              min_loc = j;
            }
          }
          ierr = PetscBTSet(touched,row_cmat_indices[min_loc]);CHKERRQ(ierr);
          constraints_index[n++] = row_cmat_indices[min_loc];
        }
        ierr = MatRestoreRow(pcbddc->ConstraintMatrix,i,&size_of_constraint,(const PetscInt**)&row_cmat_indices,NULL);CHKERRQ(ierr);
      }
      ierr = PetscBTDestroy(&touched);CHKERRQ(ierr);
      ierr = PetscFree(row_cmat_global_indices);CHKERRQ(ierr);
    }
  }
  *n_constraints = n;
  if (constraints_idx) *constraints_idx = constraints_index;
  PetscFunctionReturn(0);
}

/* the next two functions has been adapted from pcis.c */
#undef __FUNCT__
#define __FUNCT__ "PCBDDCApplySchur"
PetscErrorCode  PCBDDCApplySchur(PC pc, Vec v, Vec vec1_B, Vec vec2_B, Vec vec1_D, Vec vec2_D)
{
  PetscErrorCode ierr;
  PC_IS          *pcis = (PC_IS*)(pc->data);

  PetscFunctionBegin;
  if (!vec2_B) { vec2_B = v; }
  ierr = MatMult(pcis->A_BB,v,vec1_B);CHKERRQ(ierr);
  ierr = MatMult(pcis->A_IB,v,vec1_D);CHKERRQ(ierr);
  ierr = KSPSolve(pcis->ksp_D,vec1_D,vec2_D);CHKERRQ(ierr);
  ierr = MatMult(pcis->A_BI,vec2_D,vec2_B);CHKERRQ(ierr);
  ierr = VecAXPY(vec1_B,-1.0,vec2_B);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCBDDCApplySchurTranspose"
PetscErrorCode  PCBDDCApplySchurTranspose(PC pc, Vec v, Vec vec1_B, Vec vec2_B, Vec vec1_D, Vec vec2_D)
{
  PetscErrorCode ierr;
  PC_IS          *pcis = (PC_IS*)(pc->data);

  PetscFunctionBegin;
  if (!vec2_B) { vec2_B = v; }
  ierr = MatMultTranspose(pcis->A_BB,v,vec1_B);CHKERRQ(ierr);
  ierr = MatMultTranspose(pcis->A_BI,v,vec1_D);CHKERRQ(ierr);
  ierr = KSPSolveTranspose(pcis->ksp_D,vec1_D,vec2_D);CHKERRQ(ierr);
  ierr = MatMultTranspose(pcis->A_IB,vec2_D,vec2_B);CHKERRQ(ierr);
  ierr = VecAXPY(vec1_B,-1.0,vec2_B);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCBDDCSubsetNumbering"
PetscErrorCode PCBDDCSubsetNumbering(MPI_Comm comm,ISLocalToGlobalMapping l2gmap, PetscInt n_local_dofs, PetscInt local_dofs[], PetscInt local_dofs_mult[], PetscInt* n_global_subset, PetscInt* global_numbering_subset[])
{
  Vec            local_vec,global_vec;
  IS             seqis,paris;
  VecScatter     scatter_ctx;
  PetscScalar    *array;
  PetscInt       *temp_global_dofs;
  PetscScalar    globalsum;
  PetscInt       i,j,s;
  PetscInt       nlocals,first_index,old_index,max_local;
  PetscMPIInt    rank_prec_comm,size_prec_comm,max_global;
  PetscMPIInt    *dof_sizes,*dof_displs;
  PetscBool      first_found;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* mpi buffers */
  ierr = MPI_Comm_size(comm,&size_prec_comm);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm,&rank_prec_comm);CHKERRQ(ierr);
  j = ( !rank_prec_comm ? size_prec_comm : 0);
  ierr = PetscMalloc1(j,&dof_sizes);CHKERRQ(ierr);
  ierr = PetscMalloc1(j,&dof_displs);CHKERRQ(ierr);
  /* get maximum size of subset */
  ierr = PetscMalloc1(n_local_dofs,&temp_global_dofs);CHKERRQ(ierr);
  ierr = ISLocalToGlobalMappingApply(l2gmap,n_local_dofs,local_dofs,temp_global_dofs);CHKERRQ(ierr);
  max_local = 0;
  for (i=0;i<n_local_dofs;i++) {
    if (max_local < temp_global_dofs[i] ) {
      max_local = temp_global_dofs[i];
    }
  }
  ierr = MPI_Allreduce(&max_local,&max_global,1,MPIU_INT,MPI_MAX,comm);CHKERRQ(ierr);
  max_global++;
  max_local = 0;
  for (i=0;i<n_local_dofs;i++) {
    if (max_local < local_dofs[i] ) {
      max_local = local_dofs[i];
    }
  }
  max_local++;
  /* allocate workspace */
  ierr = VecCreate(PETSC_COMM_SELF,&local_vec);CHKERRQ(ierr);
  ierr = VecSetSizes(local_vec,PETSC_DECIDE,max_local);CHKERRQ(ierr);
  ierr = VecSetType(local_vec,VECSEQ);CHKERRQ(ierr);
  ierr = VecCreate(comm,&global_vec);CHKERRQ(ierr);
  ierr = VecSetSizes(global_vec,PETSC_DECIDE,max_global);CHKERRQ(ierr);
  ierr = VecSetType(global_vec,VECMPI);CHKERRQ(ierr);
  /* create scatter */
  ierr = ISCreateGeneral(PETSC_COMM_SELF,n_local_dofs,local_dofs,PETSC_COPY_VALUES,&seqis);CHKERRQ(ierr);
  ierr = ISCreateGeneral(comm,n_local_dofs,temp_global_dofs,PETSC_COPY_VALUES,&paris);CHKERRQ(ierr);
  ierr = VecScatterCreate(local_vec,seqis,global_vec,paris,&scatter_ctx);CHKERRQ(ierr);
  ierr = ISDestroy(&seqis);CHKERRQ(ierr);
  ierr = ISDestroy(&paris);CHKERRQ(ierr);
  /* init array */
  ierr = VecSet(global_vec,0.0);CHKERRQ(ierr);
  ierr = VecSet(local_vec,0.0);CHKERRQ(ierr);
  ierr = VecGetArray(local_vec,&array);CHKERRQ(ierr);
  if (local_dofs_mult) {
    for (i=0;i<n_local_dofs;i++) {
      array[local_dofs[i]]=(PetscScalar)local_dofs_mult[i];
    }
  } else {
    for (i=0;i<n_local_dofs;i++) {
      array[local_dofs[i]]=1.0;
    }
  }
  ierr = VecRestoreArray(local_vec,&array);CHKERRQ(ierr);
  /* scatter into global vec and get total number of global dofs */
  ierr = VecScatterBegin(scatter_ctx,local_vec,global_vec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(scatter_ctx,local_vec,global_vec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecSum(global_vec,&globalsum);CHKERRQ(ierr);
  *n_global_subset = (PetscInt)PetscRealPart(globalsum);
  /* Fill global_vec with cumulative function for global numbering */
  ierr = VecGetArray(global_vec,&array);CHKERRQ(ierr);
  ierr = VecGetLocalSize(global_vec,&s);CHKERRQ(ierr);
  nlocals = 0;
  first_index = -1;
  first_found = PETSC_FALSE;
  for (i=0;i<s;i++) {
    if (!first_found && PetscRealPart(array[i]) > 0.1) {
      first_found = PETSC_TRUE;
      first_index = i;
    }
    nlocals += (PetscInt)PetscRealPart(array[i]);
  }
  ierr = MPI_Gather(&nlocals,1,MPIU_INT,dof_sizes,1,MPIU_INT,0,comm);CHKERRQ(ierr);
  if (!rank_prec_comm) {
    dof_displs[0]=0;
    for (i=1;i<size_prec_comm;i++) {
      dof_displs[i] = dof_displs[i-1]+dof_sizes[i-1];
    }
  }
  ierr = MPI_Scatter(dof_displs,1,MPIU_INT,&nlocals,1,MPIU_INT,0,comm);CHKERRQ(ierr);
  if (first_found) {
    array[first_index] += (PetscScalar)nlocals;
    old_index = first_index;
    for (i=first_index+1;i<s;i++) {
      if (PetscRealPart(array[i]) > 0.1) {
        array[i] += array[old_index];
        old_index = i;
      }
    }
  }
  ierr = VecRestoreArray(global_vec,&array);CHKERRQ(ierr);
  ierr = VecSet(local_vec,0.0);CHKERRQ(ierr);
  ierr = VecScatterBegin(scatter_ctx,global_vec,local_vec,INSERT_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
  ierr = VecScatterEnd(scatter_ctx,global_vec,local_vec,INSERT_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
  /* get global ordering of local dofs */
  ierr = VecGetArray(local_vec,&array);CHKERRQ(ierr);
  if (local_dofs_mult) {
    for (i=0;i<n_local_dofs;i++) {
      temp_global_dofs[i] = (PetscInt)PetscRealPart(array[local_dofs[i]])-local_dofs_mult[i];
    }
  } else {
    for (i=0;i<n_local_dofs;i++) {
      temp_global_dofs[i] = (PetscInt)PetscRealPart(array[local_dofs[i]])-1;
    }
  }
  ierr = VecRestoreArray(local_vec,&array);CHKERRQ(ierr);
  /* free workspace */
  ierr = VecScatterDestroy(&scatter_ctx);CHKERRQ(ierr);
  ierr = VecDestroy(&local_vec);CHKERRQ(ierr);
  ierr = VecDestroy(&global_vec);CHKERRQ(ierr);
  ierr = PetscFree(dof_sizes);CHKERRQ(ierr);
  ierr = PetscFree(dof_displs);CHKERRQ(ierr);
  /* return pointer to global ordering of local dofs */
  *global_numbering_subset = temp_global_dofs;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCBDDCOrthonormalizeVecs"
PetscErrorCode PCBDDCOrthonormalizeVecs(PetscInt n, Vec vecs[])
{
  PetscInt       i,j;
  PetscScalar    *alphas;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* this implements stabilized Gram-Schmidt */
  ierr = PetscMalloc1(n,&alphas);CHKERRQ(ierr);
  for (i=0;i<n;i++) {
    ierr = VecNormalize(vecs[i],NULL);CHKERRQ(ierr);
    if (i<n) { ierr = VecMDot(vecs[i],n-i-1,&vecs[i+1],&alphas[i+1]);CHKERRQ(ierr); }
    for (j=i+1;j<n;j++) { ierr = VecAXPY(vecs[j],PetscConj(-alphas[j]),vecs[i]);CHKERRQ(ierr); }
  }
  ierr = PetscFree(alphas);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/* TODO
   - now preallocation is done assuming SEQDENSE local matrices
*/
#undef __FUNCT__
#define __FUNCT__ "MatISGetMPIXAIJ"
static PetscErrorCode MatISGetMPIXAIJ(Mat mat, MatType Mtype, MatReuse reuse, Mat *M)
{
  Mat                    new_mat;
  Mat_IS                 *matis = (Mat_IS*)(mat->data);
  /* info on mat */
  /* ISLocalToGlobalMapping rmapping,cmapping; */
  PetscInt               bs,rows,cols;
  PetscInt               lrows,lcols;
  PetscInt               local_rows,local_cols;
  PetscBool              isdense;
  /* values insertion */
  PetscScalar            *array;
  PetscInt               *local_indices,*global_indices;
  /* work */
  PetscInt               i,j,index_row;
  PetscErrorCode         ierr;

  PetscFunctionBegin;
  /* MISSING CHECKS
    - rectangular case not covered (it is not allowed by MATIS)
  */
  /* get info from mat */
  /* ierr = MatGetLocalToGlobalMapping(mat,&rmapping,&cmapping);CHKERRQ(ierr); */
  ierr = MatGetSize(mat,&rows,&cols);CHKERRQ(ierr);
  ierr = MatGetBlockSize(mat,&bs);CHKERRQ(ierr);
  ierr = MatGetSize(matis->A,&local_rows,&local_cols);CHKERRQ(ierr);

  /* work */
  ierr = PetscMalloc1(local_rows,&local_indices);CHKERRQ(ierr);
  for (i=0;i<local_rows;i++) local_indices[i]=i;
  /* map indices of local mat to global values */
  ierr = PetscMalloc(PetscMax(local_cols,local_rows)*sizeof(*global_indices),&global_indices);CHKERRQ(ierr);
  /* ierr = ISLocalToGlobalMappingApply(rmapping,local_rows,local_indices,global_indices);CHKERRQ(ierr); */
  ierr = ISLocalToGlobalMappingApply(matis->mapping,local_rows,local_indices,global_indices);CHKERRQ(ierr);

  if (reuse==MAT_INITIAL_MATRIX) {
    Vec         vec_dnz,vec_onz;
    PetscScalar *my_dnz,*my_onz;
    PetscInt    *dnz,*onz,*mat_ranges,*row_ownership;
    PetscInt    index_col,owner;
    PetscMPIInt nsubdomains;

    ierr = MPI_Comm_size(PetscObjectComm((PetscObject)mat),&nsubdomains);CHKERRQ(ierr);
    ierr = MatCreate(PetscObjectComm((PetscObject)mat),&new_mat);CHKERRQ(ierr);
    ierr = MatSetSizes(new_mat,PETSC_DECIDE,PETSC_DECIDE,rows,cols);CHKERRQ(ierr);
    ierr = MatSetBlockSize(new_mat,bs);CHKERRQ(ierr);
    ierr = MatSetType(new_mat,Mtype);CHKERRQ(ierr);
    ierr = MatSetUp(new_mat);CHKERRQ(ierr);
    ierr = MatGetLocalSize(new_mat,&lrows,&lcols);CHKERRQ(ierr);

    /*
      preallocation
    */

    ierr = MatPreallocateInitialize(PetscObjectComm((PetscObject)new_mat),lrows,lcols,dnz,onz);CHKERRQ(ierr);
    /*
       Some vectors are needed to sum up properly on shared interface dofs.
       Preallocation macros cannot do the job.
       Note that preallocation is not exact, since it overestimates nonzeros
    */
    ierr = MatGetVecs(new_mat,NULL,&vec_dnz);CHKERRQ(ierr);
    /* ierr = VecSetLocalToGlobalMapping(vec_dnz,rmapping);CHKERRQ(ierr); */
    ierr = VecSetLocalToGlobalMapping(vec_dnz,matis->mapping);CHKERRQ(ierr);
    ierr = VecDuplicate(vec_dnz,&vec_onz);CHKERRQ(ierr);
    /* All processes need to compute entire row ownership */
    ierr = PetscMalloc1(rows,&row_ownership);CHKERRQ(ierr);
    ierr = MatGetOwnershipRanges(new_mat,(const PetscInt**)&mat_ranges);CHKERRQ(ierr);
    for (i=0;i<nsubdomains;i++) {
      for (j=mat_ranges[i];j<mat_ranges[i+1];j++) {
        row_ownership[j]=i;
      }
    }

    /*
       my_dnz and my_onz contains exact contribution to preallocation from each local mat
       then, they will be summed up properly. This way, preallocation is always sufficient
    */
    ierr = PetscMalloc1(local_rows,&my_dnz);CHKERRQ(ierr);
    ierr = PetscMalloc1(local_rows,&my_onz);CHKERRQ(ierr);
    ierr = PetscMemzero(my_dnz,local_rows*sizeof(*my_dnz));CHKERRQ(ierr);
    ierr = PetscMemzero(my_onz,local_rows*sizeof(*my_onz));CHKERRQ(ierr);
    for (i=0;i<local_rows;i++) {
      index_row = global_indices[i];
      for (j=i;j<local_rows;j++) {
        owner = row_ownership[index_row];
        index_col = global_indices[j];
        if (index_col > mat_ranges[owner]-1 && index_col < mat_ranges[owner+1] ) { /* diag block */
          my_dnz[i] += 1.0;
        } else { /* offdiag block */
          my_onz[i] += 1.0;
        }
        /* same as before, interchanging rows and cols */
        if (i != j) {
          owner = row_ownership[index_col];
          if (index_row > mat_ranges[owner]-1 && index_row < mat_ranges[owner+1] ) {
            my_dnz[j] += 1.0;
          } else {
            my_onz[j] += 1.0;
          }
        }
      }
    }
    ierr = VecSet(vec_dnz,0.0);CHKERRQ(ierr);
    ierr = VecSet(vec_onz,0.0);CHKERRQ(ierr);
    if (local_rows) { /* multilevel guard */
      ierr = VecSetValuesLocal(vec_dnz,local_rows,local_indices,my_dnz,ADD_VALUES);CHKERRQ(ierr);
      ierr = VecSetValuesLocal(vec_onz,local_rows,local_indices,my_onz,ADD_VALUES);CHKERRQ(ierr);
    }
    ierr = VecAssemblyBegin(vec_dnz);CHKERRQ(ierr);
    ierr = VecAssemblyBegin(vec_onz);CHKERRQ(ierr);
    ierr = VecAssemblyEnd(vec_dnz);CHKERRQ(ierr);
    ierr = VecAssemblyEnd(vec_onz);CHKERRQ(ierr);
    ierr = PetscFree(my_dnz);CHKERRQ(ierr);
    ierr = PetscFree(my_onz);CHKERRQ(ierr);
    ierr = PetscFree(row_ownership);CHKERRQ(ierr);

    /* set computed preallocation in dnz and onz */
    ierr = VecGetArray(vec_dnz,&array);CHKERRQ(ierr);
    for (i=0; i<lrows; i++) dnz[i] = (PetscInt)PetscRealPart(array[i]);
    ierr = VecRestoreArray(vec_dnz,&array);CHKERRQ(ierr);
    ierr = VecGetArray(vec_onz,&array);CHKERRQ(ierr);
    for (i=0;i<lrows;i++) onz[i] = (PetscInt)PetscRealPart(array[i]);
    ierr = VecRestoreArray(vec_onz,&array);CHKERRQ(ierr);
    ierr = VecDestroy(&vec_dnz);CHKERRQ(ierr);
    ierr = VecDestroy(&vec_onz);CHKERRQ(ierr);

    /* Resize preallocation if overestimated */
    for (i=0;i<lrows;i++) {
      dnz[i] = PetscMin(dnz[i],lcols);
      onz[i] = PetscMin(onz[i],cols-lcols);
    }
    /* set preallocation */
    ierr = MatMPIAIJSetPreallocation(new_mat,0,dnz,0,onz);CHKERRQ(ierr);
    for (i=0;i<lrows/bs;i++) {
      dnz[i] = dnz[i*bs]/bs;
      onz[i] = onz[i*bs]/bs;
    }
    ierr = MatMPIBAIJSetPreallocation(new_mat,bs,0,dnz,0,onz);CHKERRQ(ierr);
    for (i=0;i<lrows/bs;i++) {
      dnz[i] = dnz[i]-i;
    }
    ierr = MatMPISBAIJSetPreallocation(new_mat,bs,0,dnz,0,onz);CHKERRQ(ierr);
    ierr = MatPreallocateFinalize(dnz,onz);CHKERRQ(ierr);
    *M = new_mat;
  } else {
    PetscInt mbs,mrows,mcols;
    /* some checks */
    ierr = MatGetBlockSize(*M,&mbs);CHKERRQ(ierr);
    ierr = MatGetSize(*M,&mrows,&mcols);CHKERRQ(ierr);
    if (mrows != rows) {
      SETERRQ2(PetscObjectComm((PetscObject)mat),PETSC_ERR_SUP,"Cannot reuse matrix. Wrong number of rows (%d != %d)",rows,mrows);
    }
    if (mrows != rows) {
      SETERRQ2(PetscObjectComm((PetscObject)mat),PETSC_ERR_SUP,"Cannot reuse matrix. Wrong number of cols (%d != %d)",cols,mcols);
    }
    if (mbs != bs) {
      SETERRQ2(PetscObjectComm((PetscObject)mat),PETSC_ERR_SUP,"Cannot reuse matrix. Wrong block size (%d != %d)",bs,mbs);
    }
    ierr = MatZeroEntries(*M);CHKERRQ(ierr);
  }
  /* set local to global mappings */
  /* ierr = MatSetLocalToGlobalMapping(*M,rmapping,cmapping);CHKERRQ(ierr); */
  /* Set values */
  ierr = PetscObjectTypeCompare((PetscObject)matis->A,MATSEQDENSE,&isdense);CHKERRQ(ierr);
  if (isdense) { /* special case for dense local matrices */
    ierr = MatSetOption(*M,MAT_ROW_ORIENTED,PETSC_FALSE);CHKERRQ(ierr);
    ierr = MatDenseGetArray(matis->A,&array);CHKERRQ(ierr);
    ierr = MatSetValues(*M,local_rows,global_indices,local_cols,global_indices,array,ADD_VALUES);CHKERRQ(ierr);
    ierr = MatDenseRestoreArray(matis->A,&array);CHKERRQ(ierr);
    ierr = PetscFree(local_indices);CHKERRQ(ierr);
    ierr = PetscFree(global_indices);CHKERRQ(ierr);
  } else { /* very basic values insertion for all other matrix types */
    ierr = PetscFree(local_indices);CHKERRQ(ierr);
    for (i=0;i<local_rows;i++) {
      ierr = MatGetRow(matis->A,i,&j,(const PetscInt**)&local_indices,(const PetscScalar**)&array);CHKERRQ(ierr);
      /* ierr = MatSetValuesLocal(*M,1,&i,j,local_indices,array,ADD_VALUES);CHKERRQ(ierr); */
      ierr = ISLocalToGlobalMappingApply(matis->mapping,j,local_indices,global_indices);CHKERRQ(ierr);
      ierr = ISLocalToGlobalMappingApply(matis->mapping,1,&i,&index_row);CHKERRQ(ierr);
      ierr = MatSetValues(*M,1,&index_row,j,global_indices,array,ADD_VALUES);CHKERRQ(ierr);
      ierr = MatRestoreRow(matis->A,i,&j,(const PetscInt**)&local_indices,(const PetscScalar**)&array);CHKERRQ(ierr);
    }
    ierr = PetscFree(global_indices);CHKERRQ(ierr);
  }
  ierr = MatAssemblyBegin(*M,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(*M,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  if (isdense) {
    ierr = MatSetOption(*M,MAT_ROW_ORIENTED,PETSC_TRUE);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatISGetSubassemblingPattern"
PetscErrorCode MatISGetSubassemblingPattern(Mat mat, PetscInt n_subdomains, PetscBool contiguous, IS* is_sends)
{
  Mat             subdomain_adj;
  IS              new_ranks,ranks_send_to;
  MatPartitioning partitioner;
  Mat_IS          *matis;
  PetscInt        n_neighs,*neighs,*n_shared,**shared;
  PetscInt        prank;
  PetscMPIInt     size,rank,color;
  PetscInt        *xadj,*adjncy,*oldranks;
  PetscInt        *adjncy_wgt,*v_wgt,*is_indices,*ranks_send_to_idx;
  PetscInt        i,j,local_size,threshold=0;
  PetscErrorCode  ierr;
  PetscBool       use_vwgt=PETSC_FALSE,use_square=PETSC_FALSE;
  PetscSubcomm    subcomm;

  PetscFunctionBegin;
  ierr = PetscOptionsGetBool(NULL,"-matis_partitioning_use_square",&use_square,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetBool(NULL,"-matis_partitioning_use_vwgt",&use_vwgt,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,"-matis_partitioning_threshold",&threshold,NULL);CHKERRQ(ierr);

  /* Get info on mapping */
  matis = (Mat_IS*)(mat->data);
  ierr = ISLocalToGlobalMappingGetSize(matis->mapping,&local_size);CHKERRQ(ierr);
  ierr = ISLocalToGlobalMappingGetInfo(matis->mapping,&n_neighs,&neighs,&n_shared,&shared);CHKERRQ(ierr);

  /* build local CSR graph of subdomains' connectivity */
  ierr = PetscMalloc1(2,&xadj);CHKERRQ(ierr);
  xadj[0] = 0;
  xadj[1] = PetscMax(n_neighs-1,0);
  ierr = PetscMalloc1(xadj[1],&adjncy);CHKERRQ(ierr);
  ierr = PetscMalloc1(xadj[1],&adjncy_wgt);CHKERRQ(ierr);

  if (threshold) {
    PetscInt* count,min_threshold;
    ierr = PetscMalloc1(local_size,&count);CHKERRQ(ierr);
    ierr = PetscMemzero(count,local_size*sizeof(PetscInt));CHKERRQ(ierr);
    for (i=1;i<n_neighs;i++) {/* i=1 so I don't count myself -> faces nodes counts to 1 */
      for (j=0;j<n_shared[i];j++) {
        count[shared[i][j]] += 1;
      }
    }
    /* adapt threshold since we dont want to lose significant connections */
    min_threshold = n_neighs;
    for (i=1;i<n_neighs;i++) {
      for (j=0;j<n_shared[i];j++) {
        min_threshold = PetscMin(count[shared[i][j]],min_threshold);
      }
    }
    threshold = PetscMax(min_threshold+1,threshold);
    xadj[1] = 0;
    for (i=1;i<n_neighs;i++) {
      for (j=0;j<n_shared[i];j++) {
        if (count[shared[i][j]] < threshold) {
          adjncy[xadj[1]] = neighs[i];
          adjncy_wgt[xadj[1]] = n_shared[i];
          xadj[1]++;
          break;
        }
      }
    }
    ierr = PetscFree(count);CHKERRQ(ierr);
  } else {
    if (xadj[1]) {
      ierr = PetscMemcpy(adjncy,&neighs[1],xadj[1]*sizeof(*adjncy));CHKERRQ(ierr);
      ierr = PetscMemcpy(adjncy_wgt,&n_shared[1],xadj[1]*sizeof(*adjncy_wgt));CHKERRQ(ierr);
    }
  }
  ierr = ISLocalToGlobalMappingRestoreInfo(matis->mapping,&n_neighs,&neighs,&n_shared,&shared);CHKERRQ(ierr);
  if (use_square) {
    for (i=0;i<xadj[1];i++) {
      adjncy_wgt[i] = adjncy_wgt[i]*adjncy_wgt[i];
    }
  }
  ierr = PetscSortIntWithArray(xadj[1],adjncy,adjncy_wgt);CHKERRQ(ierr);

  ierr = PetscMalloc(sizeof(PetscInt),&ranks_send_to_idx);CHKERRQ(ierr);

  /*
    Restrict work on active processes only.
  */
  ierr = PetscSubcommCreate(PetscObjectComm((PetscObject)mat),&subcomm);CHKERRQ(ierr);
  ierr = PetscSubcommSetNumber(subcomm,2);CHKERRQ(ierr); /* 2 groups, active process and not active processes */
  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)mat),&rank);CHKERRQ(ierr);
  ierr = PetscMPIIntCast(!local_size,&color);CHKERRQ(ierr);
  ierr = PetscSubcommSetTypeGeneral(subcomm,color,rank);CHKERRQ(ierr);
  if (color) {
    ierr = PetscFree(xadj);CHKERRQ(ierr);
    ierr = PetscFree(adjncy);CHKERRQ(ierr);
    ierr = PetscFree(adjncy_wgt);CHKERRQ(ierr);
  } else {
    PetscInt coarsening_ratio;
    ierr = MPI_Comm_size(subcomm->comm,&size);CHKERRQ(ierr);
    ierr = PetscMalloc1(size,&oldranks);CHKERRQ(ierr);
    prank = rank;
    ierr = MPI_Allgather(&prank,1,MPIU_INT,oldranks,1,MPIU_INT,subcomm->comm);CHKERRQ(ierr);
    /*
    for (i=0;i<size;i++) {
      PetscPrintf(subcomm->comm,"oldranks[%d] = %d\n",i,oldranks[i]);
    }
    */
    for (i=0;i<xadj[1];i++) {
      ierr = PetscFindInt(adjncy[i],size,oldranks,&adjncy[i]);CHKERRQ(ierr);
    }
    ierr = PetscSortIntWithArray(xadj[1],adjncy,adjncy_wgt);CHKERRQ(ierr);
    ierr = MatCreateMPIAdj(subcomm->comm,1,(PetscInt)size,xadj,adjncy,adjncy_wgt,&subdomain_adj);CHKERRQ(ierr);
    /* ierr = MatView(subdomain_adj,0);CHKERRQ(ierr); */

    /* Partition */
    ierr = MatPartitioningCreate(subcomm->comm,&partitioner);CHKERRQ(ierr);
    ierr = MatPartitioningSetAdjacency(partitioner,subdomain_adj);CHKERRQ(ierr);
    if (use_vwgt) {
      ierr = PetscMalloc(sizeof(*v_wgt),&v_wgt);CHKERRQ(ierr);
      v_wgt[0] = local_size;
      ierr = MatPartitioningSetVertexWeights(partitioner,v_wgt);CHKERRQ(ierr);
    }
    n_subdomains = PetscMin((PetscInt)size,n_subdomains);
    coarsening_ratio = size/n_subdomains;
    /* Parmetis does not always give back nparts with small graphs! this should be taken into account */
    ierr = MatPartitioningSetNParts(partitioner,n_subdomains);CHKERRQ(ierr);
    ierr = MatPartitioningSetFromOptions(partitioner);CHKERRQ(ierr);
    ierr = MatPartitioningApply(partitioner,&new_ranks);CHKERRQ(ierr);
    /* ierr = MatPartitioningView(partitioner,0);CHKERRQ(ierr); */

    ierr = ISGetIndices(new_ranks,(const PetscInt**)&is_indices);CHKERRQ(ierr);
    if (contiguous) {
      ranks_send_to_idx[0] = oldranks[is_indices[0]]; /* contiguos set of processes */
    } else {
      ranks_send_to_idx[0] = coarsening_ratio*oldranks[is_indices[0]]; /* scattered set of processes */
    }
    ierr = ISRestoreIndices(new_ranks,(const PetscInt**)&is_indices);CHKERRQ(ierr);
    /* clean up */
    ierr = PetscFree(oldranks);CHKERRQ(ierr);
    ierr = ISDestroy(&new_ranks);CHKERRQ(ierr);
    ierr = MatDestroy(&subdomain_adj);CHKERRQ(ierr);
    ierr = MatPartitioningDestroy(&partitioner);CHKERRQ(ierr);
  }
  ierr = PetscSubcommDestroy(&subcomm);CHKERRQ(ierr);

  /* assemble parallel IS for sends */
  i = 1;
  if (color) i=0;
  ierr = ISCreateGeneral(PetscObjectComm((PetscObject)mat),i,ranks_send_to_idx,PETSC_OWN_POINTER,&ranks_send_to);CHKERRQ(ierr);

  /* get back IS */
  *is_sends = ranks_send_to;
  PetscFunctionReturn(0);
}

typedef enum {MATDENSE_PRIVATE=0,MATAIJ_PRIVATE,MATBAIJ_PRIVATE,MATSBAIJ_PRIVATE}MatTypePrivate;

#undef __FUNCT__
#define __FUNCT__ "MatISSubassemble"
PetscErrorCode MatISSubassemble(Mat mat, IS is_sends, PetscInt n_subdomains, PetscBool restrict_comm, MatReuse reuse, Mat *mat_n, PetscInt nis, IS isarray[])
{
  Mat                    local_mat;
  Mat_IS                 *matis;
  IS                     is_sends_internal;
  PetscInt               rows,cols;
  PetscInt               i,bs,buf_size_idxs,buf_size_idxs_is,buf_size_vals;
  PetscBool              ismatis,isdense,destroy_mat;
  ISLocalToGlobalMapping l2gmap;
  PetscInt*              l2gmap_indices;
  const PetscInt*        is_indices;
  MatType                new_local_type;
  /* buffers */
  PetscInt               *ptr_idxs,*send_buffer_idxs,*recv_buffer_idxs;
  PetscInt               *ptr_idxs_is,*send_buffer_idxs_is,*recv_buffer_idxs_is;
  PetscScalar            *ptr_vals,*send_buffer_vals,*recv_buffer_vals;
  /* MPI */
  MPI_Comm               comm,comm_n;
  PetscSubcomm           subcomm;
  PetscMPIInt            n_sends,n_recvs,commsize;
  PetscMPIInt            *iflags,*ilengths_idxs,*ilengths_vals,*ilengths_idxs_is;
  PetscMPIInt            *onodes,*onodes_is,*olengths_idxs,*olengths_idxs_is,*olengths_vals;
  PetscMPIInt            len,tag_idxs,tag_idxs_is,tag_vals,source_dest;
  MPI_Request            *send_req_idxs,*send_req_idxs_is,*send_req_vals;
  MPI_Request            *recv_req_idxs,*recv_req_idxs_is,*recv_req_vals;
  PetscErrorCode         ierr;

  PetscFunctionBegin;
  /* TODO: add missing checks */
  PetscValidLogicalCollectiveInt(mat,n_subdomains,3);
  PetscValidLogicalCollectiveBool(mat,restrict_comm,4);
  PetscValidLogicalCollectiveEnum(mat,reuse,5);
  PetscValidLogicalCollectiveInt(mat,nis,7);
  ierr = PetscObjectTypeCompare((PetscObject)mat,MATIS,&ismatis);CHKERRQ(ierr);
  if (!ismatis) SETERRQ1(PetscObjectComm((PetscObject)mat),PETSC_ERR_SUP,"Cannot use %s on a matrix object which is not of type MATIS",__FUNCT__);
  ierr = MatISGetLocalMat(mat,&local_mat);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)local_mat,MATSEQDENSE,&isdense);CHKERRQ(ierr);
  if (!isdense) SETERRQ(PetscObjectComm((PetscObject)mat),PETSC_ERR_SUP,"Currently cannot subassemble MATIS when local matrix type is not of type SEQDENSE");
  ierr = MatGetSize(local_mat,&rows,&cols);CHKERRQ(ierr);
  if (rows != cols) SETERRQ(PetscObjectComm((PetscObject)mat),PETSC_ERR_SUP,"Local MATIS matrices should be square");
  if (reuse == MAT_REUSE_MATRIX && *mat_n) {
    PetscInt mrows,mcols,mnrows,mncols;
    ierr = PetscObjectTypeCompare((PetscObject)*mat_n,MATIS,&ismatis);CHKERRQ(ierr);
    if (!ismatis) SETERRQ(PetscObjectComm((PetscObject)*mat_n),PETSC_ERR_SUP,"Cannot reuse a matrix which is not of type MATIS");
    ierr = MatGetSize(mat,&mrows,&mcols);CHKERRQ(ierr);
    ierr = MatGetSize(*mat_n,&mnrows,&mncols);CHKERRQ(ierr);
    if (mrows != mnrows) SETERRQ2(PetscObjectComm((PetscObject)mat),PETSC_ERR_SUP,"Cannot reuse matrix! Wrong number of rows %D != %D",mrows,mnrows);
    if (mcols != mncols) SETERRQ2(PetscObjectComm((PetscObject)mat),PETSC_ERR_SUP,"Cannot reuse matrix! Wrong number of cols %D != %D",mcols,mncols);
  }
  ierr = MatGetBlockSize(local_mat,&bs);CHKERRQ(ierr);
  PetscValidLogicalCollectiveInt(mat,bs,0);
  /* prepare IS for sending if not provided */
  if (!is_sends) {
    PetscBool pcontig = PETSC_TRUE;
    if (!n_subdomains) SETERRQ(PetscObjectComm((PetscObject)mat),PETSC_ERR_SUP,"You should specify either an IS or a target number of subdomains");
    ierr = MatISGetSubassemblingPattern(mat,n_subdomains,pcontig,&is_sends_internal);CHKERRQ(ierr);
  } else {
    ierr = PetscObjectReference((PetscObject)is_sends);CHKERRQ(ierr);
    is_sends_internal = is_sends;
  }

  /* get pointer of MATIS data */
  matis = (Mat_IS*)mat->data;

  /* get comm */
  ierr = PetscObjectGetComm((PetscObject)mat,&comm);CHKERRQ(ierr);

  /* compute number of sends */
  ierr = ISGetLocalSize(is_sends_internal,&i);CHKERRQ(ierr);
  ierr = PetscMPIIntCast(i,&n_sends);CHKERRQ(ierr);

  /* compute number of receives */
  ierr = MPI_Comm_size(comm,&commsize);CHKERRQ(ierr);
  ierr = PetscMalloc1(commsize,&iflags);CHKERRQ(ierr);
  ierr = PetscMemzero(iflags,commsize*sizeof(*iflags));CHKERRQ(ierr);
  ierr = ISGetIndices(is_sends_internal,&is_indices);CHKERRQ(ierr);
  for (i=0;i<n_sends;i++) iflags[is_indices[i]] = 1;
  ierr = PetscGatherNumberOfMessages(comm,iflags,NULL,&n_recvs);CHKERRQ(ierr);
  ierr = PetscFree(iflags);CHKERRQ(ierr);

  /* restrict comm if requested */
  subcomm = 0;
  destroy_mat = PETSC_FALSE;
  if (restrict_comm) {
    PetscMPIInt color,rank,subcommsize;
    ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
    color = 0;
    if (n_sends && !n_recvs) color = 1; /* sending only processes will not partecipate in new comm */
    ierr = MPI_Allreduce(&color,&subcommsize,1,MPI_INT,MPI_SUM,comm);CHKERRQ(ierr);
    subcommsize = commsize - subcommsize;
    /* check if reuse has been requested */
    if (reuse == MAT_REUSE_MATRIX) {
      if (*mat_n) {
        PetscMPIInt subcommsize2;
        ierr = MPI_Comm_size(PetscObjectComm((PetscObject)*mat_n),&subcommsize2);CHKERRQ(ierr);
        if (subcommsize != subcommsize2) SETERRQ2(PetscObjectComm((PetscObject)*mat_n),PETSC_ERR_PLIB,"Cannot reuse matrix! wrong subcomm size %d != %d",subcommsize,subcommsize2);
        comm_n = PetscObjectComm((PetscObject)*mat_n);
      } else {
        comm_n = PETSC_COMM_SELF;
      }
    } else { /* MAT_INITIAL_MATRIX */
      ierr = PetscSubcommCreate(comm,&subcomm);CHKERRQ(ierr);
      ierr = PetscSubcommSetNumber(subcomm,2);CHKERRQ(ierr);
      ierr = PetscSubcommSetTypeGeneral(subcomm,color,rank);CHKERRQ(ierr);
      comm_n = subcomm->comm;
    }
    /* flag to destroy *mat_n if not significative */
    if (color) destroy_mat = PETSC_TRUE;
  } else {
    comm_n = comm;
  }

  /* prepare send/receive buffers */
  ierr = PetscMalloc1(commsize,&ilengths_idxs);CHKERRQ(ierr);
  ierr = PetscMemzero(ilengths_idxs,commsize*sizeof(*ilengths_idxs));CHKERRQ(ierr);
  ierr = PetscMalloc1(commsize,&ilengths_vals);CHKERRQ(ierr);
  ierr = PetscMemzero(ilengths_vals,commsize*sizeof(*ilengths_vals));CHKERRQ(ierr);
  if (nis) {
    ierr = PetscMalloc(commsize*sizeof(*ilengths_idxs_is),&ilengths_idxs_is);CHKERRQ(ierr);
    ierr = PetscMemzero(ilengths_idxs_is,commsize*sizeof(*ilengths_idxs_is));CHKERRQ(ierr);
  }

  /* Get data from local matrices */
  if (!isdense) {
    /* TODO: See below some guidelines on how to prepare the local buffers */
    /*
       send_buffer_vals should contain the raw values of the local matrix
       send_buffer_idxs should contain:
       - MatType_PRIVATE type
       - PetscInt        size_of_l2gmap
       - PetscInt        global_row_indices[size_of_l2gmap]
       - PetscInt        all_other_info_which_is_needed_to_compute_preallocation_and_set_values
    */
  } else {
    ierr = MatDenseGetArray(local_mat,&send_buffer_vals);CHKERRQ(ierr);
    ierr = ISLocalToGlobalMappingGetSize(matis->mapping,&i);CHKERRQ(ierr);
    ierr = PetscMalloc1((i+2),&send_buffer_idxs);CHKERRQ(ierr);
    send_buffer_idxs[0] = (PetscInt)MATDENSE_PRIVATE;
    send_buffer_idxs[1] = i;
    ierr = ISLocalToGlobalMappingGetIndices(matis->mapping,(const PetscInt**)&ptr_idxs);CHKERRQ(ierr);
    ierr = PetscMemcpy(&send_buffer_idxs[2],ptr_idxs,i*sizeof(PetscInt));CHKERRQ(ierr);
    ierr = ISLocalToGlobalMappingRestoreIndices(matis->mapping,(const PetscInt**)&ptr_idxs);CHKERRQ(ierr);
    ierr = PetscMPIIntCast(i,&len);CHKERRQ(ierr);
    for (i=0;i<n_sends;i++) {
      ilengths_vals[is_indices[i]] = len*len;
      ilengths_idxs[is_indices[i]] = len+2;
    }
  }
  ierr = PetscGatherMessageLengths2(comm,n_sends,n_recvs,ilengths_idxs,ilengths_vals,&onodes,&olengths_idxs,&olengths_vals);CHKERRQ(ierr);
  /* additional is (if any) */
  if (nis) {
    PetscMPIInt psum;
    PetscInt j;
    for (j=0,psum=0;j<nis;j++) {
      PetscInt plen;
      ierr = ISGetLocalSize(isarray[j],&plen);CHKERRQ(ierr);
      ierr = PetscMPIIntCast(plen,&len);CHKERRQ(ierr);
      psum += len+1; /* indices + lenght */
    }
    ierr = PetscMalloc(psum*sizeof(PetscInt),&send_buffer_idxs_is);CHKERRQ(ierr);
    for (j=0,psum=0;j<nis;j++) {
      PetscInt plen;
      const PetscInt *is_array_idxs;
      ierr = ISGetLocalSize(isarray[j],&plen);CHKERRQ(ierr);
      send_buffer_idxs_is[psum] = plen;
      ierr = ISGetIndices(isarray[j],&is_array_idxs);CHKERRQ(ierr);
      ierr = PetscMemcpy(&send_buffer_idxs_is[psum+1],is_array_idxs,plen*sizeof(PetscInt));CHKERRQ(ierr);
      ierr = ISRestoreIndices(isarray[j],&is_array_idxs);CHKERRQ(ierr);
      psum += plen+1; /* indices + lenght */
    }
    for (i=0;i<n_sends;i++) {
      ilengths_idxs_is[is_indices[i]] = psum;
    }
    ierr = PetscGatherMessageLengths(comm,n_sends,n_recvs,ilengths_idxs_is,&onodes_is,&olengths_idxs_is);CHKERRQ(ierr);
  }

  buf_size_idxs = 0;
  buf_size_vals = 0;
  buf_size_idxs_is = 0;
  for (i=0;i<n_recvs;i++) {
    buf_size_idxs += (PetscInt)olengths_idxs[i];
    buf_size_vals += (PetscInt)olengths_vals[i];
    if (nis) buf_size_idxs_is += (PetscInt)olengths_idxs_is[i];
  }
  ierr = PetscMalloc1(buf_size_idxs,&recv_buffer_idxs);CHKERRQ(ierr);
  ierr = PetscMalloc1(buf_size_vals,&recv_buffer_vals);CHKERRQ(ierr);
  ierr = PetscMalloc1(buf_size_idxs_is,&recv_buffer_idxs_is);CHKERRQ(ierr);

  /* get new tags for clean communications */
  ierr = PetscObjectGetNewTag((PetscObject)mat,&tag_idxs);CHKERRQ(ierr);
  ierr = PetscObjectGetNewTag((PetscObject)mat,&tag_vals);CHKERRQ(ierr);
  ierr = PetscObjectGetNewTag((PetscObject)mat,&tag_idxs_is);CHKERRQ(ierr);

  /* allocate for requests */
  ierr = PetscMalloc1(n_sends,&send_req_idxs);CHKERRQ(ierr);
  ierr = PetscMalloc1(n_sends,&send_req_vals);CHKERRQ(ierr);
  ierr = PetscMalloc1(n_sends,&send_req_idxs_is);CHKERRQ(ierr);
  ierr = PetscMalloc1(n_recvs,&recv_req_idxs);CHKERRQ(ierr);
  ierr = PetscMalloc1(n_recvs,&recv_req_vals);CHKERRQ(ierr);
  ierr = PetscMalloc1(n_recvs,&recv_req_idxs_is);CHKERRQ(ierr);

  /* communications */
  ptr_idxs = recv_buffer_idxs;
  ptr_vals = recv_buffer_vals;
  ptr_idxs_is = recv_buffer_idxs_is;
  for (i=0;i<n_recvs;i++) {
    source_dest = onodes[i];
    ierr = MPI_Irecv(ptr_idxs,olengths_idxs[i],MPIU_INT,source_dest,tag_idxs,comm,&recv_req_idxs[i]);CHKERRQ(ierr);
    ierr = MPI_Irecv(ptr_vals,olengths_vals[i],MPIU_SCALAR,source_dest,tag_vals,comm,&recv_req_vals[i]);CHKERRQ(ierr);
    ptr_idxs += olengths_idxs[i];
    ptr_vals += olengths_vals[i];
    if (nis) {
      ierr = MPI_Irecv(ptr_idxs_is,olengths_idxs_is[i],MPIU_INT,source_dest,tag_idxs_is,comm,&recv_req_idxs_is[i]);CHKERRQ(ierr);
      ptr_idxs_is += olengths_idxs_is[i];
    }
  }
  for (i=0;i<n_sends;i++) {
    ierr = PetscMPIIntCast(is_indices[i],&source_dest);CHKERRQ(ierr);
    ierr = MPI_Isend(send_buffer_idxs,ilengths_idxs[source_dest],MPIU_INT,source_dest,tag_idxs,comm,&send_req_idxs[i]);CHKERRQ(ierr);
    ierr = MPI_Isend(send_buffer_vals,ilengths_vals[source_dest],MPIU_SCALAR,source_dest,tag_vals,comm,&send_req_vals[i]);CHKERRQ(ierr);
    if (nis) {
      ierr = MPI_Isend(send_buffer_idxs_is,ilengths_idxs_is[source_dest],MPIU_INT,source_dest,tag_idxs_is,comm,&send_req_idxs_is[i]);CHKERRQ(ierr);
    }
  }
  ierr = ISRestoreIndices(is_sends_internal,&is_indices);CHKERRQ(ierr);
  ierr = ISDestroy(&is_sends_internal);CHKERRQ(ierr);

  /* assemble new l2g map */
  ierr = MPI_Waitall(n_recvs,recv_req_idxs,MPI_STATUSES_IGNORE);CHKERRQ(ierr);
  ptr_idxs = recv_buffer_idxs;
  buf_size_idxs = 0;
  for (i=0;i<n_recvs;i++) {
    buf_size_idxs += *(ptr_idxs+1); /* second element is the local size of the l2gmap */
    ptr_idxs += olengths_idxs[i];
  }
  ierr = PetscMalloc1(buf_size_idxs,&l2gmap_indices);CHKERRQ(ierr);
  ptr_idxs = recv_buffer_idxs;
  buf_size_idxs = 0;
  for (i=0;i<n_recvs;i++) {
    ierr = PetscMemcpy(&l2gmap_indices[buf_size_idxs],ptr_idxs+2,(*(ptr_idxs+1))*sizeof(PetscInt));CHKERRQ(ierr);
    buf_size_idxs += *(ptr_idxs+1); /* second element is the local size of the l2gmap */
    ptr_idxs += olengths_idxs[i];
  }
  ierr = PetscSortRemoveDupsInt(&buf_size_idxs,l2gmap_indices);CHKERRQ(ierr);
  ierr = ISLocalToGlobalMappingCreate(comm_n,1,buf_size_idxs,l2gmap_indices,PETSC_COPY_VALUES,&l2gmap);CHKERRQ(ierr);
  ierr = PetscFree(l2gmap_indices);CHKERRQ(ierr);

  /* infer new local matrix type from received local matrices type */
  /* currently if all local matrices are of type X, then the resulting matrix will be of type X, except for the dense case */
  /* it also assumes that if the block size is set, than it is the same among all local matrices (see checks at the beginning of the function) */
  if (n_recvs) {
    MatTypePrivate new_local_type_private = (MatTypePrivate)send_buffer_idxs[0];
    ptr_idxs = recv_buffer_idxs;
    for (i=0;i<n_recvs;i++) {
      if ((PetscInt)new_local_type_private != *ptr_idxs) {
        new_local_type_private = MATAIJ_PRIVATE;
        break;
      }
      ptr_idxs += olengths_idxs[i];
    }
    switch (new_local_type_private) {
      case MATDENSE_PRIVATE:
        if (n_recvs>1) { /* subassembling of dense matrices does not give a dense matrix! */
          new_local_type = MATSEQAIJ;
          bs = 1;
        } else { /* if I receive only 1 dense matrix */
          new_local_type = MATSEQDENSE;
          bs = 1;
        }
        break;
      case MATAIJ_PRIVATE:
        new_local_type = MATSEQAIJ;
        bs = 1;
        break;
      case MATBAIJ_PRIVATE:
        new_local_type = MATSEQBAIJ;
        break;
      case MATSBAIJ_PRIVATE:
        new_local_type = MATSEQSBAIJ;
        break;
      default:
        SETERRQ2(comm,PETSC_ERR_PLIB,"Unkwown private type %d in %s",new_local_type_private,__FUNCT__);
        break;
    }
  } else { /* by default, new_local_type is seqdense */
    new_local_type = MATSEQDENSE;
    bs = 1;
  }

  /* create MATIS object if needed */
  if (reuse == MAT_INITIAL_MATRIX) {
    ierr = MatGetSize(mat,&rows,&cols);CHKERRQ(ierr);
    ierr = MatCreateIS(comm_n,bs,PETSC_DECIDE,PETSC_DECIDE,rows,cols,l2gmap,mat_n);CHKERRQ(ierr);
  } else {
    /* it also destroys the local matrices */
    ierr = MatSetLocalToGlobalMapping(*mat_n,l2gmap,l2gmap);CHKERRQ(ierr);
  }
  ierr = ISLocalToGlobalMappingDestroy(&l2gmap);CHKERRQ(ierr);
  ierr = MatISGetLocalMat(*mat_n,&local_mat);CHKERRQ(ierr);
  ierr = MatSetType(local_mat,new_local_type);CHKERRQ(ierr);
  ierr = MatSetUp(local_mat);CHKERRQ(ierr); /* WARNING -> no preallocation yet */

  /* set values */
  ierr = MPI_Waitall(n_recvs,recv_req_vals,MPI_STATUSES_IGNORE);CHKERRQ(ierr);
  ptr_vals = recv_buffer_vals;
  ptr_idxs = recv_buffer_idxs;
  for (i=0;i<n_recvs;i++) {
    if (*ptr_idxs == (PetscInt)MATDENSE_PRIVATE) { /* values insertion provided for dense case only */
      ierr = MatSetOption(local_mat,MAT_ROW_ORIENTED,PETSC_FALSE);CHKERRQ(ierr);
      ierr = MatSetValues(*mat_n,*(ptr_idxs+1),ptr_idxs+2,*(ptr_idxs+1),ptr_idxs+2,ptr_vals,ADD_VALUES);CHKERRQ(ierr);
      ierr = MatAssemblyBegin(local_mat,MAT_FLUSH_ASSEMBLY);CHKERRQ(ierr);
      ierr = MatAssemblyEnd(local_mat,MAT_FLUSH_ASSEMBLY);CHKERRQ(ierr);
      ierr = MatSetOption(local_mat,MAT_ROW_ORIENTED,PETSC_TRUE);CHKERRQ(ierr);
    } else {
      /* TODO */
    }
    ptr_idxs += olengths_idxs[i];
    ptr_vals += olengths_vals[i];
  }
  ierr = MatAssemblyBegin(local_mat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(local_mat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(*mat_n,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(*mat_n,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

#if 0
  if (!restrict_comm) { /* check */
    Vec       lvec,rvec;
    PetscReal infty_error;

    ierr = MatGetVecs(mat,&rvec,&lvec);CHKERRQ(ierr);
    ierr = VecSetRandom(rvec,NULL);CHKERRQ(ierr);
    ierr = MatMult(mat,rvec,lvec);CHKERRQ(ierr);
    ierr = VecScale(lvec,-1.0);CHKERRQ(ierr);
    ierr = MatMultAdd(*mat_n,rvec,lvec,lvec);CHKERRQ(ierr);
    ierr = VecNorm(lvec,NORM_INFINITY,&infty_error);CHKERRQ(ierr);
    ierr = PetscPrintf(PetscObjectComm((PetscObject)mat),"Infinity error subassembling %1.6e\n",infty_error);
    ierr = VecDestroy(&rvec);CHKERRQ(ierr);
    ierr = VecDestroy(&lvec);CHKERRQ(ierr);
  }
#endif

  /* assemble new additional is (if any) */
  if (nis) {
    PetscInt **temp_idxs,*count_is,j,psum;

    ierr = MPI_Waitall(n_recvs,recv_req_idxs_is,MPI_STATUSES_IGNORE);CHKERRQ(ierr);
    ierr = PetscMalloc(nis*sizeof(PetscInt),&count_is);CHKERRQ(ierr);
    ierr = PetscMemzero(count_is,nis*sizeof(PetscInt));CHKERRQ(ierr);
    ptr_idxs = recv_buffer_idxs_is;
    psum = 0;
    for (i=0;i<n_recvs;i++) {
      for (j=0;j<nis;j++) {
        PetscInt plen = *(ptr_idxs); /* first element is the local size of IS's indices */
        count_is[j] += plen; /* increment counting of buffer for j-th IS */
        psum += plen;
        ptr_idxs += plen+1; /* shift pointer to received data */
      }
    }
    ierr = PetscMalloc(nis*sizeof(PetscInt*),&temp_idxs);CHKERRQ(ierr);
    ierr = PetscMalloc(psum*sizeof(PetscInt),&temp_idxs[0]);CHKERRQ(ierr);
    for (i=1;i<nis;i++) {
      temp_idxs[i] = temp_idxs[i-1]+count_is[i-1];
    }
    ierr = PetscMemzero(count_is,nis*sizeof(PetscInt));CHKERRQ(ierr);
    ptr_idxs = recv_buffer_idxs_is;
    for (i=0;i<n_recvs;i++) {
      for (j=0;j<nis;j++) {
        PetscInt plen = *(ptr_idxs); /* first element is the local size of IS's indices */
        ierr = PetscMemcpy(&temp_idxs[j][count_is[j]],ptr_idxs+1,plen*sizeof(PetscInt));CHKERRQ(ierr);
        count_is[j] += plen; /* increment starting point of buffer for j-th IS */
        ptr_idxs += plen+1; /* shift pointer to received data */
      }
    }
    for (i=0;i<nis;i++) {
      ierr = ISDestroy(&isarray[i]);CHKERRQ(ierr);
      ierr = PetscSortRemoveDupsInt(&count_is[i],temp_idxs[i]);CHKERRQ(ierr);CHKERRQ(ierr);
      ierr = ISCreateGeneral(comm_n,count_is[i],temp_idxs[i],PETSC_COPY_VALUES,&isarray[i]);CHKERRQ(ierr);
    }
    ierr = PetscFree(count_is);CHKERRQ(ierr);
    ierr = PetscFree(temp_idxs[0]);CHKERRQ(ierr);
    ierr = PetscFree(temp_idxs);CHKERRQ(ierr);
  }
  /* free workspace */
  ierr = PetscFree(recv_buffer_idxs);CHKERRQ(ierr);
  ierr = PetscFree(recv_buffer_vals);CHKERRQ(ierr);
  ierr = PetscFree(recv_buffer_idxs_is);CHKERRQ(ierr);
  ierr = MPI_Waitall(n_sends,send_req_idxs,MPI_STATUSES_IGNORE);CHKERRQ(ierr);
  ierr = PetscFree(send_buffer_idxs);CHKERRQ(ierr);
  ierr = MPI_Waitall(n_sends,send_req_vals,MPI_STATUSES_IGNORE);CHKERRQ(ierr);
  if (isdense) {
    ierr = MatISGetLocalMat(mat,&local_mat);CHKERRQ(ierr);
    ierr = MatDenseRestoreArray(local_mat,&send_buffer_vals);CHKERRQ(ierr);
  } else {
    /* ierr = PetscFree(send_buffer_vals);CHKERRQ(ierr); */
  }
  if (nis) {
    ierr = MPI_Waitall(n_sends,send_req_idxs_is,MPI_STATUSES_IGNORE);CHKERRQ(ierr);
    ierr = PetscFree(send_buffer_idxs_is);CHKERRQ(ierr);
  }
  ierr = PetscFree(recv_req_idxs);CHKERRQ(ierr);
  ierr = PetscFree(recv_req_vals);CHKERRQ(ierr);
  ierr = PetscFree(recv_req_idxs_is);CHKERRQ(ierr);
  ierr = PetscFree(send_req_idxs);CHKERRQ(ierr);
  ierr = PetscFree(send_req_vals);CHKERRQ(ierr);
  ierr = PetscFree(send_req_idxs_is);CHKERRQ(ierr);
  ierr = PetscFree(ilengths_vals);CHKERRQ(ierr);
  ierr = PetscFree(ilengths_idxs);CHKERRQ(ierr);
  ierr = PetscFree(olengths_vals);CHKERRQ(ierr);
  ierr = PetscFree(olengths_idxs);CHKERRQ(ierr);
  ierr = PetscFree(onodes);CHKERRQ(ierr);
  if (nis) {
    ierr = PetscFree(ilengths_idxs_is);CHKERRQ(ierr);
    ierr = PetscFree(olengths_idxs_is);CHKERRQ(ierr);
    ierr = PetscFree(onodes_is);CHKERRQ(ierr);
  }
  ierr = PetscSubcommDestroy(&subcomm);CHKERRQ(ierr);
  if (destroy_mat) { /* destroy mat is true only if restrict comm is true and process will not partecipate */
    ierr = MatDestroy(mat_n);CHKERRQ(ierr);
    for (i=0;i<nis;i++) {
      ierr = ISDestroy(&isarray[i]);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

/* temporary hack into ksp private data structure */
#include <petsc-private/kspimpl.h>

#undef __FUNCT__
#define __FUNCT__ "PCBDDCSetUpCoarseSolver"
PetscErrorCode PCBDDCSetUpCoarseSolver(PC pc,PetscScalar* coarse_submat_vals)
{
  PC_BDDC                *pcbddc = (PC_BDDC*)pc->data;
  PC_IS                  *pcis = (PC_IS*)pc->data;
  Mat                    coarse_mat,coarse_mat_is,coarse_submat_dense;
  MatNullSpace           CoarseNullSpace=NULL;
  ISLocalToGlobalMapping coarse_islg;
  IS                     coarse_is,*isarray;
  PetscInt               i,im_active=-1,active_procs=-1;
  PetscInt               nis,nisdofs,nisneu;
  PC                     pc_temp;
  PCType                 coarse_pc_type;
  KSPType                coarse_ksp_type;
  PetscBool              multilevel_requested,multilevel_allowed;
  PetscBool              isredundant,isbddc,isnn,coarse_reuse;
  Mat                    t_coarse_mat_is;
  PetscInt               void_procs,ncoarse_ml,ncoarse_ds,ncoarse;
  PetscMPIInt            all_procs;
  PetscBool              csin_ml,csin_ds,csin,csin_type_simple;
  PetscBool              compute_vecs = PETSC_FALSE;
  PetscErrorCode         ierr;

  PetscFunctionBegin;
  /* Assign global numbering to coarse dofs */
  if (pcbddc->new_primal_space || pcbddc->coarse_size == -1) { /* a new primal space is present or it is the first initialization, so recompute global numbering */
    compute_vecs = PETSC_TRUE;
    PetscInt ocoarse_size;
    ocoarse_size = pcbddc->coarse_size;
    ierr = PetscFree(pcbddc->global_primal_indices);CHKERRQ(ierr);
    ierr = PCBDDCComputePrimalNumbering(pc,&pcbddc->coarse_size,&pcbddc->global_primal_indices);CHKERRQ(ierr);
    /* see if we can avoid some work */
    if (pcbddc->coarse_ksp) { /* coarse ksp has already been created */
      if (ocoarse_size != pcbddc->coarse_size) { /* ...but with different size, so reset it and set reuse flag to false */
        ierr = KSPReset(pcbddc->coarse_ksp);CHKERRQ(ierr);
        coarse_reuse = PETSC_FALSE;
      } else { /* we can safely reuse already computed coarse matrix */
        coarse_reuse = PETSC_TRUE;
      }
    } else { /* there's no coarse ksp, so we need to create the coarse matrix too */
      coarse_reuse = PETSC_FALSE;
    }
    /* reset any subassembling information */
    ierr = ISDestroy(&pcbddc->coarse_subassembling);CHKERRQ(ierr);
    ierr = ISDestroy(&pcbddc->coarse_subassembling_init);CHKERRQ(ierr);
  } else { /* primal space is unchanged, so we can reuse coarse matrix */
    coarse_reuse = PETSC_TRUE;
  }

  /* count "active" (i.e. with positive local size) and "void" processes */
  im_active = !!(pcis->n);
  ierr = MPI_Allreduce(&im_active,&active_procs,1,MPIU_INT,MPI_SUM,PetscObjectComm((PetscObject)pc));CHKERRQ(ierr);
  ierr = MPI_Comm_size(PetscObjectComm((PetscObject)pc),&all_procs);CHKERRQ(ierr);
  void_procs = all_procs-active_procs;
  csin_type_simple = PETSC_TRUE;
  if (pcbddc->current_level) {
    csin_ml = PETSC_TRUE;
    ncoarse_ml = void_procs;
    csin_ds = PETSC_TRUE;
    ncoarse_ds = void_procs;
    if (!void_procs) SETERRQ(PetscObjectComm((PetscObject)pc),PETSC_ERR_PLIB,"This should not happen");
  } else {
    csin_ml = PETSC_FALSE;
    ncoarse_ml = all_procs;
    if (void_procs) {
      csin_ds = PETSC_TRUE;
      ncoarse_ds = void_procs;
      csin_type_simple = PETSC_FALSE;
    } else {
      csin_ds = PETSC_FALSE;
      ncoarse_ds = all_procs;
    }
  }

  /*
    test if we can go multilevel: three conditions must be satisfied:
    - we have not exceeded the number of levels requested
    - we can actually subassemble the active processes
    - we can find a suitable number of MPI processes where we can place the subassembled problem
  */
  multilevel_allowed = PETSC_FALSE;
  multilevel_requested = PETSC_FALSE;
  if (pcbddc->current_level < pcbddc->max_levels) {
    multilevel_requested = PETSC_TRUE;
    if (active_procs/pcbddc->coarsening_ratio < 2 || ncoarse_ml/pcbddc->coarsening_ratio < 2) {
      multilevel_allowed = PETSC_FALSE;
    } else {
      multilevel_allowed = PETSC_TRUE;
    }
  }
  /* determine number of process partecipating to coarse solver */
  if (multilevel_allowed) {
    ncoarse = ncoarse_ml;
    csin = csin_ml;
  } else {
    ncoarse = ncoarse_ds;
    csin = csin_ds;
  }

  /* creates temporary l2gmap and IS for coarse indexes */
  ierr = ISCreateGeneral(PetscObjectComm((PetscObject)pc),pcbddc->local_primal_size,pcbddc->global_primal_indices,PETSC_COPY_VALUES,&coarse_is);CHKERRQ(ierr);
  ierr = ISLocalToGlobalMappingCreateIS(coarse_is,&coarse_islg);CHKERRQ(ierr);

  /* creates temporary MATIS object for coarse matrix */
  ierr = MatCreateSeqDense(PETSC_COMM_SELF,pcbddc->local_primal_size,pcbddc->local_primal_size,coarse_submat_vals,&coarse_submat_dense);CHKERRQ(ierr);
#if 0
  {
    PetscViewer viewer;
    char filename[256];
    sprintf(filename,"local_coarse_mat%d.m",PetscGlobalRank);
    ierr = PetscViewerASCIIOpen(PETSC_COMM_SELF,filename,&viewer);CHKERRQ(ierr);
    ierr = PetscViewerSetFormat(viewer,PETSC_VIEWER_ASCII_MATLAB);CHKERRQ(ierr);
    ierr = MatView(coarse_submat_dense,viewer);CHKERRQ(ierr);
    ierr = PetscViewerDestroy(&viewer);CHKERRQ(ierr);
  }
#endif
  ierr = MatCreateIS(PetscObjectComm((PetscObject)pc),1,PETSC_DECIDE,PETSC_DECIDE,pcbddc->coarse_size,pcbddc->coarse_size,coarse_islg,&t_coarse_mat_is);CHKERRQ(ierr);
  ierr = MatISSetLocalMat(t_coarse_mat_is,coarse_submat_dense);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(t_coarse_mat_is,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(t_coarse_mat_is,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatDestroy(&coarse_submat_dense);CHKERRQ(ierr);

  /* compute dofs splitting and neumann boundaries for coarse dofs */
  if (multilevel_allowed && (pcbddc->n_ISForDofsLocal || pcbddc->NeumannBoundariesLocal) ) { /* protects from unneded computations */
    PetscInt               *tidxs,*tidxs2,nout,tsize,i;
    const PetscInt         *idxs;
    ISLocalToGlobalMapping tmap;

    /* create map between primal indices (in local representative ordering) and local primal numbering */
    ierr = ISLocalToGlobalMappingCreate(PETSC_COMM_SELF,1,pcbddc->local_primal_size,pcbddc->primal_indices_local_idxs,PETSC_COPY_VALUES,&tmap);CHKERRQ(ierr);
    /* allocate space for temporary storage */
    ierr = PetscMalloc(pcbddc->local_primal_size*sizeof(PetscInt),&tidxs);CHKERRQ(ierr);
    ierr = PetscMalloc(pcbddc->local_primal_size*sizeof(PetscInt),&tidxs2);CHKERRQ(ierr);
    /* allocate for IS array */
    nisdofs = pcbddc->n_ISForDofsLocal;
    nisneu = !!pcbddc->NeumannBoundariesLocal;
    nis = nisdofs + nisneu;
    ierr = PetscMalloc(nis*sizeof(IS),&isarray);CHKERRQ(ierr);
    /* dofs splitting */
    for (i=0;i<nisdofs;i++) {
      /* ierr = ISView(pcbddc->ISForDofsLocal[i],0);CHKERRQ(ierr); */
      ierr = ISGetLocalSize(pcbddc->ISForDofsLocal[i],&tsize);CHKERRQ(ierr);
      ierr = ISGetIndices(pcbddc->ISForDofsLocal[i],&idxs);CHKERRQ(ierr);
      ierr = ISGlobalToLocalMappingApply(tmap,IS_GTOLM_DROP,tsize,idxs,&nout,tidxs);CHKERRQ(ierr);
      ierr = ISRestoreIndices(pcbddc->ISForDofsLocal[i],&idxs);CHKERRQ(ierr);
      ierr = ISLocalToGlobalMappingApply(coarse_islg,nout,tidxs,tidxs2);CHKERRQ(ierr);
      ierr = ISCreateGeneral(PetscObjectComm((PetscObject)pcbddc->ISForDofsLocal[i]),nout,tidxs2,PETSC_COPY_VALUES,&isarray[i]);CHKERRQ(ierr);
      /* ierr = ISView(isarray[i],0);CHKERRQ(ierr); */
    }
    /* neumann boundaries */
    if (pcbddc->NeumannBoundariesLocal) {
      /* ierr = ISView(pcbddc->NeumannBoundariesLocal,0);CHKERRQ(ierr); */
      ierr = ISGetLocalSize(pcbddc->NeumannBoundariesLocal,&tsize);CHKERRQ(ierr);
      ierr = ISGetIndices(pcbddc->NeumannBoundariesLocal,&idxs);CHKERRQ(ierr);
      ierr = ISGlobalToLocalMappingApply(tmap,IS_GTOLM_DROP,tsize,idxs,&nout,tidxs);CHKERRQ(ierr);
      ierr = ISRestoreIndices(pcbddc->NeumannBoundariesLocal,&idxs);CHKERRQ(ierr);
      ierr = ISLocalToGlobalMappingApply(coarse_islg,nout,tidxs,tidxs2);CHKERRQ(ierr);
      ierr = ISCreateGeneral(PetscObjectComm((PetscObject)pcbddc->NeumannBoundariesLocal),nout,tidxs2,PETSC_COPY_VALUES,&isarray[nisdofs]);CHKERRQ(ierr);
      /* ierr = ISView(isarray[nisdofs],0);CHKERRQ(ierr); */
    }
    /* free memory */
    ierr = PetscFree(tidxs);CHKERRQ(ierr);
    ierr = PetscFree(tidxs2);CHKERRQ(ierr);
    ierr = ISLocalToGlobalMappingDestroy(&tmap);CHKERRQ(ierr);
  } else {
    nis = 0;
    nisdofs = 0;
    nisneu = 0;
    isarray = NULL;
  }
  /* destroy no longer needed map */
  ierr = ISLocalToGlobalMappingDestroy(&coarse_islg);CHKERRQ(ierr);

  /* restrict on coarse candidates (if needed) */
  coarse_mat_is = NULL;
  if (csin) {
    if (!pcbddc->coarse_subassembling_init ) { /* creates subassembling init pattern if not present */
      PetscInt j,tissize,*nisindices;
      PetscInt *coarse_candidates;
      const PetscInt* tisindices;
      /* get coarse candidates' ranks in pc communicator */
      ierr = PetscMalloc(all_procs*sizeof(PetscInt),&coarse_candidates);CHKERRQ(ierr);
      ierr = MPI_Allgather(&im_active,1,MPIU_INT,coarse_candidates,1,MPIU_INT,PetscObjectComm((PetscObject)pc));CHKERRQ(ierr);
      for (i=0,j=0;i<all_procs;i++) {
        if (!coarse_candidates[i]) {
          coarse_candidates[j]=i;
          j++;
        }
      }
      if (j < ncoarse) SETERRQ2(PetscObjectComm((PetscObject)pc),PETSC_ERR_PLIB,"This should not happen! %d < %d",j,ncoarse);
      /* get a suitable subassembling pattern */
      if (csin_type_simple) {
        PetscMPIInt rank;
        PetscInt    issize,isidx;
        ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)pc),&rank);CHKERRQ(ierr);
        if (im_active) {
          issize = 1;
          isidx = (PetscInt)rank;
        } else {
          issize = 0;
          isidx = -1;
        }
        ierr = ISCreateGeneral(PetscObjectComm((PetscObject)pc),issize,&isidx,PETSC_COPY_VALUES,&pcbddc->coarse_subassembling_init);CHKERRQ(ierr);
      } else {
        ierr = MatISGetSubassemblingPattern(t_coarse_mat_is,ncoarse,PETSC_TRUE,&pcbddc->coarse_subassembling_init);CHKERRQ(ierr);
      }
      if (pcbddc->dbg_flag) {
        ierr = PetscViewerASCIIPrintf(pcbddc->dbg_viewer,"--------------------------------------------------\n");CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(pcbddc->dbg_viewer,"Subassembling pattern init (before shift)\n");CHKERRQ(ierr);
        ierr = ISView(pcbddc->coarse_subassembling_init,pcbddc->dbg_viewer);CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(pcbddc->dbg_viewer,"Coarse candidates\n");CHKERRQ(ierr);
        for (i=0;i<j;i++) {
          ierr = PetscViewerASCIIPrintf(pcbddc->dbg_viewer,"%d ",coarse_candidates[i]);CHKERRQ(ierr);
        }
        ierr = PetscViewerASCIIPrintf(pcbddc->dbg_viewer,"\n");CHKERRQ(ierr);
        ierr = PetscViewerFlush(pcbddc->dbg_viewer);CHKERRQ(ierr);
      }
      /* shift the pattern on coarse candidates */
      ierr = ISGetLocalSize(pcbddc->coarse_subassembling_init,&tissize);CHKERRQ(ierr);
      ierr = ISGetIndices(pcbddc->coarse_subassembling_init,&tisindices);CHKERRQ(ierr);
      ierr = PetscMalloc(tissize*sizeof(PetscInt),&nisindices);CHKERRQ(ierr);
      for (i=0;i<tissize;i++) nisindices[i] = coarse_candidates[tisindices[i]];
      ierr = ISRestoreIndices(pcbddc->coarse_subassembling_init,&tisindices);CHKERRQ(ierr);
      ierr = ISGeneralSetIndices(pcbddc->coarse_subassembling_init,tissize,nisindices,PETSC_OWN_POINTER);CHKERRQ(ierr);
      ierr = PetscFree(coarse_candidates);CHKERRQ(ierr);
    }
    if (pcbddc->dbg_flag) {
      ierr = PetscViewerASCIIPrintf(pcbddc->dbg_viewer,"--------------------------------------------------\n");CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(pcbddc->dbg_viewer,"Subassembling pattern init\n");CHKERRQ(ierr);
      ierr = ISView(pcbddc->coarse_subassembling_init,pcbddc->dbg_viewer);CHKERRQ(ierr);
      ierr = PetscViewerFlush(pcbddc->dbg_viewer);CHKERRQ(ierr);
    }
    /* get temporary coarse mat in IS format restricted on coarse procs (plus additional index sets of isarray) */
    ierr = MatISSubassemble(t_coarse_mat_is,pcbddc->coarse_subassembling_init,0,PETSC_TRUE,MAT_INITIAL_MATRIX,&coarse_mat_is,nis,isarray);CHKERRQ(ierr);
  } else {
    if (pcbddc->dbg_flag) {
      ierr = PetscViewerASCIIPrintf(pcbddc->dbg_viewer,"--------------------------------------------------\n");CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(pcbddc->dbg_viewer,"Subassembling pattern init not needed\n");CHKERRQ(ierr);
      ierr = PetscViewerFlush(pcbddc->dbg_viewer);CHKERRQ(ierr);
    }
    ierr = PetscObjectReference((PetscObject)t_coarse_mat_is);CHKERRQ(ierr);
    coarse_mat_is = t_coarse_mat_is;
  }

  /* create local to global scatters for coarse problem */
  if (compute_vecs) {
    PetscInt lrows;
    ierr = VecDestroy(&pcbddc->coarse_vec);CHKERRQ(ierr);
    if (coarse_mat_is) {
      ierr = MatGetLocalSize(coarse_mat_is,&lrows,NULL);CHKERRQ(ierr);
    } else {
      lrows = 0;
    }
    ierr = VecCreate(PetscObjectComm((PetscObject)pc),&pcbddc->coarse_vec);CHKERRQ(ierr);
    ierr = VecSetSizes(pcbddc->coarse_vec,lrows,PETSC_DECIDE);CHKERRQ(ierr);
    ierr = VecSetType(pcbddc->coarse_vec,VECSTANDARD);CHKERRQ(ierr);
    ierr = VecScatterDestroy(&pcbddc->coarse_loc_to_glob);CHKERRQ(ierr);
    ierr = VecScatterCreate(pcbddc->vec1_P,NULL,pcbddc->coarse_vec,coarse_is,&pcbddc->coarse_loc_to_glob);CHKERRQ(ierr);
  }
  ierr = ISDestroy(&coarse_is);CHKERRQ(ierr);
  ierr = MatDestroy(&t_coarse_mat_is);CHKERRQ(ierr);

  /* set defaults for coarse KSP and PC */
  if (multilevel_allowed) {
    coarse_ksp_type = KSPRICHARDSON;
    coarse_pc_type = PCBDDC;
  } else {
    coarse_ksp_type = KSPPREONLY;
    coarse_pc_type = PCREDUNDANT;
  }

  /* print some info if requested */
  if (pcbddc->dbg_flag) {
    if (!multilevel_allowed) {
      ierr = PetscViewerASCIIPrintf(pcbddc->dbg_viewer,"--------------------------------------------------\n");CHKERRQ(ierr);
      if (multilevel_requested) {
        ierr = PetscViewerASCIIPrintf(pcbddc->dbg_viewer,"Not enough active processes on level %d (active processes %d, coarsening ratio %d)\n",pcbddc->current_level,active_procs,pcbddc->coarsening_ratio);CHKERRQ(ierr);
      } else if (pcbddc->max_levels) {
        ierr = PetscViewerASCIIPrintf(pcbddc->dbg_viewer,"Maximum number of requested levels reached (%d)\n",pcbddc->max_levels);CHKERRQ(ierr);
      }
      ierr = PetscViewerFlush(pcbddc->dbg_viewer);CHKERRQ(ierr);
    }
  }

  /* create the coarse KSP object only once with defaults */
  if (coarse_mat_is) {
    MatReuse coarse_mat_reuse;
    PetscViewer dbg_viewer = NULL;
    if (pcbddc->dbg_flag) {
      dbg_viewer = PETSC_VIEWER_STDOUT_(PetscObjectComm((PetscObject)coarse_mat_is));
      ierr = PetscViewerASCIIAddTab(dbg_viewer,2*pcbddc->current_level);CHKERRQ(ierr);
    }
    if (!pcbddc->coarse_ksp) {
      char prefix[256],str_level[16];
      size_t len;
      ierr = KSPCreate(PetscObjectComm((PetscObject)coarse_mat_is),&pcbddc->coarse_ksp);CHKERRQ(ierr);
      ierr = PetscObjectIncrementTabLevel((PetscObject)pcbddc->coarse_ksp,(PetscObject)pc,1);CHKERRQ(ierr);
      ierr = KSPSetTolerances(pcbddc->coarse_ksp,PETSC_DEFAULT,PETSC_DEFAULT,PETSC_DEFAULT,1);CHKERRQ(ierr);
      ierr = KSPSetOperators(pcbddc->coarse_ksp,coarse_mat_is,coarse_mat_is);CHKERRQ(ierr);
      ierr = KSPSetType(pcbddc->coarse_ksp,coarse_ksp_type);CHKERRQ(ierr);
      ierr = KSPSetNormType(pcbddc->coarse_ksp,KSP_NORM_NONE);CHKERRQ(ierr);
      ierr = KSPGetPC(pcbddc->coarse_ksp,&pc_temp);CHKERRQ(ierr);
      ierr = PCSetType(pc_temp,coarse_pc_type);CHKERRQ(ierr);
      /* prefix */
      ierr = PetscStrcpy(prefix,"");CHKERRQ(ierr);
      ierr = PetscStrcpy(str_level,"");CHKERRQ(ierr);
      if (!pcbddc->current_level) {
        ierr = PetscStrcpy(prefix,((PetscObject)pc)->prefix);CHKERRQ(ierr);
        ierr = PetscStrcat(prefix,"pc_bddc_coarse_");CHKERRQ(ierr);
      } else {
        ierr = PetscStrlen(((PetscObject)pc)->prefix,&len);CHKERRQ(ierr);
        if (pcbddc->current_level>1) len -= 3; /* remove "lX_" with X level number */
        if (pcbddc->current_level>10) len -= 1; /* remove another char from level number */
        ierr = PetscStrncpy(prefix,((PetscObject)pc)->prefix,len+1);CHKERRQ(ierr);
        sprintf(str_level,"l%d_",(int)(pcbddc->current_level));
        ierr = PetscStrcat(prefix,str_level);CHKERRQ(ierr);
      }
      ierr = KSPSetOptionsPrefix(pcbddc->coarse_ksp,prefix);CHKERRQ(ierr);
      /* allow user customization */
      ierr = KSPSetFromOptions(pcbddc->coarse_ksp);CHKERRQ(ierr);
      ierr = PCFactorSetReuseFill(pc_temp,PETSC_TRUE);CHKERRQ(ierr);
    }

    /* get some info after set from options */
    ierr = KSPGetPC(pcbddc->coarse_ksp,&pc_temp);CHKERRQ(ierr);
    ierr = PetscObjectTypeCompare((PetscObject)pc_temp,PCNN,&isnn);CHKERRQ(ierr);
    ierr = PetscObjectTypeCompare((PetscObject)pc_temp,PCBDDC,&isbddc);CHKERRQ(ierr);
    ierr = PetscObjectTypeCompare((PetscObject)pc_temp,PCREDUNDANT,&isredundant);CHKERRQ(ierr);
    if (isbddc && !multilevel_allowed) { /* multilevel can only be requested via pc_bddc_set_levels */
      ierr = PCSetType(pc_temp,coarse_pc_type);CHKERRQ(ierr);
      isbddc = PETSC_FALSE;
    }
    if (isredundant) {
      KSP inner_ksp;
      PC inner_pc;
      ierr = PCRedundantGetKSP(pc_temp,&inner_ksp);CHKERRQ(ierr);
      ierr = KSPGetPC(inner_ksp,&inner_pc);CHKERRQ(ierr);
      ierr = PCFactorSetReuseFill(inner_pc,PETSC_TRUE);CHKERRQ(ierr);
    }

    /* propagate BDDC info to the next level (these are dummy calls if pc_temp is not of type PCBDDC) */
    ierr = PCBDDCSetLevel(pc_temp,pcbddc->current_level+1);CHKERRQ(ierr);
    ierr = PCBDDCSetCoarseningRatio(pc_temp,pcbddc->coarsening_ratio);CHKERRQ(ierr);
    ierr = PCBDDCSetLevels(pc_temp,pcbddc->max_levels);CHKERRQ(ierr);
    if (nisdofs) {
      ierr = PCBDDCSetDofsSplitting(pc_temp,nisdofs,isarray);CHKERRQ(ierr);
      for (i=0;i<nisdofs;i++) {
        ierr = ISDestroy(&isarray[i]);CHKERRQ(ierr);
      }
    }
    if (nisneu) {
      ierr = PCBDDCSetNeumannBoundaries(pc_temp,isarray[nisdofs]);CHKERRQ(ierr);
      ierr = ISDestroy(&isarray[nisdofs]);CHKERRQ(ierr);
    }

    /* assemble coarse matrix */
    if (coarse_reuse) {
      ierr = KSPGetOperators(pcbddc->coarse_ksp,&coarse_mat,NULL);CHKERRQ(ierr);
      ierr = PetscObjectReference((PetscObject)coarse_mat);CHKERRQ(ierr);
      coarse_mat_reuse = MAT_REUSE_MATRIX;
    } else {
      coarse_mat_reuse = MAT_INITIAL_MATRIX;
    }
    if (isbddc || isnn) {
      if (!pcbddc->coarse_subassembling) { /* subassembling info is not present */
        ierr = MatISGetSubassemblingPattern(coarse_mat_is,active_procs/pcbddc->coarsening_ratio,PETSC_TRUE,&pcbddc->coarse_subassembling);CHKERRQ(ierr);
        if (pcbddc->dbg_flag) {
          ierr = PetscViewerASCIIPrintf(dbg_viewer,"--------------------------------------------------\n");CHKERRQ(ierr);
          ierr = PetscViewerASCIIPrintf(dbg_viewer,"Subassembling pattern\n");CHKERRQ(ierr);
          ierr = ISView(pcbddc->coarse_subassembling,dbg_viewer);CHKERRQ(ierr);
          ierr = PetscViewerFlush(dbg_viewer);CHKERRQ(ierr);
        }
      }
      ierr = MatISSubassemble(coarse_mat_is,pcbddc->coarse_subassembling,0,PETSC_FALSE,coarse_mat_reuse,&coarse_mat,0,NULL);CHKERRQ(ierr);
    } else {
      ierr = MatISGetMPIXAIJ(coarse_mat_is,MATMPIAIJ,coarse_mat_reuse,&coarse_mat);CHKERRQ(ierr);
    }
    ierr = MatDestroy(&coarse_mat_is);CHKERRQ(ierr);

    /* propagate symmetry info to coarse matrix */
    ierr = MatSetOption(coarse_mat,MAT_SYMMETRIC,pcbddc->issym);CHKERRQ(ierr);
    ierr = MatSetOption(coarse_mat,MAT_STRUCTURALLY_SYMMETRIC,PETSC_TRUE);CHKERRQ(ierr);

    /* set operators */
    ierr = KSPSetOperators(pcbddc->coarse_ksp,coarse_mat,coarse_mat);CHKERRQ(ierr);
    if (pcbddc->dbg_flag) {
      ierr = PetscViewerASCIISubtractTab(dbg_viewer,2*pcbddc->current_level);CHKERRQ(ierr);
    }
  } else { /* processes non partecipating to coarse solver (if any) */
    coarse_mat = 0;
  }
  ierr = PetscFree(isarray);CHKERRQ(ierr);
#if 0
  {
    PetscViewer viewer;
    char filename[256];
    sprintf(filename,"coarse_mat.m");
    ierr = PetscViewerASCIIOpen(PETSC_COMM_WORLD,filename,&viewer);CHKERRQ(ierr);
    ierr = PetscViewerSetFormat(viewer,PETSC_VIEWER_ASCII_MATLAB);CHKERRQ(ierr);
    ierr = MatView(coarse_mat,viewer);CHKERRQ(ierr);
    ierr = PetscViewerDestroy(&viewer);CHKERRQ(ierr);
  }
#endif

  /* Compute coarse null space (special handling by BDDC only) */
  if (pcbddc->NullSpace) {
    ierr = PCBDDCNullSpaceAssembleCoarse(pc,coarse_mat,&CoarseNullSpace);CHKERRQ(ierr);
  }

  if (pcbddc->coarse_ksp) {
    Vec crhs,csol;
    PetscBool ispreonly;
    if (CoarseNullSpace) {
      if (isbddc) {
        ierr = PCBDDCSetNullSpace(pc_temp,CoarseNullSpace);CHKERRQ(ierr);
      } else {
        ierr = KSPSetNullSpace(pcbddc->coarse_ksp,CoarseNullSpace);CHKERRQ(ierr);
      }
    }
    /* setup coarse ksp */
    ierr = KSPSetUp(pcbddc->coarse_ksp);CHKERRQ(ierr);
    ierr = KSPGetSolution(pcbddc->coarse_ksp,&csol);CHKERRQ(ierr);
    ierr = KSPGetRhs(pcbddc->coarse_ksp,&crhs);CHKERRQ(ierr);
    /* hack */
    if (!csol) {
      ierr = MatGetVecs(coarse_mat,&((pcbddc->coarse_ksp)->vec_sol),NULL);CHKERRQ(ierr);
    }
    if (!crhs) {
      ierr = MatGetVecs(coarse_mat,NULL,&((pcbddc->coarse_ksp)->vec_rhs));CHKERRQ(ierr);
    }
    /* Check coarse problem if in debug mode or if solving with an iterative method */
    ierr = PetscObjectTypeCompare((PetscObject)pcbddc->coarse_ksp,KSPPREONLY,&ispreonly);CHKERRQ(ierr);
    if (pcbddc->dbg_flag || (!ispreonly && pcbddc->use_coarse_estimates) ) {
      KSP       check_ksp;
      KSPType   check_ksp_type;
      PC        check_pc;
      Vec       check_vec,coarse_vec;
      PetscReal abs_infty_error,infty_error,lambda_min=1.0,lambda_max=1.0;
      PetscInt  its;
      PetscBool compute_eigs;
      PetscReal *eigs_r,*eigs_c;
      PetscInt  neigs;
      const char *prefix;

      /* Create ksp object suitable for estimation of extreme eigenvalues */
      ierr = KSPCreate(PetscObjectComm((PetscObject)pcbddc->coarse_ksp),&check_ksp);CHKERRQ(ierr);
      ierr = KSPSetOperators(check_ksp,coarse_mat,coarse_mat);CHKERRQ(ierr);
      ierr = KSPSetTolerances(check_ksp,1.e-12,1.e-12,PETSC_DEFAULT,pcbddc->coarse_size);CHKERRQ(ierr);
      if (ispreonly) {
        check_ksp_type = KSPPREONLY;
        compute_eigs = PETSC_FALSE;
      } else {
        check_ksp_type = KSPGMRES;
        compute_eigs = PETSC_TRUE;
      }
      ierr = KSPSetType(check_ksp,check_ksp_type);CHKERRQ(ierr);
      ierr = KSPSetComputeSingularValues(check_ksp,compute_eigs);CHKERRQ(ierr);
      ierr = KSPSetComputeEigenvalues(check_ksp,compute_eigs);CHKERRQ(ierr);
      ierr = KSPGMRESSetRestart(check_ksp,pcbddc->coarse_size+1);CHKERRQ(ierr);
      ierr = KSPGetOptionsPrefix(pcbddc->coarse_ksp,&prefix);CHKERRQ(ierr);
      ierr = KSPSetOptionsPrefix(check_ksp,prefix);CHKERRQ(ierr);
      ierr = KSPAppendOptionsPrefix(check_ksp,"check_");CHKERRQ(ierr);
      ierr = KSPSetFromOptions(check_ksp);CHKERRQ(ierr);
      ierr = KSPSetUp(check_ksp);CHKERRQ(ierr);
      ierr = KSPGetPC(pcbddc->coarse_ksp,&check_pc);CHKERRQ(ierr);
      ierr = KSPSetPC(check_ksp,check_pc);CHKERRQ(ierr);
      /* create random vec */
      ierr = KSPGetSolution(pcbddc->coarse_ksp,&coarse_vec);CHKERRQ(ierr);
      ierr = VecDuplicate(coarse_vec,&check_vec);CHKERRQ(ierr);
      ierr = VecSetRandom(check_vec,NULL);CHKERRQ(ierr);
      if (CoarseNullSpace) {
        ierr = MatNullSpaceRemove(CoarseNullSpace,check_vec);CHKERRQ(ierr);
      }
      ierr = MatMult(coarse_mat,check_vec,coarse_vec);CHKERRQ(ierr);
      /* solve coarse problem */
      ierr = KSPSolve(check_ksp,coarse_vec,coarse_vec);CHKERRQ(ierr);
      if (CoarseNullSpace) {
        ierr = MatNullSpaceRemove(CoarseNullSpace,coarse_vec);CHKERRQ(ierr);
      }
      /* set eigenvalue estimation if preonly has not been requested */
      if (compute_eigs) {
        ierr = PetscMalloc((pcbddc->coarse_size+1)*sizeof(PetscReal),&eigs_r);CHKERRQ(ierr);
        ierr = PetscMalloc((pcbddc->coarse_size+1)*sizeof(PetscReal),&eigs_c);CHKERRQ(ierr);
        ierr = KSPComputeEigenvalues(check_ksp,pcbddc->coarse_size+1,eigs_r,eigs_c,&neigs);CHKERRQ(ierr);
        lambda_max = eigs_r[neigs-1];
        lambda_min = eigs_r[0];
        if (pcbddc->use_coarse_estimates) {
          if (lambda_max>lambda_min) {
            ierr = KSPChebyshevSetEigenvalues(pcbddc->coarse_ksp,lambda_max,lambda_min);CHKERRQ(ierr);
            ierr = KSPRichardsonSetScale(pcbddc->coarse_ksp,2.0/(lambda_max+lambda_min));CHKERRQ(ierr);
          }
        }
      }

      /* check coarse problem residual error */
      if (pcbddc->dbg_flag) {
        PetscViewer dbg_viewer = PETSC_VIEWER_STDOUT_(PetscObjectComm((PetscObject)pcbddc->coarse_ksp));
        ierr = PetscViewerASCIIAddTab(dbg_viewer,2*(pcbddc->current_level+1));CHKERRQ(ierr);
        ierr = VecAXPY(check_vec,-1.0,coarse_vec);CHKERRQ(ierr);
        ierr = VecNorm(check_vec,NORM_INFINITY,&infty_error);CHKERRQ(ierr);
        ierr = MatMult(coarse_mat,check_vec,coarse_vec);CHKERRQ(ierr);
        ierr = VecNorm(coarse_vec,NORM_INFINITY,&abs_infty_error);CHKERRQ(ierr);
        ierr = VecDestroy(&check_vec);CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(dbg_viewer,"Coarse problem details (%d)\n",pcbddc->use_coarse_estimates);CHKERRQ(ierr);
        ierr = PetscObjectPrintClassNamePrefixType((PetscObject)(pcbddc->coarse_ksp),dbg_viewer);CHKERRQ(ierr);
        ierr = PetscObjectPrintClassNamePrefixType((PetscObject)(check_pc),dbg_viewer);CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(dbg_viewer,"Coarse problem exact infty_error   : %1.6e\n",infty_error);CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(dbg_viewer,"Coarse problem residual infty_error: %1.6e\n",abs_infty_error);CHKERRQ(ierr);
        if (compute_eigs) {
          PetscReal lambda_max_s,lambda_min_s;
          ierr = KSPGetIterationNumber(check_ksp,&its);CHKERRQ(ierr);
          ierr = KSPComputeExtremeSingularValues(check_ksp,&lambda_max_s,&lambda_min_s);CHKERRQ(ierr);
          ierr = PetscViewerASCIIPrintf(dbg_viewer,"Coarse problem eigenvalues (estimated with %d iterations of %s): %1.6e %1.6e (%1.6e %1.6e)\n",its,check_ksp_type,lambda_min,lambda_max,lambda_min_s,lambda_max_s);CHKERRQ(ierr);
          for (i=0;i<neigs;i++) {
            ierr = PetscViewerASCIIPrintf(dbg_viewer,"%1.6e %1.6ei\n",eigs_r[i],eigs_c[i]);CHKERRQ(ierr);
          }
        }
        ierr = PetscViewerFlush(dbg_viewer);CHKERRQ(ierr);
        ierr = PetscViewerASCIISubtractTab(dbg_viewer,2*(pcbddc->current_level+1));CHKERRQ(ierr);
      }
      ierr = KSPDestroy(&check_ksp);CHKERRQ(ierr);
      if (compute_eigs) {
        ierr = PetscFree(eigs_r);CHKERRQ(ierr);
        ierr = PetscFree(eigs_c);CHKERRQ(ierr);
      }
    }
  }
  /* print additional info */
  if (pcbddc->dbg_flag) {
    /* waits until all processes reaches this point */
    ierr = PetscBarrier((PetscObject)pc);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(pcbddc->dbg_viewer,"Coarse solver setup completed at level %d\n",pcbddc->current_level);CHKERRQ(ierr);
    ierr = PetscViewerFlush(pcbddc->dbg_viewer);CHKERRQ(ierr);
  }

  /* free memory */
  ierr = MatNullSpaceDestroy(&CoarseNullSpace);CHKERRQ(ierr);
  ierr = MatDestroy(&coarse_mat);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCBDDCComputePrimalNumbering"
PetscErrorCode PCBDDCComputePrimalNumbering(PC pc,PetscInt* coarse_size_n,PetscInt** local_primal_indices_n)
{
  PC_BDDC*       pcbddc = (PC_BDDC*)pc->data;
  PC_IS*         pcis = (PC_IS*)pc->data;
  Mat_IS*        matis = (Mat_IS*)pc->pmat->data;
  PetscInt       i,coarse_size;
  PetscInt       *local_primal_indices;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* Compute global number of coarse dofs */
  if (!pcbddc->primal_indices_local_idxs && pcbddc->local_primal_size) {
    SETERRQ(PetscObjectComm((PetscObject)pc),PETSC_ERR_PLIB,"BDDC Local primal indices have not been created");
  }
  ierr = PCBDDCSubsetNumbering(PetscObjectComm((PetscObject)(pc->pmat)),matis->mapping,pcbddc->local_primal_size,pcbddc->primal_indices_local_idxs,NULL,&coarse_size,&local_primal_indices);CHKERRQ(ierr);

  /* check numbering */
  if (pcbddc->dbg_flag) {
    PetscScalar coarsesum,*array;
    PetscBool set_error = PETSC_FALSE,set_error_reduced = PETSC_FALSE;

    ierr = PetscViewerFlush(pcbddc->dbg_viewer);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(pcbddc->dbg_viewer,"--------------------------------------------------\n");CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(pcbddc->dbg_viewer,"Check coarse indices\n");CHKERRQ(ierr);
    ierr = PetscViewerASCIISynchronizedAllow(pcbddc->dbg_viewer,PETSC_TRUE);CHKERRQ(ierr);
    ierr = VecSet(pcis->vec1_N,0.0);CHKERRQ(ierr);
    for (i=0;i<pcbddc->local_primal_size;i++) {
      ierr = VecSetValue(pcis->vec1_N,pcbddc->primal_indices_local_idxs[i],1.0,INSERT_VALUES);CHKERRQ(ierr);
    }
    ierr = VecAssemblyBegin(pcis->vec1_N);CHKERRQ(ierr);
    ierr = VecAssemblyEnd(pcis->vec1_N);CHKERRQ(ierr);
    ierr = VecSet(pcis->vec1_global,0.0);CHKERRQ(ierr);
    ierr = VecScatterBegin(matis->ctx,pcis->vec1_N,pcis->vec1_global,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
    ierr = VecScatterEnd(matis->ctx,pcis->vec1_N,pcis->vec1_global,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
    ierr = VecScatterBegin(matis->ctx,pcis->vec1_global,pcis->vec1_N,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(matis->ctx,pcis->vec1_global,pcis->vec1_N,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecGetArray(pcis->vec1_N,&array);CHKERRQ(ierr);
    for (i=0;i<pcis->n;i++) {
      if (array[i] == 1.0) {
        set_error = PETSC_TRUE;
        ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"Subdomain %04d: local index %d owned by a single process!\n",PetscGlobalRank,i);CHKERRQ(ierr);
      }
    }
    ierr = MPI_Allreduce(&set_error,&set_error_reduced,1,MPIU_BOOL,MPI_LOR,PetscObjectComm((PetscObject)pc));CHKERRQ(ierr);
    ierr = PetscViewerFlush(pcbddc->dbg_viewer);CHKERRQ(ierr);
    for (i=0;i<pcis->n;i++) {
      if (PetscRealPart(array[i]) > 0.0) array[i] = 1.0/PetscRealPart(array[i]);
    }
    ierr = VecRestoreArray(pcis->vec1_N,&array);CHKERRQ(ierr);
    ierr = VecSet(pcis->vec1_global,0.0);CHKERRQ(ierr);
    ierr = VecScatterBegin(matis->ctx,pcis->vec1_N,pcis->vec1_global,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
    ierr = VecScatterEnd(matis->ctx,pcis->vec1_N,pcis->vec1_global,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
    ierr = VecSum(pcis->vec1_global,&coarsesum);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(pcbddc->dbg_viewer,"Size of coarse problem is %d (%lf)\n",coarse_size,PetscRealPart(coarsesum));CHKERRQ(ierr);
    if (pcbddc->dbg_flag > 1 || set_error_reduced) {
      ierr = PetscViewerASCIIPrintf(pcbddc->dbg_viewer,"Distribution of local primal indices\n");CHKERRQ(ierr);
      ierr = PetscViewerFlush(pcbddc->dbg_viewer);CHKERRQ(ierr);
      ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"Subdomain %04d\n",PetscGlobalRank);CHKERRQ(ierr);
      for (i=0;i<pcbddc->local_primal_size;i++) {
        ierr = PetscViewerASCIISynchronizedPrintf(pcbddc->dbg_viewer,"local_primal_indices[%d]=%d (%d)\n",i,local_primal_indices[i],pcbddc->primal_indices_local_idxs[i]);
      }
      ierr = PetscViewerFlush(pcbddc->dbg_viewer);CHKERRQ(ierr);
    }
    ierr = PetscViewerFlush(pcbddc->dbg_viewer);CHKERRQ(ierr);
    if (set_error_reduced) {
      SETERRQ(PetscObjectComm((PetscObject)pc),PETSC_ERR_PLIB,"BDDC Numbering of coarse dofs failed");
    }
  }
  /* get back data */
  *coarse_size_n = coarse_size;
  *local_primal_indices_n = local_primal_indices;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PCBDDCGlobalToLocal"
PetscErrorCode PCBDDCGlobalToLocal(VecScatter g2l_ctx,Vec gwork, Vec lwork, IS globalis, IS* localis)
{
  IS             localis_t;
  PetscInt       i,lsize,*idxs,n;
  PetscScalar    *vals;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* get indices in local ordering exploiting local to global map */
  ierr = ISGetLocalSize(globalis,&lsize);CHKERRQ(ierr);
  ierr = PetscMalloc(lsize*sizeof(PetscScalar),&vals);CHKERRQ(ierr);
  for (i=0;i<lsize;i++) vals[i] = 1.0;
  ierr = ISGetIndices(globalis,(const PetscInt**)&idxs);CHKERRQ(ierr);
  ierr = VecSet(gwork,0.0);CHKERRQ(ierr);
  ierr = VecSet(lwork,0.0);CHKERRQ(ierr);
  if (idxs) { /* multilevel guard */
    ierr = VecSetValues(gwork,lsize,idxs,vals,INSERT_VALUES);CHKERRQ(ierr);
  }
  ierr = VecAssemblyBegin(gwork);CHKERRQ(ierr);
  ierr = ISRestoreIndices(globalis,(const PetscInt**)&idxs);CHKERRQ(ierr);
  ierr = PetscFree(vals);CHKERRQ(ierr);
  ierr = VecAssemblyEnd(gwork);CHKERRQ(ierr);
  /* now compute set in local ordering */
  ierr = VecScatterBegin(g2l_ctx,gwork,lwork,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(g2l_ctx,gwork,lwork,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecGetArrayRead(lwork,(const PetscScalar**)&vals);CHKERRQ(ierr);
  ierr = VecGetSize(lwork,&n);CHKERRQ(ierr);
  for (i=0,lsize=0;i<n;i++) {
    if (PetscRealPart(vals[i]) > 0.5) {
      lsize++;
    }
  }
  ierr = PetscMalloc(lsize*sizeof(PetscInt),&idxs);CHKERRQ(ierr);
  for (i=0,lsize=0;i<n;i++) {
    if (PetscRealPart(vals[i]) > 0.5) {
      idxs[lsize++] = i;
    }
  }
  ierr = VecRestoreArrayRead(lwork,(const PetscScalar**)&vals);CHKERRQ(ierr);
  ierr = ISCreateGeneral(PetscObjectComm((PetscObject)gwork),lsize,idxs,PETSC_OWN_POINTER,&localis_t);CHKERRQ(ierr);
  *localis = localis_t;
  PetscFunctionReturn(0);
}
