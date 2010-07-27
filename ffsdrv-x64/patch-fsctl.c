--- fsctl.c	Wed Nov  3 03:42:56 2004
+++ fsctl.c	Tue Jul 27 19:33:21 2010
@@ -87,7 +87,8 @@ FFSGetPartition(
 {
     CHAR                  Buffer[2048];
     PIRP                  Irp;
-    IO_STATUS_BLOCK       IoStatus;
+    /* IO_STATUS_BLOCK //x64 fix */       
+	IO_STATUS_BLOCK32 IoStatus;
     KEVENT                Event;
     NTSTATUS              Status;
     PARTITION_INFORMATION *PartInfo;
@@ -753,7 +754,8 @@ FFSIsMediaWriteProtected(
 	PIRP            Irp;
 	KEVENT          Event;
 	NTSTATUS        Status;
-	IO_STATUS_BLOCK IoStatus;
+	/* IO_STATUS_BLOCK //x64 fix */ 
+	IO_STATUS_BLOCK32 IoStatus;
 
 	KeInitializeEvent(&Event, NotificationEvent, FALSE);
 
