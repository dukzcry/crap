--- block.c	Sat Oct  9 00:17:54 2004
+++ block.c	Tue Jul 27 19:33:21 2010
@@ -487,7 +487,8 @@ FFSDiskIoControl(
 	ULONG           OutBufferSize = 0;
 	KEVENT          Event;
 	PIRP            Irp;
-	IO_STATUS_BLOCK IoStatus;
+	/* IO_STATUS_BLOCK IoStatus; */
+	IO_STATUS_BLOCK32 IoStatus; /* x64 fix */
 	NTSTATUS        Status;
 
 	ASSERT(DeviceObject != NULL);
@@ -559,7 +560,8 @@ FFSMediaEjectControl(
 	KEVENT                  Event;
 	NTSTATUS                Status;
 	PREVENT_MEDIA_REMOVAL   Prevent;
-	IO_STATUS_BLOCK         IoStatus;
+	/* IO_STATUS_BLOCK //x64 fix */        
+	IO_STATUS_BLOCK32 IoStatus;
 
 
 	ExAcquireResourceExclusiveLite(
