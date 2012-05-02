--- /boot/home/temp/tmp/intel_extreme/src/includes/intel_extreme/intel_extreme.h.orig	2012-05-01 21:09:36.300154880 +0400
+++ /boot/home/temp/tmp/intel_extreme/src/includes/intel_extreme/intel_extreme.h	2012-05-02 14:53:35.177995776 +0400
@@ -152,7 +152,7 @@ struct intel_shared_info {
 	area_id			mode_list_area;		// area containing display mode list
 	uint32			mode_count;
 
-	display_mode	current_mode;
+	display_mode		current_mode; // not really used by us
 	uint32			bytes_per_row;
 	uint32			bits_per_pixel;
 	uint32			dpms_mode;
@@ -168,6 +168,9 @@ struct intel_shared_info {
 	addr_t			frame_buffer;
 	uint32			frame_buffer_offset;
 
+	bool			got_vbt;
+	display_mode		vbt_mode;
+
 	struct lock		accelerant_lock;
 	struct lock		engine_lock;
 
