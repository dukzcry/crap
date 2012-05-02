--- /boot/home/temp/tmp/intel_extreme/src/accelerants/intel_extreme/mode.cpp.orig	2012-05-01 21:13:41.520880128 +0400
+++ /boot/home/temp/tmp/intel_extreme/src/accelerants/intel_extreme/mode.cpp	2012-05-02 16:30:21.556793856 +0400
@@ -24,7 +24,7 @@
 #include <validate_display_mode.h>
 
 
-#define TRACE_MODE
+#define TRACE_MODE 1
 #ifdef TRACE_MODE
 extern "C" void _sPrintf(const char* format, ...);
 #	define TRACE(x) _sPrintf x
@@ -69,6 +69,7 @@ struct pll_limits {
 	uint32			max_vco;
 };
 
+static void vbt_fill_missing_bits(display_mode *);
 
 static status_t
 get_i2c_signals(void* cookie, int* _clock, int* _data)
@@ -184,8 +185,15 @@ create_mode_list(void)
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
 
@@ -196,8 +204,16 @@ create_mode_list(void)
 				if (area < B_OK)
 					return area;
 
-				memcpy(list, &gInfo->lvds_panel_mode, sizeof(display_mode));
-
+				/* if got one, prefer info from VBT over standard BIOS
+				 * info */
+				if (gInfo->shared_info->got_vbt) {
+					memcpy(list, &gInfo->shared_info->vbt_mode,
+						sizeof(display_mode));
+					vbt_fill_missing_bits(list);
+				}
+				else
+					memcpy(list, &gInfo->lvds_panel_mode,
+						sizeof(display_mode));
 				gInfo->mode_list_area = area;
 				gInfo->mode_list = list;
 				gInfo->shared_info->mode_list_area = gInfo->mode_list_area;
@@ -391,6 +407,30 @@ compute_pll_divisors(const display_mode 
 		divisors.m, divisors.m1, divisors.m2));
 }
 
+static void
+vbt_fill_missing_bits(display_mode *mode)
+{
+        uint32 value = read32(INTEL_DISPLAY_B_CONTROL);
+
+        switch (value & DISPLAY_CONTROL_COLOR_MASK) {
+                case DISPLAY_CONTROL_RGB32:
+                default:
+                        mode->space = B_RGB32;
+                        break;
+                case DISPLAY_CONTROL_RGB16:
+                        mode->space = B_RGB16;
+                        break;
+                case DISPLAY_CONTROL_RGB15:
+                        mode->space = B_RGB15;
+                        break;
+                case DISPLAY_CONTROL_CMAP8:
+                        mode->space = B_CMAP8;
+                        break;
+        }
+
+        mode->flags = B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
+                | B_DPMS | B_SUPPORTS_OVERLAYS;
+}
 
 void
 retrieve_current_mode(display_mode& mode, uint32 pllRegister)
