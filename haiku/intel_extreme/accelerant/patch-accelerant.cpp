--- /boot/home/temp/tmp/intel_extreme/src/accelerants/intel_extreme/accelerant.cpp.orig	2012-05-17 13:22:45.251920384 +0400
+++ /boot/home/temp/tmp/intel_extreme/src/accelerants/intel_extreme/accelerant.cpp	2012-05-19 13:53:38.781189120 +0400
@@ -31,6 +31,7 @@ extern "C" void _sPrintf(const char* for
 
 
 struct accelerant_info* gInfo;
+extern void intel_acpi_init();
 
 
 class AreaCloner {
@@ -214,6 +215,9 @@ intel_init_accelerant(int device)
 		|| (!hasPCH && (lvds & DISPLAY_PIPE_ENABLED) != 0)) {
 		save_lvds_mode();
 		gInfo->head_mode |= HEAD_MODE_LVDS_PANEL;
+
+		/* register backlight control */
+		intel_acpi_init();
 	}
 
 	TRACE(("head detected: %#x\n", gInfo->head_mode));
