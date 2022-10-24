
/***************************************************************************
*
*   Copyright (c) 1999-2000 Timpanogas Research Group, Inc.
*   7260 SE Tenino St.
*   Portland, Oregon 97206
*   jmerkey@timpanogas.com
*
*   This program is free software; you can redistribute it and/or modify it
*   under the terms of the TRG Windows NT/2000(tm) Public License (WPL) as
*   published by the Timpanogas Research Group, Inc.  (Windows NT/2000
*   is a registered trademark of Microsoft Corporation).
*
*   This program is distributed in the hope that it will be useful, but
*   WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
*   You should have received a copy of the WPL Public License along
*   with this program; if not, a copy can be obtained from
*   www.timpanogas.com.
*
*   You are free to modify and re-distribute this program in accordance
*   with the terms specified in the TRG Windows NT/2000 Public License (WPL).
*   The copyright contained in this code is required to be present in
*   any derivative works and you are required to provide the source code
*   for this program as part of any commercial or non-commercial
*   distribution. You are required to respect the rights of the
*   Copyright holders named within this code.
*
*   jmerkey@timpanogas.com and TRG, Inc. are the official maintainers of
*   this code.  You are encouraged to report any bugs, problems, fixes,
*   suggestions, and comments about this software to jmerkey@timpanogas.com
*   or linux-kernel@vger.rutgers.edu.  New releases, patches, bug fixes, and
*   technical documentation can be found at www.timpanogas.com.  TRG will
*   periodically post new releases of this software to www.timpanogas.com
*   that contain bug fixes and enhanced capabilities.
*
****************************************************************************
/*++

Copyright (c) 1999-2000  Timpanogas Research Group, Inc.

Module Name:

    DevIoSup.c

Abstract:

    This module implements the low lever disk read support for Nwfs.

  Authors:

    Jeff Merkey (jmerkey@timpanogas.com)
    David Goebel (davidg@balder.com) 11-Mar-1999

--*/

#include "NwProcs.h"
#include "Fenris.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NWFS_BUG_CHECK_DEVIOSUP)

BOOLEAN NwMcbDebug = FALSE;

//
//  Local structure definitions

#define HOLE_LBO ((ULONGLONG)0)
#define HOLE_LBN ((ULONG)0)

//
// Completion Routine and local support declarations.
//

NTSTATUS
NwNtStatusFromNwStatus(
    IN NW_STATUS NwStatus
    );

NTSTATUS
NwLookupFileAllocation(
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN VBN StartingVbn,
    OUT PDEVICE_OBJECT *TargetDevice,
    OUT LBN *Lbn,
    OUT ULONG *SectorCount
    );

VOID
NwSingleAsync (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT TargetDevice,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PIRP Irp
    );

VOID
NwReadSingleSector (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PDEVICE_OBJECT TargetDevice,
    IN LBN Lbn
    );

NTSTATUS
NwMultiAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

NTSTATUS
NwSingleAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NwSingleAsync)
#pragma alloc_text(PAGE, NwNonCachedIo)
#pragma alloc_text(PAGE, NwReadSingleSector)
#pragma alloc_text(PAGE, NwNonCachedNonAlignedRead)
#pragma alloc_text(PAGE, NwCreateUserMdl)
#endif

NTSTATUS
NwNonCachedIo (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN VBN StartingVbn,
    IN ULONG ByteCount
    )

/*++

Routine Description:

    This routine performs the non-cached read described in its parameters.

Arguments:

    IrpContext - Supplies context.

    Irp - Supplies the requesting Irp.

    Fcb - Supplies the file to read from.

    StartingVbn - The starting point for the read.

    ByteCount - The lengh of the read.

Return Value:

    NTSTATUS - STATUS_SUCCESS if the read consisted entirely of a hole.
               STATUS_PENDING if the request was sent to the lower device
                    driver and the Irp will be completed in the for us.
               An error code if the Irp must be completed by us.
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IrpSp;

    ULONG CurrentByteCount = 0;
    ULONG BytesProcessed = 0;
    ULONG SectorsProcessed = 0;

    VBN CurrentVbn;         
    LBN CurrentLbn;        
    ULONG CurrentSectorCount; 
    PDEVICE_OBJECT CurrentTargetDevice;

    VBN NextVbn;         
    LBN NextLbn;        
    ULONG NextSectorCount; 
    PDEVICE_OBJECT NextTargetDevice;
    
    ULONG SectorCount;
    ULONG DispatchedIrpCount = 0;
    BOOLEAN FinalIrpDispatch = FALSE;

    PMDL PartialMdl = NULL;
    PIRP AssociatedIrp = NULL;

    PUCHAR UserBuffer = NULL;

    //
    //  For nonbuffered I/O, we need the buffer locked in all
    //  cases.
    //  
    //  This call may raise.  If this call succeeds and a subsequent
    //  condition is raised, the buffers are unlocked automatically
    //  by the I/O system when the request is completed, via the
    //  Irp->MdlAddress field.
    //

    NwLockUserBuffer( IrpContext, ByteCount );

    try {

        CurrentVbn = StartingVbn;
        SectorCount = SectorsFromBytes( ByteCount );

        //
        //  Here is our strategy to get this read done.  An important issue is
        //  that we don't know how many associated Irps will be required until
        //  we actually send them all.  So we basically just start sending them
        //  down.  If we get an error condition looking up extents, then we will
        //  wait for any associated Irps already sent to complete, and fail the
        //  Irp ourselves.  If an I/O error is encountered, the Irp will be
        //  completed by the I/O system.
        //
        //  Lookup the extent information.  This routine handles allocation
        //  holes, hot fixes and all that other stuff.
        //

        Status = NwLookupFileAllocation( IrpContext,
                                         Fcb,
                                         CurrentVbn,
                                         &CurrentTargetDevice,
                                         &CurrentLbn,
                                         &CurrentSectorCount );

        //
        //  If we don't have a mapping for the range, the pre-existing file
        //  must have an allocation error.
        //

        if (!NT_SUCCESS(Status)) {
            NwPopUpFileCorrupt( IrpContext );
            try_return( Status );
        }

        //
        //  We are going to make one optimization here.  If this first extent is
        //  sufficient for the entire read, then take a special path that does
        //  not involve creating an associated Irp.
        //

        if (CurrentSectorCount >= SectorCount) {

            //
            //  If we are even more lucky and this is a hole, just zero the
            //  range of the file covered by the extent.
            //

            if (CurrentLbn == HOLE_LBN) {

                if (UserBuffer == NULL) {
                    UserBuffer = NwMapUserBuffer( IrpContext );
                }

                RtlZeroMemory( UserBuffer, BytesFromSectors(SectorCount) );

                try_return( Status = STATUS_SUCCESS );
            }

            //
            //  Oh well, just send it down.
            //

            NwSingleAsync( IrpContext,
                           CurrentTargetDevice,
                           CurrentLbn,
                           SectorCount,
                           Irp );

            try_return( Status = STATUS_PENDING );
        }

        //
        //  Otherwise we just loop through the read range.
        //

        while (SectorsProcessed < SectorCount) {

            //
            //  If this extent extends beyond what we need, reduce it.
            //

            if (CurrentSectorCount > SectorCount - SectorsProcessed) {
                CurrentSectorCount = SectorCount - SectorsProcessed;
            }
            
            //
            //  Keep track of some byte quantities.
            //

            BytesProcessed = BytesFromSectors(SectorsProcessed);
            CurrentByteCount = BytesFromSectors(CurrentSectorCount);

            //
            //  Lookup the next extent information.  We need to do this because 
            //  it is critical to know the total number of Irps that will be
            //  dispatched *before* the final Irp is dispatched.  Since the final
            //  run on a file can be a hole, we need to know that in advance.
            //
            //  If we are currently processing the final extent (allocated or not)
            //  there is no reason to try and look up the next one.
            //

            if (SectorsProcessed + CurrentSectorCount < SectorCount) {

                NextVbn = CurrentVbn + CurrentSectorCount;

                Status = NwLookupFileAllocation( IrpContext,
                                                 Fcb,
                                                 NextVbn,
                                                 &NextTargetDevice,
                                                 &NextLbn,
                                                 &NextSectorCount );

                //
                //  If we don't have a mapping for the range, the pre-existing file
                //  must have an allocation error.
                //

                if (!NT_SUCCESS(Status)) {
                    NwPopUpFileCorrupt( IrpContext );
                    try_return( Status );
                }

                //
                //  Now if this next extent is the final extent and is a hole,
                //  then we must take final Irp dispatch steps on this itteration.
                //

                if ((NextLbn == HOLE_LBN) &&
                    (SectorsProcessed + CurrentSectorCount + NextSectorCount >= SectorCount)) {

                    FinalIrpDispatch = TRUE;
                }

            } else {

                //
                //  This is the final extent.  If this is real allocation, note
                //  that this is the final Irp dispatch.
                //

                if (CurrentLbn != HOLE_LBN) {

                    ASSERT( FinalIrpDispatch == FALSE );

                    FinalIrpDispatch = TRUE;

                } else {

                    ASSERT( FinalIrpDispatch == TRUE );
                }
            }

            //
            //  If this run is a hole, just zero the range of the file, set up
            //  the 'current' variables for a new run and continue.
            //

            if (CurrentLbn == HOLE_LBN) {

                if (UserBuffer == NULL) {
                    UserBuffer = NwMapUserBuffer( IrpContext );
                }

                RtlZeroMemory( UserBuffer + BytesProcessed, CurrentByteCount );

                SectorsProcessed += CurrentSectorCount;

                CurrentVbn = NextVbn;
                CurrentLbn = NextLbn;         
                CurrentSectorCount = NextSectorCount;   
                CurrentTargetDevice = NextTargetDevice;

                continue;
            }

            //
            //  We actually need to read the file.  Create an associated IRP
            //  (making sure there is one stack entry for us as well), and a
            //  partial MDL for the tranfer.
            //

            AssociatedIrp = IoMakeAssociatedIrp( Irp,
                                                 (CCHAR)(CurrentTargetDevice->StackSize + 1) );

            if (AssociatedIrp == NULL) {
                try_return( Status = STATUS_INSUFFICIENT_RESOURCES );
            }

            //
            // Allocate and build a partial Mdl for the request.
            //

            PartialMdl = IoAllocateMdl( (PCHAR)Irp->UserBuffer + BytesProcessed,
                                        CurrentByteCount,
                                        FALSE,
                                        FALSE,
                                        AssociatedIrp );

            if (PartialMdl == NULL) {
                try_return( Status = STATUS_INSUFFICIENT_RESOURCES );
            }

            //
            //  Sanity Check
            //

            ASSERT( PartialMdl == AssociatedIrp->MdlAddress );

            IoBuildPartialMdl( Irp->MdlAddress,
                               PartialMdl,
                               (PCHAR)Irp->UserBuffer + BytesProcessed,
                               CurrentByteCount );

            //
            //  Get the first IRP stack location in the associated Irp.
            //

            IoSetNextIrpStackLocation( AssociatedIrp );
            IrpSp = IoGetCurrentIrpStackLocation( AssociatedIrp );

            //
            //  Setup the Stack location to describe our (top-level) read.
            //

            IrpSp->MajorFunction = IrpContext->MajorFunction;
            IrpSp->Parameters.Read.Length = CurrentByteCount;
            IrpSp->Parameters.Read.ByteOffset.QuadPart = LlBytesFromSectors(CurrentVbn);

            //
            // Set up the completion routine address in our stack frame.
            //

            IoSetCompletionRoutine( AssociatedIrp,
                                    &NwMultiAsyncCompletionRoutine,
                                    IrpContext->NwIoContext,
                                    TRUE,
                                    TRUE,
                                    TRUE );

            //
            //  Setup the next IRP stack location in the associated Irp for the disk
            //  driver beneath us.
            //

            IrpSp = IoGetNextIrpStackLocation( AssociatedIrp );

            //
            //  Setup the Stack location to do a read from the disk driver.
            //

            IrpSp->MajorFunction = IrpContext->MajorFunction;
            IrpSp->Parameters.Read.Length = CurrentByteCount;
            IrpSp->Parameters.Read.ByteOffset.QuadPart = LlBytesFromSectors(CurrentLbn);

            //
            //  Before dispatching this Irp, see if this is the final Irp to be
            //  dispatched.  If so, we need to set up several counts so that the
            //  Irp will be completed correctly in the completion routine.
            //

            DispatchedIrpCount++;

            if (FinalIrpDispatch) {

                //
                //  Set the count in the Master Irp.  This will cause the I/O 
                //  system to complete it as soon as the last last I/O completes.
                //
                //  NOTE: Since only 1 associated Irp will return STATUS_SUCCESS
                //  from it's completion routine, the I/O system will consider
                //  that there was only 1 associated Irp, thus setting 1 in 
                //  this field is completely correct and makes no assumption about
                //  I/O completion ordering or anything of the sort.
                //

                Irp->AssociatedIrp.IrpCount = 1;

                //
                //  Now we set the Irp count in the I/O context.  This is what
                //  guarentees that only a single assocuiated Irp will return 
                //  STATUS_SUCCESS from its completion routine.
                //

                IrpContext->NwIoContext->IrpCount = DispatchedIrpCount;
            }

            //
            //  If IoCallDriver returns an error, it has completed the Irp and the
            //  error will be caught by our completion routines and dealt with as
            //  a normal IO error.
            //

            (VOID)IoCallDriver( CurrentTargetDevice, AssociatedIrp );

            //
            //  NULL these so that we know there is recovery to perform.
            //

            PartialMdl = NULL;
            AssociatedIrp = NULL;

            //
            //  Now move the 'Next' variables to 'Current' and loop.
            //

            SectorsProcessed += CurrentSectorCount;

            CurrentVbn = NextVbn;
            CurrentLbn = NextLbn;         
            CurrentSectorCount = NextSectorCount;   
            CurrentTargetDevice = NextTargetDevice;

        } // while ( SectorsProcessed < SectorCount )

        //
        //  If we got here, then everything went fine.  We either successfully
        //  zeroed the user's buffer, dispatched associated Irps, or some
        //  combination of these two to satisfy the user's read.
        //
        //  If we dispatched *any* Irps then we must return STATUS_PENDING.
        //

        if (DispatchedIrpCount == 0) {

            Status = STATUS_SUCCESS;

        } else {

            Status = STATUS_PENDING;
        }

    try_exit: NOTHING;
    } finally {

        //
        //  First, if there is an unused Irp and/or Mdl, free them.
        //

        if (AssociatedIrp) {
            if (PartialMdl) {
                IoFreeMdl( PartialMdl );
            }
            IoFreeIrp( AssociatedIrp );
        }

        //
        //  Now if we received an error condition (either an exception or
        //  a failed API call above), we need to block until all outstanding
        //  I/O requests complete, and then complete the Irp ourselves or let
        //  the exception continue.
        //

        if (AbnormalTermination() || !NT_SUCCESS(Status)) {

            while (DispatchedIrpCount--) {
        
                KeWaitForSingleObject( &IrpContext->NwIoContext->Semaphore,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL );
            }
        }

        //
        //  The above logic is based on the fact that a final loop itteration
        //  that simply zeros the user's buffer cannot raise an exception.
        //  Assert this fact.
        //

        ASSERT( !(AbnormalTermination() &&
                 (SectorsProcessed + CurrentSectorCount >= SectorCount) &&
                 (CurrentLbn == HOLE_LBN)) );
    }

    //
    //  Now just return the status.  The code in read.c will complete the Irp
    //  if PENDING is not returned.
    //

    return Status;
}

NTSTATUS
NwNonCachedNonAlignedRead (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN ULONG StartingVbo,
    IN ULONG ByteCount
    )

/*++

Routine Description:

    This routine performs the non-cached disk read described in its parameters.
    This routine differs from the above in that the range does not have to be
    sector aligned.  This accomplished with the use of intermediate buffers.

Arguments:

    IrpContext - Supplies context.

    Irp - Supplies the requesting Irp.

    Fcb - Supplies the file to read from.

    StartingVbo - The starting point of the read.

    SectorCount - The lengh of the read.

Return Value:

    NTSTATUS - The result of the operation.

--*/

{
    //
    //  Declare some local variables for enumeration through the
    //  runs of the file, and an array to store parameters for
    //  parallel I/Os
    //

    NTSTATUS Status = STATUS_SUCCESS;

    LBN Lbn;
    ULONG SectorCount;

    ULONG BytesToCopy;
    ULONG OriginalByteCount;
    ULONG OriginalStartingVbo;

    PUCHAR UserBuffer;
    PUCHAR SectorBuffer = NULL;

    PMDL PartialMdl = NULL;
    PMDL SectorMdl = NULL;
    PMDL SavedMdl = NULL;
    PVOID SavedUserBuffer = NULL;

    PDEVICE_OBJECT TargetDevice;

    //
    //  Initialize some locals.
    //

    OriginalByteCount = ByteCount;
    OriginalStartingVbo = StartingVbo;

    ASSERT( FlagOn(IrpContext->Flags, IRP_CONTEXT_FLAG_WAIT) );

    //
    //  For nonbuffered I/O, we need the buffer locked in all
    //  cases.
    //
    //  This call may raise.  If this call succeeds and a subsequent
    //  condition is raised, the buffers are unlocked automatically
    //  by the I/O system when the request is completed, via the
    //  Irp->MdlAddress field.
    //

    NwLockUserBuffer( IrpContext, ByteCount );

    UserBuffer = NwMapUserBuffer( IrpContext );

    //
    //  Allocate the local buffer, and MDL.
    //

    SectorBuffer = FsRtlAllocatePoolWithTag( NonPagedPoolCacheAligned,
                                             SECTOR_SIZE,
                                             TAG_TEMP_BUFFER );

    SavedMdl = Irp->MdlAddress;
    SavedUserBuffer = Irp->UserBuffer;
    Irp->UserBuffer = SectorBuffer;

    SectorMdl = IoAllocateMdl( SectorBuffer, SECTOR_SIZE, FALSE, FALSE, Irp );

    if (SectorMdl == NULL) {
        Irp->MdlAddress = SavedMdl;
        Irp->UserBuffer = SavedUserBuffer;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    MmBuildMdlForNonPagedPool( SectorMdl );

    DbgPrint("Nwfsro: Non-aligned read, Fcb = 0x%08lx, Vbo = 0x%08lx, Length = 0x%08lx\n.",
             Fcb, StartingVbo, ByteCount);

    //
    //  We use a try block here to ensure the buffer is freed, and to
    //  fill in the correct Sector count in the Iosb.Information field.
    //

    try {

        //
        //  If the beginning of the request was not aligned correctly, read in
        //  the first part first.
        //

        if (SectorOffset( StartingVbo )) {

            ULONG Hole;

            //
            // Try to lookup the first run.
            //

            Status = NwLookupFileAllocation( IrpContext,
                                             Fcb,
                                             SectorsFromBytes(StartingVbo),
                                             &TargetDevice,
                                             &Lbn,
                                             &SectorCount );

            if (!NT_SUCCESS(Status)) {
                NwPopUpFileCorrupt( IrpContext );
                try_return( Status );
            }

            //
            //  Now copy the part of the first sector that we want to the user
            //  buffer.
            //

            Hole = SectorOffset(StartingVbo);

            BytesToCopy = ByteCount >= SECTOR_SIZE - Hole ?
                                       SECTOR_SIZE - Hole : ByteCount;

            if (Lbn != HOLE_LBN) {

                //
                //  We either read the sector from disk and copy it to the user
                //  buffer, or just zero the user's buffer
                //

                NwReadSingleSector( IrpContext,
                                    Irp,
                                    TargetDevice,
                                    Lbn );

                if (!NT_SUCCESS( Irp->IoStatus.Status )) {

                    try_return( Status = Irp->IoStatus.Status );
                }

                RtlCopyMemory( UserBuffer, SectorBuffer + Hole, BytesToCopy );

            } else {

                RtlZeroMemory( UserBuffer, BytesToCopy );
            }

            StartingVbo += BytesToCopy;
            ByteCount -= BytesToCopy;

            if ( ByteCount == 0 ) {

                try_return( Status );
            }
        }

        ASSERT( SectorOffset(StartingVbo) == 0 );

        //
        //  If there is a tail part that is not sector aligned, read it.
        //

        if ( SectorOffset(ByteCount) ) {

            ULONG LastSectorVbo;

            LastSectorVbo = StartingVbo + SectorTruncate(ByteCount);

            //
            // Try to lookup the last part of the requested range.
            //

            Status = NwLookupFileAllocation( IrpContext,
                                             Fcb,
                                             SectorsFromBytes(LastSectorVbo),
                                             &TargetDevice,
                                             &Lbn,
                                             &SectorCount );

            if (!NT_SUCCESS(Status)) {
                NwPopUpFileCorrupt( IrpContext );
                try_return( Status );
            }

            //
            //  Now copy over the part of this last sector that we need.
            //

            BytesToCopy = SectorOffset(ByteCount);

            UserBuffer += LastSectorVbo - OriginalStartingVbo;

            if (Lbn != HOLE_LBN) {

                //
                //  We either read the sector from disk and copy it to the user
                //  buffer, or just zero the user's buffer
                //

                NwReadSingleSector( IrpContext,
                                    Irp,
                                    TargetDevice,
                                    Lbn );

                if (!NT_SUCCESS( Irp->IoStatus.Status )) {

                    try_return( Status = Irp->IoStatus.Status );
                }

                RtlCopyMemory( UserBuffer, SectorBuffer, BytesToCopy );

            } else {

                RtlZeroMemory( UserBuffer, BytesToCopy );
            }

            ByteCount -= BytesToCopy;

            if ( ByteCount == 0 ) {

                try_return( Status );
            }
        }

        ASSERT( SectorOffset(StartingVbo | ByteCount) == 0 );

        //
        //  We now flush the user's buffer to memory.  This is a no-op on x86.
        //

        KeFlushIoBuffers( Irp->MdlAddress, TRUE, FALSE );

        //
        //  Now build a Mdl describing the sector aligned balance of the transfer,
        //  and put it in the Irp, and read that part.
        //

        Irp->MdlAddress = NULL;
        Irp->UserBuffer = (PUCHAR)MmGetMdlVirtualAddress( SavedMdl ) +
                          (StartingVbo - OriginalStartingVbo);

        PartialMdl = IoAllocateMdl( Irp->UserBuffer,
                                    ByteCount,
                                    FALSE,
                                    FALSE,
                                    Irp );

        if (PartialMdl == NULL) {

            Irp->MdlAddress = SavedMdl;
            Irp->UserBuffer = SavedUserBuffer;
            try_return( Status = STATUS_INSUFFICIENT_RESOURCES );
        }

        IoBuildPartialMdl( SavedMdl,
                           PartialMdl,
                           Irp->UserBuffer,
                           ByteCount );

        //
        //  Try to read in the sector aligned pages.
        //

        Status = NwNonCachedIo( IrpContext,
                                Irp,
                                Fcb,
                                SectorsFromBytes(StartingVbo),
                                ByteCount );

    try_exit: NOTHING;

    } finally {

        //
        //  We always free out temp one sector buffer and its MDL.
        //

        if (SectorMdl) {
            IoFreeMdl( SectorMdl );
        }

        if (SectorBuffer) {
            ExFreePool( SectorBuffer );
        }

        //
        //  If we got STATUS_PENDING, then the Irp will be completed by
        //  the completion routine and thus we need to free the original
        //  MDL.  For any other status code restore the original MDL and
        //  free the PartialMdl is present.
        //

        if (Status == STATUS_PENDING) {

            if (SavedMdl) {
                IoFreeMdl( SavedMdl );
            }

        } else {

            if (Irp->MdlAddress == PartialMdl) {

                Irp->MdlAddress = SavedMdl;
                Irp->UserBuffer = SavedUserBuffer;

                if (PartialMdl) {
                    IoFreeMdl(PartialMdl);
                }
            }

            //
            //  Also if got an actual success value (could happen if the
            //  sector aligned balance of the transfer was a hole), update
            //  the total Sector count in the Irp information field.
            //

            if ( !AbnormalTermination() && NT_SUCCESS(Status) ) {
                Irp->IoStatus.Information = OriginalByteCount;
            }
        }

    }

    return Status;
}

VOID
NwSingleAsync (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT TargetDevice,
    IN LBN Lbn,
    IN ULONG SectorCount,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine reads one or more contiguous sectors from a device
    asynchronously, and is used if there is only one read necessary to
    complete the IRP.  It implements the read by simply filling in the next
    stack frame in the Irp, and passing it on.  The transfer occurs to the
    single buffer originally specified in the user request.

Arguments:

    TargetDevice - Supplies the device to read

    Lbn - Supplies the starting Logical Sector Offset to begin reading from.

    SectorCount - Supplies the number of Sectors to read from the device

    Irp - Supplies the master Irp to associated with the async
          request.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION IrpSp;

    //
    // Set up the completion routine address in our stack frame.
    //

    IoSetCompletionRoutine( Irp,
                            &NwSingleAsyncCompletionRoutine,
                            IrpContext->NwIoContext,
                            TRUE,
                            TRUE,
                            TRUE );

    //
    //  Setup the next IRP stack location in the associated Irp for the disk
    //  driver beneath us.
    //

    IrpSp = IoGetNextIrpStackLocation( Irp );

    //
    //  Setup the Stack location to do a read from the disk driver.
    //

    IrpSp->MajorFunction = IrpContext->MajorFunction;
    IrpSp->Parameters.Read.Length = BytesFromSectors(SectorCount);
    IrpSp->Parameters.Read.ByteOffset.QuadPart = LlBytesFromSectors(Lbn);

    //
    //  Issue the read request
    //
    //  If IoCallDriver returns an error, it has completed the Irp
    //  and the error will be caught by our completion routines
    //  and dealt with as a normal IO error.
    //

    (VOID)IoCallDriver( TargetDevice, Irp );

    //
    //  And return to our caller
    //

    return;
}


VOID
NwReadSingleSector (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PDEVICE_OBJECT TargetDevice,
    IN LBN Lbn
    )

/*++

Routine Description:

    This routine reads  one or more contiguous sectors from a device
    synchronously, and does so to a buffer described by the Mdl in the Irp.
    It implements the read by simply filling in the next stack frame in the
    Irp, and passing it on.

Arguments:

    Irp - Irp->Mdl is the target of the read.
    
    TargetDevice - Supplies the device to read from.

    Lbo - Supplies the starting Logical Sector Offset to begin reading from.

Return Value:

    None.

--*/

{
    KEVENT Event;
    PIO_STACK_LOCATION IrpSp;

    //
    // Set up the completion routine address in our stack frame.
    //

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    IoSetCompletionRoutine( Irp,
                            &NwResyncCompletionRoutine,
                            &Event,
                            TRUE,
                            TRUE,
                            TRUE );

    //
    //  Setup the next IRP stack location in the associated Irp for the disk
    //  driver beneath us.
    //

    IrpSp = IoGetNextIrpStackLocation( Irp );

    //
    //  Setup the Stack location to do a read from the disk driver.
    //

    IrpSp->MajorFunction = IrpContext->MajorFunction;
    IrpSp->Parameters.Read.Length = SECTOR_SIZE;
    IrpSp->Parameters.Read.ByteOffset.QuadPart = LlBytesFromSectors(Lbn);;

    //
    //  Issue the read request
    //
    //  If IoCallDriver returns an error, it has completed the Irp and the
    //  error will be caught by our completion routines and dealt with as a
    //  normal IO error.
    //

    (VOID)IoCallDriver( TargetDevice, Irp );

    (VOID)KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, NULL );

    //
    //  And return to our caller
    //

    return;
}


//
// Internal Support Routine
//

NTSTATUS
NwMultiAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

/*++

Routine Description:

    This is the completion routine for all reads that span multiple extents.

Arguments:

    DeviceObject - Pointer to the file system device object.

    Irp - Pointer to the associated Irp which is being completed.  (This
          Irp will no longer be accessible after this routine returns.)

    Contxt - The context parameter which was specified for all of
             the multiple asynch I/O requests for this MasterIrp.

Return Value:

    The routine returns STATUS_SUCCESS so that IoCompleteRequest will
    automatically complete the MasterIrp when its internal  IrpCount
    goes to 0.

--*/

{
    ULONG CompletedIrpCount;
    ULONG DispatchedIrpCount;
    PNWFS_IO_CONTEXT Context = Contxt;
    PIRP MasterIrp = Context->MasterIrp;

    //
    //  If this Irp was a failure, note it in the MasterIrp, and mark this
    //  partition as bad.
    //

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {

        MasterIrp->IoStatus.Status = Irp->IoStatus.Status;

        ASSERT( IoGetNextIrpStackLocation(Irp)->DeviceObject );
        NwMarkPartitionBad( IoGetNextIrpStackLocation(Irp)->DeviceObject );
    }

    //
    //  Now we release the semaphore and atomically get the number of
    //  completed Irps in exchange.  This allows one, and only one, associated
    //  Irp to know that it is the final Irp.  Nobody else can touch the Irp
    //  or the Context after releasing the semaphore.
    //

    DispatchedIrpCount = Context->IrpCount;
    CompletedIrpCount = (ULONG)KeReleaseSemaphore( &Context->Semaphore, 0, 1, FALSE ) + 1;

    //
    //  If we are not the annointed one, just return MORE_PROCESSING so that
    //  IoCompleteRequest stop processing on this Irp.  We must do our own
    //  cleanup here since IoCompleteRequest won't get a chance on this
    //  associated Irp.
    //

    if (CompletedIrpCount != DispatchedIrpCount) {
        IoFreeMdl( Irp->MdlAddress );
        IoFreeIrp( Irp );

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    //
    //  We are 'the one'.  We just call NwSingleAsyncCompletionRoutine()
    //  since it has all the logic we need.  We do have to NULL out the
    //  stack's DeviceObject though so that it doesn't try to report it
    //  as bad in case of error.
    //

    IoGetNextIrpStackLocation(MasterIrp)->DeviceObject = NULL;

    return NwSingleAsyncCompletionRoutine( DeviceObject, MasterIrp, Contxt );
}

//
// Internal Support Routine
//

NTSTATUS
NwSingleAsyncCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

/*++

Routine Description:

    This is the completion routine for a single extent read.

Arguments:

    DeviceObject - Pointer to the file system device object.

    Irp - Pointer to the Irp for this request.  (This Irp will no longer
    be accessible after this routine returns.)

    Contxt - The context parameter which was specified in the call to
             NwReadSingleAsynch.

Return Value:

    Currently always returns STATUS_SUCCESS so that the Irp is completed.

--*/

{
    PNWFS_IO_CONTEXT Context = Contxt;

    //
    //  Fill in the information field correctly if this worked.
    //

    if (NT_SUCCESS(Irp->IoStatus.Status)) {

        Irp->IoStatus.Information = Context->RequestedByteCount;

        //
        //  Now if this wasn't PagingIo, set the read bit.
        //

        if (!FlagOn(Irp->Flags, IRP_PAGING_IO)) {

            SetFlag( Context->FileObject->Flags, FO_FILE_FAST_IO_READ );

            //
            //  If this was a synchronous FileObject, set the file position.
            //

            if (FlagOn( Context->FileObject->Flags, FO_SYNCHRONOUS_IO )) {

                Context->FileObject->CurrentByteOffset = Context->ByteRange;
            }
        }

        //
        //  If we have to zero a sector tail, do it here.
        //

        if (Context->SectorTailToZero) {

            RtlZeroMemory( Context->SectorTailToZero,
                           Context->BytesToZero );
        }

    } else {

        //
        //  If this Irp was used for I/O (i.e., we are not invoked from
        //  NwMultiAsyncCompletionRoutine()), then report this Partition
        //  as bad.
        //

        PDEVICE_OBJECT BadPartition;

        BadPartition = IoGetNextIrpStackLocation(Irp)->DeviceObject;

        if (BadPartition) {
            NwMarkPartitionBad( BadPartition );
        }
    }

    //
    //  Now release the resource
    //

    if (Context->Resource != NULL) {

        ExReleaseResourceForThread( Context->Resource,
                                    Context->ResourceThreadId );
    }

    //
    //  Mark the Irp pending.
    //

    IoMarkIrpPending( Irp );

    //
    //  and finally, free the context record.
    //

    ExFreePool( Context );

    UNREFERENCED_PARAMETER( DeviceObject );

    return STATUS_SUCCESS;
}


NTSTATUS
NwCreateUserMdl (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG BufferLength,
    IN BOOLEAN RaiseOnError
    )

/*++

Routine Description:

    This routine locks the specified buffer for read access (we only write into
    the buffer).  The file system requires this routine since it does not
    ask the I/O system to lock its buffers for direct I/O.  This routine
    may only be called from the Fsd while still in the user context.

    This routine is only called if there is not already an Mdl.

Arguments:

    BufferLength - Length of user buffer.

    RaiseOnError - Indicates if our caller wants this routine to raise on
        an error condition.

Return Value:

    NTSTATUS - Status from this routine.  Error status only returned if
        RaiseOnError is FALSE.

--*/

{
    NTSTATUS Status = STATUS_INSUFFICIENT_RESOURCES;
    PMDL Mdl;

    PAGED_CODE();

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_IRP( IrpContext->Irp );
    ASSERT( IrpContext->Irp->MdlAddress == NULL );

    //
    // Allocate the Mdl, and Raise if we fail.
    //

    Mdl = IoAllocateMdl( IrpContext->Irp->UserBuffer,
                         BufferLength,
                         FALSE,
                         FALSE,
                         IrpContext->Irp );

    if (Mdl != NULL) {

        //
        //  Now probe the buffer described by the Irp.  If we get an exception,
        //  deallocate the Mdl and return the appropriate "expected" status.
        //

        try {

            MmProbeAndLockPages( Mdl, IrpContext->Irp->RequestorMode, IoWriteAccess );

            Status = STATUS_SUCCESS;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            Status = GetExceptionCode();

            IoFreeMdl( Mdl );
            IrpContext->Irp->MdlAddress = NULL;

            if (!FsRtlIsNtstatusExpected( Status )) {

                Status = STATUS_INVALID_USER_BUFFER;
            }
        }
    }

    //
    //  Check if we are to raise or return
    //

    if (Status != STATUS_SUCCESS) {

        if (RaiseOnError) {

            NwRaiseStatus( IrpContext, Status );
        }
    }

    //
    //  Return the status code.
    //

    return Status;
}

NTSTATUS
NwLookupFileAllocation(
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN VBN StartingVbn,
    OUT PDEVICE_OBJECT *TargetDevice,
    OUT LBN *Lbn,
    OUT ULONG *SectorCount
    )

/*++

Routine Description:

    This routine is responsible for providing the extent information (i.e.
    where the data for a file lives) in order to satisfy a read request.
    It takes this data from an MCB entry, which in turn is filled in from
    data supplied by the routines that read the Netware disk format.
    
    Finally, since a Netware volume can span multiple partitions (and thus
    disks), we have to call another routine which converts this volume 
    relative offset into a disk device object and its associated offset. 

Arguments:

    Fcb - The opened file that we are attempting to read.
    
    StartingVbn - The starting sector offset of the read in the file.
    
    TargetDevice - Receives the target device object that will actually 
        perform the read.  Currently the disk, not partition, device.
    
    Lbn - Receives the sector offset from the begining of the TargetDevice
        where the read should begin.
    
    SectorCount - Receives the number of sectors in this extent.
    
Return Value:

    NTSTATUS - Status from this routine.
    
--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILE_OBJECT FileObject;
    NW_STATUS NwStatus;

    VBN Vbn;
    ULONG Index;
    ULONG RunSectorCount;
    ULONG PartitionSectorCount;
    ULONG AllocationSectors;

    //
    //  Do a sanity check.
    //

    AllocationSectors = SectorsFromBytes(Fcb->AllocationSize.LowPart);

    if (StartingVbn >= AllocationSectors) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  If the Mcb is not yet loaded, load it.  We do an optimistic check first.
    //

    if (!FlagOn(Fcb->FcbState, FCB_STATE_MCB_LOADED)) {

        ExAcquireFastMutex( &Fcb->McbMutex );

        if (!FlagOn(Fcb->FcbState, FCB_STATE_MCB_LOADED)) {

            ULONG CurrentSector;
            ULONG Context;
            ULONG VolumeSector;
            ULONG SectorCount;

            NW_STATUS NwStatus;

            if (NwMcbDebug) {
                DbgPrint("Nwfsro: Enumerating Runs on File Hash = 0x%08lx).\n", Fcb->Hash);
            }

            Context = 0;
            CurrentSector = 0;

            while (CurrentSector < AllocationSectors) {

                NwStatus = TrgLookupFileAllocation( Fcb->Vcb->VolumeNumber,
                                                    Fcb->Hash,
                                                    &CurrentSector,
                                                    &Context,
                                                    &VolumeSector,
                                                    &SectorCount );

                //
                //  If we get an unexpected error, break here.
                //

                if ((NwStatus != NwSuccess) && (NwStatus != NwEndOfFile)) {
                    if (NwMcbDebug) {
                        DbgPrint("  TrgLookup returned error NwStatus = 0x%x.\n\n", NwStatus);
                    }
                    Status = NwNtStatusFromNwStatus(NwStatus);
                    break;
                }

                //
                //  If we get a premature end of file, we are going to treat this
                //  as a hole at the end of the file.
                //

                if (NwStatus == NwEndOfFile) {
                    if (NwMcbDebug) {
                        DbgPrint("  Found Premature end in allocation.\n");
                    }
                    break;
                }

                //
                //  Otherwise, let's add this run to the Mcb if it wasn't a hole.
                //

                if (NwMcbDebug) {
                    DbgPrint("  AddMcb: File = 0x%08lx, Volume = 0x%08lx, Count = 0x%08lx.\n",
                             CurrentSector, VolumeSector, SectorCount);
                }

                if (VolumeSector && !FsRtlAddMcbEntry( &Fcb->Mcb,
                                                       CurrentSector,
                                                       VolumeSector,
                                                       SectorCount )) {

                    if (NwMcbDebug) {
                        DbgPrint("  AddMcb failed!!.\n");
                    }
                    Status = STATUS_FILE_CORRUPT_ERROR;
                    break;
                }

                //
                //  If this wasn't a hole, bump our valid data sectors.
                //

                if (VolumeSector) {
                    Fcb->ValidDataSectors = CurrentSector + SectorCount;
                }

                CurrentSector += SectorCount;
            }


            //
            //  Remember the highest run for which we found actual allocation.
            //

            if (NT_SUCCESS(Status)) {

                SetFlag(Fcb->FcbState, FCB_STATE_MCB_LOADED);

            } else {

                FsRtlTruncateMcb( &Fcb->Mcb, 0 );

                Fcb->ValidDataSectors = 0;
            }
        }

        ExReleaseFastMutex( &Fcb->McbMutex );
    }


    //
    //  If we got an error trying to enumerate the runs, fail here.
    //

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    //  If this is a request bwtween Fcb->ValidDataSectors and
    //  Fcb->AllocationSize (i.e. the FileSize is bogus), just treat
    //  this as a file with a hole on the end.
    //

    if (StartingVbn >= Fcb->ValidDataSectors) {

        *TargetDevice = NULL;
        *Lbn = HOLE_LBN;
        *SectorCount = AllocationSectors - StartingVbn;

        return STATUS_SUCCESS;
    }

    //
    //  Now finally, get the run from the Mcb.
    //

    if (!FsRtlLookupMcbEntry( &Fcb->Mcb,
                              StartingVbn,
                              Lbn,
                              &RunSectorCount,
                              &Index )) {

        if (NwMcbDebug) {
            DbgPrint("Nwfsro: Mcb lookup failed!.  Mcb = 0x%08lx. Start = 0x%08lx.\n",
                     &Fcb->Mcb, StartingVbn);
        }
        return STATUS_FILE_CORRUPT_ERROR;
    }

    if (*Lbn == HOLE_LBN) {
        *TargetDevice = NULL;
        *SectorCount = RunSectorCount;

        return STATUS_SUCCESS;
    }

    NwStatus = TrgGetPartitionInfoFromVolumeInfo( Fcb->Vcb->VolumeNumber,
                                                  *Lbn,
                                                  RunSectorCount,
                                                  &FileObject,
                                                  Lbn,
                                                  &PartitionSectorCount );

    if (NwStatus != NwSuccess) {

        if (NwMcbDebug) {
            DbgPrint("Nwfsro: TrgGetPartInfo failed!.  Volume = 0x%x, VolumeSector = 0x%08lx.\n",
                     Fcb->Vcb->VolumeNumber, Lbn);
        }
        return NwNtStatusFromNwStatus(NwStatus);
    }

    //
    //  If we hit a hotfix, we may return less than was in the volume extent.
    //

    if (RunSectorCount > PartitionSectorCount) {
        RunSectorCount = PartitionSectorCount;
    }

    //
    //  Ok it worked, return it to the user.
    //

    *TargetDevice = IoGetAttachedDevice(FileObject->DeviceObject);
    *SectorCount = RunSectorCount;

    return STATUS_SUCCESS;
}


NTSTATUS
NwNtStatusFromNwStatus(
    IN NW_STATUS NwStatus
    )

/*++

Routine Description:

    This routine translate an error recieved from the generic Netware modules
    into an NTSTATUS for use in NT.

Arguments:

    NW_STATUS - The error code to translate.

Return Value:

    NTSTATUS - The converted error code.  STATUS_UNSUCCESSFUL is the default
        should we receive a code we don't know about.

--*/
{
    switch (NwStatus) {

    case NwSuccess:
        return STATUS_SUCCESS;

    case NwFileCorrupt:
        return STATUS_FILE_CORRUPT_ERROR;

    case NwVolumeCorrupt:
        return STATUS_DISK_CORRUPT_ERROR;

    case NwDiskIoError:
        return STATUS_IO_DEVICE_ERROR;

    case NwInsufficientResources:
        return STATUS_INSUFFICIENT_RESOURCES;

    case NwInvalidParameter:
        return STATUS_INVALID_PARAMETER;

    case NwNoMoreMirrors:
        return STATUS_DISK_OPERATION_FAILED;

    case NwOtherError:
        return STATUS_UNSUCCESSFUL;

    case NwEndOfFile:
        return STATUS_END_OF_FILE;

    default:
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NwPerformDevIoCtrl (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT Device,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN BOOLEAN InternalDeviceIoControl,
    IN BOOLEAN OverrideVerify,
    OUT PIO_STATUS_BLOCK Iosb OPTIONAL
    )

/*++

Routine Description:

    This routine is called to perform DevIoCtrl functions internally within
    the filesystem.  We take the status from the driver and return it to our
    caller.

Arguments:

    IoControlCode - Code to send to driver.

    Device - This is the device to send the request to.

    OutPutBuffer - Pointer to output buffer.

    OutputBufferLength - Length of output buffer above.

    InternalDeviceIoControl - Indicates if this is an internal or external
        Io control code.

    OverrideVerify - Indicates if we should tell the driver not to return
        STATUS_VERIFY_REQUIRED for mount and verify.

    Iosb - If specified, we return the results of the operation here.

Return Value:

    NTSTATUS - Status returned by next lower driver.

--*/

{
    NTSTATUS Status;
    PIRP Irp;
    KEVENT Event;
    IO_STATUS_BLOCK LocalIosb;
    PIO_STATUS_BLOCK IosbToUse = &LocalIosb;

    PAGED_CODE();

    //
    //  Check if the user gave us an Iosb.
    //

    if (ARGUMENT_PRESENT( Iosb )) {

        IosbToUse = Iosb;
    }

    IosbToUse->Status = 0;
    IosbToUse->Information = 0;

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    Irp = IoBuildDeviceIoControlRequest( IoControlCode,
                                         Device,
                                         NULL,
                                         0,
                                         OutputBuffer,
                                         OutputBufferLength,
                                         InternalDeviceIoControl,
                                         &Event,
                                         IosbToUse );

    if (Irp == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (OverrideVerify) {

        SetFlag( IoGetNextIrpStackLocation( Irp )->Flags, SL_OVERRIDE_VERIFY_VOLUME );
    }

    Status = IoCallDriver( Device, Irp );

    //
    //  We check for device not ready by first checking Status
    //  and then if status pending was returned, the Iosb status
    //  value.
    //

    if (Status == STATUS_PENDING) {

        (VOID) KeWaitForSingleObject( &Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER)NULL );

        Status = IosbToUse->Status;
    }

    return Status;
}

NTSTATUS
NwToggleMediaEjectDisable (
    IN PDEVICE_OBJECT Partition,
    IN BOOLEAN PreventRemoval
    )

/*++

Routine Description:

    The routine either enables or disables the eject button on removable
    media.  Any error conditions are ignored.

Arguments:

    Partition - Descibes the physical partition to operate on.

    PreventRemoval - TRUE if we should disable the media eject button.  FALSE
        if we want to enable it.

Return Value:

    NTSTATUS - The result of the operation.

--*/

{
    PIRP Irp;
    KEVENT Event;
    KIRQL SavedIrql;
    NTSTATUS Status;
    IO_STATUS_BLOCK Iosb;
    PREVENT_MEDIA_REMOVAL Prevent;

    RtlZeroMemory( &Prevent, sizeof(PREVENT_MEDIA_REMOVAL) );

    Prevent.PreventMediaRemoval = PreventRemoval;

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    //
    //  Build the Irp.  If it fails, just return.
    //

    Irp = IoBuildDeviceIoControlRequest( IOCTL_DISK_MEDIA_REMOVAL,
                                         Partition,
                                         &Prevent,
                                         sizeof(PREVENT_MEDIA_REMOVAL),
                                         NULL,
                                         0,
                                         FALSE,
                                         &Event,
                                         &Iosb );

    if (Irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    SetFlag( IoGetNextIrpStackLocation(Irp)->Flags, SL_OVERRIDE_VERIFY_VOLUME );

    Status = IoCallDriver( Partition, Irp );

    if ( Status == STATUS_PENDING ) {

        (VOID) KeWaitForSingleObject( &Event,
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      (PLARGE_INTEGER)NULL );

        Status = Iosb.Status;
    }

    return Status;
}

NTSTATUS
NwResyncCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    //
    //  Set the event so that our call will wake up.
    //

    KeSetEvent( (PKEVENT)Context, 0, FALSE );

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    return STATUS_MORE_PROCESSING_REQUIRED;
}
