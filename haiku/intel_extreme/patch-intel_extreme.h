--- intel_extreme.h.orig	2012-05-11 10:57:01.052690944 +0400
+++ intel_extreme.h	2012-05-11 10:28:07.625475584 +0400
@@ -152,7 +152,8 @@ struct intel_shared_info {
 	area_id			mode_list_area;		// area containing display mode list
 	uint32			mode_count;
 
-	display_mode	current_mode;
+	display_mode		current_mode; // it's used to store current mode, but we can
+					      // use it to pass our vbt info to accelerant
 	uint32			bytes_per_row;
 	uint32			bits_per_pixel;
 	uint32			dpms_mode;
@@ -168,6 +169,9 @@ struct intel_shared_info {
 	addr_t			frame_buffer;
 	uint32			frame_buffer_offset;
 
+	bool			got_vbt;
+	bool			single_head_locked;
+
 	struct lock		accelerant_lock;
 	struct lock		engine_lock;
 
