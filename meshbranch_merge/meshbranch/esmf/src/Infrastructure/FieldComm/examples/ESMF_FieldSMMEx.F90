! $Id: ESMF_FieldSMMEx.F90,v 1.13 2010/04/16 16:25:51 feiliu Exp $
!
! Earth System Modeling Framework
! Copyright 2002-2010, University Corporation for Atmospheric Research,
! Massachusetts Institute of Technology, Geophysical Fluid Dynamics
! Laboratory, University of Michigan, National Centers for Environmental
! Prediction, Los Alamos National Laboratory, Argonne National Laboratory,
! NASA Goddard Space Flight Center.
! Licensed under the University of Illinois-NCSA License.
!
!==============================================================================

     program FieldSMMEx

!-------------------------------------------------------------------------
!ESMF_MULTI_PROC_EXAMPLE        String used by test script to count examples.
!==============================================================================
!
! !PROGRAM: ESMF_FieldSMMEx - Field SMMribution
!     
! !DESCRIPTION:
!     
! This program shows examples of Field interfaces for 
! sparse matrix multiplication of data.
!-----------------------------------------------------------------------------
#include "ESMF.h"
#include "ESMF_Macros.inc"
#undef ESMF_METHOD
#define ESMF_METHOD "ESMF_FieldSMMEx"
     ! ESMF Framework module
     use ESMF_Mod
     use ESMF_TestMod
     implicit none

!------------------------------------------------------------------------------
! The following line turns the CVS identifier string into a printable variable.
    character(*), parameter :: version = &
    '$Id: ESMF_FieldSMMEx.F90,v 1.13 2010/04/16 16:25:51 feiliu Exp $'
!------------------------------------------------------------------------------

    ! Local variables
    integer :: rc, finalrc
    type(ESMF_Field)                            :: srcField, dstField
    type(ESMF_Field)                            :: srcFieldA, dstFieldA
    type(ESMF_Grid)                             :: grid
    type(ESMF_DistGrid)                         :: distgrid
    type(ESMF_VM)                               :: vm
    type(ESMF_RouteHandle)                      :: routehandle
    type(ESMF_Array)                            :: srcArray, dstArray
    type(ESMF_ArraySpec)                        :: arrayspec
    integer                                     :: localrc, lpe, i

    integer, allocatable                        :: src_farray(:), dst_farray(:)
    integer                                     :: fa_shape(1)
    integer, pointer                            :: fptr(:)
        
    integer(ESMF_KIND_I4), allocatable          :: factorList(:)
    integer, allocatable                        :: factorIndexList(:,:)

    rc = ESMF_SUCCESS
    finalrc = ESMF_SUCCESS
!------------------------------------------------------------------------------
    call ESMF_Initialize(rc=rc)
    if (rc /= ESMF_SUCCESS) call ESMF_Finalize(terminationflag=ESMF_ABORT)

    if (.not. ESMF_TestMinPETs(4, ESMF_SRCLINE)) &
        call ESMF_Finalize(terminationflag=ESMF_ABORT)
!------------------------------------------------------------------------------
!BOE
! \subsubsection{Sparse matrix multiplication from source Field to destination Field}
! \label{sec:field:usage:smm_1dptr}
!
! A user can use {\tt ESMF\_FieldSMM()} interface to perform sparse matrix multiplication 
! from 
! source Field to destination Field. This interface is overloaded by type and kind;
! 
! In this example, we first create two 1D Fields, a source Field and a destination
! Field. Then we use {\tt ESMF\_FieldSMM} to
! perform sparse matrix multiplication from source Field to destination Field.
!
! The source and destination Field data are arranged such that each of the 4 PETs has 4
! data elements. Moreover, the source Field has all its data elements initialized to a linear
! function based on local PET number. 
! Then collectively on each PET, a SMM according to the following formula
! is preformed: \newline
! $dstField(i) = i * srcField(i), i = 1 ... 4$ \newline
! \newline
!
! Because source Field data are initialized to a linear function based on local PET number, 
! the formula predicts that
! the result destination Field data on each PET is {1,2,3,4}. This is verified in the
! example.
!
! Section \ref{Array:SparseMatMul} provides a detailed discussion of the 
! sparse matrix mulitiplication operation implemented in ESMF.
! 
!EOE
!BOC 

    ! Get current VM and pet number
    call ESMF_VMGetCurrent(vm, rc=rc)
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE

    call ESMF_VMGet(vm, localPet=lpe, rc=rc)
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE

    ! create distgrid and grid
    distgrid = ESMF_DistGridCreate(minIndex=(/1/), maxIndex=(/16/), &
        regDecomp=(/4/), &
        rc=rc)
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE

    grid = ESMF_GridCreate(distgrid=distgrid, &
        gridEdgeLWidth=(/0/), gridEdgeUWidth=(/0/), &
        name="grid", rc=rc)
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE

    call ESMF_FieldGet(grid, localDe=0, totalCount=fa_shape, rc=rc)
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE

    ! create src_farray, srcArray, and srcField
    ! +--------+--------+--------+--------+
    !      1        2        3        4            ! value
    ! 1        4        8        12       16       ! bounds
    allocate(src_farray(fa_shape(1)) )
    src_farray = lpe+1
    srcArray = ESMF_ArrayCreate(src_farray, distgrid=distgrid, indexflag=ESMF_INDEX_DELOCAL, &
        rc=rc)
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE

    srcField = ESMF_FieldCreate(grid, srcArray, rc=rc)
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE

    ! create dst_farray, dstArray, and dstField
    ! +--------+--------+--------+--------+
    !      0        0        0        0            ! value
    ! 1        4        8        12       16       ! bounds
    allocate(dst_farray(fa_shape(1)) )
    dst_farray = 0
    dstArray = ESMF_ArrayCreate(dst_farray, distgrid=distgrid, indexflag=ESMF_INDEX_DELOCAL, &
        rc=rc)
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE

    dstField = ESMF_FieldCreate(grid, dstArray, rc=rc)
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE

    ! perform sparse matrix multiplication
    ! 1. setup routehandle from source Field to destination Field
    ! initialize factorList and factorIndexList
    allocate(factorList(4))
    allocate(factorIndexList(2,4))
    factorList = (/1,2,3,4/)
    factorIndexList(1,:) = (/lpe*4+1,lpe*4+2,lpe*4+3,lpe*4+4/)
    factorIndexList(2,:) = (/lpe*4+1,lpe*4+2,lpe*4+3,lpe*4+4/)

    call ESMF_FieldSMMStore(srcField, dstField, routehandle, &
        factorList, factorIndexList, rc=localrc)
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE

    ! 2. use precomputed routehandle to perform SMM
    call ESMF_FieldSMM(srcfield, dstField, routehandle, rc=rc)
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE

    ! verify sparse matrix multiplication
    call ESMF_FieldGet(dstField, localDe=0, farrayPtr=fptr, rc=rc)
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE

    ! Verify that the result data in dstField is correct.
    ! Before the SMM op, the dst Field contains all 0. 
    ! The SMM op reset the values to the index value, verify this is the case.
    ! +--------+--------+--------+--------+
    !  1 2 3 4  2 4 6 8  3 6 9 12  4 8 12 16       ! value
    ! 1        4        8        12       16       ! bounds
    do i = lbound(fptr, 1), ubound(fptr, 1)
        if(fptr(i) /= i*(lpe+1)) rc = ESMF_FAILURE
    enddo
!EOC
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE

!BOE
! Field sparse matrix matmul can also be performed between weakly congruent Fields.
! In this case, source and destination Fields can have ungridded dimensions
! with size different from the Field pair used to compute the routehandle. 
!EOE
!BOC
    call ESMF_ArraySpecSet(arrayspec, typekind=ESMF_TYPEKIND_I4, rank=2, rc=rc)
!EOC
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE
!BOE
! Create two fields with ungridded dimensions using the Grid created previously.
! The new Field pair has matching number of elements. The ungridded dimension
! is mapped to the first dimension of either Field.
!EOE
!BOC
    srcFieldA = ESMF_FieldCreate(grid, arrayspec, gridToFieldMap=(/2/), &
        ungriddedLBound=(/1/), ungriddedUBound=(/10/), rc=rc)
!EOC
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE
!BOC
    dstFieldA = ESMF_FieldCreate(grid, arrayspec, gridToFieldMap=(/2/), &
        ungriddedLBound=(/1/), ungriddedUBound=(/10/), rc=rc)
!EOC
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE
!BOE
! Using the previously computed routehandle, weakly congruent Fields can perform
! sparse matrix matmul.
!EOE
!BOC
    call ESMF_FieldSMM(srcfieldA, dstFieldA, routehandle, rc=rc)
!EOC
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE

!BOC
    ! release route handle
    call ESMF_FieldSMMRelease(routehandle, rc=rc)
!EOC
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE

    ! destroy all objects created in this example to prevent memory leak
    call ESMF_FieldDestroy(srcField, rc=rc)
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE
    call ESMF_FieldDestroy(dstField, rc=rc)
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE
    call ESMF_FieldDestroy(srcFieldA, rc=rc)
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE
    call ESMF_FieldDestroy(dstFieldA, rc=rc)
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE
    call ESMF_ArrayDestroy(srcArray, rc=rc)
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE
    call ESMF_ArrayDestroy(dstArray, rc=rc)
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE
    call ESMF_GridDestroy(grid, rc=rc)
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE
    call ESMF_DistGridDestroy(distgrid, rc=rc)
    if(rc .ne. ESMF_SUCCESS) finalrc = ESMF_FAILURE
    deallocate(src_farray, dst_farray, factorList, factorIndexList)

     call ESMF_Finalize(rc=rc)

     if (rc.NE.ESMF_SUCCESS) then
       finalrc = ESMF_FAILURE
     end if

     if (finalrc.EQ.ESMF_SUCCESS) then
       print *, "PASS: ESMF_FieldSMMEx.F90"
     else
       print *, "FAIL: ESMF_FieldSMMEx.F90"
     end if

    end program FieldSMMEx
