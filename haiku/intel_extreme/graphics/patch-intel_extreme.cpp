--- /boot/home/temp/tmp/intel_extreme/src/graphics/intel_extreme/intel_extreme.cpp.orig	2012-05-01 21:10:47.613416960 +0400
+++ /boot/home/temp/tmp/intel_extreme/src/graphics/intel_extreme/intel_extreme.cpp	2012-05-02 15:04:40.493092864 +0400
@@ -29,6 +29,7 @@
 #	define TRACE(x) ;
 #endif
 
+bool get_lvds_mode_from_bios(display_mode *);
 
 static void
 init_overlay_registers(overlay_registers* registers)
@@ -334,6 +335,8 @@ intel_extreme_init(intel_info &info)
 	info.shared_info->frame_buffer = 0;
 	info.shared_info->dpms_mode = B_DPMS_ON;
 
+	info.shared_info->got_vbt = get_lvds_mode_from_bios(&info.shared_info->vbt_mode);
+
 	if (info.device_type.InFamily(INTEL_TYPE_9xx)) {
 		info.shared_info->pll_info.reference_frequency = 96000;	// 96 kHz
 		info.shared_info->pll_info.max_frequency = 400000;
