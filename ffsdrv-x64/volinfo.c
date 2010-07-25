/* 
 * FFS File System Driver for Windows
 *
 * volinfo.c
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
#pragma alloc_text(PAGE, FFSQueryVolumeInformation)
#pragma alloc_text(PAGE, FFSSetVolumeInformation)
#endif


NTSTATUS
FFSQueryVolumeInformation(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	PDEVICE_OBJECT          DeviceObject;
	NTSTATUS                Status = STATUS_UNSUCCESSFUL;
	PFFS_VCB                Vcb;
	PIRP                    Irp;
	PIO_STACK_LOCATION      IoStackLocation;
	FS_INFORMATION_CLASS    FsInformationClass;
	ULONG                   Length;
	PVOID                   Buffer;
	BOOLEAN                 VcbResourceAcquired = FALSE;

	__try
	{
		ASSERT(IrpContext != NULL);

		ASSERT((IrpContext->Identifier.Type == FFSICX) &&
				(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

		DeviceObject = IrpContext->DeviceObject;

		//
		// This request is not allowed on the main device object
		//
		if (DeviceObject == FFSGlobal->DeviceObject)
		{
			Status = STATUS_INVALID_DEVICE_REQUEST;
			__leave;
		}

		Vcb = (PFFS_VCB)DeviceObject->DeviceExtension;

		ASSERT(Vcb != NULL);

		ASSERT((Vcb->Identifier.Type == FFSVCB) &&
				(Vcb->Identifier.Size == sizeof(FFS_VCB)));

		ASSERT(IsMounted(Vcb));

		if (!ExAcquireResourceSharedLite(
					&Vcb->MainResource,
					IrpContext->IsSynchronous))
		{
			Status = STATUS_PENDING;
			__leave;
		}

		VcbResourceAcquired = TRUE;

		Irp = IrpContext->Irp;

		IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

		FsInformationClass =
			IoStackLocation->Parameters.QueryVolume.FsInformationClass;

		Length = IoStackLocation->Parameters.QueryVolume.Length;

		Buffer = Irp->AssociatedIrp.SystemBuffer;

		RtlZeroMemory(Buffer, Length);

		switch (FsInformationClass)
		{
			case FileFsVolumeInformation:
				{
					PFILE_FS_VOLUME_INFORMATION FsVolInfo;
					ULONG                       VolumeLabelLength;
					ULONG                       RequiredLength;

					if (Length < sizeof(FILE_FS_VOLUME_INFORMATION))
					{
						Status = STATUS_INFO_LENGTH_MISMATCH;
						__leave;
					}

					FsVolInfo = (PFILE_FS_VOLUME_INFORMATION)Buffer;

					FsVolInfo->VolumeCreationTime.QuadPart = 0;

					FsVolInfo->VolumeSerialNumber = Vcb->Vpb->SerialNumber;

					VolumeLabelLength = Vcb->Vpb->VolumeLabelLength;

					FsVolInfo->VolumeLabelLength = VolumeLabelLength;

					// I don't know what this means
					FsVolInfo->SupportsObjects = FALSE;

					RequiredLength = sizeof(FILE_FS_VOLUME_INFORMATION)
						+ VolumeLabelLength - sizeof(WCHAR);

					if (Length < RequiredLength)
					{
						Irp->IoStatus.Information =
							sizeof(FILE_FS_VOLUME_INFORMATION);
						Status = STATUS_BUFFER_OVERFLOW;
						__leave;
					}

					RtlCopyMemory(FsVolInfo->VolumeLabel, Vcb->Vpb->VolumeLabel, Vcb->Vpb->VolumeLabelLength);

					Irp->IoStatus.Information = RequiredLength;
					Status = STATUS_SUCCESS;
					__leave;
				}

			case FileFsSizeInformation:
				{
					PFILE_FS_SIZE_INFORMATION FsSizeInfo;

					if (Length < sizeof(FILE_FS_SIZE_INFORMATION))
					{
						Status = STATUS_INFO_LENGTH_MISMATCH;
						__leave;
					}

					FsSizeInfo = (PFILE_FS_SIZE_INFORMATION)Buffer;

					{
						if (FS_VERSION == 1)
						{
							FsSizeInfo->TotalAllocationUnits.QuadPart =
								(Vcb->ffs_super_block->fs_old_size / 8);

							FsSizeInfo->AvailableAllocationUnits.QuadPart =
								(Vcb->ffs_super_block->fs_old_cstotal.cs_nbfree / 8);
						}
						else
						{
							FsSizeInfo->TotalAllocationUnits.QuadPart =
								(Vcb->ffs_super_block->fs_size / 8);

							FsSizeInfo->AvailableAllocationUnits.QuadPart =
								(Vcb->ffs_super_block->fs_cstotal.cs_nbfree / 8);
						}
					}

					FsSizeInfo->SectorsPerAllocationUnit =
						Vcb->BlockSize / Vcb->DiskGeometry.BytesPerSector;

					FsSizeInfo->BytesPerSector =
						Vcb->DiskGeometry.BytesPerSector;

					Irp->IoStatus.Information = sizeof(FILE_FS_SIZE_INFORMATION);
					Status = STATUS_SUCCESS;
					__leave;
				}

			case FileFsDeviceInformation:
				{
					PFILE_FS_DEVICE_INFORMATION FsDevInfo;

					if (Length < sizeof(FILE_FS_DEVICE_INFORMATION))
					{
						Status = STATUS_INFO_LENGTH_MISMATCH;
						__leave;
					}

					FsDevInfo = (PFILE_FS_DEVICE_INFORMATION)Buffer;

					FsDevInfo->DeviceType =
						Vcb->TargetDeviceObject->DeviceType;

					FsDevInfo->Characteristics =
						Vcb->TargetDeviceObject->Characteristics;

					if (FlagOn(Vcb->Flags, VCB_READ_ONLY))
					{
						SetFlag(FsDevInfo->Characteristics,
								FILE_READ_ONLY_DEVICE);
					}

					Irp->IoStatus.Information = sizeof(FILE_FS_DEVICE_INFORMATION);
					Status = STATUS_SUCCESS;
					__leave;
				}

			case FileFsAttributeInformation:
				{
					PFILE_FS_ATTRIBUTE_INFORMATION  FsAttrInfo;
					ULONG                           RequiredLength;

					if (Length < sizeof(FILE_FS_ATTRIBUTE_INFORMATION))
					{
						Status = STATUS_INFO_LENGTH_MISMATCH;
						__leave;
					}

					FsAttrInfo =
						(PFILE_FS_ATTRIBUTE_INFORMATION)Buffer;

					FsAttrInfo->FileSystemAttributes =
						FILE_CASE_SENSITIVE_SEARCH | FILE_CASE_PRESERVED_NAMES;

					FsAttrInfo->MaximumComponentNameLength = FFS_NAME_LEN;

					FsAttrInfo->FileSystemNameLength = 10;

					RequiredLength = sizeof(FILE_FS_ATTRIBUTE_INFORMATION) +
						10 - sizeof(WCHAR);

					if (Length < RequiredLength)
					{
						Irp->IoStatus.Information =
							sizeof(FILE_FS_ATTRIBUTE_INFORMATION);
						Status = STATUS_BUFFER_OVERFLOW;
						__leave;
					}

					RtlCopyMemory(
							FsAttrInfo->FileSystemName,
							L"FFS\0", 8);

					Irp->IoStatus.Information = RequiredLength;
					Status = STATUS_SUCCESS;
					__leave;
				}

#if (_WIN32_WINNT >= 0x0500)

			case FileFsFullSizeInformation:
				{
					PFILE_FS_FULL_SIZE_INFORMATION PFFFSI;

					if (Length < sizeof(FILE_FS_FULL_SIZE_INFORMATION))
					{
						Status = STATUS_INFO_LENGTH_MISMATCH;
						__leave;
					}

					PFFFSI = (PFILE_FS_FULL_SIZE_INFORMATION)Buffer;

					/*
					typedef struct _FILE_FS_FULL_SIZE_INFORMATION {
						LARGE_INTEGER   TotalAllocationUnits;
						LARGE_INTEGER   CallerAvailableAllocationUnits;
						LARGE_INTEGER   ActualAvailableAllocationUnits;
						ULONG           SectorsPerAllocationUnit;
						ULONG           BytesPerSector;
					} FILE_FS_FULL_SIZE_INFORMATION, *PFILE_FS_FULL_SIZE_INFORMATION;
					*/

					{
						if (FS_VERSION == 1)
						{
							PFFFSI->TotalAllocationUnits.QuadPart =
								(Vcb->ffs_super_block->fs_old_size / 8);

							PFFFSI->CallerAvailableAllocationUnits.QuadPart =
								(Vcb->ffs_super_block->fs_old_cstotal.cs_nbfree / 8);

							PFFFSI->ActualAvailableAllocationUnits.QuadPart =
								(Vcb->ffs_super_block->fs_old_cstotal.cs_nbfree / 8);
						}
						else
						{
							PFFFSI->TotalAllocationUnits.QuadPart =
								(Vcb->ffs_super_block->fs_size / 8);

							PFFFSI->CallerAvailableAllocationUnits.QuadPart =
								(Vcb->ffs_super_block->fs_cstotal.cs_nbfree / 8);

							PFFFSI->ActualAvailableAllocationUnits.QuadPart =
								(Vcb->ffs_super_block->fs_cstotal.cs_nbfree / 8);
						}
					}

					PFFFSI->SectorsPerAllocationUnit =
						Vcb->BlockSize / Vcb->DiskGeometry.BytesPerSector;

					PFFFSI->BytesPerSector = Vcb->DiskGeometry.BytesPerSector;

					Irp->IoStatus.Information = sizeof(FILE_FS_FULL_SIZE_INFORMATION);
					Status = STATUS_SUCCESS;
					__leave;
				}

#endif // (_WIN32_WINNT >= 0x0500)

			default:
				Status = STATUS_INVALID_INFO_CLASS;
		}
	}

	__finally
	{
		if (VcbResourceAcquired)
		{
			ExReleaseResourceForThreadLite(
					&Vcb->MainResource,
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


NTSTATUS
FFSSetVolumeInformation(
	IN PFFS_IRP_CONTEXT IrpContext)
{
	PDEVICE_OBJECT          DeviceObject;
	NTSTATUS                Status = STATUS_UNSUCCESSFUL;
	PFFS_VCB                Vcb;
	PIRP                    Irp;
	PIO_STACK_LOCATION      IoStackLocation;
	FS_INFORMATION_CLASS    FsInformationClass;

	__try
	{
		ASSERT(IrpContext != NULL);

		ASSERT((IrpContext->Identifier.Type == FFSICX) &&
				(IrpContext->Identifier.Size == sizeof(FFS_IRP_CONTEXT)));

		DeviceObject = IrpContext->DeviceObject;

		//
		// This request is not allowed on the main device object
		//
		if (DeviceObject == FFSGlobal->DeviceObject)
		{
			Status = STATUS_INVALID_DEVICE_REQUEST;
			__leave;
		}

		Vcb = (PFFS_VCB)DeviceObject->DeviceExtension;

		ASSERT(Vcb != NULL);

		ASSERT((Vcb->Identifier.Type == FFSVCB) &&
				(Vcb->Identifier.Size == sizeof(FFS_VCB)));

		ASSERT(IsMounted(Vcb));

		if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY))
		{
			Status = STATUS_MEDIA_WRITE_PROTECTED;
			__leave;
		}

		Irp = IrpContext->Irp;

		IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

		//Notes: SetVolume is not defined in ntddk.h of win2k ddk,
		//       But it's same to QueryVolume ....
		FsInformationClass =
			IoStackLocation->Parameters./*SetVolume*/QueryVolume.FsInformationClass;

		switch (FsInformationClass)
		{
			case FileFsLabelInformation:
				{
					PFILE_FS_LABEL_INFORMATION      VolLabelInfo = NULL;
					ULONG                           VolLabelLen;
					UNICODE_STRING                  LabelName  ;

					OEM_STRING                      OemName;

					VolLabelInfo = (PFILE_FS_LABEL_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

					VolLabelLen = VolLabelInfo->VolumeLabelLength;

					if(VolLabelLen > (16 * sizeof(WCHAR)))
					{
						Status = STATUS_INVALID_VOLUME_LABEL;
						__leave;
					}

					RtlCopyMemory(Vcb->Vpb->VolumeLabel,
							VolLabelInfo->VolumeLabel,
							VolLabelLen);

					RtlZeroMemory(Vcb->ffs_super_block->fs_volname, 16);

					LabelName.Buffer = VolLabelInfo->VolumeLabel;
					LabelName.MaximumLength = (USHORT)16 * sizeof(WCHAR);
					LabelName.Length = (USHORT)VolLabelLen;

					OemName.Buffer = SUPER_BLOCK->fs_volname;
					OemName.Length  = 0;
					OemName.MaximumLength = 16;

					FFSUnicodeToOEM(&OemName,
							&LabelName);

					Vcb->Vpb->VolumeLabelLength = 
						(USHORT)VolLabelLen;

					if (FFSSaveSuper(IrpContext, Vcb))
					{
						Status = STATUS_SUCCESS;
					}

					Irp->IoStatus.Information = 0;

				}
				break;

			default:
				Status = STATUS_INVALID_INFO_CLASS;
		}
	}

	__finally
	{

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
