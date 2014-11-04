#include <petsc-private/petscdsimpl.h> /*I "petscds.h" I*/

PetscClassId PETSCDS_CLASSID = 0;

PetscFunctionList PetscDSList              = NULL;
PetscBool         PetscDSRegisterAllCalled = PETSC_FALSE;

#undef __FUNCT__
#define __FUNCT__ "PetscDSRegister"
/*@C
  PetscDSRegister - Adds a new PetscDS implementation

  Not Collective

  Input Parameters:
+ name        - The name of a new user-defined creation routine
- create_func - The creation routine itself

  Notes:
  PetscDSRegister() may be called multiple times to add several user-defined PetscDSs

  Sample usage:
.vb
    PetscDSRegister("my_ds", MyPetscDSCreate);
.ve

  Then, your PetscDS type can be chosen with the procedural interface via
.vb
    PetscDSCreate(MPI_Comm, PetscDS *);
    PetscDSSetType(PetscDS, "my_ds");
.ve
   or at runtime via the option
.vb
    -petscds_type my_ds
.ve

  Level: advanced

.keywords: PetscDS, register
.seealso: PetscDSRegisterAll(), PetscDSRegisterDestroy()

@*/
PetscErrorCode PetscDSRegister(const char sname[], PetscErrorCode (*function)(PetscDS))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFunctionListAdd(&PetscDSList, sname, function);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetType"
/*@C
  PetscDSSetType - Builds a particular PetscDS

  Collective on PetscDS

  Input Parameters:
+ prob - The PetscDS object
- name - The kind of system

  Options Database Key:
. -petscds_type <type> - Sets the PetscDS type; use -help for a list of available types

  Level: intermediate

.keywords: PetscDS, set, type
.seealso: PetscDSGetType(), PetscDSCreate()
@*/
PetscErrorCode PetscDSSetType(PetscDS prob, PetscDSType name)
{
  PetscErrorCode (*r)(PetscDS);
  PetscBool      match;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  ierr = PetscObjectTypeCompare((PetscObject) prob, name, &match);CHKERRQ(ierr);
  if (match) PetscFunctionReturn(0);

  if (!PetscDSRegisterAllCalled) {ierr = PetscDSRegisterAll();CHKERRQ(ierr);}
  ierr = PetscFunctionListFind(PetscDSList, name, &r);CHKERRQ(ierr);
  if (!r) SETERRQ1(PetscObjectComm((PetscObject) prob), PETSC_ERR_ARG_UNKNOWN_TYPE, "Unknown PetscDS type: %s", name);

  if (prob->ops->destroy) {
    ierr             = (*prob->ops->destroy)(prob);CHKERRQ(ierr);
    prob->ops->destroy = NULL;
  }
  ierr = (*r)(prob);CHKERRQ(ierr);
  ierr = PetscObjectChangeTypeName((PetscObject) prob, name);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetType"
/*@C
  PetscDSGetType - Gets the PetscDS type name (as a string) from the object.

  Not Collective

  Input Parameter:
. prob  - The PetscDS

  Output Parameter:
. name - The PetscDS type name

  Level: intermediate

.keywords: PetscDS, get, type, name
.seealso: PetscDSSetType(), PetscDSCreate()
@*/
PetscErrorCode PetscDSGetType(PetscDS prob, PetscDSType *name)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(name, 2);
  if (!PetscDSRegisterAllCalled) {ierr = PetscDSRegisterAll();CHKERRQ(ierr);}
  *name = ((PetscObject) prob)->type_name;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSView"
/*@C
  PetscDSView - Views a PetscDS

  Collective on PetscDS

  Input Parameter:
+ prob - the PetscDS object to view
- v  - the viewer

  Level: developer

.seealso PetscDSDestroy()
@*/
PetscErrorCode PetscDSView(PetscDS prob, PetscViewer v)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if (!v) {ierr = PetscViewerASCIIGetStdout(PetscObjectComm((PetscObject) prob), &v);CHKERRQ(ierr);}
  if (prob->ops->view) {ierr = (*prob->ops->view)(prob, v);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSViewFromOptions"
/*
  PetscDSViewFromOptions - Processes command line options to determine if/how a PetscDS is to be viewed.

  Collective on PetscDS

  Input Parameters:
+ prob   - the PetscDS
. prefix - prefix to use for viewing, or NULL to use prefix of 'rnd'
- optionname - option to activate viewing

  Level: intermediate

.keywords: PetscDS, view, options, database
.seealso: VecViewFromOptions(), MatViewFromOptions()
*/
PetscErrorCode PetscDSViewFromOptions(PetscDS prob, const char prefix[], const char optionname[])
{
  PetscViewer       viewer;
  PetscViewerFormat format;
  PetscBool         flg;
  PetscErrorCode    ierr;

  PetscFunctionBegin;
  if (prefix) {ierr = PetscOptionsGetViewer(PetscObjectComm((PetscObject) prob), prefix, optionname, &viewer, &format, &flg);CHKERRQ(ierr);}
  else        {ierr = PetscOptionsGetViewer(PetscObjectComm((PetscObject) prob), ((PetscObject) prob)->prefix, optionname, &viewer, &format, &flg);CHKERRQ(ierr);}
  if (flg) {
    ierr = PetscViewerPushFormat(viewer, format);CHKERRQ(ierr);
    ierr = PetscDSView(prob, viewer);CHKERRQ(ierr);
    ierr = PetscViewerPopFormat(viewer);CHKERRQ(ierr);
    ierr = PetscViewerDestroy(&viewer);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetFromOptions"
/*@
  PetscDSSetFromOptions - sets parameters in a PetscDS from the options database

  Collective on PetscDS

  Input Parameter:
. prob - the PetscDS object to set options for

  Options Database:

  Level: developer

.seealso PetscDSView()
@*/
PetscErrorCode PetscDSSetFromOptions(PetscDS prob)
{
  const char    *defaultType;
  char           name[256];
  PetscBool      flg;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if (!((PetscObject) prob)->type_name) {
    defaultType = PETSCDSBASIC;
  } else {
    defaultType = ((PetscObject) prob)->type_name;
  }
  if (!PetscDSRegisterAllCalled) {ierr = PetscDSRegisterAll();CHKERRQ(ierr);}

  ierr = PetscObjectOptionsBegin((PetscObject) prob);CHKERRQ(ierr);
  ierr = PetscOptionsFList("-petscds_type", "Discrete System", "PetscDSSetType", PetscDSList, defaultType, name, 256, &flg);CHKERRQ(ierr);
  if (flg) {
    ierr = PetscDSSetType(prob, name);CHKERRQ(ierr);
  } else if (!((PetscObject) prob)->type_name) {
    ierr = PetscDSSetType(prob, defaultType);CHKERRQ(ierr);
  }
  if (prob->ops->setfromoptions) {ierr = (*prob->ops->setfromoptions)(prob);CHKERRQ(ierr);}
  /* process any options handlers added with PetscObjectAddOptionsHandler() */
  ierr = PetscObjectProcessOptionsHandlers((PetscObject) prob);CHKERRQ(ierr);
  ierr = PetscOptionsEnd();CHKERRQ(ierr);
  ierr = PetscDSViewFromOptions(prob, NULL, "-petscds_view");CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetUp"
/*@C
  PetscDSSetUp - Construct data structures for the PetscDS

  Collective on PetscDS

  Input Parameter:
. prob - the PetscDS object to setup

  Level: developer

.seealso PetscDSView(), PetscDSDestroy()
@*/
PetscErrorCode PetscDSSetUp(PetscDS prob)
{
  const PetscInt Nf = prob->Nf;
  PetscInt       dim, work, NcMax = 0, NqMax = 0, f;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if (prob->setup) PetscFunctionReturn(0);
  /* Calculate sizes */
  ierr = PetscDSGetSpatialDimension(prob, &dim);CHKERRQ(ierr);
  prob->totDim = prob->totDimBd = prob->totComp = 0;
  ierr = PetscMalloc4(Nf,&prob->basis,Nf,&prob->basisDer,Nf,&prob->basisBd,Nf,&prob->basisDerBd);CHKERRQ(ierr);
  for (f = 0; f < Nf; ++f) {
    PetscFE         fe   = (PetscFE) prob->disc[f];
    PetscFE         feBd = (PetscFE) prob->discBd[f];
    PetscQuadrature q;
    PetscInt        Nq, Nb, Nc;

    /* TODO Dispatch on discretization type*/
    ierr = PetscFEGetQuadrature(fe, &q);CHKERRQ(ierr);
    ierr = PetscQuadratureGetData(q, NULL, &Nq, NULL, NULL);CHKERRQ(ierr);
    ierr = PetscFEGetDimension(fe, &Nb);CHKERRQ(ierr);
    ierr = PetscFEGetNumComponents(fe, &Nc);CHKERRQ(ierr);
    ierr = PetscFEGetDefaultTabulation(fe, &prob->basis[f], &prob->basisDer[f], NULL);CHKERRQ(ierr);
    NqMax          = PetscMax(NqMax, Nq);
    NcMax          = PetscMax(NcMax, Nc);
    prob->totDim  += Nb*Nc;
    prob->totComp += Nc;
    if (feBd) {
      ierr = PetscFEGetDimension(feBd, &Nb);CHKERRQ(ierr);
      ierr = PetscFEGetNumComponents(feBd, &Nc);CHKERRQ(ierr);
      ierr = PetscFEGetDefaultTabulation(feBd, &prob->basisBd[f], &prob->basisDerBd[f], NULL);CHKERRQ(ierr);
      prob->totDimBd += Nb*Nc;
    }
  }
  work = PetscMax(prob->totComp*dim, PetscSqr(NcMax*dim));
  /* Allocate works space */
  ierr = PetscMalloc5(prob->totComp,&prob->u,prob->totComp,&prob->u_t,prob->totComp*dim,&prob->u_x,dim,&prob->x,work,&prob->refSpaceDer);CHKERRQ(ierr);
  ierr = PetscMalloc6(NqMax*NcMax,&prob->f0,NqMax*NcMax*dim,&prob->f1,NqMax*NcMax*NcMax,&prob->g0,NqMax*NcMax*NcMax*dim,&prob->g1,NqMax*NcMax*NcMax*dim,&prob->g2,NqMax*NcMax*NcMax*dim*dim,&prob->g3);CHKERRQ(ierr);
  if (prob->ops->setup) {ierr = (*prob->ops->setup)(prob);CHKERRQ(ierr);}
  prob->setup = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSDestroyStructs_Static"
static PetscErrorCode PetscDSDestroyStructs_Static(PetscDS prob)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFree4(prob->basis,prob->basisDer,prob->basisBd,prob->basisDerBd);CHKERRQ(ierr);
  ierr = PetscFree5(prob->u,prob->u_t,prob->u_x,prob->x,prob->refSpaceDer);CHKERRQ(ierr);
  ierr = PetscFree6(prob->f0,prob->f1,prob->g0,prob->g1,prob->g2,prob->g3);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSEnlarge_Static"
static PetscErrorCode PetscDSEnlarge_Static(PetscDS prob, PetscInt NfNew)
{
  PetscObject   *tmpd, *tmpdbd;
  PointFunc     *tmpobj, *tmpf, *tmpg;
  BdPointFunc   *tmpfbd, *tmpgbd;
  PetscInt       Nf = prob->Nf, f;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (Nf >= NfNew) PetscFunctionReturn(0);
  prob->setup = PETSC_FALSE;
  ierr = PetscDSDestroyStructs_Static(prob);CHKERRQ(ierr);
  ierr = PetscMalloc1(NfNew, &tmpd);CHKERRQ(ierr);
  for (f = 0; f < Nf; ++f) tmpd[f] = prob->disc[f];
  for (f = Nf; f < NfNew; ++f) {ierr = PetscContainerCreate(PetscObjectComm((PetscObject) prob), (PetscContainer *) &tmpd[f]);CHKERRQ(ierr);}
  ierr = PetscFree(prob->disc);CHKERRQ(ierr);
  prob->Nf   = NfNew;
  prob->disc = tmpd;
  ierr = PetscCalloc3(NfNew, &tmpobj, NfNew*2, &tmpf, NfNew*NfNew*4, &tmpg);CHKERRQ(ierr);
  for (f = 0; f < Nf; ++f) tmpobj[f] = prob->obj[f];
  for (f = 0; f < Nf*2; ++f) tmpf[f] = prob->f[f];
  for (f = 0; f < Nf*Nf*4; ++f) tmpg[f] = prob->g[f];
  for (f = Nf; f < NfNew; ++f) tmpobj[f] = NULL;
  for (f = Nf*2; f < NfNew*2; ++f) tmpf[f] = NULL;
  for (f = Nf*Nf*4; f < NfNew*NfNew*4; ++f) tmpg[f] = NULL;
  ierr = PetscFree3(prob->obj, prob->f, prob->g);CHKERRQ(ierr);
  prob->obj = tmpobj;
  prob->f   = tmpf;
  prob->g   = tmpg;
  ierr = PetscMalloc1(NfNew, &tmpdbd);CHKERRQ(ierr);
  for (f = 0; f < Nf; ++f) tmpdbd[f] = prob->discBd[f];
  for (f = Nf; f < NfNew; ++f) tmpdbd[f] = NULL;
  ierr = PetscFree(prob->discBd);CHKERRQ(ierr);
  prob->discBd = tmpdbd;
  ierr = PetscCalloc2(NfNew*2, &tmpfbd, NfNew*NfNew*4, &tmpgbd);CHKERRQ(ierr);
  for (f = 0; f < Nf*2; ++f) tmpfbd[f] = prob->fBd[f];
  for (f = 0; f < Nf*Nf*4; ++f) tmpgbd[f] = prob->gBd[f];
  for (f = Nf*2; f < NfNew*2; ++f) tmpfbd[f] = NULL;
  for (f = Nf*Nf*4; f < NfNew*NfNew*4; ++f) tmpgbd[f] = NULL;
  ierr = PetscFree2(prob->fBd, prob->gBd);CHKERRQ(ierr);
  prob->fBd = tmpfbd;
  prob->gBd = tmpgbd;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSDestroy"
/*@
  PetscDSDestroy - Destroys a PetscDS object

  Collective on PetscDS

  Input Parameter:
. prob - the PetscDS object to destroy

  Level: developer

.seealso PetscDSView()
@*/
PetscErrorCode PetscDSDestroy(PetscDS *prob)
{
  PetscInt       f;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!*prob) PetscFunctionReturn(0);
  PetscValidHeaderSpecific((*prob), PETSCDS_CLASSID, 1);

  if (--((PetscObject)(*prob))->refct > 0) {*prob = 0; PetscFunctionReturn(0);}
  ((PetscObject) (*prob))->refct = 0;
  ierr = PetscDSDestroyStructs_Static(*prob);CHKERRQ(ierr);
  for (f = 0; f < (*prob)->Nf; ++f) {
    ierr = PetscObjectDereference((*prob)->disc[f]);CHKERRQ(ierr);
    ierr = PetscObjectDereference((*prob)->discBd[f]);CHKERRQ(ierr);
  }
  ierr = PetscFree((*prob)->disc);CHKERRQ(ierr);
  ierr = PetscFree((*prob)->discBd);CHKERRQ(ierr);
  ierr = PetscFree3((*prob)->obj,(*prob)->f,(*prob)->g);CHKERRQ(ierr);
  ierr = PetscFree2((*prob)->fBd,(*prob)->gBd);CHKERRQ(ierr);
  if ((*prob)->ops->destroy) {ierr = (*(*prob)->ops->destroy)(*prob);CHKERRQ(ierr);}
  ierr = PetscHeaderDestroy(prob);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSCreate"
/*@
  PetscDSCreate - Creates an empty PetscDS object. The type can then be set with PetscDSSetType().

  Collective on MPI_Comm

  Input Parameter:
. comm - The communicator for the PetscDS object

  Output Parameter:
. prob - The PetscDS object

  Level: beginner

.seealso: PetscDSSetType(), PETSCDSBASIC
@*/
PetscErrorCode PetscDSCreate(MPI_Comm comm, PetscDS *prob)
{
  PetscDS   p;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidPointer(prob, 2);
  *prob  = NULL;
  ierr = PetscDSInitializePackage();CHKERRQ(ierr);

  ierr = PetscHeaderCreate(p, _p_PetscDS, struct _PetscDSOps, PETSCDS_CLASSID, "PetscDS", "Discrete System", "PetscDS", comm, PetscDSDestroy, PetscDSView);CHKERRQ(ierr);
  ierr = PetscMemzero(p->ops, sizeof(struct _PetscDSOps));CHKERRQ(ierr);

  p->Nf    = 0;
  p->setup = PETSC_FALSE;

  *prob = p;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetNumFields"
PetscErrorCode PetscDSGetNumFields(PetscDS prob, PetscInt *Nf)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(Nf, 2);
  *Nf = prob->Nf;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetSpatialDimension"
PetscErrorCode PetscDSGetSpatialDimension(PetscDS prob, PetscInt *dim)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(dim, 2);
  *dim = 0;
  if (prob->Nf) {ierr = PetscFEGetSpatialDimension((PetscFE) prob->disc[0], dim);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetTotalDimension"
PetscErrorCode PetscDSGetTotalDimension(PetscDS prob, PetscInt *dim)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  ierr = PetscDSSetUp(prob);CHKERRQ(ierr);
  PetscValidPointer(dim, 2);
  *dim = prob->totDim;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetTotalBdDimension"
PetscErrorCode PetscDSGetTotalBdDimension(PetscDS prob, PetscInt *dim)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  ierr = PetscDSSetUp(prob);CHKERRQ(ierr);
  PetscValidPointer(dim, 2);
  *dim = prob->totDimBd;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetTotalComponents"
PetscErrorCode PetscDSGetTotalComponents(PetscDS prob, PetscInt *Nc)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  ierr = PetscDSSetUp(prob);CHKERRQ(ierr);
  PetscValidPointer(Nc, 2);
  *Nc = prob->totComp;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetDiscretization"
PetscErrorCode PetscDSGetDiscretization(PetscDS prob, PetscInt f, PetscObject *disc)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(disc, 3);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  *disc = prob->disc[f];
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetBdDiscretization"
PetscErrorCode PetscDSGetBdDiscretization(PetscDS prob, PetscInt f, PetscObject *disc)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(disc, 3);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  *disc = prob->discBd[f];
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetDiscretization"
PetscErrorCode PetscDSSetDiscretization(PetscDS prob, PetscInt f, PetscObject disc)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(disc, 3);
  if (f < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be non-negative", f);
  ierr = PetscDSEnlarge_Static(prob, f+1);CHKERRQ(ierr);
  if (prob->disc[f]) {ierr = PetscObjectDereference(prob->disc[f]);CHKERRQ(ierr);}
  prob->disc[f] = disc;
  ierr = PetscObjectReference(disc);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetBdDiscretization"
PetscErrorCode PetscDSSetBdDiscretization(PetscDS prob, PetscInt f, PetscObject disc)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if (disc) PetscValidPointer(disc, 3);
  if (f < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be non-negative", f);
  ierr = PetscDSEnlarge_Static(prob, f+1);CHKERRQ(ierr);
  if (prob->discBd[f]) {ierr = PetscObjectDereference(prob->discBd[f]);CHKERRQ(ierr);}
  prob->discBd[f] = disc;
  ierr = PetscObjectReference(disc);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSAddDiscretization"
PetscErrorCode PetscDSAddDiscretization(PetscDS prob, PetscObject disc)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscDSSetDiscretization(prob, prob->Nf, disc);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSAddBdDiscretization"
PetscErrorCode PetscDSAddBdDiscretization(PetscDS prob, PetscObject disc)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscDSSetBdDiscretization(prob, prob->Nf, disc);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetObjective"
PetscErrorCode PetscDSGetObjective(PetscDS prob, PetscInt f,
                                        void (**obj)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], PetscScalar obj[]))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(obj, 2);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  *obj = prob->obj[f];
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetObjective"
PetscErrorCode PetscDSSetObjective(PetscDS prob, PetscInt f,
                                        void (*obj)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], PetscScalar obj[]))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidFunction(obj, 2);
  if (f < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be non-negative", f);
  ierr = PetscDSEnlarge_Static(prob, f+1);CHKERRQ(ierr);
  prob->obj[f] = obj;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetResidual"
PetscErrorCode PetscDSGetResidual(PetscDS prob, PetscInt f,
                                       void (**f0)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], PetscScalar f0[]),
                                       void (**f1)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], PetscScalar f1[]))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  if (f0) {PetscValidPointer(f0, 3); *f0 = prob->f[f*2+0];}
  if (f1) {PetscValidPointer(f1, 4); *f1 = prob->f[f*2+1];}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetResidual"
PetscErrorCode PetscDSSetResidual(PetscDS prob, PetscInt f,
                                       void (*f0)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], PetscScalar f0[]),
                                       void (*f1)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], PetscScalar f1[]))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidFunction(f0, 3);
  PetscValidFunction(f1, 4);
  if (f < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be non-negative", f);
  ierr = PetscDSEnlarge_Static(prob, f+1);CHKERRQ(ierr);
  prob->f[f*2+0] = f0;
  prob->f[f*2+1] = f1;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetJacobian"
PetscErrorCode PetscDSGetJacobian(PetscDS prob, PetscInt f, PetscInt g,
                                       void (**g0)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], PetscScalar g0[]),
                                       void (**g1)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], PetscScalar g1[]),
                                       void (**g2)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], PetscScalar g2[]),
                                       void (**g3)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], PetscScalar g3[]))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  if ((g < 0) || (g >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", g, prob->Nf);
  if (g0) {PetscValidPointer(g0, 4); *g0 = prob->g[(f*prob->Nf + g)*4+0];}
  if (g1) {PetscValidPointer(g1, 5); *g1 = prob->g[(f*prob->Nf + g)*4+1];}
  if (g2) {PetscValidPointer(g2, 6); *g2 = prob->g[(f*prob->Nf + g)*4+2];}
  if (g3) {PetscValidPointer(g3, 7); *g3 = prob->g[(f*prob->Nf + g)*4+3];}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetJacobian"
PetscErrorCode PetscDSSetJacobian(PetscDS prob, PetscInt f, PetscInt g,
                                       void (*g0)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], PetscScalar g0[]),
                                       void (*g1)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], PetscScalar g1[]),
                                       void (*g2)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], PetscScalar g2[]),
                                       void (*g3)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], PetscScalar g3[]))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if (g0) PetscValidFunction(g0, 4);
  if (g1) PetscValidFunction(g1, 5);
  if (g2) PetscValidFunction(g2, 6);
  if (g3) PetscValidFunction(g3, 7);
  if (f < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be non-negative", f);
  if (g < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be non-negative", g);
  ierr = PetscDSEnlarge_Static(prob, PetscMax(f, g)+1);CHKERRQ(ierr);
  prob->g[(f*prob->Nf + g)*4+0] = g0;
  prob->g[(f*prob->Nf + g)*4+1] = g1;
  prob->g[(f*prob->Nf + g)*4+2] = g2;
  prob->g[(f*prob->Nf + g)*4+3] = g3;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetBdResidual"
PetscErrorCode PetscDSGetBdResidual(PetscDS prob, PetscInt f,
                                         void (**f0)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], const PetscReal n[], PetscScalar f0[]),
                                         void (**f1)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], const PetscReal n[], PetscScalar f1[]))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  if (f0) {PetscValidPointer(f0, 3); *f0 = prob->fBd[f*2+0];}
  if (f1) {PetscValidPointer(f1, 4); *f1 = prob->fBd[f*2+1];}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetBdResidual"
PetscErrorCode PetscDSSetBdResidual(PetscDS prob, PetscInt f,
                                         void (*f0)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], const PetscReal n[], PetscScalar f0[]),
                                         void (*f1)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], const PetscReal n[], PetscScalar f1[]))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if (f < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be non-negative", f);
  ierr = PetscDSEnlarge_Static(prob, f+1);CHKERRQ(ierr);
  if (f0) {PetscValidFunction(f0, 3); prob->fBd[f*2+0] = f0;}
  if (f1) {PetscValidFunction(f1, 4); prob->fBd[f*2+1] = f1;}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetBdJacobian"
PetscErrorCode PetscDSGetBdJacobian(PetscDS prob, PetscInt f, PetscInt g,
                                         void (**g0)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], const PetscReal n[], PetscScalar g0[]),
                                         void (**g1)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], const PetscReal n[], PetscScalar g1[]),
                                         void (**g2)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], const PetscReal n[], PetscScalar g2[]),
                                         void (**g3)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], const PetscReal n[], PetscScalar g3[]))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  if ((g < 0) || (g >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", g, prob->Nf);
  if (g0) {PetscValidPointer(g0, 4); *g0 = prob->gBd[(f*prob->Nf + g)*4+0];}
  if (g1) {PetscValidPointer(g1, 5); *g1 = prob->gBd[(f*prob->Nf + g)*4+1];}
  if (g2) {PetscValidPointer(g2, 6); *g2 = prob->gBd[(f*prob->Nf + g)*4+2];}
  if (g3) {PetscValidPointer(g3, 7); *g3 = prob->gBd[(f*prob->Nf + g)*4+3];}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSSetBdJacobian"
PetscErrorCode PetscDSSetBdJacobian(PetscDS prob, PetscInt f, PetscInt g,
                                         void (*g0)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], const PetscReal n[], PetscScalar g0[]),
                                         void (*g1)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], const PetscReal n[], PetscScalar g1[]),
                                         void (*g2)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], const PetscReal n[], PetscScalar g2[]),
                                         void (*g3)(const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[], const PetscReal x[], const PetscReal n[], PetscScalar g3[]))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  if (g0) PetscValidFunction(g0, 4);
  if (g1) PetscValidFunction(g1, 5);
  if (g2) PetscValidFunction(g2, 6);
  if (g3) PetscValidFunction(g3, 7);
  if (f < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be non-negative", f);
  if (g < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be non-negative", g);
  ierr = PetscDSEnlarge_Static(prob, PetscMax(f, g)+1);CHKERRQ(ierr);
  prob->gBd[(f*prob->Nf + g)*4+0] = g0;
  prob->gBd[(f*prob->Nf + g)*4+1] = g1;
  prob->gBd[(f*prob->Nf + g)*4+2] = g2;
  prob->gBd[(f*prob->Nf + g)*4+3] = g3;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetFieldOffset"
PetscErrorCode PetscDSGetFieldOffset(PetscDS prob, PetscInt f, PetscInt *off)
{
  PetscInt       g;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(off, 3);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  *off = 0;
  for (g = 0; g < f; ++g) {
    PetscFE  fe = (PetscFE) prob->disc[g];
    PetscInt Nb, Nc;

    ierr = PetscFEGetDimension(fe, &Nb);CHKERRQ(ierr);
    ierr = PetscFEGetNumComponents(fe, &Nc);CHKERRQ(ierr);
    *off += Nb*Nc;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetBdFieldOffset"
PetscErrorCode PetscDSGetBdFieldOffset(PetscDS prob, PetscInt f, PetscInt *off)
{
  PetscInt       g;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  PetscValidPointer(off, 3);
  if ((f < 0) || (f >= prob->Nf)) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Field number %d must be in [0, %d)", f, prob->Nf);
  *off = 0;
  for (g = 0; g < f; ++g) {
    PetscFE  fe = (PetscFE) prob->discBd[g];
    PetscInt Nb, Nc;

    ierr = PetscFEGetDimension(fe, &Nb);CHKERRQ(ierr);
    ierr = PetscFEGetNumComponents(fe, &Nc);CHKERRQ(ierr);
    *off += Nb*Nc;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetTabulation"
PetscErrorCode PetscDSGetTabulation(PetscDS prob, PetscReal ***basis, PetscReal ***basisDer)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  ierr = PetscDSSetUp(prob);CHKERRQ(ierr);
  if (basis)    {PetscValidPointer(basis, 2);    *basis    = prob->basis;}
  if (basisDer) {PetscValidPointer(basisDer, 3); *basisDer = prob->basisDer;}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetBdTabulation"
PetscErrorCode PetscDSGetBdTabulation(PetscDS prob, PetscReal ***basis, PetscReal ***basisDer)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  ierr = PetscDSSetUp(prob);CHKERRQ(ierr);
  if (basis)    {PetscValidPointer(basis, 2);    *basis    = prob->basisBd;}
  if (basisDer) {PetscValidPointer(basisDer, 3); *basisDer = prob->basisDerBd;}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetEvaluationArrays"
PetscErrorCode PetscDSGetEvaluationArrays(PetscDS prob, PetscScalar **u, PetscScalar **u_t, PetscScalar **u_x)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  ierr = PetscDSSetUp(prob);CHKERRQ(ierr);
  if (u)   {PetscValidPointer(u, 2);   *u   = prob->u;}
  if (u_t) {PetscValidPointer(u_t, 3); *u_t = prob->u_t;}
  if (u_x) {PetscValidPointer(u_x, 4); *u_x = prob->u_x;}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetWeakFormArrays"
PetscErrorCode PetscDSGetWeakFormArrays(PetscDS prob, PetscScalar **f0, PetscScalar **f1, PetscScalar **g0, PetscScalar **g1, PetscScalar **g2, PetscScalar **g3)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  ierr = PetscDSSetUp(prob);CHKERRQ(ierr);
  if (f0) {PetscValidPointer(f0, 2); *f0 = prob->f0;}
  if (f1) {PetscValidPointer(f1, 3); *f1 = prob->f1;}
  if (g0) {PetscValidPointer(g0, 4); *g0 = prob->g0;}
  if (g1) {PetscValidPointer(g1, 5); *g1 = prob->g1;}
  if (g2) {PetscValidPointer(g2, 6); *g2 = prob->g2;}
  if (g3) {PetscValidPointer(g3, 7); *g3 = prob->g3;}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSGetRefCoordArrays"
PetscErrorCode PetscDSGetRefCoordArrays(PetscDS prob, PetscReal **x, PetscScalar **refSpaceDer)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCDS_CLASSID, 1);
  ierr = PetscDSSetUp(prob);CHKERRQ(ierr);
  if (x)           {PetscValidPointer(x, 2);           *x           = prob->x;}
  if (refSpaceDer) {PetscValidPointer(refSpaceDer, 3); *refSpaceDer = prob->refSpaceDer;}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSDestroy_Basic"
PetscErrorCode PetscDSDestroy_Basic(PetscDS prob)
{
  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSInitialize_Basic"
PetscErrorCode PetscDSInitialize_Basic(PetscDS prob)
{
  PetscFunctionBegin;
  prob->ops->setfromoptions = NULL;
  prob->ops->setup          = NULL;
  prob->ops->view           = NULL;
  prob->ops->destroy        = PetscDSDestroy_Basic;
  PetscFunctionReturn(0);
}

/*MC
  PETSCDSBASIC = "basic" - A discrete system with pointwise residual and boundary residual functions

  Level: intermediate

.seealso: PetscDSType, PetscDSCreate(), PetscDSSetType()
M*/

#undef __FUNCT__
#define __FUNCT__ "PetscDSCreate_Basic"
PETSC_EXTERN PetscErrorCode PetscDSCreate_Basic(PetscDS prob)
{
  PetscDS_Basic *b;
  PetscErrorCode      ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(prob, PETSCSPACE_CLASSID, 1);
  ierr       = PetscNewLog(prob, &b);CHKERRQ(ierr);
  prob->data = b;

  ierr = PetscDSInitialize_Basic(prob);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
