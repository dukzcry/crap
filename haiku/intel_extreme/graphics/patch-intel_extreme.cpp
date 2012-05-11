--- intel_extreme.cpp.orig	2012-05-11 11:03:37.057147392 +0400
+++ intel_extreme.cpp	2012-05-11 10:59:34.826540032 +0400
@@ -29,6 +29,7 @@
 #	define TRACE(x) ;
 #endif
 
+extern bool get_lvds_mode_from_bios(display_mode *);
 
 static void
 init_overlay_registers(overlay_registers* registers)
@@ -334,6 +335,11 @@ intel_extreme_init(intel_info &info)
 	info.shared_info->frame_buffer = 0;
 	info.shared_info->dpms_mode = B_DPMS_ON;
 
+	info.shared_info->got_vbt = get_lvds_mode_from_bios(&info.shared_info->current_mode);
+	/* at least 855gm can't drive more than one head at time */
+	if (info.device_type.InFamily(INTEL_TYPE_8xx))
+		info.shared_info->single_head_locked = 1;
+
 	if (info.device_type.InFamily(INTEL_TYPE_9xx)) {
 		info.shared_info->pll_info.reference_frequency = 96000;	// 96 kHz
 		info.shared_info->pll_info.max_frequency = 400000;
