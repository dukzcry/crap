--- mode.cpp.orig	2012-05-11 11:01:05.053739520 +0400
+++ mode.cpp	2012-05-11 15:17:15.736624640 +0400
@@ -24,7 +24,7 @@
 #include <validate_display_mode.h>
 
 
-#define TRACE_MODE
+#define TRACE_MODE 1
 #ifdef TRACE_MODE
 extern "C" void _sPrintf(const char* format, ...);
 #	define TRACE(x) _sPrintf x
@@ -69,6 +69,17 @@ struct pll_limits {
 	uint32			max_vco;
 };
 
+static struct display_mode_hook {
+	bool active;
+	display_mode *dm;
+	struct {
+		uint16 width;
+		uint16 height;
+		uint16 space;
+	} mode;
+} display_mode_hook;
+
+static void mode_fill_missing_bits(display_mode *, uint32);
 
 static status_t
 get_i2c_signals(void* cookie, int* _clock, int* _data)
@@ -127,12 +138,12 @@ set_frame_buffer_base()
 	uint32 baseRegister;
 	uint32 surfaceRegister;
 
-	if (gInfo->head_mode & HEAD_MODE_A_ANALOG) {
-		baseRegister = INTEL_DISPLAY_A_BASE;
-		surfaceRegister = INTEL_DISPLAY_A_SURFACE;
-	} else {
+	if (gInfo->head_mode & HEAD_MODE_B_DIGITAL) {
 		baseRegister = INTEL_DISPLAY_B_BASE;
 		surfaceRegister = INTEL_DISPLAY_B_SURFACE;
+	} else {
+		baseRegister = INTEL_DISPLAY_A_BASE;
+		surfaceRegister = INTEL_DISPLAY_A_SURFACE;
 	}
 
 	if (sharedInfo.device_type.InGroup(INTEL_TYPE_96x)
@@ -169,6 +180,8 @@ create_mode_list(void)
 	if (error == B_OK) {
 		edid_dump(&gInfo->edid_info);
 		gInfo->has_edid = true;
+		if (gInfo->shared_info->single_head_locked)
+			gInfo->head_mode = HEAD_MODE_A_ANALOG;
 	} else {
 		TRACE(("intel_extreme: getting EDID on port A (analog) failed : %s. "
 			"Trying on port C (lvds)\n", strerror(error)));
@@ -184,8 +197,15 @@ create_mode_list(void)
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
 
@@ -196,7 +216,35 @@ create_mode_list(void)
 				if (area < B_OK)
 					return area;
 
-				memcpy(list, &gInfo->lvds_panel_mode, sizeof(display_mode));
+				/* if got one, prefer info from VBT over standard BIOS
+				 * info */
+				if (gInfo->shared_info->got_vbt &&
+					/* optional: deprefer VBT mode in case it
+					 * doesn't outnumber one we recieved via BIOS call */
+					gInfo->shared_info->current_mode.virtual_width >=
+					gInfo->lvds_panel_mode.virtual_width &&
+					gInfo->shared_info->current_mode.virtual_height >=
+					gInfo->lvds_panel_mode.virtual_height
+				) {
+					memcpy(list, &gInfo->shared_info->current_mode,
+						sizeof(display_mode));
+					mode_fill_missing_bits(list, INTEL_DISPLAY_B_CONTROL);
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
@@ -391,6 +439,30 @@ compute_pll_divisors(const display_mode 
 		divisors.m, divisors.m1, divisors.m2));
 }
 
+static void
+mode_fill_missing_bits(display_mode *mode, uint32 cntrl)
+{
+        uint32 value = read32(cntrl);
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
@@ -521,27 +593,10 @@ retrieve_current_mode(display_mode& mode
 	if (mode.virtual_height < mode.timing.v_display)
 		mode.virtual_height = mode.timing.v_display;
 
-	value = read32(controlRegister);
-	switch (value & DISPLAY_CONTROL_COLOR_MASK) {
-		case DISPLAY_CONTROL_RGB32:
-		default:
-			mode.space = B_RGB32;
-			break;
-		case DISPLAY_CONTROL_RGB16:
-			mode.space = B_RGB16;
-			break;
-		case DISPLAY_CONTROL_RGB15:
-			mode.space = B_RGB15;
-			break;
-		case DISPLAY_CONTROL_CMAP8:
-			mode.space = B_CMAP8;
-			break;
-	}
+	mode_fill_missing_bits(&mode, controlRegister);
 
 	mode.h_display_start = 0;
 	mode.v_display_start = 0;
-	mode.flags = B_8_BIT_DAC | B_HARDWARE_CURSOR | B_PARALLEL_ACCESS
-		| B_DPMS | B_SUPPORTS_OVERLAYS;
 }
 
 
@@ -681,13 +736,22 @@ intel_propose_display_mode(display_mode*
 
 
 status_t
-intel_set_display_mode(display_mode* mode)
+intel_set_display_mode(display_mode* dm)
 {
-	TRACE(("intel_set_display_mode(%ldx%ld)\n", mode->virtual_width,
-		mode->virtual_height));
+	display_mode* mode = dm;
 
-	if (mode == NULL)
+	if (display_mode_hook.active) {
+		mode = display_mode_hook.dm;
+		display_mode_hook.active = false;
+	}
+
+	if (mode == NULL) {
+		TRACE(("intel_set_display_mode(mode = NULL)\n"));
 		return B_BAD_VALUE;
+	}
+
+	TRACE(("intel_set_display_mode(%ldx%ld)\n", mode->virtual_width,
+		mode->virtual_height));
 
 	display_mode target = *mode;
 
@@ -702,9 +766,12 @@ intel_set_display_mode(display_mode* mod
 	uint32 colorMode, bytesPerRow, bitsPerPixel;
 	get_color_space_format(target, colorMode, bytesPerRow, bitsPerPixel);
 
-	// TODO: do not go further if the mode is identical to the current one.
-	// This would avoid the screen being off when switching workspaces when they
-	// have the same resolution.
+	// avoid screen being off when switching workspaces when they
+        // have the same screen mode.
+	if (target.virtual_width == display_mode_hook.mode.width &&
+		target.virtual_height == display_mode_hook.mode.height &&
+		target.space == display_mode_hook.mode.space)
+			return B_OK;
 
 #if 0
 static bool first = true;
@@ -1006,7 +1073,9 @@ if (first) {
 			read32(INTEL_DISPLAY_B_PIPE_CONTROL) | DISPLAY_PIPE_ENABLED);
 		read32(INTEL_DISPLAY_B_PIPE_CONTROL);
 	}
-
+	/* lot of bus writes and waits, better to take it off */
+	else
+	/* */
 	if ((gInfo->head_mode & HEAD_MODE_A_ANALOG) != 0) {
 		pll_divisors divisors;
 		compute_pll_divisors(target, divisors, false);
@@ -1140,6 +1209,10 @@ if (first) {
 	sharedInfo.current_mode = target;
 	sharedInfo.bits_per_pixel = bitsPerPixel;
 
+	display_mode_hook.mode.width = target.virtual_width;
+	display_mode_hook.mode.height = target.virtual_height;
+	display_mode_hook.mode.space = target.space;
+
 	return B_OK;
 }
 
