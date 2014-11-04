#include <petsc-private/viewerimpl.h>    /*I   "petscsys.h"   I*/
#include <petscviewerhdf5.h>    /*I   "petscviewerhdf5.h"   I*/

typedef struct GroupList {
  const char       *name;
  struct GroupList *next;
} GroupList;

typedef struct {
  char          *filename;
  PetscFileMode btype;
  hid_t         file_id;
  PetscInt      timestep;
  GroupList     *groups;
} PetscViewer_HDF5;

#undef __FUNCT__
#define __FUNCT__ "PetscViewerFileClose_HDF5"
static PetscErrorCode PetscViewerFileClose_HDF5(PetscViewer viewer)
{
  PetscViewer_HDF5 *hdf5 = (PetscViewer_HDF5*)viewer->data;
  PetscErrorCode   ierr;

  PetscFunctionBegin;
  ierr = PetscFree(hdf5->filename);CHKERRQ(ierr);
  if (hdf5->file_id) H5Fclose(hdf5->file_id);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerDestroy_HDF5"
PetscErrorCode PetscViewerDestroy_HDF5(PetscViewer viewer)
{
  PetscViewer_HDF5 *hdf5 = (PetscViewer_HDF5*) viewer->data;
  PetscErrorCode   ierr;

  PetscFunctionBegin;
  ierr = PetscViewerFileClose_HDF5(viewer);CHKERRQ(ierr);
  while (hdf5->groups) {
    GroupList *tmp = hdf5->groups->next;

    ierr         = PetscFree(hdf5->groups->name);CHKERRQ(ierr);
    ierr         = PetscFree(hdf5->groups);CHKERRQ(ierr);
    hdf5->groups = tmp;
  }
  ierr = PetscFree(hdf5);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)viewer,"PetscViewerFileSetName_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)viewer,"PetscViewerFileSetMode_C",NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerFileSetMode_HDF5"
PetscErrorCode  PetscViewerFileSetMode_HDF5(PetscViewer viewer, PetscFileMode type)
{
  PetscViewer_HDF5 *hdf5 = (PetscViewer_HDF5*) viewer->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer, PETSC_VIEWER_CLASSID, 1);
  hdf5->btype = type;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerFileSetName_HDF5"
PetscErrorCode  PetscViewerFileSetName_HDF5(PetscViewer viewer, const char name[])
{
  PetscViewer_HDF5 *hdf5 = (PetscViewer_HDF5*) viewer->data;
#if defined(PETSC_HAVE_H5PSET_FAPL_MPIO)
  MPI_Info          info = MPI_INFO_NULL;
#endif
  hid_t             plist_id;
  herr_t            herr;
  PetscErrorCode    ierr;

  PetscFunctionBegin;
  ierr = PetscStrallocpy(name, &hdf5->filename);CHKERRQ(ierr);
  /* Set up file access property list with parallel I/O access */
  plist_id = H5Pcreate(H5P_FILE_ACCESS);
#if defined(PETSC_HAVE_H5PSET_FAPL_MPIO)
  herr = H5Pset_fapl_mpio(plist_id, PetscObjectComm((PetscObject)viewer), info);CHKERRQ(herr);
#endif
  /* Create or open the file collectively */
  switch (hdf5->btype) {
  case FILE_MODE_READ:
    hdf5->file_id = H5Fopen(name, H5F_ACC_RDONLY, plist_id);
    break;
  case FILE_MODE_APPEND:
    hdf5->file_id = H5Fopen(name, H5F_ACC_RDWR, plist_id);
    break;
  case FILE_MODE_WRITE:
    hdf5->file_id = H5Fcreate(name, H5F_ACC_TRUNC, H5P_DEFAULT, plist_id);
    break;
  default:
    SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ORDER, "Must call PetscViewerFileSetMode() before PetscViewerFileSetName()");
  }
  if (hdf5->file_id < 0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB, "H5Fcreate failed for %s", name);
  H5Pclose(plist_id);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerCreate_HDF5"
PETSC_EXTERN PetscErrorCode PetscViewerCreate_HDF5(PetscViewer v)
{
  PetscViewer_HDF5 *hdf5;
  PetscErrorCode   ierr;

  PetscFunctionBegin;
  ierr = PetscNewLog(v,&hdf5);CHKERRQ(ierr);

  v->data         = (void*) hdf5;
  v->ops->destroy = PetscViewerDestroy_HDF5;
  v->ops->flush   = 0;
  hdf5->btype     = (PetscFileMode) -1;
  hdf5->filename  = 0;
  hdf5->timestep  = -1;
  hdf5->groups    = NULL;

  ierr = PetscObjectComposeFunction((PetscObject)v,"PetscViewerFileSetName_C",PetscViewerFileSetName_HDF5);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)v,"PetscViewerFileSetMode_C",PetscViewerFileSetMode_HDF5);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerHDF5Open"
/*@C
   PetscViewerHDF5Open - Opens a file for HDF5 input/output.

   Collective on MPI_Comm

   Input Parameters:
+  comm - MPI communicator
.  name - name of file
-  type - type of file
$    FILE_MODE_WRITE - create new file for binary output
$    FILE_MODE_READ - open existing file for binary input
$    FILE_MODE_APPEND - open existing file for binary output

   Output Parameter:
.  hdf5v - PetscViewer for HDF5 input/output to use with the specified file

   Level: beginner

   Note:
   This PetscViewer should be destroyed with PetscViewerDestroy().

   Concepts: HDF5 files
   Concepts: PetscViewerHDF5^creating

.seealso: PetscViewerASCIIOpen(), PetscViewerSetFormat(), PetscViewerDestroy(),
          VecView(), MatView(), VecLoad(), MatLoad(),
          PetscFileMode, PetscViewer
@*/
PetscErrorCode  PetscViewerHDF5Open(MPI_Comm comm, const char name[], PetscFileMode type, PetscViewer *hdf5v)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscViewerCreate(comm, hdf5v);CHKERRQ(ierr);
  ierr = PetscViewerSetType(*hdf5v, PETSCVIEWERHDF5);CHKERRQ(ierr);
  ierr = PetscViewerFileSetMode(*hdf5v, type);CHKERRQ(ierr);
  ierr = PetscViewerFileSetName(*hdf5v, name);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerHDF5GetFileId"
/*@C
  PetscViewerHDF5GetFileId - Retrieve the file id, this file ID then can be used in direct HDF5 calls

  Not collective

  Input Parameter:
. viewer - the PetscViewer

  Output Parameter:
. file_id - The file id

  Level: intermediate

.seealso: PetscViewerHDF5Open()
@*/
PetscErrorCode  PetscViewerHDF5GetFileId(PetscViewer viewer, hid_t *file_id)
{
  PetscViewer_HDF5 *hdf5 = (PetscViewer_HDF5*) viewer->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,1);
  if (file_id) *file_id = hdf5->file_id;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerHDF5PushGroup"
/*@C
  PetscViewerHDF5PushGroup - Set the current HDF5 group for output

  Not collective

  Input Parameters:
+ viewer - the PetscViewer
- name - The group name

  Level: intermediate

.seealso: PetscViewerHDF5Open(),PetscViewerHDF5PopGroup(),PetscViewerHDF5GetGroup()
@*/
PetscErrorCode  PetscViewerHDF5PushGroup(PetscViewer viewer, const char *name)
{
  PetscViewer_HDF5 *hdf5 = (PetscViewer_HDF5*) viewer->data;
  GroupList        *groupNode;
  PetscErrorCode   ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,1);
  PetscValidCharPointer(name,2);
  ierr = PetscMalloc(sizeof(GroupList), &groupNode);CHKERRQ(ierr);
  ierr = PetscStrallocpy(name, (char**) &groupNode->name);CHKERRQ(ierr);

  groupNode->next = hdf5->groups;
  hdf5->groups    = groupNode;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerHDF5PopGroup"
/*@
  PetscViewerHDF5PopGroup - Return the current HDF5 group for output to the previous value

  Not collective

  Input Parameter:
. viewer - the PetscViewer

  Level: intermediate

.seealso: PetscViewerHDF5Open(),PetscViewerHDF5PushGroup(),PetscViewerHDF5GetGroup()
@*/
PetscErrorCode  PetscViewerHDF5PopGroup(PetscViewer viewer)
{
  PetscViewer_HDF5 *hdf5 = (PetscViewer_HDF5*) viewer->data;
  GroupList        *groupNode;
  PetscErrorCode   ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,1);
  if (!hdf5->groups) SETERRQ(PetscObjectComm((PetscObject)viewer), PETSC_ERR_ARG_WRONGSTATE, "HDF5 group stack is empty, cannot pop");
  groupNode    = hdf5->groups;
  hdf5->groups = hdf5->groups->next;
  ierr         = PetscFree(groupNode->name);CHKERRQ(ierr);
  ierr         = PetscFree(groupNode);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerHDF5GetGroup"
/*@C
  PetscViewerHDF5GetGroup - Get the current HDF5 group for output. If none has been assigned, returns NULL.

  Not collective

  Input Parameter:
. viewer - the PetscViewer

  Output Parameter:
. name - The group name

  Level: intermediate

.seealso: PetscViewerHDF5Open(),PetscViewerHDF5PushGroup(),PetscViewerHDF5PopGroup()
@*/
PetscErrorCode  PetscViewerHDF5GetGroup(PetscViewer viewer, const char **name)
{
  PetscViewer_HDF5 *hdf5 = (PetscViewer_HDF5 *) viewer->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,1);
  PetscValidPointer(name,2);
  if (hdf5->groups) *name = hdf5->groups->name;
  else *name = NULL;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerHDF5IncrementTimestep"
/*@
  PetscViewerHDF5IncrementTimestep - Increments the current timestep for the HDF5 output. Fields are stacked in time.

  Not collective

  Input Parameter:
. viewer - the PetscViewer

  Level: intermediate

.seealso: PetscViewerHDF5Open(), PetscViewerHDF5SetTimestep(), PetscViewerHDF5GetTimestep()
@*/
PetscErrorCode PetscViewerHDF5IncrementTimestep(PetscViewer viewer)
{
  PetscViewer_HDF5 *hdf5 = (PetscViewer_HDF5*) viewer->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,1);
  ++hdf5->timestep;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerHDF5SetTimestep"
/*@
  PetscViewerHDF5SetTimestep - Set the current timestep for the HDF5 output. Fields are stacked in time. A timestep
  of -1 disables blocking with timesteps.

  Not collective

  Input Parameters:
+ viewer - the PetscViewer
- timestep - The timestep number

  Level: intermediate

.seealso: PetscViewerHDF5Open(), PetscViewerHDF5IncrementTimestep(), PetscViewerHDF5GetTimestep()
@*/
PetscErrorCode  PetscViewerHDF5SetTimestep(PetscViewer viewer, PetscInt timestep)
{
  PetscViewer_HDF5 *hdf5 = (PetscViewer_HDF5*) viewer->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,1);
  hdf5->timestep = timestep;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerHDF5GetTimestep"
/*@
  PetscViewerHDF5GetTimestep - Get the current timestep for the HDF5 output. Fields are stacked in time.

  Not collective

  Input Parameter:
. viewer - the PetscViewer

  Output Parameter:
. timestep - The timestep number

  Level: intermediate

.seealso: PetscViewerHDF5Open(), PetscViewerHDF5IncrementTimestep(), PetscViewerHDF5SetTimestep()
@*/
PetscErrorCode  PetscViewerHDF5GetTimestep(PetscViewer viewer, PetscInt *timestep)
{
  PetscViewer_HDF5 *hdf5 = (PetscViewer_HDF5*) viewer->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,1);
  PetscValidPointer(timestep,2);
  *timestep = hdf5->timestep;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "PetscDataTypeToHDF5DataType"
/*@C
  PetscDataTypeToHDF5DataType - Converts the PETSc name of a datatype to its HDF5 name.

  Not collective

  Input Parameter:
. ptype - the PETSc datatype name (for example PETSC_DOUBLE)

  Output Parameter:
. mtype - the MPI datatype (for example MPI_DOUBLE, ...)

  Level: advanced

.seealso: PetscDataType, PetscHDF5DataTypeToPetscDataType()
@*/
PetscErrorCode PetscDataTypeToHDF5DataType(PetscDataType ptype, hid_t *htype)
{
  PetscFunctionBegin;
  if (ptype == PETSC_INT)
#if defined(PETSC_USE_64BIT_INDICES)
                                       *htype = H5T_NATIVE_LLONG;
#else
                                       *htype = H5T_NATIVE_INT;
#endif
  else if (ptype == PETSC_DOUBLE)      *htype = H5T_NATIVE_DOUBLE;
  else if (ptype == PETSC_LONG)        *htype = H5T_NATIVE_LONG;
  else if (ptype == PETSC_SHORT)       *htype = H5T_NATIVE_SHORT;
  else if (ptype == PETSC_ENUM)        *htype = H5T_NATIVE_DOUBLE;
  else if (ptype == PETSC_BOOL)        *htype = H5T_NATIVE_DOUBLE;
  else if (ptype == PETSC_FLOAT)       *htype = H5T_NATIVE_FLOAT;
  else if (ptype == PETSC_CHAR)        *htype = H5T_NATIVE_CHAR;
  else if (ptype == PETSC_BIT_LOGICAL) *htype = H5T_NATIVE_UCHAR;
  else if (ptype == PETSC_STRING)      *htype = H5Tcopy(H5T_C_S1);
  else SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Unsupported PETSc datatype");
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscHDF5DataTypeToPetscDataType"
/*@C
  PetscHDF5DataTypeToPetscDataType - Finds the PETSc name of a datatype from its HDF5 name

  Not collective

  Input Parameter:
. htype - the HDF5 datatype (for example H5T_NATIVE_DOUBLE, ...)

  Output Parameter:
. ptype - the PETSc datatype name (for example PETSC_DOUBLE)

  Level: advanced

.seealso: PetscDataType, PetscHDF5DataTypeToPetscDataType()
@*/
PetscErrorCode PetscHDF5DataTypeToPetscDataType(hid_t htype, PetscDataType *ptype)
{
  PetscFunctionBegin;
#if defined(PETSC_USE_64BIT_INDICES)
  if      (htype == H5T_NATIVE_INT)    *ptype = PETSC_LONG;
  else if (htype == H5T_NATIVE_LLONG)  *ptype = PETSC_INT;
#else
  if      (htype == H5T_NATIVE_INT)    *ptype = PETSC_INT;
#endif
  else if (htype == H5T_NATIVE_DOUBLE) *ptype = PETSC_DOUBLE;
  else if (htype == H5T_NATIVE_LONG)   *ptype = PETSC_LONG;
  else if (htype == H5T_NATIVE_SHORT)  *ptype = PETSC_SHORT;
  else if (htype == H5T_NATIVE_FLOAT)  *ptype = PETSC_FLOAT;
  else if (htype == H5T_NATIVE_CHAR)   *ptype = PETSC_CHAR;
  else if (htype == H5T_NATIVE_UCHAR)  *ptype = PETSC_CHAR;
  else if (htype == H5T_C_S1)          *ptype = PETSC_STRING;
  else SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Unsupported HDF5 datatype");
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerHDF5WriteAttribute"
/*@
 PetscViewerHDF5WriteAttribute - Write a scalar attribute

  Input Parameters:
+ viewer - The HDF5 viewer
. parent - The parent name
. name   - The attribute name
. datatype - The attribute type
- value    - The attribute value

  Level: advanced

.seealso: PetscViewerHDF5Open(), PetscViewerHDF5ReadAttribute(), PetscViewerHDF5HasAttribute()
@*/
PetscErrorCode PetscViewerHDF5WriteAttribute(PetscViewer viewer, const char parent[], const char name[], PetscDataType datatype, const void *value)
{
  hid_t          h5, dataspace, dataset, attribute, dtype, status;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,1);
  PetscValidPointer(parent, 2);
  PetscValidPointer(name, 3);
  PetscValidPointer(value, 4);
  ierr = PetscDataTypeToHDF5DataType(datatype, &dtype);CHKERRQ(ierr);
  if (datatype == PETSC_STRING) {
    size_t len;
    ierr = PetscStrlen((const char *) value, &len);CHKERRQ(ierr);
    status = H5Tset_size(dtype, len+1);CHKERRQ(status);
  }
  ierr = PetscViewerHDF5GetFileId(viewer, &h5);CHKERRQ(ierr);
  dataspace = H5Screate(H5S_SCALAR);
  if (dataspace < 0) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_LIB, "Could not create dataspace for attribute %s of %s", name, parent);
#if (H5_VERS_MAJOR * 10000 + H5_VERS_MINOR * 100 + H5_VERS_RELEASE >= 10800)
  dataset = H5Dopen2(h5, parent, H5P_DEFAULT);
  if (dataset < 0) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_LIB, "Could not open parent dataset for attribute %s of %s", name, parent);
  attribute = H5Acreate2(dataset, name, dtype, dataspace, H5P_DEFAULT, H5P_DEFAULT);
#else
  dataset = H5Dopen(h5, parent);
  if (dataset < 0) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_LIB, "Could not open parent dataset for attribute %s of %s", name, parent);
  attribute = H5Acreate(dataset, name, dtype, dataspace, H5P_DEFAULT);
#endif
  if (attribute < 0) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_LIB, "Could not create attribute %s of %s", name, parent);
  status = H5Awrite(attribute, dtype, value);CHKERRQ(status);
  if (datatype == PETSC_STRING) {status = H5Tclose(dtype);CHKERRQ(status);}
  status = H5Aclose(attribute);CHKERRQ(status);
  status = H5Dclose(dataset);CHKERRQ(status);
  status = H5Sclose(dataspace);CHKERRQ(status);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerHDF5ReadAttribute"
/*@
 PetscViewerHDF5ReadAttribute - Read a scalar attribute

  Input Parameters:
+ viewer - The HDF5 viewer
. parent - The parent name
. name   - The attribute name
- datatype - The attribute type

  Output Parameter:
. value    - The attribute value

  Level: advanced

.seealso: PetscViewerHDF5Open(), PetscViewerHDF5WriteAttribute(), PetscViewerHDF5HasAttribute()
@*/
PetscErrorCode PetscViewerHDF5ReadAttribute(PetscViewer viewer, const char parent[], const char name[], PetscDataType datatype, void *value)
{
  hid_t          h5, dataspace, dataset, attribute, dtype, status;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,1);
  PetscValidPointer(parent, 2);
  PetscValidPointer(name, 3);
  PetscValidPointer(value, 4);
  ierr = PetscDataTypeToHDF5DataType(datatype, &dtype);CHKERRQ(ierr);
  ierr = PetscViewerHDF5GetFileId(viewer, &h5);CHKERRQ(ierr);
  dataspace = H5Screate(H5S_SCALAR);
  if (dataspace < 0) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_LIB, "Could not create dataspace for attribute %s of %s", name, parent);
#if (H5_VERS_MAJOR * 10000 + H5_VERS_MINOR * 100 + H5_VERS_RELEASE >= 10800)
  dataset = H5Dopen2(h5, parent, H5P_DEFAULT);
  if (dataset < 0) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_LIB, "Could not open parent dataset for attribute %s of %s", name, parent);
#else
  dataset = H5Dopen(h5, parent);
  if (dataset < 0) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_LIB, "Could not open parent dataset for attribute %s of %s", name, parent);
#endif
  attribute = H5Aopen_name(dataset, name);
  if (attribute < 0) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_LIB, "Could not open attribute %s of %s", name, parent);
  status = H5Aread(attribute, dtype, value);CHKERRQ(status);
  status = H5Aclose(attribute);CHKERRQ(status);
  status = H5Dclose(dataset);CHKERRQ(status);
  status = H5Sclose(dataspace);CHKERRQ(status);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerHDF5HasObject"
static PetscErrorCode PetscViewerHDF5HasObject(PetscViewer viewer, const char name[], H5O_type_t otype, PetscBool *has)
{
  hid_t          h5;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,1);
  PetscValidPointer(name, 2);
  PetscValidPointer(has, 3);
  *has = PETSC_FALSE;
  ierr = PetscViewerHDF5GetFileId(viewer, &h5);CHKERRQ(ierr);
  if (H5Lexists(h5, name, H5P_DEFAULT)) {
    H5O_info_t info;
    hid_t      obj;
    herr_t     err;

    obj = H5Oopen(h5, name, H5P_DEFAULT);if (obj < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_LIB, "Could not open object %s", name);
    err = H5Oget_info(obj, &info);CHKERRQ(err);
    if (otype == info.type) *has = PETSC_TRUE;
    err = H5Oclose(obj);CHKERRQ(err);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscViewerHDF5HasAttribute"
/*@
 PetscViewerHDF5HasAttribute - Check whether a scalar attribute exists

  Input Parameters:
+ viewer - The HDF5 viewer
. parent - The parent name
- name   - The attribute name

  Output Parameter:
. has    - Flag for attribute existence

  Level: advanced

.seealso: PetscViewerHDF5Open(), PetscViewerHDF5WriteAttribute(), PetscViewerHDF5ReadAttribute()
@*/
PetscErrorCode PetscViewerHDF5HasAttribute(PetscViewer viewer, const char parent[], const char name[], PetscBool *has)
{
  hid_t          h5, dataset, status;
  htri_t         hhas;
  PetscBool      exists;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,1);
  PetscValidPointer(parent, 2);
  PetscValidPointer(name, 3);
  PetscValidPointer(has, 4);
  *has = PETSC_FALSE;
  ierr = PetscViewerHDF5GetFileId(viewer, &h5);CHKERRQ(ierr);
  ierr = PetscViewerHDF5HasObject(viewer, parent, H5O_TYPE_DATASET, &exists);CHKERRQ(ierr);
  if (exists) {
#if (H5_VERS_MAJOR * 10000 + H5_VERS_MINOR * 100 + H5_VERS_RELEASE >= 10800)
    dataset = H5Dopen2(h5, parent, H5P_DEFAULT);
    if (dataset < 0) PetscFunctionReturn(0);
#else
    dataset = H5Dopen(h5, parent);
    if (dataset < 0) PetscFunctionReturn(0);
#endif
    hhas = H5Aexists(dataset, name);
    if (hhas < 0) {status = H5Dclose(dataset);CHKERRQ(status); PetscFunctionReturn(0);}
    status = H5Dclose(dataset);CHKERRQ(status);
    *has = hhas ? PETSC_TRUE : PETSC_FALSE;
  }
  PetscFunctionReturn(0);
}

/*
  The variable Petsc_Viewer_HDF5_keyval is used to indicate an MPI attribute that
  is attached to a communicator, in this case the attribute is a PetscViewer.
*/
static int Petsc_Viewer_HDF5_keyval = MPI_KEYVAL_INVALID;

#undef __FUNCT__
#define __FUNCT__ "PETSC_VIEWER_HDF5_"
/*@C
  PETSC_VIEWER_HDF5_ - Creates an HDF5 PetscViewer shared by all processors in a communicator.

  Collective on MPI_Comm

  Input Parameter:
. comm - the MPI communicator to share the HDF5 PetscViewer

  Level: intermediate

  Options Database Keys:
. -viewer_hdf5_filename <name>

  Environmental variables:
. PETSC_VIEWER_HDF5_FILENAME

  Notes:
  Unlike almost all other PETSc routines, PETSC_VIEWER_HDF5_ does not return
  an error code.  The HDF5 PetscViewer is usually used in the form
$       XXXView(XXX object, PETSC_VIEWER_HDF5_(comm));

.seealso: PetscViewerHDF5Open(), PetscViewerCreate(), PetscViewerDestroy()
@*/
PetscViewer PETSC_VIEWER_HDF5_(MPI_Comm comm)
{
  PetscErrorCode ierr;
  PetscBool      flg;
  PetscViewer    viewer;
  char           fname[PETSC_MAX_PATH_LEN];
  MPI_Comm       ncomm;

  PetscFunctionBegin;
  ierr = PetscCommDuplicate(comm,&ncomm,NULL);if (ierr) {PetscError(PETSC_COMM_SELF,__LINE__,"PETSC_VIEWER_HDF5_",__FILE__,PETSC_ERR_PLIB,PETSC_ERROR_INITIAL," ");PetscFunctionReturn(0);}
  if (Petsc_Viewer_HDF5_keyval == MPI_KEYVAL_INVALID) {
    ierr = MPI_Keyval_create(MPI_NULL_COPY_FN,MPI_NULL_DELETE_FN,&Petsc_Viewer_HDF5_keyval,0);
    if (ierr) {PetscError(PETSC_COMM_SELF,__LINE__,"PETSC_VIEWER_HDF5_",__FILE__,PETSC_ERR_PLIB,PETSC_ERROR_INITIAL," ");PetscFunctionReturn(0);}
  }
  ierr = MPI_Attr_get(ncomm,Petsc_Viewer_HDF5_keyval,(void**)&viewer,(int*)&flg);
  if (ierr) {PetscError(PETSC_COMM_SELF,__LINE__,"PETSC_VIEWER_HDF5_",__FILE__,PETSC_ERR_PLIB,PETSC_ERROR_INITIAL," ");PetscFunctionReturn(0);}
  if (!flg) { /* PetscViewer not yet created */
    ierr = PetscOptionsGetenv(ncomm,"PETSC_VIEWER_HDF5_FILENAME",fname,PETSC_MAX_PATH_LEN,&flg);
    if (ierr) {PetscError(PETSC_COMM_SELF,__LINE__,"PETSC_VIEWER_HDF5_",__FILE__,PETSC_ERR_PLIB,PETSC_ERROR_INITIAL," ");PetscFunctionReturn(0);}
    if (!flg) {
      ierr = PetscStrcpy(fname,"output.h5");
      if (ierr) {PetscError(PETSC_COMM_SELF,__LINE__,"PETSC_VIEWER_HDF5_",__FILE__,PETSC_ERR_PLIB,PETSC_ERROR_INITIAL," ");PetscFunctionReturn(0);}
    }
    ierr = PetscViewerHDF5Open(ncomm,fname,FILE_MODE_WRITE,&viewer);
    if (ierr) {PetscError(PETSC_COMM_SELF,__LINE__,"PETSC_VIEWER_HDF5_",__FILE__,PETSC_ERR_PLIB,PETSC_ERROR_INITIAL," ");PetscFunctionReturn(0);}
    ierr = PetscObjectRegisterDestroy((PetscObject)viewer);
    if (ierr) {PetscError(PETSC_COMM_SELF,__LINE__,"PETSC_VIEWER_HDF5_",__FILE__,PETSC_ERR_PLIB,PETSC_ERROR_INITIAL," ");PetscFunctionReturn(0);}
    ierr = MPI_Attr_put(ncomm,Petsc_Viewer_HDF5_keyval,(void*)viewer);
    if (ierr) {PetscError(PETSC_COMM_SELF,__LINE__,"PETSC_VIEWER_HDF5_",__FILE__,PETSC_ERR_PLIB,PETSC_ERROR_INITIAL," ");PetscFunctionReturn(0);}
  }
  ierr = PetscCommDestroy(&ncomm);
  if (ierr) {PetscError(PETSC_COMM_SELF,__LINE__,"PETSC_VIEWER_HDF5_",__FILE__,PETSC_ERR_PLIB,PETSC_ERROR_INITIAL," ");PetscFunctionReturn(0);}
  PetscFunctionReturn(viewer);
}

#if defined(oldhdf4stuff)
#undef __FUNCT__
#define __FUNCT__ "PetscViewerHDF5WriteSDS"
PetscErrorCode  PetscViewerHDF5WriteSDS(PetscViewer viewer, float *xf, int d, int *dims,int bs)
{
  int              i;
  PetscViewer_HDF5 *vhdf5 = (PetscViewer_HDF5*)viewer->data;
  int32            sds_id,zero32[3],dims32[3];

  PetscFunctionBegin;
  for (i = 0; i < d; i++) {
    zero32[i] = 0;
    dims32[i] = dims[i];
  }
  sds_id = SDcreate(vhdf5->sd_id, "Vec", DFNT_FLOAT32, d, dims32);
  if (sds_id < 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_LIB,"SDcreate failed");
  SDwritedata(sds_id, zero32, 0, dims32, xf);
  SDendaccess(sds_id);
  PetscFunctionReturn(0);
}

#endif
