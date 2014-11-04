#include <petsc-private/dmimpl.h>     /*I      "petscdm.h"     I*/
#include <petscsf.h>
#include <petscds.h>

PetscClassId  DM_CLASSID;
PetscLogEvent DM_Convert, DM_GlobalToLocal, DM_LocalToGlobal, DM_LocalToLocal;

const char *const DMBoundaryTypes[] = {"NONE","GHOSTED","MIRROR","PERIODIC","TWIST","DM_BOUNDARY_",0};

#undef __FUNCT__
#define __FUNCT__ "DMCreate"
/*@
  DMCreate - Creates an empty DM object. The type can then be set with DMSetType().

   If you never  call DMSetType()  it will generate an
   error when you try to use the vector.

  Collective on MPI_Comm

  Input Parameter:
. comm - The communicator for the DM object

  Output Parameter:
. dm - The DM object

  Level: beginner

.seealso: DMSetType(), DMDA, DMSLICED, DMCOMPOSITE
@*/
PetscErrorCode  DMCreate(MPI_Comm comm,DM *dm)
{
  DM             v;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidPointer(dm,2);
  *dm = NULL;
  ierr = PetscSysInitializePackage();CHKERRQ(ierr);
  ierr = VecInitializePackage();CHKERRQ(ierr);
  ierr = MatInitializePackage();CHKERRQ(ierr);
  ierr = DMInitializePackage();CHKERRQ(ierr);

  ierr = PetscHeaderCreate(v, _p_DM, struct _DMOps, DM_CLASSID, "DM", "Distribution Manager", "DM", comm, DMDestroy, DMView);CHKERRQ(ierr);
  ierr = PetscMemzero(v->ops, sizeof(struct _DMOps));CHKERRQ(ierr);


  v->ltogmap              = NULL;
  v->bs                   = 1;
  v->coloringtype         = IS_COLORING_GLOBAL;
  ierr                    = PetscSFCreate(comm, &v->sf);CHKERRQ(ierr);
  ierr                    = PetscSFCreate(comm, &v->defaultSF);CHKERRQ(ierr);
  v->defaultSection       = NULL;
  v->defaultGlobalSection = NULL;
  v->L                    = NULL;
  v->maxCell              = NULL;
  {
    PetscInt i;
    for (i = 0; i < 10; ++i) {
      v->nullspaceConstructors[i] = NULL;
    }
  }
  ierr = PetscDSCreate(comm, &v->prob);CHKERRQ(ierr);
  v->dmBC = NULL;
  v->outputSequenceNum = -1;
  v->outputSequenceVal = 0.0;
  ierr = DMSetVecType(v,VECSTANDARD);CHKERRQ(ierr);
  ierr = DMSetMatType(v,MATAIJ);CHKERRQ(ierr);
  *dm = v;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMClone"
/*@
  DMClone - Creates a DM object with the same topology as the original.

  Collective on MPI_Comm

  Input Parameter:
. dm - The original DM object

  Output Parameter:
. newdm  - The new DM object

  Level: beginner

.keywords: DM, topology, create
@*/
PetscErrorCode DMClone(DM dm, DM *newdm)
{
  PetscSF        sf;
  Vec            coords;
  void          *ctx;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscValidPointer(newdm,2);
  ierr = DMCreate(PetscObjectComm((PetscObject)dm), newdm);CHKERRQ(ierr);
  if (dm->ops->clone) {
    ierr = (*dm->ops->clone)(dm, newdm);CHKERRQ(ierr);
  }
  (*newdm)->setupcalled = PETSC_TRUE;
  ierr = DMGetPointSF(dm, &sf);CHKERRQ(ierr);
  ierr = DMSetPointSF(*newdm, sf);CHKERRQ(ierr);
  ierr = DMGetApplicationContext(dm, &ctx);CHKERRQ(ierr);
  ierr = DMSetApplicationContext(*newdm, ctx);CHKERRQ(ierr);
  ierr = DMGetCoordinatesLocal(dm, &coords);CHKERRQ(ierr);
  if (coords) {
    ierr = DMSetCoordinatesLocal(*newdm, coords);CHKERRQ(ierr);
  } else {
    ierr = DMGetCoordinates(dm, &coords);CHKERRQ(ierr);
    if (coords) {ierr = DMSetCoordinates(*newdm, coords);CHKERRQ(ierr);}
  }
  if (dm->maxCell) {
    const PetscReal *maxCell, *L;
    ierr = DMGetPeriodicity(dm,     &maxCell, &L);CHKERRQ(ierr);
    ierr = DMSetPeriodicity(*newdm,  maxCell,  L);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetVecType"
/*@C
       DMSetVecType - Sets the type of vector created with DMCreateLocalVector() and DMCreateGlobalVector()

   Logically Collective on DMDA

   Input Parameter:
+  da - initial distributed array
.  ctype - the vector type, currently either VECSTANDARD or VECCUSP

   Options Database:
.   -dm_vec_type ctype

   Level: intermediate

.seealso: DMDACreate1d(), DMDACreate2d(), DMDACreate3d(), DMDestroy(), DMDA, DMDAInterpolationType, VecType, DMGetVecType()
@*/
PetscErrorCode  DMSetVecType(DM da,VecType ctype)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DM_CLASSID,1);
  ierr = PetscFree(da->vectype);CHKERRQ(ierr);
  ierr = PetscStrallocpy(ctype,(char**)&da->vectype);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetVecType"
/*@C
       DMGetVecType - Gets the type of vector created with DMCreateLocalVector() and DMCreateGlobalVector()

   Logically Collective on DMDA

   Input Parameter:
.  da - initial distributed array

   Output Parameter:
.  ctype - the vector type

   Level: intermediate

.seealso: DMDACreate1d(), DMDACreate2d(), DMDACreate3d(), DMDestroy(), DMDA, DMDAInterpolationType, VecType
@*/
PetscErrorCode  DMGetVecType(DM da,VecType *ctype)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DM_CLASSID,1);
  *ctype = da->vectype;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecGetDM"
/*@
  VecGetDM - Gets the DM defining the data layout of the vector

  Not collective

  Input Parameter:
. v - The Vec

  Output Parameter:
. dm - The DM

  Level: intermediate

.seealso: VecSetDM(), DMGetLocalVector(), DMGetGlobalVector(), DMSetVecType()
@*/
PetscErrorCode VecGetDM(Vec v, DM *dm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(v,VEC_CLASSID,1);
  PetscValidPointer(dm,2);
  ierr = PetscObjectQuery((PetscObject) v, "__PETSc_dm", (PetscObject*) dm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecSetDM"
/*@
  VecSetDM - Sets the DM defining the data layout of the vector

  Not collective

  Input Parameters:
+ v - The Vec
- dm - The DM

  Level: intermediate

.seealso: VecGetDM(), DMGetLocalVector(), DMGetGlobalVector(), DMSetVecType()
@*/
PetscErrorCode VecSetDM(Vec v, DM dm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(v,VEC_CLASSID,1);
  if (dm) PetscValidHeaderSpecific(dm,DM_CLASSID,2);
  ierr = PetscObjectCompose((PetscObject) v, "__PETSc_dm", (PetscObject) dm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetMatType"
/*@C
       DMSetMatType - Sets the type of matrix created with DMCreateMatrix()

   Logically Collective on DM

   Input Parameter:
+  dm - the DM context
.  ctype - the matrix type

   Options Database:
.   -dm_mat_type ctype

   Level: intermediate

.seealso: DMDACreate1d(), DMDACreate2d(), DMDACreate3d(), DMCreateMatrix(), DMSetMatrixPreallocateOnly(), MatType, DMGetMatType()
@*/
PetscErrorCode  DMSetMatType(DM dm,MatType ctype)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = PetscFree(dm->mattype);CHKERRQ(ierr);
  ierr = PetscStrallocpy(ctype,(char**)&dm->mattype);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetMatType"
/*@C
       DMGetMatType - Gets the type of matrix created with DMCreateMatrix()

   Logically Collective on DM

   Input Parameter:
.  dm - the DM context

   Output Parameter:
.  ctype - the matrix type

   Options Database:
.   -dm_mat_type ctype

   Level: intermediate

.seealso: DMDACreate1d(), DMDACreate2d(), DMDACreate3d(), DMCreateMatrix(), DMSetMatrixPreallocateOnly(), MatType, DMSetMatType()
@*/
PetscErrorCode  DMGetMatType(DM dm,MatType *ctype)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  *ctype = dm->mattype;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetDM"
/*@
  MatGetDM - Gets the DM defining the data layout of the matrix

  Not collective

  Input Parameter:
. A - The Mat

  Output Parameter:
. dm - The DM

  Level: intermediate

.seealso: MatSetDM(), DMCreateMatrix(), DMSetMatType()
@*/
PetscErrorCode MatGetDM(Mat A, DM *dm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(A,MAT_CLASSID,1);
  PetscValidPointer(dm,2);
  ierr = PetscObjectQuery((PetscObject) A, "__PETSc_dm", (PetscObject*) dm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetDM"
/*@
  MatSetDM - Sets the DM defining the data layout of the matrix

  Not collective

  Input Parameters:
+ A - The Mat
- dm - The DM

  Level: intermediate

.seealso: MatGetDM(), DMCreateMatrix(), DMSetMatType()
@*/
PetscErrorCode MatSetDM(Mat A, DM dm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(A,MAT_CLASSID,1);
  if (dm) PetscValidHeaderSpecific(dm,DM_CLASSID,2);
  ierr = PetscObjectCompose((PetscObject) A, "__PETSc_dm", (PetscObject) dm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetOptionsPrefix"
/*@C
   DMSetOptionsPrefix - Sets the prefix used for searching for all
   DMDA options in the database.

   Logically Collective on DMDA

   Input Parameter:
+  da - the DMDA context
-  prefix - the prefix to prepend to all option names

   Notes:
   A hyphen (-) must NOT be given at the beginning of the prefix name.
   The first character of all runtime options is AUTOMATICALLY the hyphen.

   Level: advanced

.keywords: DMDA, set, options, prefix, database

.seealso: DMSetFromOptions()
@*/
PetscErrorCode  DMSetOptionsPrefix(DM dm,const char prefix[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = PetscObjectSetOptionsPrefix((PetscObject)dm,prefix);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMDestroy"
/*@
    DMDestroy - Destroys a vector packer or DMDA.

    Collective on DM

    Input Parameter:
.   dm - the DM object to destroy

    Level: developer

.seealso DMView(), DMCreateGlobalVector(), DMCreateInterpolation(), DMCreateColoring(), DMCreateMatrix()

@*/
PetscErrorCode  DMDestroy(DM *dm)
{
  PetscInt       i, cnt = 0, Nf = 0, f;
  DMNamedVecLink nlink,nnext;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!*dm) PetscFunctionReturn(0);
  PetscValidHeaderSpecific((*dm),DM_CLASSID,1);

  if ((*dm)->prob) {
    PetscObject disc;

    /* I think it makes sense to dump all attached things when you are destroyed, which also eliminates circular references */
    ierr = PetscDSGetNumFields((*dm)->prob, &Nf);CHKERRQ(ierr);
    for (f = 0; f < Nf; ++f) {
      ierr = PetscDSGetDiscretization((*dm)->prob, f, &disc);CHKERRQ(ierr);
      ierr = PetscObjectCompose(disc, "pmat", NULL);CHKERRQ(ierr);
      ierr = PetscObjectCompose(disc, "nullspace", NULL);CHKERRQ(ierr);
      ierr = PetscObjectCompose(disc, "nearnullspace", NULL);CHKERRQ(ierr);
    }
  }
  /* count all the circular references of DM and its contained Vecs */
  for (i=0; i<DM_MAX_WORK_VECTORS; i++) {
    if ((*dm)->localin[i])  cnt++;
    if ((*dm)->globalin[i]) cnt++;
  }
  for (nlink=(*dm)->namedglobal; nlink; nlink=nlink->next) cnt++;
  for (nlink=(*dm)->namedlocal; nlink; nlink=nlink->next) cnt++;
  if ((*dm)->x) {
    DM obj;
    ierr = VecGetDM((*dm)->x, &obj);CHKERRQ(ierr);
    if (obj == *dm) cnt++;
  }

  if (--((PetscObject)(*dm))->refct - cnt > 0) {*dm = 0; PetscFunctionReturn(0);}
  /*
     Need this test because the dm references the vectors that
     reference the dm, so destroying the dm calls destroy on the
     vectors that cause another destroy on the dm
  */
  if (((PetscObject)(*dm))->refct < 0) PetscFunctionReturn(0);
  ((PetscObject) (*dm))->refct = 0;
  for (i=0; i<DM_MAX_WORK_VECTORS; i++) {
    if ((*dm)->localout[i]) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Destroying a DM that has a local vector obtained with DMGetLocalVector()");
    ierr = VecDestroy(&(*dm)->localin[i]);CHKERRQ(ierr);
  }
  nnext=(*dm)->namedglobal;
  (*dm)->namedglobal = NULL;
  for (nlink=nnext; nlink; nlink=nnext) { /* Destroy the named vectors */
    nnext = nlink->next;
    if (nlink->status != DMVEC_STATUS_IN) SETERRQ1(((PetscObject)*dm)->comm,PETSC_ERR_ARG_WRONGSTATE,"DM still has Vec named '%s' checked out",nlink->name);
    ierr = PetscFree(nlink->name);CHKERRQ(ierr);
    ierr = VecDestroy(&nlink->X);CHKERRQ(ierr);
    ierr = PetscFree(nlink);CHKERRQ(ierr);
  }
  nnext=(*dm)->namedlocal;
  (*dm)->namedlocal = NULL;
  for (nlink=nnext; nlink; nlink=nnext) { /* Destroy the named local vectors */
    nnext = nlink->next;
    if (nlink->status != DMVEC_STATUS_IN) SETERRQ1(((PetscObject)*dm)->comm,PETSC_ERR_ARG_WRONGSTATE,"DM still has Vec named '%s' checked out",nlink->name);
    ierr = PetscFree(nlink->name);CHKERRQ(ierr);
    ierr = VecDestroy(&nlink->X);CHKERRQ(ierr);
    ierr = PetscFree(nlink);CHKERRQ(ierr);
  }

  /* Destroy the list of hooks */
  {
    DMCoarsenHookLink link,next;
    for (link=(*dm)->coarsenhook; link; link=next) {
      next = link->next;
      ierr = PetscFree(link);CHKERRQ(ierr);
    }
    (*dm)->coarsenhook = NULL;
  }
  {
    DMRefineHookLink link,next;
    for (link=(*dm)->refinehook; link; link=next) {
      next = link->next;
      ierr = PetscFree(link);CHKERRQ(ierr);
    }
    (*dm)->refinehook = NULL;
  }
  {
    DMSubDomainHookLink link,next;
    for (link=(*dm)->subdomainhook; link; link=next) {
      next = link->next;
      ierr = PetscFree(link);CHKERRQ(ierr);
    }
    (*dm)->subdomainhook = NULL;
  }
  {
    DMGlobalToLocalHookLink link,next;
    for (link=(*dm)->gtolhook; link; link=next) {
      next = link->next;
      ierr = PetscFree(link);CHKERRQ(ierr);
    }
    (*dm)->gtolhook = NULL;
  }
  /* Destroy the work arrays */
  {
    DMWorkLink link,next;
    if ((*dm)->workout) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Work array still checked out");
    for (link=(*dm)->workin; link; link=next) {
      next = link->next;
      ierr = PetscFree(link->mem);CHKERRQ(ierr);
      ierr = PetscFree(link);CHKERRQ(ierr);
    }
    (*dm)->workin = NULL;
  }

  ierr = PetscObjectDestroy(&(*dm)->dmksp);CHKERRQ(ierr);
  ierr = PetscObjectDestroy(&(*dm)->dmsnes);CHKERRQ(ierr);
  ierr = PetscObjectDestroy(&(*dm)->dmts);CHKERRQ(ierr);

  if ((*dm)->ctx && (*dm)->ctxdestroy) {
    ierr = (*(*dm)->ctxdestroy)(&(*dm)->ctx);CHKERRQ(ierr);
  }
  ierr = VecDestroy(&(*dm)->x);CHKERRQ(ierr);
  ierr = MatFDColoringDestroy(&(*dm)->fd);CHKERRQ(ierr);
  ierr = DMClearGlobalVectors(*dm);CHKERRQ(ierr);
  ierr = ISLocalToGlobalMappingDestroy(&(*dm)->ltogmap);CHKERRQ(ierr);
  ierr = PetscFree((*dm)->vectype);CHKERRQ(ierr);
  ierr = PetscFree((*dm)->mattype);CHKERRQ(ierr);

  ierr = PetscSectionDestroy(&(*dm)->defaultSection);CHKERRQ(ierr);
  ierr = PetscSectionDestroy(&(*dm)->defaultGlobalSection);CHKERRQ(ierr);
  ierr = PetscLayoutDestroy(&(*dm)->map);CHKERRQ(ierr);
  ierr = PetscSFDestroy(&(*dm)->sf);CHKERRQ(ierr);
  ierr = PetscSFDestroy(&(*dm)->defaultSF);CHKERRQ(ierr);

  ierr = DMDestroy(&(*dm)->coordinateDM);CHKERRQ(ierr);
  ierr = VecDestroy(&(*dm)->coordinates);CHKERRQ(ierr);
  ierr = VecDestroy(&(*dm)->coordinatesLocal);CHKERRQ(ierr);
  ierr = PetscFree((*dm)->maxCell);CHKERRQ(ierr);
  ierr = PetscFree((*dm)->L);CHKERRQ(ierr);

  ierr = PetscDSDestroy(&(*dm)->prob);CHKERRQ(ierr);
  ierr = DMDestroy(&(*dm)->dmBC);CHKERRQ(ierr);
  /* if memory was published with SAWs then destroy it */
  ierr = PetscObjectSAWsViewOff((PetscObject)*dm);CHKERRQ(ierr);

  ierr = (*(*dm)->ops->destroy)(*dm);CHKERRQ(ierr);
  /* We do not destroy (*dm)->data here so that we can reference count backend objects */
  ierr = PetscHeaderDestroy(dm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetUp"
/*@
    DMSetUp - sets up the data structures inside a DM object

    Collective on DM

    Input Parameter:
.   dm - the DM object to setup

    Level: developer

.seealso DMView(), DMCreateGlobalVector(), DMCreateInterpolation(), DMCreateColoring(), DMCreateMatrix()

@*/
PetscErrorCode  DMSetUp(DM dm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  if (dm->setupcalled) PetscFunctionReturn(0);
  if (dm->ops->setup) {
    ierr = (*dm->ops->setup)(dm);CHKERRQ(ierr);
  }
  dm->setupcalled = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetFromOptions"
/*@
    DMSetFromOptions - sets parameters in a DM from the options database

    Collective on DM

    Input Parameter:
.   dm - the DM object to set options for

    Options Database:
+   -dm_preallocate_only: Only preallocate the matrix for DMCreateMatrix(), but do not fill it with zeros
.   -dm_vec_type <type>  type of vector to create inside DM
.   -dm_mat_type <type>  type of matrix to create inside DM
-   -dm_coloring_type <global or ghosted>

    Level: developer

.seealso DMView(), DMCreateGlobalVector(), DMCreateInterpolation(), DMCreateColoring(), DMCreateMatrix()

@*/
PetscErrorCode  DMSetFromOptions(DM dm)
{
  char           typeName[256];
  PetscBool      flg;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = PetscObjectOptionsBegin((PetscObject)dm);CHKERRQ(ierr);
  ierr = PetscOptionsBool("-dm_preallocate_only","only preallocate matrix, but do not set column indices","DMSetMatrixPreallocateOnly",dm->prealloc_only,&dm->prealloc_only,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsFList("-dm_vec_type","Vector type used for created vectors","DMSetVecType",VecList,dm->vectype,typeName,256,&flg);CHKERRQ(ierr);
  if (flg) {
    ierr = DMSetVecType(dm,typeName);CHKERRQ(ierr);
  }
  ierr = PetscOptionsFList("-dm_mat_type","Matrix type used for created matrices","DMSetMatType",MatList,dm->mattype ? dm->mattype : typeName,typeName,sizeof(typeName),&flg);CHKERRQ(ierr);
  if (flg) {
    ierr = DMSetMatType(dm,typeName);CHKERRQ(ierr);
  }
  ierr = PetscOptionsEnum("-dm_is_coloring_type","Global or local coloring of Jacobian","ISColoringType",ISColoringTypes,(PetscEnum)dm->coloringtype,(PetscEnum*)&dm->coloringtype,NULL);CHKERRQ(ierr);
  if (dm->ops->setfromoptions) {
    ierr = (*dm->ops->setfromoptions)(dm);CHKERRQ(ierr);
  }
  /* process any options handlers added with PetscObjectAddOptionsHandler() */
  ierr = PetscObjectProcessOptionsHandlers((PetscObject) dm);CHKERRQ(ierr);
  ierr = PetscOptionsEnd();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMView"
/*@C
    DMView - Views a vector packer or DMDA.

    Collective on DM

    Input Parameter:
+   dm - the DM object to view
-   v - the viewer

    Level: developer

.seealso DMDestroy(), DMCreateGlobalVector(), DMCreateInterpolation(), DMCreateColoring(), DMCreateMatrix()

@*/
PetscErrorCode  DMView(DM dm,PetscViewer v)
{
  PetscErrorCode ierr;
  PetscBool      isbinary;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  if (!v) {
    ierr = PetscViewerASCIIGetStdout(PetscObjectComm((PetscObject)dm),&v);CHKERRQ(ierr);
  }
  ierr = PetscObjectPrintClassNamePrefixType((PetscObject)dm,v);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)v,PETSCVIEWERBINARY,&isbinary);CHKERRQ(ierr);
  if (isbinary) {
    PetscInt classid = DM_FILE_CLASSID;
    char     type[256];

    ierr = PetscViewerBinaryWrite(v,&classid,1,PETSC_INT,PETSC_FALSE);CHKERRQ(ierr);
    ierr = PetscStrncpy(type,((PetscObject)dm)->type_name,256);CHKERRQ(ierr);
    ierr = PetscViewerBinaryWrite(v,type,256,PETSC_CHAR,PETSC_FALSE);CHKERRQ(ierr);
  }
  if (dm->ops->view) {
    ierr = (*dm->ops->view)(dm,v);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCreateGlobalVector"
/*@
    DMCreateGlobalVector - Creates a global vector from a DMDA or DMComposite object

    Collective on DM

    Input Parameter:
.   dm - the DM object

    Output Parameter:
.   vec - the global vector

    Level: beginner

.seealso DMDestroy(), DMView(), DMCreateInterpolation(), DMCreateColoring(), DMCreateMatrix()

@*/
PetscErrorCode  DMCreateGlobalVector(DM dm,Vec *vec)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = (*dm->ops->createglobalvector)(dm,vec);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCreateLocalVector"
/*@
    DMCreateLocalVector - Creates a local vector from a DMDA or DMComposite object

    Not Collective

    Input Parameter:
.   dm - the DM object

    Output Parameter:
.   vec - the local vector

    Level: beginner

.seealso DMDestroy(), DMView(), DMCreateInterpolation(), DMCreateColoring(), DMCreateMatrix()

@*/
PetscErrorCode  DMCreateLocalVector(DM dm,Vec *vec)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = (*dm->ops->createlocalvector)(dm,vec);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetLocalToGlobalMapping"
/*@
   DMGetLocalToGlobalMapping - Accesses the local-to-global mapping in a DM.

   Collective on DM

   Input Parameter:
.  dm - the DM that provides the mapping

   Output Parameter:
.  ltog - the mapping

   Level: intermediate

   Notes:
   This mapping can then be used by VecSetLocalToGlobalMapping() or
   MatSetLocalToGlobalMapping().

.seealso: DMCreateLocalVector()
@*/
PetscErrorCode  DMGetLocalToGlobalMapping(DM dm,ISLocalToGlobalMapping *ltog)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(ltog,2);
  if (!dm->ltogmap) {
    PetscSection section, sectionGlobal;

    ierr = DMGetDefaultSection(dm, &section);CHKERRQ(ierr);
    if (section) {
      PetscInt *ltog;
      PetscInt pStart, pEnd, size, p, l;

      ierr = DMGetDefaultGlobalSection(dm, &sectionGlobal);CHKERRQ(ierr);
      ierr = PetscSectionGetChart(section, &pStart, &pEnd);CHKERRQ(ierr);
      ierr = PetscSectionGetStorageSize(section, &size);CHKERRQ(ierr);
      ierr = PetscMalloc1(size, &ltog);CHKERRQ(ierr); /* We want the local+overlap size */
      for (p = pStart, l = 0; p < pEnd; ++p) {
        PetscInt dof, off, c;

        /* Should probably use constrained dofs */
        ierr = PetscSectionGetDof(section, p, &dof);CHKERRQ(ierr);
        ierr = PetscSectionGetOffset(sectionGlobal, p, &off);CHKERRQ(ierr);
        for (c = 0; c < dof; ++c, ++l) {
          ltog[l] = off+c;
        }
      }
      ierr = ISLocalToGlobalMappingCreate(PETSC_COMM_SELF, 1,size, ltog, PETSC_OWN_POINTER, &dm->ltogmap);CHKERRQ(ierr);
      ierr = PetscLogObjectParent((PetscObject)dm, (PetscObject)dm->ltogmap);CHKERRQ(ierr);
    } else {
      if (!dm->ops->getlocaltoglobalmapping) SETERRQ(PetscObjectComm((PetscObject)dm),PETSC_ERR_SUP,"DM can not create LocalToGlobalMapping");
      ierr = (*dm->ops->getlocaltoglobalmapping)(dm);CHKERRQ(ierr);
    }
  }
  *ltog = dm->ltogmap;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetBlockSize"
/*@
   DMGetBlockSize - Gets the inherent block size associated with a DM

   Not Collective

   Input Parameter:
.  dm - the DM with block structure

   Output Parameter:
.  bs - the block size, 1 implies no exploitable block structure

   Level: intermediate

.seealso: ISCreateBlock(), VecSetBlockSize(), MatSetBlockSize(), DMGetLocalToGlobalMapping()
@*/
PetscErrorCode  DMGetBlockSize(DM dm,PetscInt *bs)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(bs,2);
  if (dm->bs < 1) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"DM does not have enough information to provide a block size yet");
  *bs = dm->bs;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCreateInterpolation"
/*@
    DMCreateInterpolation - Gets interpolation matrix between two DMDA or DMComposite objects

    Collective on DM

    Input Parameter:
+   dm1 - the DM object
-   dm2 - the second, finer DM object

    Output Parameter:
+  mat - the interpolation
-  vec - the scaling (optional)

    Level: developer

    Notes:  For DMDA objects this only works for "uniform refinement", that is the refined mesh was obtained DMRefine() or the coarse mesh was obtained by
        DMCoarsen(). The coordinates set into the DMDA are completely ignored in computing the interpolation.

        For DMDA objects you can use this interpolation (more precisely the interpolation from the DMGetCoordinateDM()) to interpolate the mesh coordinate vectors
        EXCEPT in the periodic case where it does not make sense since the coordinate vectors are not periodic.


.seealso DMDestroy(), DMView(), DMCreateGlobalVector(), DMCreateColoring(), DMCreateMatrix(), DMRefine(), DMCoarsen()

@*/
PetscErrorCode  DMCreateInterpolation(DM dm1,DM dm2,Mat *mat,Vec *vec)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm1,DM_CLASSID,1);
  PetscValidHeaderSpecific(dm2,DM_CLASSID,2);
  ierr = (*dm1->ops->createinterpolation)(dm1,dm2,mat,vec);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCreateInjection"
/*@
    DMCreateInjection - Gets injection matrix between two DMDA or DMComposite objects

    Collective on DM

    Input Parameter:
+   dm1 - the DM object
-   dm2 - the second, finer DM object

    Output Parameter:
.   ctx - the injection

    Level: developer

   Notes:  For DMDA objects this only works for "uniform refinement", that is the refined mesh was obtained DMRefine() or the coarse mesh was obtained by
        DMCoarsen(). The coordinates set into the DMDA are completely ignored in computing the injection.

.seealso DMDestroy(), DMView(), DMCreateGlobalVector(), DMCreateColoring(), DMCreateMatrix(), DMCreateInterpolation()

@*/
PetscErrorCode  DMCreateInjection(DM dm1,DM dm2,VecScatter *ctx)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm1,DM_CLASSID,1);
  PetscValidHeaderSpecific(dm2,DM_CLASSID,2);
  ierr = (*dm1->ops->getinjection)(dm1,dm2,ctx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCreateColoring"
/*@
    DMCreateColoring - Gets coloring for a DM

    Collective on DM

    Input Parameter:
+   dm - the DM object
-   ctype - IS_COLORING_GHOSTED or IS_COLORING_GLOBAL

    Output Parameter:
.   coloring - the coloring

    Level: developer

.seealso DMDestroy(), DMView(), DMCreateGlobalVector(), DMCreateInterpolation(), DMCreateMatrix(), DMSetMatType()

@*/
PetscErrorCode  DMCreateColoring(DM dm,ISColoringType ctype,ISColoring *coloring)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  if (!dm->ops->getcoloring) SETERRQ(PetscObjectComm((PetscObject)dm),PETSC_ERR_SUP,"No coloring for this type of DM yet");
  ierr = (*dm->ops->getcoloring)(dm,ctype,coloring);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCreateMatrix"
/*@
    DMCreateMatrix - Gets empty Jacobian for a DMDA or DMComposite

    Collective on DM

    Input Parameter:
.   dm - the DM object

    Output Parameter:
.   mat - the empty Jacobian

    Level: beginner

    Notes: This properly preallocates the number of nonzeros in the sparse matrix so you
       do not need to do it yourself.

       By default it also sets the nonzero structure and puts in the zero entries. To prevent setting
       the nonzero pattern call DMDASetMatPreallocateOnly()

       For structured grid problems, when you call MatView() on this matrix it is displayed using the global natural ordering, NOT in the ordering used
       internally by PETSc.

       For structured grid problems, in general it is easiest to use MatSetValuesStencil() or MatSetValuesLocal() to put values into the matrix because MatSetValues() requires
       the indices for the global numbering for DMDAs which is complicated.

.seealso DMDestroy(), DMView(), DMCreateGlobalVector(), DMCreateInterpolation(), DMSetMatType()

@*/
PetscErrorCode  DMCreateMatrix(DM dm,Mat *mat)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = MatInitializePackage();CHKERRQ(ierr);
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(mat,3);
  ierr = (*dm->ops->creatematrix)(dm,mat);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetMatrixPreallocateOnly"
/*@
  DMSetMatrixPreallocateOnly - When DMCreateMatrix() is called the matrix will be properly
    preallocated but the nonzero structure and zero values will not be set.

  Logically Collective on DMDA

  Input Parameter:
+ dm - the DM
- only - PETSC_TRUE if only want preallocation

  Level: developer
.seealso DMCreateMatrix()
@*/
PetscErrorCode DMSetMatrixPreallocateOnly(DM dm, PetscBool only)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  dm->prealloc_only = only;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetWorkArray"
/*@C
  DMGetWorkArray - Gets a work array guaranteed to be at least the input size, restore with DMRestoreWorkArray()

  Not Collective

  Input Parameters:
+ dm - the DM object
. count - The minium size
- dtype - data type (PETSC_REAL, PETSC_SCALAR, PETSC_INT)

  Output Parameter:
. array - the work array

  Level: developer

.seealso DMDestroy(), DMCreate()
@*/
PetscErrorCode DMGetWorkArray(DM dm,PetscInt count,PetscDataType dtype,void *mem)
{
  PetscErrorCode ierr;
  DMWorkLink     link;
  size_t         size;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(mem,4);
  if (dm->workin) {
    link       = dm->workin;
    dm->workin = dm->workin->next;
  } else {
    ierr = PetscNewLog(dm,&link);CHKERRQ(ierr);
  }
  ierr = PetscDataTypeGetSize(dtype,&size);CHKERRQ(ierr);
  if (size*count > link->bytes) {
    ierr        = PetscFree(link->mem);CHKERRQ(ierr);
    ierr        = PetscMalloc(size*count,&link->mem);CHKERRQ(ierr);
    link->bytes = size*count;
  }
  link->next   = dm->workout;
  dm->workout  = link;
  *(void**)mem = link->mem;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMRestoreWorkArray"
/*@C
  DMRestoreWorkArray - Restores a work array guaranteed to be at least the input size, restore with DMRestoreWorkArray()

  Not Collective

  Input Parameters:
+ dm - the DM object
. count - The minium size
- dtype - data type (PETSC_REAL, PETSC_SCALAR, PETSC_INT)

  Output Parameter:
. array - the work array

  Level: developer

.seealso DMDestroy(), DMCreate()
@*/
PetscErrorCode DMRestoreWorkArray(DM dm,PetscInt count,PetscDataType dtype,void *mem)
{
  DMWorkLink *p,link;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(mem,4);
  for (p=&dm->workout; (link=*p); p=&link->next) {
    if (link->mem == *(void**)mem) {
      *p           = link->next;
      link->next   = dm->workin;
      dm->workin   = link;
      *(void**)mem = NULL;
      PetscFunctionReturn(0);
    }
  }
  SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Array was not checked out");
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetNullSpaceConstructor"
PetscErrorCode DMSetNullSpaceConstructor(DM dm, PetscInt field, PetscErrorCode (*nullsp)(DM dm, PetscInt field, MatNullSpace *nullSpace))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  if (field >= 10) SETERRQ1(PetscObjectComm((PetscObject)dm), PETSC_ERR_ARG_OUTOFRANGE, "Cannot handle %d >= 10 fields", field);
  dm->nullspaceConstructors[field] = nullsp;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCreateFieldIS"
/*@C
  DMCreateFieldIS - Creates a set of IS objects with the global indices of dofs for each field

  Not collective

  Input Parameter:
. dm - the DM object

  Output Parameters:
+ numFields  - The number of fields (or NULL if not requested)
. fieldNames - The name for each field (or NULL if not requested)
- fields     - The global indices for each field (or NULL if not requested)

  Level: intermediate

  Notes:
  The user is responsible for freeing all requested arrays. In particular, every entry of names should be freed with
  PetscFree(), every entry of fields should be destroyed with ISDestroy(), and both arrays should be freed with
  PetscFree().

.seealso DMDestroy(), DMView(), DMCreateInterpolation(), DMCreateColoring(), DMCreateMatrix()
@*/
PetscErrorCode DMCreateFieldIS(DM dm, PetscInt *numFields, char ***fieldNames, IS **fields)
{
  PetscSection   section, sectionGlobal;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  if (numFields) {
    PetscValidPointer(numFields,2);
    *numFields = 0;
  }
  if (fieldNames) {
    PetscValidPointer(fieldNames,3);
    *fieldNames = NULL;
  }
  if (fields) {
    PetscValidPointer(fields,4);
    *fields = NULL;
  }
  ierr = DMGetDefaultSection(dm, &section);CHKERRQ(ierr);
  if (section) {
    PetscInt *fieldSizes, **fieldIndices;
    PetscInt nF, f, pStart, pEnd, p;

    ierr = DMGetDefaultGlobalSection(dm, &sectionGlobal);CHKERRQ(ierr);
    ierr = PetscSectionGetNumFields(section, &nF);CHKERRQ(ierr);
    ierr = PetscMalloc2(nF,&fieldSizes,nF,&fieldIndices);CHKERRQ(ierr);
    ierr = PetscSectionGetChart(sectionGlobal, &pStart, &pEnd);CHKERRQ(ierr);
    for (f = 0; f < nF; ++f) {
      fieldSizes[f] = 0;
    }
    for (p = pStart; p < pEnd; ++p) {
      PetscInt gdof;

      ierr = PetscSectionGetDof(sectionGlobal, p, &gdof);CHKERRQ(ierr);
      if (gdof > 0) {
        for (f = 0; f < nF; ++f) {
          PetscInt fdof, fcdof;

          ierr           = PetscSectionGetFieldDof(section, p, f, &fdof);CHKERRQ(ierr);
          ierr           = PetscSectionGetFieldConstraintDof(section, p, f, &fcdof);CHKERRQ(ierr);
          fieldSizes[f] += fdof-fcdof;
        }
      }
    }
    for (f = 0; f < nF; ++f) {
      ierr          = PetscMalloc1(fieldSizes[f], &fieldIndices[f]);CHKERRQ(ierr);
      fieldSizes[f] = 0;
    }
    for (p = pStart; p < pEnd; ++p) {
      PetscInt gdof, goff;

      ierr = PetscSectionGetDof(sectionGlobal, p, &gdof);CHKERRQ(ierr);
      if (gdof > 0) {
        ierr = PetscSectionGetOffset(sectionGlobal, p, &goff);CHKERRQ(ierr);
        for (f = 0; f < nF; ++f) {
          PetscInt fdof, fcdof, fc;

          ierr = PetscSectionGetFieldDof(section, p, f, &fdof);CHKERRQ(ierr);
          ierr = PetscSectionGetFieldConstraintDof(section, p, f, &fcdof);CHKERRQ(ierr);
          for (fc = 0; fc < fdof-fcdof; ++fc, ++fieldSizes[f]) {
            fieldIndices[f][fieldSizes[f]] = goff++;
          }
        }
      }
    }
    if (numFields) *numFields = nF;
    if (fieldNames) {
      ierr = PetscMalloc1(nF, fieldNames);CHKERRQ(ierr);
      for (f = 0; f < nF; ++f) {
        const char *fieldName;

        ierr = PetscSectionGetFieldName(section, f, &fieldName);CHKERRQ(ierr);
        ierr = PetscStrallocpy(fieldName, (char**) &(*fieldNames)[f]);CHKERRQ(ierr);
      }
    }
    if (fields) {
      ierr = PetscMalloc1(nF, fields);CHKERRQ(ierr);
      for (f = 0; f < nF; ++f) {
        ierr = ISCreateGeneral(PetscObjectComm((PetscObject)dm), fieldSizes[f], fieldIndices[f], PETSC_OWN_POINTER, &(*fields)[f]);CHKERRQ(ierr);
      }
    }
    ierr = PetscFree2(fieldSizes,fieldIndices);CHKERRQ(ierr);
  } else if (dm->ops->createfieldis) {
    ierr = (*dm->ops->createfieldis)(dm, numFields, fieldNames, fields);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMCreateFieldDecomposition"
/*@C
  DMCreateFieldDecomposition - Returns a list of IS objects defining a decomposition of a problem into subproblems
                          corresponding to different fields: each IS contains the global indices of the dofs of the
                          corresponding field. The optional list of DMs define the DM for each subproblem.
                          Generalizes DMCreateFieldIS().

  Not collective

  Input Parameter:
. dm - the DM object

  Output Parameters:
+ len       - The number of subproblems in the field decomposition (or NULL if not requested)
. namelist  - The name for each field (or NULL if not requested)
. islist    - The global indices for each field (or NULL if not requested)
- dmlist    - The DMs for each field subproblem (or NULL, if not requested; if NULL is returned, no DMs are defined)

  Level: intermediate

  Notes:
  The user is responsible for freeing all requested arrays. In particular, every entry of names should be freed with
  PetscFree(), every entry of is should be destroyed with ISDestroy(), every entry of dm should be destroyed with DMDestroy(),
  and all of the arrays should be freed with PetscFree().

.seealso DMDestroy(), DMView(), DMCreateInterpolation(), DMCreateColoring(), DMCreateMatrix(), DMCreateFieldIS()
@*/
PetscErrorCode DMCreateFieldDecomposition(DM dm, PetscInt *len, char ***namelist, IS **islist, DM **dmlist)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  if (len) {
    PetscValidPointer(len,2);
    *len = 0;
  }
  if (namelist) {
    PetscValidPointer(namelist,3);
    *namelist = 0;
  }
  if (islist) {
    PetscValidPointer(islist,4);
    *islist = 0;
  }
  if (dmlist) {
    PetscValidPointer(dmlist,5);
    *dmlist = 0;
  }
  /*
   Is it a good idea to apply the following check across all impls?
   Perhaps some impls can have a well-defined decomposition before DMSetUp?
   This, however, follows the general principle that accessors are not well-behaved until the object is set up.
   */
  if (!dm->setupcalled) SETERRQ(PetscObjectComm((PetscObject)dm),PETSC_ERR_ARG_WRONGSTATE, "Decomposition defined only after DMSetUp");
  if (!dm->ops->createfielddecomposition) {
    PetscSection section;
    PetscInt     numFields, f;

    ierr = DMGetDefaultSection(dm, &section);CHKERRQ(ierr);
    if (section) {ierr = PetscSectionGetNumFields(section, &numFields);CHKERRQ(ierr);}
    if (section && numFields && dm->ops->createsubdm) {
      *len = numFields;
      if (namelist) {ierr = PetscMalloc1(numFields,namelist);CHKERRQ(ierr);}
      if (islist)   {ierr = PetscMalloc1(numFields,islist);CHKERRQ(ierr);}
      if (dmlist)   {ierr = PetscMalloc1(numFields,dmlist);CHKERRQ(ierr);}
      for (f = 0; f < numFields; ++f) {
        const char *fieldName;

        ierr = DMCreateSubDM(dm, 1, &f, islist ? &(*islist)[f] : NULL, dmlist ? &(*dmlist)[f] : NULL);CHKERRQ(ierr);
        if (namelist) {
          ierr = PetscSectionGetFieldName(section, f, &fieldName);CHKERRQ(ierr);
          ierr = PetscStrallocpy(fieldName, (char**) &(*namelist)[f]);CHKERRQ(ierr);
        }
      }
    } else {
      ierr = DMCreateFieldIS(dm, len, namelist, islist);CHKERRQ(ierr);
      /* By default there are no DMs associated with subproblems. */
      if (dmlist) *dmlist = NULL;
    }
  } else {
    ierr = (*dm->ops->createfielddecomposition)(dm,len,namelist,islist,dmlist);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCreateSubDM"
/*@C
  DMCreateSubDM - Returns an IS and DM encapsulating a subproblem defined by the fields passed in.
                  The fields are defined by DMCreateFieldIS().

  Not collective

  Input Parameters:
+ dm - the DM object
. numFields - number of fields in this subproblem
- len       - The number of subproblems in the decomposition (or NULL if not requested)

  Output Parameters:
. is - The global indices for the subproblem
- dm - The DM for the subproblem

  Level: intermediate

.seealso DMDestroy(), DMView(), DMCreateInterpolation(), DMCreateColoring(), DMCreateMatrix(), DMCreateFieldIS()
@*/
PetscErrorCode DMCreateSubDM(DM dm, PetscInt numFields, PetscInt fields[], IS *is, DM *subdm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(fields,3);
  if (is) PetscValidPointer(is,4);
  if (subdm) PetscValidPointer(subdm,5);
  if (dm->ops->createsubdm) {
    ierr = (*dm->ops->createsubdm)(dm, numFields, fields, is, subdm);CHKERRQ(ierr);
  } else SETERRQ(PetscObjectComm((PetscObject)dm), PETSC_ERR_SUP, "This type has no DMCreateSubDM implementation defined");
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMCreateDomainDecomposition"
/*@C
  DMCreateDomainDecomposition - Returns lists of IS objects defining a decomposition of a problem into subproblems
                          corresponding to restrictions to pairs nested subdomains: each IS contains the global
                          indices of the dofs of the corresponding subdomains.  The inner subdomains conceptually
                          define a nonoverlapping covering, while outer subdomains can overlap.
                          The optional list of DMs define the DM for each subproblem.

  Not collective

  Input Parameter:
. dm - the DM object

  Output Parameters:
+ len         - The number of subproblems in the domain decomposition (or NULL if not requested)
. namelist    - The name for each subdomain (or NULL if not requested)
. innerislist - The global indices for each inner subdomain (or NULL, if not requested)
. outerislist - The global indices for each outer subdomain (or NULL, if not requested)
- dmlist      - The DMs for each subdomain subproblem (or NULL, if not requested; if NULL is returned, no DMs are defined)

  Level: intermediate

  Notes:
  The user is responsible for freeing all requested arrays. In particular, every entry of names should be freed with
  PetscFree(), every entry of is should be destroyed with ISDestroy(), every entry of dm should be destroyed with DMDestroy(),
  and all of the arrays should be freed with PetscFree().

.seealso DMDestroy(), DMView(), DMCreateInterpolation(), DMCreateColoring(), DMCreateMatrix(), DMCreateDomainDecompositionDM(), DMCreateFieldDecomposition()
@*/
PetscErrorCode DMCreateDomainDecomposition(DM dm, PetscInt *len, char ***namelist, IS **innerislist, IS **outerislist, DM **dmlist)
{
  PetscErrorCode      ierr;
  DMSubDomainHookLink link;
  PetscInt            i,l;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  if (len)           {PetscValidPointer(len,2);            *len         = 0;}
  if (namelist)      {PetscValidPointer(namelist,3);       *namelist    = NULL;}
  if (innerislist)   {PetscValidPointer(innerislist,4);    *innerislist = NULL;}
  if (outerislist)   {PetscValidPointer(outerislist,5);    *outerislist = NULL;}
  if (dmlist)        {PetscValidPointer(dmlist,6);         *dmlist      = NULL;}
  /*
   Is it a good idea to apply the following check across all impls?
   Perhaps some impls can have a well-defined decomposition before DMSetUp?
   This, however, follows the general principle that accessors are not well-behaved until the object is set up.
   */
  if (!dm->setupcalled) SETERRQ(PetscObjectComm((PetscObject)dm),PETSC_ERR_ARG_WRONGSTATE, "Decomposition defined only after DMSetUp");
  if (dm->ops->createdomaindecomposition) {
    ierr = (*dm->ops->createdomaindecomposition)(dm,&l,namelist,innerislist,outerislist,dmlist);CHKERRQ(ierr);
    /* copy subdomain hooks and context over to the subdomain DMs */
    if (dmlist) {
      for (i = 0; i < l; i++) {
        for (link=dm->subdomainhook; link; link=link->next) {
          if (link->ddhook) {ierr = (*link->ddhook)(dm,(*dmlist)[i],link->ctx);CHKERRQ(ierr);}
        }
        (*dmlist)[i]->ctx = dm->ctx;
      }
    }
    if (len) *len = l;
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMCreateDomainDecompositionScatters"
/*@C
  DMCreateDomainDecompositionScatters - Returns scatters to the subdomain vectors from the global vector

  Not collective

  Input Parameters:
+ dm - the DM object
. n  - the number of subdomain scatters
- subdms - the local subdomains

  Output Parameters:
+ n     - the number of scatters returned
. iscat - scatter from global vector to nonoverlapping global vector entries on subdomain
. oscat - scatter from global vector to overlapping global vector entries on subdomain
- gscat - scatter from global vector to local vector on subdomain (fills in ghosts)

  Notes: This is an alternative to the iis and ois arguments in DMCreateDomainDecomposition that allow for the solution
  of general nonlinear problems with overlapping subdomain methods.  While merely having index sets that enable subsets
  of the residual equations to be created is fine for linear problems, nonlinear problems require local assembly of
  solution and residual data.

  Level: developer

.seealso DMDestroy(), DMView(), DMCreateInterpolation(), DMCreateColoring(), DMCreateMatrix(), DMCreateFieldIS()
@*/
PetscErrorCode DMCreateDomainDecompositionScatters(DM dm,PetscInt n,DM *subdms,VecScatter **iscat,VecScatter **oscat,VecScatter **gscat)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(subdms,3);
  if (dm->ops->createddscatters) {
    ierr = (*dm->ops->createddscatters)(dm,n,subdms,iscat,oscat,gscat);CHKERRQ(ierr);
  } else SETERRQ(PetscObjectComm((PetscObject)dm), PETSC_ERR_SUP, "This type has no DMCreateDomainDecompositionLocalScatter implementation defined");
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMRefine"
/*@
  DMRefine - Refines a DM object

  Collective on DM

  Input Parameter:
+ dm   - the DM object
- comm - the communicator to contain the new DM object (or MPI_COMM_NULL)

  Output Parameter:
. dmf - the refined DM, or NULL

  Note: If no refinement was done, the return value is NULL

  Level: developer

.seealso DMCoarsen(), DMDestroy(), DMView(), DMCreateGlobalVector(), DMCreateInterpolation()
@*/
PetscErrorCode  DMRefine(DM dm,MPI_Comm comm,DM *dmf)
{
  PetscErrorCode   ierr;
  DMRefineHookLink link;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = (*dm->ops->refine)(dm,comm,dmf);CHKERRQ(ierr);
  if (*dmf) {
    (*dmf)->ops->creatematrix = dm->ops->creatematrix;

    ierr = PetscObjectCopyFortranFunctionPointers((PetscObject)dm,(PetscObject)*dmf);CHKERRQ(ierr);

    (*dmf)->ctx       = dm->ctx;
    (*dmf)->leveldown = dm->leveldown;
    (*dmf)->levelup   = dm->levelup + 1;

    ierr = DMSetMatType(*dmf,dm->mattype);CHKERRQ(ierr);
    for (link=dm->refinehook; link; link=link->next) {
      if (link->refinehook) {
        ierr = (*link->refinehook)(dm,*dmf,link->ctx);CHKERRQ(ierr);
      }
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMRefineHookAdd"
/*@C
   DMRefineHookAdd - adds a callback to be run when interpolating a nonlinear problem to a finer grid

   Logically Collective

   Input Arguments:
+  coarse - nonlinear solver context on which to run a hook when restricting to a coarser level
.  refinehook - function to run when setting up a coarser level
.  interphook - function to run to update data on finer levels (once per SNESSolve())
-  ctx - [optional] user-defined context for provide data for the hooks (may be NULL)

   Calling sequence of refinehook:
$    refinehook(DM coarse,DM fine,void *ctx);

+  coarse - coarse level DM
.  fine - fine level DM to interpolate problem to
-  ctx - optional user-defined function context

   Calling sequence for interphook:
$    interphook(DM coarse,Mat interp,DM fine,void *ctx)

+  coarse - coarse level DM
.  interp - matrix interpolating a coarse-level solution to the finer grid
.  fine - fine level DM to update
-  ctx - optional user-defined function context

   Level: advanced

   Notes:
   This function is only needed if auxiliary data needs to be passed to fine grids while grid sequencing

   If this function is called multiple times, the hooks will be run in the order they are added.

   This function is currently not available from Fortran.

.seealso: DMCoarsenHookAdd(), SNESFASGetInterpolation(), SNESFASGetInjection(), PetscObjectCompose(), PetscContainerCreate()
@*/
PetscErrorCode DMRefineHookAdd(DM coarse,PetscErrorCode (*refinehook)(DM,DM,void*),PetscErrorCode (*interphook)(DM,Mat,DM,void*),void *ctx)
{
  PetscErrorCode   ierr;
  DMRefineHookLink link,*p;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(coarse,DM_CLASSID,1);
  for (p=&coarse->refinehook; *p; p=&(*p)->next) {} /* Scan to the end of the current list of hooks */
  ierr             = PetscMalloc(sizeof(struct _DMRefineHookLink),&link);CHKERRQ(ierr);
  link->refinehook = refinehook;
  link->interphook = interphook;
  link->ctx        = ctx;
  link->next       = NULL;
  *p               = link;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMInterpolate"
/*@
   DMInterpolate - interpolates user-defined problem data to a finer DM by running hooks registered by DMRefineHookAdd()

   Collective if any hooks are

   Input Arguments:
+  coarse - coarser DM to use as a base
.  restrct - interpolation matrix, apply using MatInterpolate()
-  fine - finer DM to update

   Level: developer

.seealso: DMRefineHookAdd(), MatInterpolate()
@*/
PetscErrorCode DMInterpolate(DM coarse,Mat interp,DM fine)
{
  PetscErrorCode   ierr;
  DMRefineHookLink link;

  PetscFunctionBegin;
  for (link=fine->refinehook; link; link=link->next) {
    if (link->interphook) {
      ierr = (*link->interphook)(coarse,interp,fine,link->ctx);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetRefineLevel"
/*@
    DMGetRefineLevel - Get's the number of refinements that have generated this DM.

    Not Collective

    Input Parameter:
.   dm - the DM object

    Output Parameter:
.   level - number of refinements

    Level: developer

.seealso DMCoarsen(), DMGetCoarsenLevel(), DMDestroy(), DMView(), DMCreateGlobalVector(), DMCreateInterpolation()

@*/
PetscErrorCode  DMGetRefineLevel(DM dm,PetscInt *level)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  *level = dm->levelup;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGlobalToLocalHookAdd"
/*@C
   DMGlobalToLocalHookAdd - adds a callback to be run when global to local is called

   Logically Collective

   Input Arguments:
+  dm - the DM
.  beginhook - function to run at the beginning of DMGlobalToLocalBegin()
.  endhook - function to run after DMGlobalToLocalEnd() has completed
-  ctx - [optional] user-defined context for provide data for the hooks (may be NULL)

   Calling sequence for beginhook:
$    beginhook(DM fine,VecScatter out,VecScatter in,DM coarse,void *ctx)

+  dm - global DM
.  g - global vector
.  mode - mode
.  l - local vector
-  ctx - optional user-defined function context


   Calling sequence for endhook:
$    endhook(DM fine,VecScatter out,VecScatter in,DM coarse,void *ctx)

+  global - global DM
-  ctx - optional user-defined function context

   Level: advanced

.seealso: DMRefineHookAdd(), SNESFASGetInterpolation(), SNESFASGetInjection(), PetscObjectCompose(), PetscContainerCreate()
@*/
PetscErrorCode DMGlobalToLocalHookAdd(DM dm,PetscErrorCode (*beginhook)(DM,Vec,InsertMode,Vec,void*),PetscErrorCode (*endhook)(DM,Vec,InsertMode,Vec,void*),void *ctx)
{
  PetscErrorCode          ierr;
  DMGlobalToLocalHookLink link,*p;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  for (p=&dm->gtolhook; *p; p=&(*p)->next) {} /* Scan to the end of the current list of hooks */
  ierr            = PetscMalloc(sizeof(struct _DMGlobalToLocalHookLink),&link);CHKERRQ(ierr);
  link->beginhook = beginhook;
  link->endhook   = endhook;
  link->ctx       = ctx;
  link->next      = NULL;
  *p              = link;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGlobalToLocalBegin"
/*@
    DMGlobalToLocalBegin - Begins updating local vectors from global vector

    Neighbor-wise Collective on DM

    Input Parameters:
+   dm - the DM object
.   g - the global vector
.   mode - INSERT_VALUES or ADD_VALUES
-   l - the local vector


    Level: beginner

.seealso DMCoarsen(), DMDestroy(), DMView(), DMCreateGlobalVector(), DMCreateInterpolation(), DMGlobalToLocalEnd(), DMLocalToGlobalBegin()

@*/
PetscErrorCode  DMGlobalToLocalBegin(DM dm,Vec g,InsertMode mode,Vec l)
{
  PetscSF                 sf;
  PetscErrorCode          ierr;
  DMGlobalToLocalHookLink link;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  for (link=dm->gtolhook; link; link=link->next) {
    if (link->beginhook) {
      ierr = (*link->beginhook)(dm,g,mode,l,link->ctx);CHKERRQ(ierr);
    }
  }
  ierr = DMGetDefaultSF(dm, &sf);CHKERRQ(ierr);
  if (sf) {
    PetscScalar *lArray, *gArray;

    if (mode == ADD_VALUES) SETERRQ1(PetscObjectComm((PetscObject)dm), PETSC_ERR_ARG_OUTOFRANGE, "Invalid insertion mode %D", mode);
    ierr = VecGetArray(l, &lArray);CHKERRQ(ierr);
    ierr = VecGetArray(g, &gArray);CHKERRQ(ierr);
    ierr = PetscSFBcastBegin(sf, MPIU_SCALAR, gArray, lArray);CHKERRQ(ierr);
    ierr = VecRestoreArray(l, &lArray);CHKERRQ(ierr);
    ierr = VecRestoreArray(g, &gArray);CHKERRQ(ierr);
  } else {
    ierr = (*dm->ops->globaltolocalbegin)(dm,g,mode == INSERT_ALL_VALUES ? INSERT_VALUES : (mode == ADD_ALL_VALUES ? ADD_VALUES : mode),l);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGlobalToLocalEnd"
/*@
    DMGlobalToLocalEnd - Ends updating local vectors from global vector

    Neighbor-wise Collective on DM

    Input Parameters:
+   dm - the DM object
.   g - the global vector
.   mode - INSERT_VALUES or ADD_VALUES
-   l - the local vector


    Level: beginner

.seealso DMCoarsen(), DMDestroy(), DMView(), DMCreateGlobalVector(), DMCreateInterpolation(), DMGlobalToLocalEnd(), DMLocalToGlobalBegin()

@*/
PetscErrorCode  DMGlobalToLocalEnd(DM dm,Vec g,InsertMode mode,Vec l)
{
  PetscSF                 sf;
  PetscErrorCode          ierr;
  PetscScalar             *lArray, *gArray;
  DMGlobalToLocalHookLink link;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDefaultSF(dm, &sf);CHKERRQ(ierr);
  if (sf) {
    if (mode == ADD_VALUES) SETERRQ1(PetscObjectComm((PetscObject)dm), PETSC_ERR_ARG_OUTOFRANGE, "Invalid insertion mode %D", mode);

    ierr = VecGetArray(l, &lArray);CHKERRQ(ierr);
    ierr = VecGetArray(g, &gArray);CHKERRQ(ierr);
    ierr = PetscSFBcastEnd(sf, MPIU_SCALAR, gArray, lArray);CHKERRQ(ierr);
    ierr = VecRestoreArray(l, &lArray);CHKERRQ(ierr);
    ierr = VecRestoreArray(g, &gArray);CHKERRQ(ierr);
  } else {
    ierr = (*dm->ops->globaltolocalend)(dm,g,mode == INSERT_ALL_VALUES ? INSERT_VALUES : (mode == ADD_ALL_VALUES ? ADD_VALUES : mode),l);CHKERRQ(ierr);
  }
  for (link=dm->gtolhook; link; link=link->next) {
    if (link->endhook) {ierr = (*link->endhook)(dm,g,mode,l,link->ctx);CHKERRQ(ierr);}
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMLocalToGlobalBegin"
/*@
    DMLocalToGlobalBegin - updates global vectors from local vectors

    Neighbor-wise Collective on DM

    Input Parameters:
+   dm - the DM object
.   l - the local vector
.   mode - if INSERT_VALUES then no parallel communication is used, if ADD_VALUES then all ghost points from the same base point accumulate into that
           base point.
- - the global vector

    Notes: In the ADD_VALUES case you normally would zero the receiving vector before beginning this operation. If you would like to simply add the non-ghosted values in the local
           array into the global array you need to either (1) zero the ghosted locations and use ADD_VALUES or (2) use INSERT_VALUES into a work global array and then add the work
           global array to the final global array with VecAXPY().

    Level: beginner

.seealso DMCoarsen(), DMDestroy(), DMView(), DMCreateGlobalVector(), DMCreateInterpolation(), DMGlobalToLocalEnd(), DMGlobalToLocalBegin()

@*/
PetscErrorCode  DMLocalToGlobalBegin(DM dm,Vec l,InsertMode mode,Vec g)
{
  PetscSF        sf;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDefaultSF(dm, &sf);CHKERRQ(ierr);
  if (sf) {
    MPI_Op      op;
    PetscScalar *lArray, *gArray;

    switch (mode) {
    case INSERT_VALUES:
    case INSERT_ALL_VALUES:
      op = MPIU_REPLACE; break;
    case ADD_VALUES:
    case ADD_ALL_VALUES:
      op = MPI_SUM; break;
    default:
      SETERRQ1(PetscObjectComm((PetscObject)dm), PETSC_ERR_ARG_OUTOFRANGE, "Invalid insertion mode %D", mode);
    }
    ierr = VecGetArray(l, &lArray);CHKERRQ(ierr);
    ierr = VecGetArray(g, &gArray);CHKERRQ(ierr);
    ierr = PetscSFReduceBegin(sf, MPIU_SCALAR, lArray, gArray, op);CHKERRQ(ierr);
    ierr = VecRestoreArray(l, &lArray);CHKERRQ(ierr);
    ierr = VecRestoreArray(g, &gArray);CHKERRQ(ierr);
  } else {
    ierr = (*dm->ops->localtoglobalbegin)(dm,l,mode == INSERT_ALL_VALUES ? INSERT_VALUES : (mode == ADD_ALL_VALUES ? ADD_VALUES : mode),g);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMLocalToGlobalEnd"
/*@
    DMLocalToGlobalEnd - updates global vectors from local vectors

    Neighbor-wise Collective on DM

    Input Parameters:
+   dm - the DM object
.   l - the local vector
.   mode - INSERT_VALUES or ADD_VALUES
-   g - the global vector


    Level: beginner

.seealso DMCoarsen(), DMDestroy(), DMView(), DMCreateGlobalVector(), DMCreateInterpolation(), DMGlobalToLocalEnd(), DMGlobalToLocalEnd()

@*/
PetscErrorCode  DMLocalToGlobalEnd(DM dm,Vec l,InsertMode mode,Vec g)
{
  PetscSF        sf;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDefaultSF(dm, &sf);CHKERRQ(ierr);
  if (sf) {
    MPI_Op      op;
    PetscScalar *lArray, *gArray;

    switch (mode) {
    case INSERT_VALUES:
    case INSERT_ALL_VALUES:
      op = MPIU_REPLACE; break;
    case ADD_VALUES:
    case ADD_ALL_VALUES:
      op = MPI_SUM; break;
    default:
      SETERRQ1(PetscObjectComm((PetscObject)dm), PETSC_ERR_ARG_OUTOFRANGE, "Invalid insertion mode %D", mode);
    }
    ierr = VecGetArray(l, &lArray);CHKERRQ(ierr);
    ierr = VecGetArray(g, &gArray);CHKERRQ(ierr);
    ierr = PetscSFReduceEnd(sf, MPIU_SCALAR, lArray, gArray, op);CHKERRQ(ierr);
    ierr = VecRestoreArray(l, &lArray);CHKERRQ(ierr);
    ierr = VecRestoreArray(g, &gArray);CHKERRQ(ierr);
  } else {
    ierr = (*dm->ops->localtoglobalend)(dm,l,mode == INSERT_ALL_VALUES ? INSERT_VALUES : (mode == ADD_ALL_VALUES ? ADD_VALUES : mode),g);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMLocalToLocalBegin"
/*@
   DMLocalToLocalBegin - Maps from a local vector (including ghost points
   that contain irrelevant values) to another local vector where the ghost
   points in the second are set correctly. Must be followed by DMLocalToLocalEnd().

   Neighbor-wise Collective on DM and Vec

   Input Parameters:
+  dm - the DM object
.  g - the original local vector
-  mode - one of INSERT_VALUES or ADD_VALUES

   Output Parameter:
.  l  - the local vector with correct ghost values

   Level: intermediate

   Notes:
   The local vectors used here need not be the same as those
   obtained from DMCreateLocalVector(), BUT they
   must have the same parallel data layout; they could, for example, be
   obtained with VecDuplicate() from the DM originating vectors.

.keywords: DM, local-to-local, begin
.seealso DMCoarsen(), DMDestroy(), DMView(), DMCreateLocalVector(), DMCreateGlobalVector(), DMCreateInterpolation(), DMLocalToLocalEnd(), DMGlobalToLocalEnd(), DMLocalToGlobalBegin()

@*/
PetscErrorCode  DMLocalToLocalBegin(DM dm,Vec g,InsertMode mode,Vec l)
{
  PetscErrorCode          ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = (*dm->ops->localtolocalbegin)(dm,g,mode == INSERT_ALL_VALUES ? INSERT_VALUES : (mode == ADD_ALL_VALUES ? ADD_VALUES : mode),l);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMLocalToLocalEnd"
/*@
   DMLocalToLocalEnd - Maps from a local vector (including ghost points
   that contain irrelevant values) to another local vector where the ghost
   points in the second are set correctly. Must be preceded by DMLocalToLocalBegin().

   Neighbor-wise Collective on DM and Vec

   Input Parameters:
+  da - the DM object
.  g - the original local vector
-  mode - one of INSERT_VALUES or ADD_VALUES

   Output Parameter:
.  l  - the local vector with correct ghost values

   Level: intermediate

   Notes:
   The local vectors used here need not be the same as those
   obtained from DMCreateLocalVector(), BUT they
   must have the same parallel data layout; they could, for example, be
   obtained with VecDuplicate() from the DM originating vectors.

.keywords: DM, local-to-local, end
.seealso DMCoarsen(), DMDestroy(), DMView(), DMCreateLocalVector(), DMCreateGlobalVector(), DMCreateInterpolation(), DMLocalToLocalBegin(), DMGlobalToLocalEnd(), DMLocalToGlobalBegin()

@*/
PetscErrorCode  DMLocalToLocalEnd(DM dm,Vec g,InsertMode mode,Vec l)
{
  PetscErrorCode          ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = (*dm->ops->localtolocalend)(dm,g,mode == INSERT_ALL_VALUES ? INSERT_VALUES : (mode == ADD_ALL_VALUES ? ADD_VALUES : mode),l);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMCoarsen"
/*@
    DMCoarsen - Coarsens a DM object

    Collective on DM

    Input Parameter:
+   dm - the DM object
-   comm - the communicator to contain the new DM object (or MPI_COMM_NULL)

    Output Parameter:
.   dmc - the coarsened DM

    Level: developer

.seealso DMRefine(), DMDestroy(), DMView(), DMCreateGlobalVector(), DMCreateInterpolation()

@*/
PetscErrorCode  DMCoarsen(DM dm, MPI_Comm comm, DM *dmc)
{
  PetscErrorCode    ierr;
  DMCoarsenHookLink link;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr                      = (*dm->ops->coarsen)(dm, comm, dmc);CHKERRQ(ierr);
  (*dmc)->ops->creatematrix = dm->ops->creatematrix;
  ierr                      = PetscObjectCopyFortranFunctionPointers((PetscObject)dm,(PetscObject)*dmc);CHKERRQ(ierr);
  (*dmc)->ctx               = dm->ctx;
  (*dmc)->levelup           = dm->levelup;
  (*dmc)->leveldown         = dm->leveldown + 1;
  ierr                      = DMSetMatType(*dmc,dm->mattype);CHKERRQ(ierr);
  for (link=dm->coarsenhook; link; link=link->next) {
    if (link->coarsenhook) {ierr = (*link->coarsenhook)(dm,*dmc,link->ctx);CHKERRQ(ierr);}
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCoarsenHookAdd"
/*@C
   DMCoarsenHookAdd - adds a callback to be run when restricting a nonlinear problem to the coarse grid

   Logically Collective

   Input Arguments:
+  fine - nonlinear solver context on which to run a hook when restricting to a coarser level
.  coarsenhook - function to run when setting up a coarser level
.  restricthook - function to run to update data on coarser levels (once per SNESSolve())
-  ctx - [optional] user-defined context for provide data for the hooks (may be NULL)

   Calling sequence of coarsenhook:
$    coarsenhook(DM fine,DM coarse,void *ctx);

+  fine - fine level DM
.  coarse - coarse level DM to restrict problem to
-  ctx - optional user-defined function context

   Calling sequence for restricthook:
$    restricthook(DM fine,Mat mrestrict,Vec rscale,Mat inject,DM coarse,void *ctx)

+  fine - fine level DM
.  mrestrict - matrix restricting a fine-level solution to the coarse grid
.  rscale - scaling vector for restriction
.  inject - matrix restricting by injection
.  coarse - coarse level DM to update
-  ctx - optional user-defined function context

   Level: advanced

   Notes:
   This function is only needed if auxiliary data needs to be set up on coarse grids.

   If this function is called multiple times, the hooks will be run in the order they are added.

   In order to compose with nonlinear preconditioning without duplicating storage, the hook should be implemented to
   extract the finest level information from its context (instead of from the SNES).

   This function is currently not available from Fortran.

.seealso: DMRefineHookAdd(), SNESFASGetInterpolation(), SNESFASGetInjection(), PetscObjectCompose(), PetscContainerCreate()
@*/
PetscErrorCode DMCoarsenHookAdd(DM fine,PetscErrorCode (*coarsenhook)(DM,DM,void*),PetscErrorCode (*restricthook)(DM,Mat,Vec,Mat,DM,void*),void *ctx)
{
  PetscErrorCode    ierr;
  DMCoarsenHookLink link,*p;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(fine,DM_CLASSID,1);
  for (p=&fine->coarsenhook; *p; p=&(*p)->next) {} /* Scan to the end of the current list of hooks */
  ierr               = PetscMalloc(sizeof(struct _DMCoarsenHookLink),&link);CHKERRQ(ierr);
  link->coarsenhook  = coarsenhook;
  link->restricthook = restricthook;
  link->ctx          = ctx;
  link->next         = NULL;
  *p                 = link;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMRestrict"
/*@
   DMRestrict - restricts user-defined problem data to a coarser DM by running hooks registered by DMCoarsenHookAdd()

   Collective if any hooks are

   Input Arguments:
+  fine - finer DM to use as a base
.  restrct - restriction matrix, apply using MatRestrict()
.  inject - injection matrix, also use MatRestrict()
-  coarse - coarer DM to update

   Level: developer

.seealso: DMCoarsenHookAdd(), MatRestrict()
@*/
PetscErrorCode DMRestrict(DM fine,Mat restrct,Vec rscale,Mat inject,DM coarse)
{
  PetscErrorCode    ierr;
  DMCoarsenHookLink link;

  PetscFunctionBegin;
  for (link=fine->coarsenhook; link; link=link->next) {
    if (link->restricthook) {
      ierr = (*link->restricthook)(fine,restrct,rscale,inject,coarse,link->ctx);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSubDomainHookAdd"
/*@C
   DMSubDomainHookAdd - adds a callback to be run when restricting a problem to the coarse grid

   Logically Collective

   Input Arguments:
+  global - global DM
.  ddhook - function to run to pass data to the decomposition DM upon its creation
.  restricthook - function to run to update data on block solve (at the beginning of the block solve)
-  ctx - [optional] user-defined context for provide data for the hooks (may be NULL)


   Calling sequence for ddhook:
$    ddhook(DM global,DM block,void *ctx)

+  global - global DM
.  block  - block DM
-  ctx - optional user-defined function context

   Calling sequence for restricthook:
$    restricthook(DM global,VecScatter out,VecScatter in,DM block,void *ctx)

+  global - global DM
.  out    - scatter to the outer (with ghost and overlap points) block vector
.  in     - scatter to block vector values only owned locally
.  block  - block DM
-  ctx - optional user-defined function context

   Level: advanced

   Notes:
   This function is only needed if auxiliary data needs to be set up on subdomain DMs.

   If this function is called multiple times, the hooks will be run in the order they are added.

   In order to compose with nonlinear preconditioning without duplicating storage, the hook should be implemented to
   extract the global information from its context (instead of from the SNES).

   This function is currently not available from Fortran.

.seealso: DMRefineHookAdd(), SNESFASGetInterpolation(), SNESFASGetInjection(), PetscObjectCompose(), PetscContainerCreate()
@*/
PetscErrorCode DMSubDomainHookAdd(DM global,PetscErrorCode (*ddhook)(DM,DM,void*),PetscErrorCode (*restricthook)(DM,VecScatter,VecScatter,DM,void*),void *ctx)
{
  PetscErrorCode      ierr;
  DMSubDomainHookLink link,*p;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(global,DM_CLASSID,1);
  for (p=&global->subdomainhook; *p; p=&(*p)->next) {} /* Scan to the end of the current list of hooks */
  ierr               = PetscMalloc(sizeof(struct _DMSubDomainHookLink),&link);CHKERRQ(ierr);
  link->restricthook = restricthook;
  link->ddhook       = ddhook;
  link->ctx          = ctx;
  link->next         = NULL;
  *p                 = link;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSubDomainRestrict"
/*@
   DMSubDomainRestrict - restricts user-defined problem data to a block DM by running hooks registered by DMSubDomainHookAdd()

   Collective if any hooks are

   Input Arguments:
+  fine - finer DM to use as a base
.  oscatter - scatter from domain global vector filling subdomain global vector with overlap
.  gscatter - scatter from domain global vector filling subdomain local vector with ghosts
-  coarse - coarer DM to update

   Level: developer

.seealso: DMCoarsenHookAdd(), MatRestrict()
@*/
PetscErrorCode DMSubDomainRestrict(DM global,VecScatter oscatter,VecScatter gscatter,DM subdm)
{
  PetscErrorCode      ierr;
  DMSubDomainHookLink link;

  PetscFunctionBegin;
  for (link=global->subdomainhook; link; link=link->next) {
    if (link->restricthook) {
      ierr = (*link->restricthook)(global,oscatter,gscatter,subdm,link->ctx);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetCoarsenLevel"
/*@
    DMGetCoarsenLevel - Get's the number of coarsenings that have generated this DM.

    Not Collective

    Input Parameter:
.   dm - the DM object

    Output Parameter:
.   level - number of coarsenings

    Level: developer

.seealso DMCoarsen(), DMGetRefineLevel(), DMDestroy(), DMView(), DMCreateGlobalVector(), DMCreateInterpolation()

@*/
PetscErrorCode  DMGetCoarsenLevel(DM dm,PetscInt *level)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  *level = dm->leveldown;
  PetscFunctionReturn(0);
}



#undef __FUNCT__
#define __FUNCT__ "DMRefineHierarchy"
/*@C
    DMRefineHierarchy - Refines a DM object, all levels at once

    Collective on DM

    Input Parameter:
+   dm - the DM object
-   nlevels - the number of levels of refinement

    Output Parameter:
.   dmf - the refined DM hierarchy

    Level: developer

.seealso DMCoarsenHierarchy(), DMDestroy(), DMView(), DMCreateGlobalVector(), DMCreateInterpolation()

@*/
PetscErrorCode  DMRefineHierarchy(DM dm,PetscInt nlevels,DM dmf[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  if (nlevels < 0) SETERRQ(PetscObjectComm((PetscObject)dm),PETSC_ERR_ARG_OUTOFRANGE,"nlevels cannot be negative");
  if (nlevels == 0) PetscFunctionReturn(0);
  if (dm->ops->refinehierarchy) {
    ierr = (*dm->ops->refinehierarchy)(dm,nlevels,dmf);CHKERRQ(ierr);
  } else if (dm->ops->refine) {
    PetscInt i;

    ierr = DMRefine(dm,PetscObjectComm((PetscObject)dm),&dmf[0]);CHKERRQ(ierr);
    for (i=1; i<nlevels; i++) {
      ierr = DMRefine(dmf[i-1],PetscObjectComm((PetscObject)dm),&dmf[i]);CHKERRQ(ierr);
    }
  } else SETERRQ(PetscObjectComm((PetscObject)dm),PETSC_ERR_SUP,"No RefineHierarchy for this DM yet");
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCoarsenHierarchy"
/*@C
    DMCoarsenHierarchy - Coarsens a DM object, all levels at once

    Collective on DM

    Input Parameter:
+   dm - the DM object
-   nlevels - the number of levels of coarsening

    Output Parameter:
.   dmc - the coarsened DM hierarchy

    Level: developer

.seealso DMRefineHierarchy(), DMDestroy(), DMView(), DMCreateGlobalVector(), DMCreateInterpolation()

@*/
PetscErrorCode  DMCoarsenHierarchy(DM dm, PetscInt nlevels, DM dmc[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  if (nlevels < 0) SETERRQ(PetscObjectComm((PetscObject)dm),PETSC_ERR_ARG_OUTOFRANGE,"nlevels cannot be negative");
  if (nlevels == 0) PetscFunctionReturn(0);
  PetscValidPointer(dmc,3);
  if (dm->ops->coarsenhierarchy) {
    ierr = (*dm->ops->coarsenhierarchy)(dm, nlevels, dmc);CHKERRQ(ierr);
  } else if (dm->ops->coarsen) {
    PetscInt i;

    ierr = DMCoarsen(dm,PetscObjectComm((PetscObject)dm),&dmc[0]);CHKERRQ(ierr);
    for (i=1; i<nlevels; i++) {
      ierr = DMCoarsen(dmc[i-1],PetscObjectComm((PetscObject)dm),&dmc[i]);CHKERRQ(ierr);
    }
  } else SETERRQ(PetscObjectComm((PetscObject)dm),PETSC_ERR_SUP,"No CoarsenHierarchy for this DM yet");
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCreateAggregates"
/*@
   DMCreateAggregates - Gets the aggregates that map between
   grids associated with two DMs.

   Collective on DM

   Input Parameters:
+  dmc - the coarse grid DM
-  dmf - the fine grid DM

   Output Parameters:
.  rest - the restriction matrix (transpose of the projection matrix)

   Level: intermediate

.keywords: interpolation, restriction, multigrid

.seealso: DMRefine(), DMCreateInjection(), DMCreateInterpolation()
@*/
PetscErrorCode  DMCreateAggregates(DM dmc, DM dmf, Mat *rest)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dmc,DM_CLASSID,1);
  PetscValidHeaderSpecific(dmf,DM_CLASSID,2);
  ierr = (*dmc->ops->getaggregates)(dmc, dmf, rest);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetApplicationContextDestroy"
/*@C
    DMSetApplicationContextDestroy - Sets a user function that will be called to destroy the application context when the DM is destroyed

    Not Collective

    Input Parameters:
+   dm - the DM object
-   destroy - the destroy function

    Level: intermediate

.seealso DMView(), DMCreateGlobalVector(), DMCreateInterpolation(), DMCreateColoring(), DMCreateMatrix(), DMGetApplicationContext()

@*/
PetscErrorCode  DMSetApplicationContextDestroy(DM dm,PetscErrorCode (*destroy)(void**))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  dm->ctxdestroy = destroy;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetApplicationContext"
/*@
    DMSetApplicationContext - Set a user context into a DM object

    Not Collective

    Input Parameters:
+   dm - the DM object
-   ctx - the user context

    Level: intermediate

.seealso DMView(), DMCreateGlobalVector(), DMCreateInterpolation(), DMCreateColoring(), DMCreateMatrix(), DMGetApplicationContext()

@*/
PetscErrorCode  DMSetApplicationContext(DM dm,void *ctx)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  dm->ctx = ctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetApplicationContext"
/*@
    DMGetApplicationContext - Gets a user context from a DM object

    Not Collective

    Input Parameter:
.   dm - the DM object

    Output Parameter:
.   ctx - the user context

    Level: intermediate

.seealso DMView(), DMCreateGlobalVector(), DMCreateInterpolation(), DMCreateColoring(), DMCreateMatrix(), DMGetApplicationContext()

@*/
PetscErrorCode  DMGetApplicationContext(DM dm,void *ctx)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  *(void**)ctx = dm->ctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetVariableBounds"
/*@C
    DMSetVariableBounds - sets a function to compute the lower and upper bound vectors for SNESVI.

    Logically Collective on DM

    Input Parameter:
+   dm - the DM object
-   f - the function that computes variable bounds used by SNESVI (use NULL to cancel a previous function that was set)

    Level: intermediate

.seealso DMView(), DMCreateGlobalVector(), DMCreateInterpolation(), DMCreateColoring(), DMCreateMatrix(), DMGetApplicationContext(),
         DMSetJacobian()

@*/
PetscErrorCode  DMSetVariableBounds(DM dm,PetscErrorCode (*f)(DM,Vec,Vec))
{
  PetscFunctionBegin;
  dm->ops->computevariablebounds = f;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMHasVariableBounds"
/*@
    DMHasVariableBounds - does the DM object have a variable bounds function?

    Not Collective

    Input Parameter:
.   dm - the DM object to destroy

    Output Parameter:
.   flg - PETSC_TRUE if the variable bounds function exists

    Level: developer

.seealso DMView(), DMCreateGlobalVector(), DMCreateInterpolation(), DMCreateColoring(), DMCreateMatrix(), DMGetApplicationContext()

@*/
PetscErrorCode  DMHasVariableBounds(DM dm,PetscBool  *flg)
{
  PetscFunctionBegin;
  *flg =  (dm->ops->computevariablebounds) ? PETSC_TRUE : PETSC_FALSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMComputeVariableBounds"
/*@C
    DMComputeVariableBounds - compute variable bounds used by SNESVI.

    Logically Collective on DM

    Input Parameters:
+   dm - the DM object to destroy
-   x  - current solution at which the bounds are computed

    Output parameters:
+   xl - lower bound
-   xu - upper bound

    Level: intermediate

.seealso DMView(), DMCreateGlobalVector(), DMCreateInterpolation(), DMCreateColoring(), DMCreateMatrix(), DMGetApplicationContext()

@*/
PetscErrorCode  DMComputeVariableBounds(DM dm, Vec xl, Vec xu)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(xl,VEC_CLASSID,2);
  PetscValidHeaderSpecific(xu,VEC_CLASSID,2);
  if (dm->ops->computevariablebounds) {
    ierr = (*dm->ops->computevariablebounds)(dm, xl,xu);CHKERRQ(ierr);
  } else SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_WRONGSTATE, "This DM is incapable of computing variable bounds.");
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMHasColoring"
/*@
    DMHasColoring - does the DM object have a method of providing a coloring?

    Not Collective

    Input Parameter:
.   dm - the DM object

    Output Parameter:
.   flg - PETSC_TRUE if the DM has facilities for DMCreateColoring().

    Level: developer

.seealso DMHasFunction(), DMCreateColoring()

@*/
PetscErrorCode  DMHasColoring(DM dm,PetscBool  *flg)
{
  PetscFunctionBegin;
  *flg =  (dm->ops->getcoloring) ? PETSC_TRUE : PETSC_FALSE;
  PetscFunctionReturn(0);
}

#undef  __FUNCT__
#define __FUNCT__ "DMSetVec"
/*@C
    DMSetVec - set the vector at which to compute residual, Jacobian and VI bounds, if the problem is nonlinear.

    Collective on DM

    Input Parameter:
+   dm - the DM object
-   x - location to compute residual and Jacobian, if NULL is passed to those routines; will be NULL for linear problems.

    Level: developer

.seealso DMView(), DMCreateGlobalVector(), DMCreateInterpolation(), DMCreateColoring(), DMCreateMatrix(), DMGetApplicationContext()

@*/
PetscErrorCode  DMSetVec(DM dm,Vec x)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (x) {
    if (!dm->x) {
      ierr = DMCreateGlobalVector(dm,&dm->x);CHKERRQ(ierr);
    }
    ierr = VecCopy(x,dm->x);CHKERRQ(ierr);
  } else if (dm->x) {
    ierr = VecDestroy(&dm->x);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

PetscFunctionList DMList              = NULL;
PetscBool         DMRegisterAllCalled = PETSC_FALSE;

#undef __FUNCT__
#define __FUNCT__ "DMSetType"
/*@C
  DMSetType - Builds a DM, for a particular DM implementation.

  Collective on DM

  Input Parameters:
+ dm     - The DM object
- method - The name of the DM type

  Options Database Key:
. -dm_type <type> - Sets the DM type; use -help for a list of available types

  Notes:
  See "petsc/include/petscdm.h" for available DM types (for instance, DM1D, DM2D, or DM3D).

  Level: intermediate

.keywords: DM, set, type
.seealso: DMGetType(), DMCreate()
@*/
PetscErrorCode  DMSetType(DM dm, DMType method)
{
  PetscErrorCode (*r)(DM);
  PetscBool      match;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID,1);
  ierr = PetscObjectTypeCompare((PetscObject) dm, method, &match);CHKERRQ(ierr);
  if (match) PetscFunctionReturn(0);

  if (!DMRegisterAllCalled) {ierr = DMRegisterAll();CHKERRQ(ierr);}
  ierr = PetscFunctionListFind(DMList,method,&r);CHKERRQ(ierr);
  if (!r) SETERRQ1(PetscObjectComm((PetscObject)dm),PETSC_ERR_ARG_UNKNOWN_TYPE, "Unknown DM type: %s", method);

  if (dm->ops->destroy) {
    ierr             = (*dm->ops->destroy)(dm);CHKERRQ(ierr);
    dm->ops->destroy = NULL;
  }
  ierr = (*r)(dm);CHKERRQ(ierr);
  ierr = PetscObjectChangeTypeName((PetscObject)dm,method);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetType"
/*@C
  DMGetType - Gets the DM type name (as a string) from the DM.

  Not Collective

  Input Parameter:
. dm  - The DM

  Output Parameter:
. type - The DM type name

  Level: intermediate

.keywords: DM, get, type, name
.seealso: DMSetType(), DMCreate()
@*/
PetscErrorCode  DMGetType(DM dm, DMType *type)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID,1);
  PetscValidPointer(type,2);
  if (!DMRegisterAllCalled) {
    ierr = DMRegisterAll();CHKERRQ(ierr);
  }
  *type = ((PetscObject)dm)->type_name;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMConvert"
/*@C
  DMConvert - Converts a DM to another DM, either of the same or different type.

  Collective on DM

  Input Parameters:
+ dm - the DM
- newtype - new DM type (use "same" for the same type)

  Output Parameter:
. M - pointer to new DM

  Notes:
  Cannot be used to convert a sequential DM to parallel or parallel to sequential,
  the MPI communicator of the generated DM is always the same as the communicator
  of the input DM.

  Level: intermediate

.seealso: DMCreate()
@*/
PetscErrorCode DMConvert(DM dm, DMType newtype, DM *M)
{
  DM             B;
  char           convname[256];
  PetscBool      sametype, issame;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidType(dm,1);
  PetscValidPointer(M,3);
  ierr = PetscObjectTypeCompare((PetscObject) dm, newtype, &sametype);CHKERRQ(ierr);
  ierr = PetscStrcmp(newtype, "same", &issame);CHKERRQ(ierr);
  {
    PetscErrorCode (*conv)(DM, DMType, DM*) = NULL;

    /*
       Order of precedence:
       1) See if a specialized converter is known to the current DM.
       2) See if a specialized converter is known to the desired DM class.
       3) See if a good general converter is registered for the desired class
       4) See if a good general converter is known for the current matrix.
       5) Use a really basic converter.
    */

    /* 1) See if a specialized converter is known to the current DM and the desired class */
    ierr = PetscStrcpy(convname,"DMConvert_");CHKERRQ(ierr);
    ierr = PetscStrcat(convname,((PetscObject) dm)->type_name);CHKERRQ(ierr);
    ierr = PetscStrcat(convname,"_");CHKERRQ(ierr);
    ierr = PetscStrcat(convname,newtype);CHKERRQ(ierr);
    ierr = PetscStrcat(convname,"_C");CHKERRQ(ierr);
    ierr = PetscObjectQueryFunction((PetscObject)dm,convname,&conv);CHKERRQ(ierr);
    if (conv) goto foundconv;

    /* 2)  See if a specialized converter is known to the desired DM class. */
    ierr = DMCreate(PetscObjectComm((PetscObject)dm), &B);CHKERRQ(ierr);
    ierr = DMSetType(B, newtype);CHKERRQ(ierr);
    ierr = PetscStrcpy(convname,"DMConvert_");CHKERRQ(ierr);
    ierr = PetscStrcat(convname,((PetscObject) dm)->type_name);CHKERRQ(ierr);
    ierr = PetscStrcat(convname,"_");CHKERRQ(ierr);
    ierr = PetscStrcat(convname,newtype);CHKERRQ(ierr);
    ierr = PetscStrcat(convname,"_C");CHKERRQ(ierr);
    ierr = PetscObjectQueryFunction((PetscObject)B,convname,&conv);CHKERRQ(ierr);
    if (conv) {
      ierr = DMDestroy(&B);CHKERRQ(ierr);
      goto foundconv;
    }

#if 0
    /* 3) See if a good general converter is registered for the desired class */
    conv = B->ops->convertfrom;
    ierr = DMDestroy(&B);CHKERRQ(ierr);
    if (conv) goto foundconv;

    /* 4) See if a good general converter is known for the current matrix */
    if (dm->ops->convert) {
      conv = dm->ops->convert;
    }
    if (conv) goto foundconv;
#endif

    /* 5) Use a really basic converter. */
    SETERRQ2(PetscObjectComm((PetscObject)dm), PETSC_ERR_SUP, "No conversion possible between DM types %s and %s", ((PetscObject) dm)->type_name, newtype);

foundconv:
    ierr = PetscLogEventBegin(DM_Convert,dm,0,0,0);CHKERRQ(ierr);
    ierr = (*conv)(dm,newtype,M);CHKERRQ(ierr);
    ierr = PetscLogEventEnd(DM_Convert,dm,0,0,0);CHKERRQ(ierr);
  }
  ierr = PetscObjectStateIncrease((PetscObject) *M);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*--------------------------------------------------------------------------------------------------------------------*/

#undef __FUNCT__
#define __FUNCT__ "DMRegister"
/*@C
  DMRegister -  Adds a new DM component implementation

  Not Collective

  Input Parameters:
+ name        - The name of a new user-defined creation routine
- create_func - The creation routine itself

  Notes:
  DMRegister() may be called multiple times to add several user-defined DMs


  Sample usage:
.vb
    DMRegister("my_da", MyDMCreate);
.ve

  Then, your DM type can be chosen with the procedural interface via
.vb
    DMCreate(MPI_Comm, DM *);
    DMSetType(DM,"my_da");
.ve
   or at runtime via the option
.vb
    -da_type my_da
.ve

  Level: advanced

.keywords: DM, register
.seealso: DMRegisterAll(), DMRegisterDestroy()

@*/
PetscErrorCode  DMRegister(const char sname[],PetscErrorCode (*function)(DM))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFunctionListAdd(&DMList,sname,function);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMLoad"
/*@C
  DMLoad - Loads a DM that has been stored in binary  with DMView().

  Collective on PetscViewer

  Input Parameters:
+ newdm - the newly loaded DM, this needs to have been created with DMCreate() or
           some related function before a call to DMLoad().
- viewer - binary file viewer, obtained from PetscViewerBinaryOpen() or
           HDF5 file viewer, obtained from PetscViewerHDF5Open()

   Level: intermediate

  Notes:
   The type is determined by the data in the file, any type set into the DM before this call is ignored.

  Notes for advanced users:
  Most users should not need to know the details of the binary storage
  format, since DMLoad() and DMView() completely hide these details.
  But for anyone who's interested, the standard binary matrix storage
  format is
.vb
     has not yet been determined
.ve

.seealso: PetscViewerBinaryOpen(), DMView(), MatLoad(), VecLoad()
@*/
PetscErrorCode  DMLoad(DM newdm, PetscViewer viewer)
{
  PetscBool      isbinary, ishdf5;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(newdm,DM_CLASSID,1);
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,2);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERBINARY,&isbinary);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERHDF5,&ishdf5);CHKERRQ(ierr);
  if (isbinary) {
    PetscInt classid;
    char     type[256];

    ierr = PetscViewerBinaryRead(viewer,&classid,1,PETSC_INT);CHKERRQ(ierr);
    if (classid != DM_FILE_CLASSID) SETERRQ1(PetscObjectComm((PetscObject)newdm),PETSC_ERR_ARG_WRONG,"Not DM next in file, classid found %d",(int)classid);
    ierr = PetscViewerBinaryRead(viewer,type,256,PETSC_CHAR);CHKERRQ(ierr);
    ierr = DMSetType(newdm, type);CHKERRQ(ierr);
    if (newdm->ops->load) {ierr = (*newdm->ops->load)(newdm,viewer);CHKERRQ(ierr);}
  } else if (ishdf5) {
    if (newdm->ops->load) {ierr = (*newdm->ops->load)(newdm,viewer);CHKERRQ(ierr);}
  } else SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Invalid viewer; open viewer with PetscViewerBinaryOpen() or PetscViewerHDF5Open()");
  PetscFunctionReturn(0);
}

/******************************** FEM Support **********************************/

#undef __FUNCT__
#define __FUNCT__ "DMPrintCellVector"
PetscErrorCode DMPrintCellVector(PetscInt c, const char name[], PetscInt len, const PetscScalar x[])
{
  PetscInt       f;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscPrintf(PETSC_COMM_SELF, "Cell %D Element %s\n", c, name);CHKERRQ(ierr);
  for (f = 0; f < len; ++f) {
    ierr = PetscPrintf(PETSC_COMM_SELF, "  | %g |\n", (double)PetscRealPart(x[f]));CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPrintCellMatrix"
PetscErrorCode DMPrintCellMatrix(PetscInt c, const char name[], PetscInt rows, PetscInt cols, const PetscScalar A[])
{
  PetscInt       f, g;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscPrintf(PETSC_COMM_SELF, "Cell %D Element %s\n", c, name);CHKERRQ(ierr);
  for (f = 0; f < rows; ++f) {
    ierr = PetscPrintf(PETSC_COMM_SELF, "  |");CHKERRQ(ierr);
    for (g = 0; g < cols; ++g) {
      ierr = PetscPrintf(PETSC_COMM_SELF, " % 9.5g", PetscRealPart(A[f*cols+g]));CHKERRQ(ierr);
    }
    ierr = PetscPrintf(PETSC_COMM_SELF, " |\n");CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPrintLocalVec"
PetscErrorCode DMPrintLocalVec(DM dm, const char name[], PetscReal tol, Vec X)
{
  PetscMPIInt    rank, numProcs;
  PetscInt       p;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject) dm), &rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(PetscObjectComm((PetscObject) dm), &numProcs);CHKERRQ(ierr);
  ierr = PetscPrintf(PetscObjectComm((PetscObject) dm), "%s:\n", name);CHKERRQ(ierr);
  for (p = 0; p < numProcs; ++p) {
    if (p == rank) {
      Vec x;

      ierr = VecDuplicate(X, &x);CHKERRQ(ierr);
      ierr = VecCopy(X, x);CHKERRQ(ierr);
      ierr = VecChop(x, tol);CHKERRQ(ierr);
      ierr = VecView(x, PETSC_VIEWER_STDOUT_SELF);CHKERRQ(ierr);
      ierr = VecDestroy(&x);CHKERRQ(ierr);
      ierr = PetscViewerFlush(PETSC_VIEWER_STDOUT_SELF);CHKERRQ(ierr);
    }
    ierr = PetscBarrier((PetscObject) dm);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetDefaultSection"
/*@
  DMGetDefaultSection - Get the PetscSection encoding the local data layout for the DM.

  Input Parameter:
. dm - The DM

  Output Parameter:
. section - The PetscSection

  Level: intermediate

  Note: This gets a borrowed reference, so the user should not destroy this PetscSection.

.seealso: DMSetDefaultSection(), DMGetDefaultGlobalSection()
@*/
PetscErrorCode DMGetDefaultSection(DM dm, PetscSection *section)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscValidPointer(section, 2);
  if (!dm->defaultSection && dm->ops->createdefaultsection) {ierr = (*dm->ops->createdefaultsection)(dm);CHKERRQ(ierr);}
  *section = dm->defaultSection;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetDefaultSection"
/*@
  DMSetDefaultSection - Set the PetscSection encoding the local data layout for the DM.

  Input Parameters:
+ dm - The DM
- section - The PetscSection

  Level: intermediate

  Note: Any existing Section will be destroyed

.seealso: DMSetDefaultSection(), DMGetDefaultGlobalSection()
@*/
PetscErrorCode DMSetDefaultSection(DM dm, PetscSection section)
{
  PetscInt       numFields;
  PetscInt       f;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscValidHeaderSpecific(section,PETSC_SECTION_CLASSID,2);
  ierr = PetscObjectReference((PetscObject)section);CHKERRQ(ierr);
  ierr = PetscSectionDestroy(&dm->defaultSection);CHKERRQ(ierr);
  dm->defaultSection = section;
  ierr = PetscSectionGetNumFields(dm->defaultSection, &numFields);CHKERRQ(ierr);
  if (numFields) {
    ierr = DMSetNumFields(dm, numFields);CHKERRQ(ierr);
    for (f = 0; f < numFields; ++f) {
      PetscObject disc;
      const char *name;

      ierr = PetscSectionGetFieldName(dm->defaultSection, f, &name);CHKERRQ(ierr);
      ierr = DMGetField(dm, f, &disc);CHKERRQ(ierr);
      ierr = PetscObjectSetName(disc, name);CHKERRQ(ierr);
    }
  }
  /* The global section will be rebuilt in the next call to DMGetDefaultGlobalSection(). */
  ierr = PetscSectionDestroy(&dm->defaultGlobalSection);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetDefaultGlobalSection"
/*@
  DMGetDefaultGlobalSection - Get the PetscSection encoding the global data layout for the DM.

  Collective on DM

  Input Parameter:
. dm - The DM

  Output Parameter:
. section - The PetscSection

  Level: intermediate

  Note: This gets a borrowed reference, so the user should not destroy this PetscSection.

.seealso: DMSetDefaultSection(), DMGetDefaultSection()
@*/
PetscErrorCode DMGetDefaultGlobalSection(DM dm, PetscSection *section)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscValidPointer(section, 2);
  if (!dm->defaultGlobalSection) {
    PetscSection s;

    ierr = DMGetDefaultSection(dm, &s);CHKERRQ(ierr);
    if (!s)  SETERRQ(PetscObjectComm((PetscObject) dm), PETSC_ERR_ARG_WRONGSTATE, "DM must have a default PetscSection in order to create a global PetscSection");
    if (!dm->sf) SETERRQ(PetscObjectComm((PetscObject)dm), PETSC_ERR_ARG_WRONGSTATE, "DM must have a default PetscSF in order to create a global PetscSection");
    ierr = PetscSectionCreateGlobalSection(s, dm->sf, PETSC_FALSE, &dm->defaultGlobalSection);CHKERRQ(ierr);
    ierr = PetscLayoutDestroy(&dm->map);CHKERRQ(ierr);
    ierr = PetscSectionGetValueLayout(PetscObjectComm((PetscObject)dm), dm->defaultGlobalSection, &dm->map);CHKERRQ(ierr);
  }
  *section = dm->defaultGlobalSection;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetDefaultGlobalSection"
/*@
  DMSetDefaultGlobalSection - Set the PetscSection encoding the global data layout for the DM.

  Input Parameters:
+ dm - The DM
- section - The PetscSection, or NULL

  Level: intermediate

  Note: Any existing Section will be destroyed

.seealso: DMGetDefaultGlobalSection(), DMSetDefaultSection()
@*/
PetscErrorCode DMSetDefaultGlobalSection(DM dm, PetscSection section)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  if (section) PetscValidHeaderSpecific(section,PETSC_SECTION_CLASSID,2);
  ierr = PetscObjectReference((PetscObject)section);CHKERRQ(ierr);
  ierr = PetscSectionDestroy(&dm->defaultGlobalSection);CHKERRQ(ierr);
  dm->defaultGlobalSection = section;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetDefaultSF"
/*@
  DMGetDefaultSF - Get the PetscSF encoding the parallel dof overlap for the DM. If it has not been set,
  it is created from the default PetscSection layouts in the DM.

  Input Parameter:
. dm - The DM

  Output Parameter:
. sf - The PetscSF

  Level: intermediate

  Note: This gets a borrowed reference, so the user should not destroy this PetscSF.

.seealso: DMSetDefaultSF(), DMCreateDefaultSF()
@*/
PetscErrorCode DMGetDefaultSF(DM dm, PetscSF *sf)
{
  PetscInt       nroots;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscValidPointer(sf, 2);
  ierr = PetscSFGetGraph(dm->defaultSF, &nroots, NULL, NULL, NULL);CHKERRQ(ierr);
  if (nroots < 0) {
    PetscSection section, gSection;

    ierr = DMGetDefaultSection(dm, &section);CHKERRQ(ierr);
    if (section) {
      ierr = DMGetDefaultGlobalSection(dm, &gSection);CHKERRQ(ierr);
      ierr = DMCreateDefaultSF(dm, section, gSection);CHKERRQ(ierr);
    } else {
      *sf = NULL;
      PetscFunctionReturn(0);
    }
  }
  *sf = dm->defaultSF;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetDefaultSF"
/*@
  DMSetDefaultSF - Set the PetscSF encoding the parallel dof overlap for the DM

  Input Parameters:
+ dm - The DM
- sf - The PetscSF

  Level: intermediate

  Note: Any previous SF is destroyed

.seealso: DMGetDefaultSF(), DMCreateDefaultSF()
@*/
PetscErrorCode DMSetDefaultSF(DM dm, PetscSF sf)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscValidHeaderSpecific(sf, PETSCSF_CLASSID, 2);
  ierr          = PetscSFDestroy(&dm->defaultSF);CHKERRQ(ierr);
  dm->defaultSF = sf;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCreateDefaultSF"
/*@C
  DMCreateDefaultSF - Create the PetscSF encoding the parallel dof overlap for the DM based upon the PetscSections
  describing the data layout.

  Input Parameters:
+ dm - The DM
. localSection - PetscSection describing the local data layout
- globalSection - PetscSection describing the global data layout

  Level: intermediate

.seealso: DMGetDefaultSF(), DMSetDefaultSF()
@*/
PetscErrorCode DMCreateDefaultSF(DM dm, PetscSection localSection, PetscSection globalSection)
{
  MPI_Comm       comm;
  PetscLayout    layout;
  const PetscInt *ranges;
  PetscInt       *local;
  PetscSFNode    *remote;
  PetscInt       pStart, pEnd, p, nroots, nleaves = 0, l;
  PetscMPIInt    size, rank;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)dm,&comm);CHKERRQ(ierr);
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  ierr = MPI_Comm_size(comm, &size);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm, &rank);CHKERRQ(ierr);
  ierr = PetscSectionGetChart(globalSection, &pStart, &pEnd);CHKERRQ(ierr);
  ierr = PetscSectionGetConstrainedStorageSize(globalSection, &nroots);CHKERRQ(ierr);
  ierr = PetscLayoutCreate(comm, &layout);CHKERRQ(ierr);
  ierr = PetscLayoutSetBlockSize(layout, 1);CHKERRQ(ierr);
  ierr = PetscLayoutSetLocalSize(layout, nroots);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp(layout);CHKERRQ(ierr);
  ierr = PetscLayoutGetRanges(layout, &ranges);CHKERRQ(ierr);
  for (p = pStart; p < pEnd; ++p) {
    PetscInt gdof, gcdof;

    ierr     = PetscSectionGetDof(globalSection, p, &gdof);CHKERRQ(ierr);
    ierr     = PetscSectionGetConstraintDof(globalSection, p, &gcdof);CHKERRQ(ierr);
    if (gcdof > (gdof < 0 ? -(gdof+1) : gdof)) SETERRQ3(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Point %d has %d constraints > %d dof", p, gcdof, (gdof < 0 ? -(gdof+1) : gdof));
    nleaves += gdof < 0 ? -(gdof+1)-gcdof : gdof-gcdof;
  }
  ierr = PetscMalloc1(nleaves, &local);CHKERRQ(ierr);
  ierr = PetscMalloc1(nleaves, &remote);CHKERRQ(ierr);
  for (p = pStart, l = 0; p < pEnd; ++p) {
    const PetscInt *cind;
    PetscInt       dof, cdof, off, gdof, gcdof, goff, gsize, d, c;

    ierr = PetscSectionGetDof(localSection, p, &dof);CHKERRQ(ierr);
    ierr = PetscSectionGetOffset(localSection, p, &off);CHKERRQ(ierr);
    ierr = PetscSectionGetConstraintDof(localSection, p, &cdof);CHKERRQ(ierr);
    ierr = PetscSectionGetConstraintIndices(localSection, p, &cind);CHKERRQ(ierr);
    ierr = PetscSectionGetDof(globalSection, p, &gdof);CHKERRQ(ierr);
    ierr = PetscSectionGetConstraintDof(globalSection, p, &gcdof);CHKERRQ(ierr);
    ierr = PetscSectionGetOffset(globalSection, p, &goff);CHKERRQ(ierr);
    if (!gdof) continue; /* Censored point */
    gsize = gdof < 0 ? -(gdof+1)-gcdof : gdof-gcdof;
    if (gsize != dof-cdof) {
      if (gsize != dof) SETERRQ4(comm, PETSC_ERR_ARG_WRONG, "Global dof %d for point %d is neither the constrained size %d, nor the unconstrained %d", gsize, p, dof-cdof, dof);
      cdof = 0; /* Ignore constraints */
    }
    for (d = 0, c = 0; d < dof; ++d) {
      if ((c < cdof) && (cind[c] == d)) {++c; continue;}
      local[l+d-c] = off+d;
    }
    if (gdof < 0) {
      for (d = 0; d < gsize; ++d, ++l) {
        PetscInt offset = -(goff+1) + d, r;

        ierr = PetscFindInt(offset,size+1,ranges,&r);CHKERRQ(ierr);
        if (r < 0) r = -(r+2);
        if ((r < 0) || (r >= size)) SETERRQ4(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Point %d mapped to invalid process %d (%d, %d)", p, r, gdof, goff);
        remote[l].rank  = r;
        remote[l].index = offset - ranges[r];
      }
    } else {
      for (d = 0; d < gsize; ++d, ++l) {
        remote[l].rank  = rank;
        remote[l].index = goff+d - ranges[rank];
      }
    }
  }
  if (l != nleaves) SETERRQ2(comm, PETSC_ERR_PLIB, "Iteration error, l %d != nleaves %d", l, nleaves);
  ierr = PetscLayoutDestroy(&layout);CHKERRQ(ierr);
  ierr = PetscSFSetGraph(dm->defaultSF, nroots, nleaves, local, PETSC_OWN_POINTER, remote, PETSC_OWN_POINTER);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetPointSF"
/*@
  DMGetPointSF - Get the PetscSF encoding the parallel section point overlap for the DM.

  Input Parameter:
. dm - The DM

  Output Parameter:
. sf - The PetscSF

  Level: intermediate

  Note: This gets a borrowed reference, so the user should not destroy this PetscSF.

.seealso: DMSetPointSF(), DMGetDefaultSF(), DMSetDefaultSF(), DMCreateDefaultSF()
@*/
PetscErrorCode DMGetPointSF(DM dm, PetscSF *sf)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscValidPointer(sf, 2);
  *sf = dm->sf;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetPointSF"
/*@
  DMSetPointSF - Set the PetscSF encoding the parallel section point overlap for the DM.

  Input Parameters:
+ dm - The DM
- sf - The PetscSF

  Level: intermediate

.seealso: DMGetPointSF(), DMGetDefaultSF(), DMSetDefaultSF(), DMCreateDefaultSF()
@*/
PetscErrorCode DMSetPointSF(DM dm, PetscSF sf)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscValidHeaderSpecific(sf, PETSCSF_CLASSID, 1);
  ierr   = PetscSFDestroy(&dm->sf);CHKERRQ(ierr);
  ierr   = PetscObjectReference((PetscObject) sf);CHKERRQ(ierr);
  dm->sf = sf;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetDS"
/*@
  DMGetDS - Get the PetscDS

  Input Parameter:
. dm - The DM

  Output Parameter:
. prob - The PetscDS

  Level: developer

.seealso: DMSetDS()
@*/
PetscErrorCode DMGetDS(DM dm, PetscDS *prob)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscValidPointer(prob, 2);
  *prob = dm->prob;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetDS"
/*@
  DMSetDS - Set the PetscDS

  Input Parameters:
+ dm - The DM
- prob - The PetscDS

  Level: developer

.seealso: DMGetDS()
@*/
PetscErrorCode DMSetDS(DM dm, PetscDS prob)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 2);
  ierr = PetscDSDestroy(&dm->prob);CHKERRQ(ierr);
  dm->prob = prob;
  ierr = PetscObjectReference((PetscObject) dm->prob);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetNumFields"
PetscErrorCode DMGetNumFields(DM dm, PetscInt *numFields)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  ierr = PetscDSGetNumFields(dm->prob, numFields);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetNumFields"
PetscErrorCode DMSetNumFields(DM dm, PetscInt numFields)
{
  PetscInt       Nf, f;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  ierr = PetscDSGetNumFields(dm->prob, &Nf);CHKERRQ(ierr);
  for (f = Nf; f < numFields; ++f) {
    PetscContainer obj;

    ierr = PetscContainerCreate(PetscObjectComm((PetscObject) dm), &obj);CHKERRQ(ierr);
    ierr = PetscDSSetDiscretization(dm->prob, f, (PetscObject) obj);CHKERRQ(ierr);
    ierr = PetscContainerDestroy(&obj);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetField"
/*@
  DMGetField - Return the discretization object for a given DM field

  Not collective

  Input Parameters:
+ dm - The DM
- f  - The field number

  Output Parameter:
. field - The discretization object

  Level: developer

.seealso: DMSetField()
@*/
PetscErrorCode DMGetField(DM dm, PetscInt f, PetscObject *field)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  ierr = PetscDSGetDiscretization(dm->prob, f, field);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetField"
/*@
  DMSetField - Set the discretization object for a given DM field

  Logically collective on DM

  Input Parameters:
+ dm - The DM
. f  - The field number
- field - The discretization object

  Level: developer

.seealso: DMGetField()
@*/
PetscErrorCode DMSetField(DM dm, PetscInt f, PetscObject field)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  ierr = PetscDSSetDiscretization(dm->prob, f, field);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMRestrictHook_Coordinates"
PetscErrorCode DMRestrictHook_Coordinates(DM dm,DM dmc,void *ctx)
{
  DM dm_coord,dmc_coord;
  PetscErrorCode ierr;
  Vec coords,ccoords;
  VecScatter scat;
  PetscFunctionBegin;
  ierr = DMGetCoordinateDM(dm,&dm_coord);CHKERRQ(ierr);
  ierr = DMGetCoordinateDM(dmc,&dmc_coord);CHKERRQ(ierr);
  ierr = DMGetCoordinates(dm,&coords);CHKERRQ(ierr);
  ierr = DMGetCoordinates(dmc,&ccoords);CHKERRQ(ierr);
  if (coords && !ccoords) {
    ierr = DMCreateGlobalVector(dmc_coord,&ccoords);CHKERRQ(ierr);
    ierr = DMCreateInjection(dmc_coord,dm_coord,&scat);CHKERRQ(ierr);
    ierr = VecScatterBegin(scat,coords,ccoords,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(scat,coords,ccoords,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = DMSetCoordinates(dmc,ccoords);CHKERRQ(ierr);
    ierr = VecScatterDestroy(&scat);CHKERRQ(ierr);
    ierr = VecDestroy(&ccoords);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSubDomainHook_Coordinates"
static PetscErrorCode DMSubDomainHook_Coordinates(DM dm,DM subdm,void *ctx)
{
  DM dm_coord,subdm_coord;
  PetscErrorCode ierr;
  Vec coords,ccoords,clcoords;
  VecScatter *scat_i,*scat_g;
  PetscFunctionBegin;
  ierr = DMGetCoordinateDM(dm,&dm_coord);CHKERRQ(ierr);
  ierr = DMGetCoordinateDM(subdm,&subdm_coord);CHKERRQ(ierr);
  ierr = DMGetCoordinates(dm,&coords);CHKERRQ(ierr);
  ierr = DMGetCoordinates(subdm,&ccoords);CHKERRQ(ierr);
  if (coords && !ccoords) {
    ierr = DMCreateGlobalVector(subdm_coord,&ccoords);CHKERRQ(ierr);
    ierr = DMCreateLocalVector(subdm_coord,&clcoords);CHKERRQ(ierr);
    ierr = DMCreateDomainDecompositionScatters(dm_coord,1,&subdm_coord,NULL,&scat_i,&scat_g);CHKERRQ(ierr);
    ierr = VecScatterBegin(scat_i[0],coords,ccoords,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterBegin(scat_g[0],coords,clcoords,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(scat_i[0],coords,ccoords,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(scat_g[0],coords,clcoords,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = DMSetCoordinates(subdm,ccoords);CHKERRQ(ierr);
    ierr = DMSetCoordinatesLocal(subdm,clcoords);CHKERRQ(ierr);
    ierr = VecScatterDestroy(&scat_i[0]);CHKERRQ(ierr);
    ierr = VecScatterDestroy(&scat_g[0]);CHKERRQ(ierr);
    ierr = VecDestroy(&ccoords);CHKERRQ(ierr);
    ierr = VecDestroy(&clcoords);CHKERRQ(ierr);
    ierr = PetscFree(scat_i);CHKERRQ(ierr);
    ierr = PetscFree(scat_g);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetCoordinates"
/*@
  DMSetCoordinates - Sets into the DM a global vector that holds the coordinates

  Collective on DM

  Input Parameters:
+ dm - the DM
- c - coordinate vector

  Note:
  The coordinates do include those for ghost points, which are in the local vector

  Level: intermediate

.keywords: distributed array, get, corners, nodes, local indices, coordinates
.seealso: DMSetCoordinatesLocal(), DMGetCoordinates(), DMGetCoordinatesLoca(), DMGetCoordinateDM()
@*/
PetscErrorCode DMSetCoordinates(DM dm, Vec c)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidHeaderSpecific(c,VEC_CLASSID,2);
  ierr            = PetscObjectReference((PetscObject) c);CHKERRQ(ierr);
  ierr            = VecDestroy(&dm->coordinates);CHKERRQ(ierr);
  dm->coordinates = c;
  ierr            = VecDestroy(&dm->coordinatesLocal);CHKERRQ(ierr);
  ierr            = DMCoarsenHookAdd(dm,DMRestrictHook_Coordinates,NULL,NULL);CHKERRQ(ierr);
  ierr            = DMSubDomainHookAdd(dm,DMSubDomainHook_Coordinates,NULL,NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetCoordinatesLocal"
/*@
  DMSetCoordinatesLocal - Sets into the DM a local vector that holds the coordinates

  Collective on DM

   Input Parameters:
+  dm - the DM
-  c - coordinate vector

  Note:
  The coordinates of ghost points can be set using DMSetCoordinates()
  followed by DMGetCoordinatesLocal(). This is intended to enable the
  setting of ghost coordinates outside of the domain.

  Level: intermediate

.keywords: distributed array, get, corners, nodes, local indices, coordinates
.seealso: DMGetCoordinatesLocal(), DMSetCoordinates(), DMGetCoordinates(), DMGetCoordinateDM()
@*/
PetscErrorCode DMSetCoordinatesLocal(DM dm, Vec c)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidHeaderSpecific(c,VEC_CLASSID,2);
  ierr = PetscObjectReference((PetscObject) c);CHKERRQ(ierr);
  ierr = VecDestroy(&dm->coordinatesLocal);CHKERRQ(ierr);

  dm->coordinatesLocal = c;

  ierr = VecDestroy(&dm->coordinates);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetCoordinates"
/*@
  DMGetCoordinates - Gets a global vector with the coordinates associated with the DM.

  Not Collective

  Input Parameter:
. dm - the DM

  Output Parameter:
. c - global coordinate vector

  Note:
  This is a borrowed reference, so the user should NOT destroy this vector

  Each process has only the local coordinates (does NOT have the ghost coordinates).

  For DMDA, in two and three dimensions coordinates are interlaced (x_0,y_0,x_1,y_1,...)
  and (x_0,y_0,z_0,x_1,y_1,z_1...)

  Level: intermediate

.keywords: distributed array, get, corners, nodes, local indices, coordinates
.seealso: DMSetCoordinates(), DMGetCoordinatesLocal(), DMGetCoordinateDM()
@*/
PetscErrorCode DMGetCoordinates(DM dm, Vec *c)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(c,2);
  if (!dm->coordinates && dm->coordinatesLocal) {
    DM cdm = NULL;

    ierr = DMGetCoordinateDM(dm, &cdm);CHKERRQ(ierr);
    ierr = DMCreateGlobalVector(cdm, &dm->coordinates);CHKERRQ(ierr);
    ierr = PetscObjectSetName((PetscObject) dm->coordinates, "coordinates");CHKERRQ(ierr);
    ierr = DMLocalToGlobalBegin(cdm, dm->coordinatesLocal, INSERT_VALUES, dm->coordinates);CHKERRQ(ierr);
    ierr = DMLocalToGlobalEnd(cdm, dm->coordinatesLocal, INSERT_VALUES, dm->coordinates);CHKERRQ(ierr);
  }
  *c = dm->coordinates;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetCoordinatesLocal"
/*@
  DMGetCoordinatesLocal - Gets a local vector with the coordinates associated with the DM.

  Collective on DM

  Input Parameter:
. dm - the DM

  Output Parameter:
. c - coordinate vector

  Note:
  This is a borrowed reference, so the user should NOT destroy this vector

  Each process has the local and ghost coordinates

  For DMDA, in two and three dimensions coordinates are interlaced (x_0,y_0,x_1,y_1,...)
  and (x_0,y_0,z_0,x_1,y_1,z_1...)

  Level: intermediate

.keywords: distributed array, get, corners, nodes, local indices, coordinates
.seealso: DMSetCoordinatesLocal(), DMGetCoordinates(), DMSetCoordinates(), DMGetCoordinateDM()
@*/
PetscErrorCode DMGetCoordinatesLocal(DM dm, Vec *c)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(c,2);
  if (!dm->coordinatesLocal && dm->coordinates) {
    DM cdm = NULL;

    ierr = DMGetCoordinateDM(dm, &cdm);CHKERRQ(ierr);
    ierr = DMCreateLocalVector(cdm, &dm->coordinatesLocal);CHKERRQ(ierr);
    ierr = PetscObjectSetName((PetscObject) dm->coordinatesLocal, "coordinates");CHKERRQ(ierr);
    ierr = DMGlobalToLocalBegin(cdm, dm->coordinates, INSERT_VALUES, dm->coordinatesLocal);CHKERRQ(ierr);
    ierr = DMGlobalToLocalEnd(cdm, dm->coordinates, INSERT_VALUES, dm->coordinatesLocal);CHKERRQ(ierr);
  }
  *c = dm->coordinatesLocal;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetCoordinateDM"
/*@
  DMGetCoordinateDM - Gets the DM that prescribes coordinate layout and scatters between global and local coordinates

  Collective on DM

  Input Parameter:
. dm - the DM

  Output Parameter:
. cdm - coordinate DM

  Level: intermediate

.keywords: distributed array, get, corners, nodes, local indices, coordinates
.seealso: DMSetCoordinateDM(), DMSetCoordinates(), DMSetCoordinatesLocal(), DMGetCoordinates(), DMGetCoordinatesLocal()
@*/
PetscErrorCode DMGetCoordinateDM(DM dm, DM *cdm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(cdm,2);
  if (!dm->coordinateDM) {
    if (!dm->ops->createcoordinatedm) SETERRQ(PetscObjectComm((PetscObject)dm), PETSC_ERR_SUP, "Unable to create coordinates for this DM");
    ierr = (*dm->ops->createcoordinatedm)(dm, &dm->coordinateDM);CHKERRQ(ierr);
  }
  *cdm = dm->coordinateDM;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetCoordinateDM"
/*@
  DMSetCoordinateDM - Sets the DM that prescribes coordinate layout and scatters between global and local coordinates

  Logically Collective on DM

  Input Parameters:
+ dm - the DM
- cdm - coordinate DM

  Level: intermediate

.keywords: distributed array, get, corners, nodes, local indices, coordinates
.seealso: DMGetCoordinateDM(), DMSetCoordinates(), DMSetCoordinatesLocal(), DMGetCoordinates(), DMGetCoordinatesLocal()
@*/
PetscErrorCode DMSetCoordinateDM(DM dm, DM cdm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidHeaderSpecific(cdm,DM_CLASSID,2);
  ierr = DMDestroy(&dm->coordinateDM);CHKERRQ(ierr);
  dm->coordinateDM = cdm;
  ierr = PetscObjectReference((PetscObject) dm->coordinateDM);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetCoordinateSection"
/*@
  DMGetCoordinateSection - Retrieve the layout of coordinate values over the mesh.

  Not Collective

  Input Parameter:
. dm - The DM object

  Output Parameter:
. section - The PetscSection object

  Level: intermediate

.keywords: mesh, coordinates
.seealso: DMGetCoordinateDM(), DMGetDefaultSection(), DMSetDefaultSection()
@*/
PetscErrorCode DMGetCoordinateSection(DM dm, PetscSection *section)
{
  DM             cdm;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscValidPointer(section, 2);
  ierr = DMGetCoordinateDM(dm, &cdm);CHKERRQ(ierr);
  ierr = DMGetDefaultSection(cdm, section);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetCoordinateSection"
/*@
  DMSetCoordinateSection - Set the layout of coordinate values over the mesh.

  Not Collective

  Input Parameters:
+ dm      - The DM object
- section - The PetscSection object

  Level: intermediate

.keywords: mesh, coordinates
.seealso: DMGetCoordinateSection(), DMGetDefaultSection(), DMSetDefaultSection()
@*/
PetscErrorCode DMSetCoordinateSection(DM dm, PetscSection section)
{
  DM             cdm;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidHeaderSpecific(section,PETSC_SECTION_CLASSID,2);
  ierr = DMGetCoordinateDM(dm, &cdm);CHKERRQ(ierr);
  ierr = DMSetDefaultSection(cdm, section);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetPeriodicity"
PetscErrorCode DMGetPeriodicity(DM dm, const PetscReal **maxCell, const PetscReal **L)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  if (L)       *L       = dm->L;
  if (maxCell) *maxCell = dm->maxCell;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetPeriodicity"
PetscErrorCode DMSetPeriodicity(DM dm, const PetscReal maxCell[], const PetscReal L[])
{
  Vec            coordinates;
  PetscInt       dim, d;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetCoordinatesLocal(dm, &coordinates);CHKERRQ(ierr);
  ierr = VecGetBlockSize(coordinates, &dim);CHKERRQ(ierr);
  ierr = PetscMalloc1(dim,&dm->L);CHKERRQ(ierr);
  ierr = PetscMalloc1(dim,&dm->maxCell);CHKERRQ(ierr);
  for (d = 0; d < dim; ++d) {dm->L[d] = L[d]; dm->maxCell[d] = maxCell[d];}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMLocatePoints"
/*@
  DMLocatePoints - Locate the points in v in the mesh and return an IS of the containing cells

  Not collective

  Input Parameters:
+ dm - The DM
- v - The Vec of points

  Output Parameter:
. cells - The local cell numbers for cells which contain the points

  Level: developer

.keywords: point location, mesh
.seealso: DMSetCoordinates(), DMSetCoordinatesLocal(), DMGetCoordinates(), DMGetCoordinatesLocal()
@*/
PetscErrorCode DMLocatePoints(DM dm, Vec v, IS *cells)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidHeaderSpecific(v,VEC_CLASSID,2);
  PetscValidPointer(cells,3);
  if (dm->ops->locatepoints) {
    ierr = (*dm->ops->locatepoints)(dm,v,cells);CHKERRQ(ierr);
  } else SETERRQ(PetscObjectComm((PetscObject)dm), PETSC_ERR_SUP, "Point location not available for this DM");
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetOutputDM"
/*@
  DMGetOutputDM - Retrieve the DM associated with the layout for output

  Input Parameter:
. dm - The original DM

  Output Parameter:
. odm - The DM which provides the layout for output

  Level: intermediate

.seealso: VecView(), DMGetDefaultGlobalSection()
@*/
PetscErrorCode DMGetOutputDM(DM dm, DM *odm)
{
  PetscSection   section;
  PetscBool      hasConstraints;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(odm,2);
  ierr = DMGetDefaultSection(dm, &section);CHKERRQ(ierr);
  ierr = PetscSectionHasConstraints(section, &hasConstraints);CHKERRQ(ierr);
  if (!hasConstraints) {
    *odm = dm;
    PetscFunctionReturn(0);
  }
  if (!dm->dmBC) {
    PetscSection newSection, gsection;
    PetscSF      sf;

    ierr = DMClone(dm, &dm->dmBC);CHKERRQ(ierr);
    ierr = PetscSectionClone(section, &newSection);CHKERRQ(ierr);
    ierr = DMSetDefaultSection(dm->dmBC, newSection);CHKERRQ(ierr);
    ierr = PetscSectionDestroy(&newSection);CHKERRQ(ierr);
    ierr = DMGetPointSF(dm->dmBC, &sf);CHKERRQ(ierr);
    ierr = PetscSectionCreateGlobalSection(section, sf, PETSC_TRUE, &gsection);CHKERRQ(ierr);
    ierr = DMSetDefaultGlobalSection(dm->dmBC, gsection);CHKERRQ(ierr);
    ierr = PetscSectionDestroy(&gsection);CHKERRQ(ierr);
  }
  *odm = dm->dmBC;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetOutputSequenceNumber"
/*@
  DMGetOutputSequenceNumber - Retrieve the sequence number/value for output

  Input Parameter:
. dm - The original DM

  Output Parameters:
+ num - The output sequence number
- val - The output sequence value

  Level: intermediate

  Note: This is intended for output that should appear in sequence, for instance
  a set of timesteps in an HDF5 file, or a set of realizations of a stochastic system.

.seealso: VecView()
@*/
PetscErrorCode DMGetOutputSequenceNumber(DM dm, PetscInt *num, PetscReal *val)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  if (num) {PetscValidPointer(num,2); *num = dm->outputSequenceNum;}
  if (val) {PetscValidPointer(val,3);*val = dm->outputSequenceVal;}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSetOutputSequenceNumber"
/*@
  DMSetOutputSequenceNumber - Set the sequence number/value for output

  Input Parameters:
+ dm - The original DM
. num - The output sequence number
- val - The output sequence value

  Level: intermediate

  Note: This is intended for output that should appear in sequence, for instance
  a set of timesteps in an HDF5 file, or a set of realizations of a stochastic system.

.seealso: VecView()
@*/
PetscErrorCode DMSetOutputSequenceNumber(DM dm, PetscInt num, PetscReal val)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  dm->outputSequenceNum = num;
  dm->outputSequenceVal = val;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMOutputSequenceLoad"
/*@C
  DMOutputSequenceLoad - Retrieve the sequence value from a Viewer

  Input Parameters:
+ dm   - The original DM
. name - The sequence name
- num  - The output sequence number

  Output Parameter:
. val  - The output sequence value

  Level: intermediate

  Note: This is intended for output that should appear in sequence, for instance
  a set of timesteps in an HDF5 file, or a set of realizations of a stochastic system.

.seealso: DMGetOutputSequenceNumber(), DMSetOutputSequenceNumber(), VecView()
@*/
PetscErrorCode DMOutputSequenceLoad(DM dm, PetscViewer viewer, const char *name, PetscInt num, PetscReal *val)
{
  PetscBool      ishdf5;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,2);
  PetscValidPointer(val,4);
  ierr = PetscObjectTypeCompare((PetscObject) viewer, PETSCVIEWERHDF5, &ishdf5);CHKERRQ(ierr);
  if (ishdf5) {
#if defined(PETSC_HAVE_HDF5)
    PetscScalar value;

    ierr = DMSequenceLoad_HDF5(dm, name, num, &value, viewer);CHKERRQ(ierr);
    *val = PetscRealPart(value);
#endif
  } else SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "Invalid viewer; open viewer with PetscViewerHDF5Open()");
  PetscFunctionReturn(0);
}
