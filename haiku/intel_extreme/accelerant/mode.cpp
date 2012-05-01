--- /boot/home/temp/tmp/intel_extreme/src/accelerants/intel_extreme/mode.cpp.orig	2012-05-01 21:13:41.520880128 +0400
+++ /boot/home/temp/tmp/intel_extreme/src/accelerants/intel_extreme/mode.cpp	2012-05-01 22:45:41.913833984 +0400
@@ -24,7 +24,7 @@
 #include <validate_display_mode.h>
 
 
-#define TRACE_MODE
+#define TRACE_MODE 1
 #ifdef TRACE_MODE
 extern "C" void _sPrintf(const char* format, ...);
 #	define TRACE(x) _sPrintf x
@@ -184,8 +184,15 @@ create_mode_list(void)
 			// We could not read any EDID info. Fallback to creating a list with
 			// only the mode set up by the BIOS.
 			// TODO: support lower modes via scaling and windowing
-			if ((gInfo->head_mode & HEAD_MODE_LVDS_PANEL) != 0
-					&& (gInfo->head_mode & HEAD_MODE_A_ANALOG) == 0) {
+			if ( ((gInfo->head_mode & HEAD_MODE_LVDS_PANEL) != 0
+					/* pay attention:
+					 * this one check is not universal, on my machine
+					 * with 855gm it shows VGA port status as used, even
+					 * then, when nothing is attached to it and LVDS
+					 * is used */ 
+					&& (gInfo->head_mode & HEAD_MODE_A_ANALOG) == 0) ||
+				((gInfo->head_mode & HEAD_MODE_LVDS_PANEL) != 0
+					&& gInfo->shared_info->got_vbt) ) {
 				size_t size = (sizeof(display_mode) + B_PAGE_SIZE - 1)
 					& ~(B_PAGE_SIZE - 1);
 
@@ -196,6 +203,9 @@ create_mode_list(void)
 				if (area < B_OK)
 					return area;
 
+				/* if got one, prefer info from VBT over standard BIOS
+				 * info */
+				//if (gInfo->shared_info->got_vbe)
 				memcpy(list, &gInfo->lvds_panel_mode, sizeof(display_mode));
 
 				gInfo->mode_list_area = area;
