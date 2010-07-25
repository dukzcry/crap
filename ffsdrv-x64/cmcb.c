/* 
 * FFS File System Driver for Windows
 *
 * cmcb.c
 *
 * 2004.5.6 ~
 *
 * Lee Jae-Hong, http://www.pyrasis.com
 *
 * See License.txt
 *
 */

#include "ntifs.h"
#include "ffsdrv.h"

/* Globals */

extern PFFS_GLOBAL FFSGlobal;

/* Definitions */

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FFSAcquireForLazyWrite)
#pragma alloc_text(PAGE, FFSReleaseFromLazyWrite)
#pragma alloc_text(PAGE, FFSAcquireForReadAhead)
#pragma alloc_text(PAGE, FFSReleaseFromReadAhead)
#pragma alloc_text(PAGE, FFSNoOpRelease)
#pragma alloc_text(PAGE, FFSNoOpRelease)
#endif


BOOLEAN
FFSAcquireForLazyWrite(
	IN PVOID    Context,
	IN BOOLEAN  Wait)
{
	//
	// On a readonly filesystem this function still has to exist but it
	// doesn't need to do anything.

	PFFS_FCB     Fcb;

	Fcb = (PFFS_FCB)Context;

	ASSERT(Fcb != NULL);

	ASSERT((Fcb->Identifier.Type == FFSFCB) &&
			(Fcb->Identifier.Size == sizeof(FFS_FCB)));

	FFSPrint((DBG_INFO, "FFSAcquireForLazyWrite: %s %s %s\n",
				FFSGetCurrentProcessName(),
				"ACQUIRE_FOR_LAZY_WRITE",
				Fcb->AnsiFileName.Buffer));


	if (!IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY))
	{
		FFSPrint((DBG_INFO, "FFSAcquireForLazyWrite: Inode=%xh %S\n", 
					Fcb->FFSMcb->Inode, Fcb->FFSMcb->ShortName.Buffer));

		if(!ExAcquireResourceSharedLite(
					&Fcb->PagingIoResource, Wait))
		{
			return FALSE;
		}
	}

	ASSERT(IoGetTopLevelIrp() == NULL);

	IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

	return TRUE;
}


VOID
FFSReleaseFromLazyWrite(
	IN PVOID Context)
{
	//
	// On a readonly filesystem this function still has to exist but it
	// doesn't need to do anything.
	PFFS_FCB Fcb;

	Fcb = (PFFS_FCB)Context;

	ASSERT(Fcb != NULL);

	ASSERT((Fcb->Identifier.Type == FFSFCB) &&
			(Fcb->Identifier.Size == sizeof(FFS_FCB)));

	FFSPrint((DBG_INFO, "FFSReleaseFromLazyWrite: %s %s %s\n",
				FFSGetCurrentProcessName(),
				"RELEASE_FROM_LAZY_WRITE",
				Fcb->AnsiFileName.Buffer));

	if (!IsFlagOn(Fcb->Vcb->Flags, VCB_READ_ONLY))
	{
		FFSPrint((DBG_INFO, "FFSReleaseFromLazyWrite: Inode=%xh %S\n", 
					Fcb->FFSMcb->Inode, Fcb->FFSMcb->ShortName.Buffer));

		ExReleaseResource(&Fcb->PagingIoResource);
	}

	ASSERT(IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

	IoSetTopLevelIrp(NULL);

}


BOOLEAN
FFSAcquireForReadAhead(
	IN PVOID    Context,
	IN BOOLEAN  Wait)
{
	PFFS_FCB    Fcb;

	Fcb = (PFFS_FCB)Context;

	ASSERT(Fcb != NULL);

	ASSERT((Fcb->Identifier.Type == FFSFCB) &&
			(Fcb->Identifier.Size == sizeof(FFS_FCB)));

	FFSPrint((DBG_INFO, "FFSAcquireForReadAhead: Inode=%xh %S\n", 
				Fcb->FFSMcb->Inode, Fcb->FFSMcb->ShortName.Buffer));

	if (!ExAcquireResourceShared(
				&Fcb->MainResource, Wait))
		return FALSE;

	ASSERT(IoGetTopLevelIrp() == NULL);

	IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

	return TRUE;
}


VOID
FFSReleaseFromReadAhead(
	IN PVOID Context)
{
	PFFS_FCB Fcb;

	Fcb = (PFFS_FCB)Context;

	ASSERT(Fcb != NULL);

	ASSERT((Fcb->Identifier.Type == FFSFCB) &&
			(Fcb->Identifier.Size == sizeof(FFS_FCB)));

	FFSPrint((DBG_INFO, "FFSReleaseFromReadAhead: Inode=%xh %S\n", 
				Fcb->FFSMcb->Inode, Fcb->FFSMcb->ShortName.Buffer));

	IoSetTopLevelIrp(NULL);

	ExReleaseResource(&Fcb->MainResource);
}


BOOLEAN
FFSNoOpAcquire(
	IN PVOID Fcb,
	IN BOOLEAN Wait)
{
	UNREFERENCED_PARAMETER(Fcb);
	UNREFERENCED_PARAMETER(Wait);

	//
	//  This is a kludge because Cc is really the top level.  We it
	//  enters the file system, we will think it is a resursive call
	//  and complete the request with hard errors or verify.  It will
	//  have to deal with them, somehow....
	//

	ASSERT(IoGetTopLevelIrp() == NULL);

	IoSetTopLevelIrp((PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

	return TRUE;
}


VOID
FFSNoOpRelease(
	IN PVOID Fcb)
{
	//
	//  Clear the kludge at this point.
	//

	ASSERT(IoGetTopLevelIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);

	IoSetTopLevelIrp(NULL);

	UNREFERENCED_PARAMETER(Fcb);

	return;
}
