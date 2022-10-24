/*++

Copyright (c) 1999-2000  Timpanogas Research Group, Inc.

Module Name:

    NwData.h

Abstract:

    This module declares the global data used by the Nwfs file system.

Author:

    David Goebel (davidg@balder.com) 11-Mar-1999

--*/

#ifndef _NWDATA_
#define _NWDATA_

//
//  Global data structures
//

extern NW_DATA NwData;
extern FAST_IO_DISPATCH NwFastIoDispatch;
extern FILE_ID FileIdZero;

extern PUCHAR NwOemUpcaseTable;

//
//  Global constants
//

//
//  This is the number of times a mounted Vcb will be referenced on behalf
//  of the system.  The count includes one for the DasdFcb and one for the
//  RootDirDcb.
//

#define NWFS_RESIDUAL_FCB_COUNT                     (2)

#define NT_DEVICE_NAME  L"\\NwFsRo"
#define DOS_DEVICE_NAME L"\\??\\NwFsRo"


//
//  Turn on pseudo-asserts if NW_FREE_ASSERTS is defined.
//

#if !DBG
#ifdef NW_FREE_ASSERTS
#undef ASSERT
#undef ASSERTMSG
#define ASSERT(exp)        if (!(exp)) { extern BOOLEAN KdDebuggerEnabled; DbgPrint("%s:%d %s\n",__FILE__,__LINE__,#exp); if (KdDebuggerEnabled) { DbgBreakPoint(); } }
#define ASSERTMSG(msg,exp) if (!(exp)) { extern BOOLEAN KdDebuggerEnabled; DbgPrint("%s:%d %s %s\n",__FILE__,__LINE__,msg,#exp); if (KdDebuggerEnabled) { DbgBreakPoint(); } }
#endif
#endif


//
//  The following assertion macros ensure that the indicated structure
//  is valid
//
//      ASSERT_STRUCT( IN PVOID Struct, IN CSHORT NodeType );
//      ASSERT_OPTIONAL_STRUCT( IN PVOID Struct OPTIONAL, IN CSHORT NodeType );
//
//      ASSERT_VCB( IN PVCB Vcb );
//      ASSERT_OPTIONAL_VCB( IN PVCB Vcb OPTIONAL );
//
//      ASSERT_FCB( IN PFCB Fcb );
//      ASSERT_OPTIONAL_FCB( IN PFCB Fcb OPTIONAL );
//
//      ASSERT_FCB_NONPAGED( IN PFCB_NONPAGED FcbNonpaged );
//      ASSERT_OPTIONAL_FCB( IN PFCB_NONPAGED FcbNonpaged OPTIONAL );
//
//      ASSERT_CCB( IN PSCB Ccb );
//      ASSERT_OPTIONAL_CCB( IN PSCB Ccb OPTIONAL );
//
//      ASSERT_IRP_CONTEXT( IN PIRP_CONTEXT IrpContext );
//      ASSERT_OPTIONAL_IRP_CONTEXT( IN PIRP_CONTEXT IrpContext OPTIONAL );
//
//      ASSERT_IRP( IN PIRP Irp );
//      ASSERT_OPTIONAL_IRP( IN PIRP Irp OPTIONAL );
//
//      ASSERT_FILE_OBJECT( IN PFILE_OBJECT FileObject );
//      ASSERT_OPTIONAL_FILE_OBJECT( IN PFILE_OBJECT FileObject OPTIONAL );
//
//  The following macros are used to check the current thread owns
//  the indicated resource
//
//      ASSERT_EXCLUSIVE_RESOURCE( IN PERESOURCE Resource );
//
//      ASSERT_SHARED_RESOURCE( IN PERESOURCE Resource );
//
//      ASSERT_RESOURCE_NOT_MINE( IN PERESOURCE Resource );
//
//  The following macros are used to check whether the current thread
//  owns the resoures in the given structures.
//
//      ASSERT_EXCLUSIVE_NWDATA
//
//      ASSERT_EXCLUSIVE_VCB( IN PVCB Vcb );
//
//      ASSERT_SHARED_VCB( IN PVCB Vcb );
//
//      ASSERT_EXCLUSIVE_FCB( IN PFCB Fcb );
//
//      ASSERT_SHARED_FCB( IN PFCB Fcb );
//
//      ASSERT_LOCKED_NW();
//
//      ASSERT_NOT_LOCKED_NW();
//

//
//  Turn on the sanity checks if this is DBG or NW_FREE_ASSERTS
//

#if DBG
#undef NW_SANITY
#define NW_SANITY
#endif

#ifdef NW_SANITY

#define ASSERT_STRUCT(S,T)                  ASSERT( SafeNodeType( S ) == (T) )
#define ASSERT_OPTIONAL_STRUCT(S,T)         ASSERT( ((S) == NULL) ||  (SafeNodeType( S ) == (T)) )

#define ASSERT_VCB(V)                       ASSERT_STRUCT( (V), NWFS_NTC_VCB )
#define ASSERT_OPTIONAL_VCB(V)              ASSERT_OPTIONAL_STRUCT( (V), NWFS_NTC_VCB )

#define ASSERT_FCB(F)                                \
    ASSERT( (SafeNodeType( F ) == NWFS_NTC_FCB ) ||  \
            (SafeNodeType( F ) == NWFS_NTC_DCB ) )

#define ASSERT_OPTIONAL_FCB(F)                       \
    ASSERT( ((F) == NULL) ||                         \
            (SafeNodeType( F ) == NWFS_NTC_FCB ) ||  \
            (SafeNodeType( F ) == NWFS_NTC_DCB ) )

#define ASSERT_FCB_NONPAGED(FN)             ASSERT_STRUCT( (FN), NWFS_NTC_FCB_NONPAGED )
#define ASSERT_OPTIONAL_FCB_NONPAGED(FN)    ASSERT_OPTIONAL_STRUCT( (FN), NWFS_NTC_FCB_NONPAGED )

#define ASSERT_CCB(C)                       ASSERT_STRUCT( (C), NWFS_NTC_CCB )
#define ASSERT_OPTIONAL_CCB(C)              ASSERT_OPTIONAL_STRUCT( (C), NWFS_NTC_CCB )

#define ASSERT_IRP_CONTEXT(IC)              ASSERT_STRUCT( (IC), NWFS_NTC_IRP_CONTEXT )
#define ASSERT_OPTIONAL_IRP_CONTEXT(IC)     ASSERT_OPTIONAL_STRUCT( (IC), NWFS_NTC_IRP_CONTEXT )

#define ASSERT_IRP(I)                       ASSERT_STRUCT( (I), IO_TYPE_IRP )
#define ASSERT_OPTIONAL_IRP(I)              ASSERT_OPTIONAL_STRUCT( (I), IO_TYPE_IRP )

#define ASSERT_FILE_OBJECT(FO)              ASSERT_STRUCT( (FO), IO_TYPE_FILE )
#define ASSERT_OPTIONAL_FILE_OBJECT(FO)     ASSERT_OPTIONAL_STRUCT( (FO), IO_TYPE_FILE )

#define ASSERT_EXCLUSIVE_RESOURCE(R)        ASSERT( ExIsResourceAcquiredExclusive( R ))

#define ASSERT_SHARED_RESOURCE(R)           ASSERT( ExIsResourceAcquiredShared( R ))

#define ASSERT_RESOURCE_NOT_MINE(R)         ASSERT( !ExIsResourceAcquiredShared( R ))

#define ASSERT_EXCLUSIVE_NWDATA             ASSERT( ExIsResourceAcquiredExclusive( &NwData.DataResource ))
#define ASSERT_EXCLUSIVE_VCB(V)             ASSERT( ExIsResourceAcquiredExclusive( &(V)->VcbResource ))
#define ASSERT_SHARED_VCB(V)                ASSERT( ExIsResourceAcquiredShared( &(V)->VcbResource ))

#define ASSERT_EXCLUSIVE_FCB(F)             ASSERT( ExIsResourceAcquiredExclusive( &(F)->FcbNonpaged->FcbResource ))
#define ASSERT_SHARED_FCB(F)                ASSERT( ExIsResourceAcquiredShared( &(F)->FcbNonpaged->FcbResource ))

#define ASSERT_LOCKED_NW()                  ASSERT( NwData.NwDataLockThread == PsGetCurrentThread() )
#define ASSERT_NOT_LOCKED_NW()              ASSERT( NwData.NwDataLockThread != PsGetCurrentThread() )

#else

#define ASSERT_STRUCT(S,T)              { NOTHING; }
#define ASSERT_OPTIONAL_STRUCT(S,T)     { NOTHING; }
#define ASSERT_VCB(V)                   { NOTHING; }
#define ASSERT_OPTIONAL_VCB(V)          { NOTHING; }
#define ASSERT_FCB(F)                   { NOTHING; }
#define ASSERT_OPTIONAL_FCB(F)          { NOTHING; }
#define ASSERT_FCB_NONPAGED(FN)         { NOTHING; }
#define ASSERT_OPTIONAL_FCB(FN)         { NOTHING; }
#define ASSERT_CCB(C)                   { NOTHING; }
#define ASSERT_OPTIONAL_CCB(C)          { NOTHING; }
#define ASSERT_IRP_CONTEXT(IC)          { NOTHING; }
#define ASSERT_OPTIONAL_IRP_CONTEXT(IC) { NOTHING; }
#define ASSERT_IRP(I)                   { NOTHING; }
#define ASSERT_OPTIONAL_IRP(I)          { NOTHING; }
#define ASSERT_FILE_OBJECT(FO)          { NOTHING; }
#define ASSERT_OPTIONAL_FILE_OBJECT(FO) { NOTHING; }
#define ASSERT_EXCLUSIVE_RESOURCE(R)    { NOTHING; }
#define ASSERT_SHARED_RESOURCE(R)       { NOTHING; }
#define ASSERT_RESOURCE_NOT_MINE(R)     { NOTHING; }
#define ASSERT_EXCLUSIVE_NWDATA         { NOTHING; }
#define ASSERT_EXCLUSIVE_VCB(V)         { NOTHING; }
#define ASSERT_SHARED_VCB(V)            { NOTHING; }
#define ASSERT_EXCLUSIVE_FCB(F)         { NOTHING; }
#define ASSERT_SHARED_FCB(F)            { NOTHING; }
#define ASSERT_LOCKED_NW()              { NOTHING; }
#define ASSERT_NOT_LOCKED_NW()          { NOTHING; }

#endif

#endif // _NWDATA_
