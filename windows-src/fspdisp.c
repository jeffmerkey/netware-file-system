
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

Module Name:

    FspDisp.c

Abstract:

    This module implements the main dispatch procedure/thread for the Nwfs
    Fsp

  Authors:

    Jeff Merkey (jmerkey@timpanogas.com)
    David Goebel (davidg@balder.com) 11-Mar-1999

***************************************************************************/

#include "NwProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NWFS_BUG_CHECK_FSPDISP)


VOID
NwFspDispatch (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This is the main FSP thread routine that is executed to receive
    and dispatch IRP requests.  Each FSP thread begins its execution here.
    There is one thread created at system initialization time and subsequent
    threads created as needed.

Arguments:

    IrpContext - IrpContext for a request to process.

Return Value:

    None

--*/

{
    THREAD_CONTEXT ThreadContext;
    NTSTATUS Status;

    PIRP Irp = IrpContext->Irp;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );

    PVOLUME_DEVICE_OBJECT VolDo = NULL;

    //
    //  If this request has an associated volume device object, remember it.
    //

    if (IrpSp->FileObject != NULL) {

        VolDo = CONTAINING_RECORD( IrpSp->DeviceObject,
                                   VOLUME_DEVICE_OBJECT,
                                   DeviceObject );
    }

    //
    //  Now case on the function code.  For each major function code,
    //  either call the appropriate worker routine.  This routine that
    //  we call is responsible for completing the IRP, and not us.
    //  That way the routine can complete the IRP and then continue
    //  post processing as required.  For example, a read can be
    //  satisfied right away and then read can be done.
    //
    //  We'll do all of the work within an exception handler that
    //  will be invoked if ever some underlying operation gets into
    //  trouble.
    //

    while ( TRUE ) {

        //
        //  Set all the flags indicating we are in the Fsp.
        //

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FSP_FLAGS );

        FsRtlEnterFileSystem();

        NwSetThreadContext( IrpContext, &ThreadContext );

        while (TRUE) {

            try {

                //
                //  Reinitialize for the next try at completing this
                //  request.
                //

                Status = IrpContext->ExceptionStatus = STATUS_SUCCESS;

                //
                //  Initialize the Io status field in the Irp.
                //

                Irp->IoStatus.Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = 0;

                //
                //  Case on the major irp code.
                //

                switch (IrpContext->MajorFunction) {

                case IRP_MJ_CREATE :

                    NwCommonCreate( IrpContext, Irp );
                    break;

                case IRP_MJ_CLOSE :

                    ASSERT( FALSE );
                    break;

                case IRP_MJ_READ :

                    NwCommonRead( IrpContext, Irp );
                    break;

                case IRP_MJ_QUERY_INFORMATION :

                    NwCommonQueryInfo( IrpContext, Irp );
                    break;

                case IRP_MJ_SET_INFORMATION :

                    NwCommonSetInfo( IrpContext, Irp );
                    break;

                case IRP_MJ_QUERY_VOLUME_INFORMATION :

                    NwCommonQueryVolInfo( IrpContext, Irp );
                    break;

                case IRP_MJ_DIRECTORY_CONTROL :

                    NwCommonDirControl( IrpContext, Irp );
                    break;

                case IRP_MJ_FILE_SYSTEM_CONTROL :

                    NwCommonFsControl( IrpContext, Irp );
                    break;

                case IRP_MJ_DEVICE_CONTROL :

                    NwCommonDevControl( IrpContext, Irp );
                    break;

                case IRP_MJ_LOCK_CONTROL :

                    NwCommonLockControl( IrpContext, Irp );
                    break;

                case IRP_MJ_CLEANUP :

                    NwCommonCleanup( IrpContext, Irp );
                    break;

                default :

                    Status = STATUS_INVALID_DEVICE_REQUEST;
                    NwCompleteRequest( IrpContext, Irp, Status );
                }

            } except( NwExceptionFilter( IrpContext, GetExceptionInformation() )) {

                Status = NwProcessException( IrpContext, Irp, GetExceptionCode() );
            }

            //
            //  Break out of the loop if we didn't get CANT_WAIT.
            //

            if (Status != STATUS_CANT_WAIT) { break; }

            //
            //  We are retrying this request.  Cleanup the IrpContext for the retry.
            //

            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_MORE_PROCESSING );
            NwCleanupIrpContext( IrpContext, FALSE );
        }

        FsRtlExitFileSystem();

        //
        //  If there are any entries on this volume's overflow queue, service
        //  them.
        //

        if (VolDo != NULL) {

            KIRQL SavedIrql;
            PVOID Entry = NULL;

            //
            //  We have a volume device object so see if there is any work
            //  left to do in its overflow queue.
            //

            NwLock();

            if (VolDo->OverflowQueueCount > 0) {

                //
                //  There is overflow work to do in this volume so we'll
                //  decrement the Overflow count, dequeue the IRP, and release
                //  the Event
                //

                VolDo->OverflowQueueCount -= 1;

                Entry = RemoveHeadList( &VolDo->OverflowQueue );
            }

            NwUnlock();

            //
            //  There wasn't an entry, break out of the loop and return to
            //  the Ex Worker thread.
            //

            if (Entry == NULL) { break; }

            //
            //  Extract the IrpContext , Irp, set wait to TRUE, and loop.
            //

            IrpContext = CONTAINING_RECORD( Entry,
                                            IRP_CONTEXT,
                                            WorkQueueItem.List );

            Irp = IrpContext->Irp;
            IrpSp = IoGetCurrentIrpStackLocation( Irp );

            continue;
        }

        break;
    }

    //
    //  Decrement the PostedRequestCount if there was a volume device object.
    //

    if (VolDo) {

        NwLock();
        VolDo->PostedRequestCount--;
        NwUnlock();
    }

    return;
}



