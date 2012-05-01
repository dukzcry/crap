--- /boot/home/temp/tmp/intel_extreme/src/includes/intel_extreme/intel_extreme.h.orig	2012-05-01 21:09:36.300154880 +0400
+++ /boot/home/temp/tmp/intel_extreme/src/includes/intel_extreme/intel_extreme.h	2012-05-01 21:09:18.546308096 +0400
@@ -168,6 +168,8 @@ struct intel_shared_info {
 	addr_t			frame_buffer;
 	uint32			frame_buffer_offset;
 
+	bool			got_vbt;
+
 	struct lock		accelerant_lock;
 	struct lock		engine_lock;
 
