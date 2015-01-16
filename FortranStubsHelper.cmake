file(READ ${FILE} contents)

# Stupid semicolon - list thing in cmake
string(REPLACE ";" "@@@" contents "${contents}")

# Replace stuff - see generatefortranstubs.py:FixFile
string(REPLACE "\nvoid " "\nPETSC_EXTERN void PETSC_STDCALL " contents ${contents})
string(REPLACE "\nPetscErrorCode " "\nPETSC_EXTERN void PETSC_STDCALL " contents ${contents})
string(REPLACE "PetscToPointer(int)" "PetscToPointer(void*)" contents ${contents})
string(REPLACE "PetscRmPointer(int)" "PetscRmPointer(void*)" contents ${contents})
string(REPLACE "PetscToPointer(a) (a)" "PetscToPointer(a) (*(PetscFortranAddr *)(a))" contents ${contents})
string(REPLACE "PetscFromPointer(a) (int)(a)" "PetscFromPointer(a) (PetscFortranAddr)(a)" contents ${contents})
string(REPLACE "PetscToPointer( *(int*)" "PetscToPointer(" contents ${contents})
string(REPLACE "MPI_Comm comm" "MPI_Comm *comm" contents ${contents})
string(REPLACE "(MPI_Comm)PetscToPointer( (comm) )" "MPI_Comm_f2c(*(MPI_Fint*)(comm))" contents ${contents})
string(REPLACE "(PetscInt* )PetscToPointer" "" contents ${contents})
string(REPLACE "(Tao* )PetscToPointer" "" contents ${contents})
string(REPLACE "(TaoConvergedReason* )PetscToPointer" "" contents ${contents})
string(REPLACE "(TaoLineSearch* )PetscToPointer" "" contents ${contents})
string(REPLACE "(TaoLineSearchConvergedReason* )PetscToPointer" "" contents ${contents})
string(REGEX REPLACE "^(PETSC|TAO)(_DLL|VEC_DLL|MAT_DLL|DM_DLL|KSP_DLL|SNES_DLL|TS_DLL|FORTRAN_DLL)(EXPORT)" "" contents ${contents})

# Stupid semicolon - list thing in cmake - revert
string(REPLACE "@@@" ";" contents "${contents}")

file(WRITE ${FILE} "#include \"petscsys.h\"\n#include \"petscfix.h\"\n#include \"petsc-private/fortranimpl.h\"\n${contents}")