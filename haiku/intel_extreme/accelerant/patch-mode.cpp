--- /boot/home/temp/tmp/intel_extreme/src/accelerants/intel_extreme/mode.cpp.orig	2012-05-01 21:13:41.520880128 +0400
+++ /boot/home/temp/tmp/intel_extreme/src/accelerants/intel_extreme/mode.cpp	2012-05-02 19:01:55.498597888 +0400
@@ -24,7 +24,7 @@
 #include <validate_display_mode.h>
 
 
-#define TRACE_MODE
+#define TRACE_MODE 1
 #ifdef TRACE_MODE
 extern "C" void _sPrintf(const char* format, ...);
 #	define TRACE(x) _sPrintf x
@@ -69,6 +69,12 @@ struct pll_limits {
 	uint32			max_vco;
 };
 
+static struct display_mode_hook {
+	bool active;
+	display_mode *dm;
+} display_mode_hook;
+
+static void vbt_fill_missing_bits(display_mode *);
 
 static status_t
 get_i2c_signals(void* cookie, int* _clock, int* _data)
@@ -184,8 +190,15 @@ create_mode_list(void)
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
 
@@ -196,7 +209,35 @@ create_mode_list(void)
 				if (area < B_OK)
 					return area;
 
-				memcpy(list, &gInfo->lvds_panel_mode, sizeof(display_mode));
+				/* if got one, prefer info from VBT over standard BIOS
+				 * info */
+				if (gInfo->shared_info->got_vbt &&
+					/* optional: deprefer VBT mode in case it
+					 * doesn't outnumber one we recieved via BIOS call */
+					gInfo->shared_info->vbt_mode.virtual_width >=
+					gInfo->lvds_panel_mode.virtual_width &&
+					gInfo->shared_info->vbt_mode.virtual_height >=
+					gInfo->lvds_panel_mode.virtual_height
+				) {
+					memcpy(list, &gInfo->shared_info->vbt_mode,
+						sizeof(display_mode));
+					vbt_fill_missing_bits(list);
+				}
+				else {
+					memcpy(list, &gInfo->lvds_panel_mode,
+						sizeof(display_mode));
+
+					if (gInfo->shared_info->got_vbt)
+						TRACE(("intel_extreme: VBT_mode.res_x*y "
+							"doesn't outnumber BIOS_call_mode.res_x*y. "
+							"Ignoring VBT mode.\n"));
+				}
+
+				/* since we're there, prefer native LFP mode by default, 
+				 * i.e. user didn't even set the native resolution mode
+				 * by himself */
+				display_mode_hook.active = true;
+				display_mode_hook.dm = list;
 
 				gInfo->mode_list_area = area;
 				gInfo->mode_list = list;
@@ -391,6 +432,30 @@ compute_pll_divisors(const display_mode 
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
@@ -681,8 +746,15 @@ intel_propose_display_mode(display_mode*
 
 
 status_t
-intel_set_display_mode(display_mode* mode)
+intel_set_display_mode(display_mode* dm)
 {
+	display_mode* mode = dm;
+
+	if (display_mode_hook.active) {
+		mode = display_mode_hook.dm;
+		display_mode_hook.active = false;
+	}
+
 	TRACE(("intel_set_display_mode(%ldx%ld)\n", mode->virtual_width,
 		mode->virtual_height));
 
