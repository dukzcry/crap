/* 
 * FFS File System Driver for Windows
 *
 * read.c
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
#pragma alloc_text(PAGE, FFSShutDown)
#endif


NTSTATUS
FFSShutDown(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	NTSTATUS                Status;

	PKEVENT                 Event;

	PIRP                    Irp;
	PIO_STACK_LOCATION      IrpSp;

	PFFS_VCB                Vcb;
	PLIST_ENTRY             ListEntry;

	BOOLEAN                 GlobalResourceAcquired = FALSE;

	__try
	{
		ASSERT(IrpContext);

		ASSERT((IrpContext->Identifier.Type == FFSICX) &&
				(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

		Status = STATUS_SUCCESS;

		Irp = IrpContext->Irp;

		IrpSp = IoGetCurrentIrpStackLocation(Irp);

		if (!ExAcquireResourceExclusiveLite(
					&FFSGlobal->Resource,
					IrpContext->IsSynchronous))
		{
			Status = STATUS_PENDING;
			__leave;
		}

		GlobalResourceAcquired = TRUE;

		Event = ExAllocatePool(NonPagedPool, sizeof(KEVENT));
		KeInitializeEvent(Event, NotificationEvent, FALSE);

		for (ListEntry = FFSGlobal->VcbList.Flink;
				ListEntry != &(FFSGlobal->VcbList);
				ListEntry = ListEntry->Flink)
		{
			Vcb = CONTAINING_RECORD(ListEntry, FFS_VCB, Next);

			if (ExAcquireResourceExclusiveLite(
						&Vcb->MainResource,
						TRUE))
			{

				Status = FFSFlushFiles(Vcb, TRUE);
				if(!NT_SUCCESS(Status))
				{
					FFSBreakPoint();
				}

				Status = FFSFlushVolume(Vcb, TRUE);

				if(!NT_SUCCESS(Status))
				{
					FFSBreakPoint();
				}

				FFSDiskShutDown(Vcb);

				ExReleaseResourceForThreadLite(
						&Vcb->MainResource,
						ExGetCurrentResourceThread());
			}
		}

		/*
		IoUnregisterFileSystem(FFSGlobal->DeviceObject);
		*/
	}

	__finally
	{
		if (GlobalResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
					&FFSGlobal->Resource,
					ExGetCurrentResourceThread());
		}

		if (!IrpContext->ExceptionInProgress)
		{
			if (Status == STATUS_PENDING)
			{
				FFSQueueRequest(IrpContext);
			}
			else
			{
				FFSCompleteIrpContext(IrpContext, Status);
			}
		}
	}

	return Status;
}
