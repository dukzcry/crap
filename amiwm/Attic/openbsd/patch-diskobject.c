--- diskobject.c	Sun Mar 15 00:26:12 1998
+++ ../../amiwm-mod/diskobject.c	Sun Dec  2 14:32:00 2007
@@ -67,9 +67,21 @@ void load_do(const char *filename, Pixmap *p1, Pixmap 
   char *fn=alloca(rl);
 #else
   char fn[1024];
+#endif 
+{
+  
+  /* We'll use snprintf() for OpenBSD and sprintf() for others */
+  
+  #if __BSD_VISIBLE
+        snprintf(fn, 1024, "%s/%s", prefs.icondir, filename);
+  #else
+        sprintf(fn, "%s/%s", prefs.icondir, filename);
+  #endif
+   
+  /*                                                           */
+
+}
 #endif
-  sprintf(fn, "%s/%s", prefs.icondir, filename);
-#endif
   fn[strlen(fn)-5]=0;
   if((dobj=GetDiskObject(fn))) {
     *p1=image_to_pixmap_scr(scr, (struct Image *)dobj->do_Gadget.GadgetRender,
@@ -152,16 +164,61 @@ static int custom_palette_from_file(char *fn, int igno
       g = getc(file);
       b = getc(file);
     }
-    if(mv==255)
-      sprintf(nam, "#%02x%02x%02x", r&0xff, g&0xff, b&0xff);
-    else if(mv==65536)
-      sprintf(nam, "#%04x%04x%04x", r&0xffff, g&0xffff, b&0xffff);
-    else if(mv<255)
-      sprintf(nam, "#%02x%02x%02x", (r*0xff/mv)&0xff,
-	      (g*0xff/mv)&0xff, (b*0xff/mv)&0xff);
-    else
-      sprintf(nam, "#%04x%04x%04x", (r*0xffff/mv)&0xffff,
-	      (g*0xffff/mv)&0xffff, (b*0xffff/mv)&0xffff);
+    if(mv==255) {
+      /* We'll use snprintf() for OpenBSD and sprintf() for others */
+  
+      #if __BSD_VISIBLE
+        snprintf(nam, 16, "#%02x%02x%02x", r&0xff, g&0xff, b&0xff);
+      #else
+        sprintf(nam, "#%02x%02x%02x", r&0xff, g&0xff, b&0xff);
+      #endif
+   
+      /*                                                           */
+
+    }
+    else if(mv==65536) { 
+
+      /* We'll use snprintf() for OpenBSD and sprintf() for others */
+ 
+      #if __BSD_VISIBLE
+        snprintf(nam, 16, "#%04x%04x%04x", r&0xffff, g&0xffff, b&0xffff);
+      #else
+        sprintf(nam, "#%04x%04x%04x", r&0xffff, g&0xffff, b&0xffff);
+      #endif
+ 
+      /*                                                           */
+
+    }
+    else if(mv<255) {
+
+    /* We'll use snprintf() for OpenBSD and sprintf() for others */
+  
+    #if __BSD_VISIBLE
+        snprintf(nam, 16, "#%02x%02x%02x", (r*0xff/mv)&0xff, (g*0xff/mv)&0xff, (b*0xff/mv)&0xff);
+    #else
+        sprintf(nam, "#%02x%02x%02x", (r*0xff/mv)&0xff, (g*0xff/mv)&0xff, (b*0xff/mv)&0xff);
+
+    #endif
+   
+    /*                                                           */
+
+    }
+    else {
+  
+      /* We'll use snprintf() for OpenBSD and sprintf() for others */
+  
+      #if __BSD_VISIBLE
+        snprintf(nam, 16, "#%04x%04x%04x", (r*0xffff/mv)&0xffff,
+                (g*0xffff/mv)&0xffff, (b*0xffff/mv)&0xffff);
+      #else
+        sprintf(nam, "#%04x%04x%04x", (r*0xffff/mv)&0xffff,
+        (g*0xffff/mv)&0xffff, (b*0xffff/mv)&0xffff);
+
+      #endif
+   
+      /*                                                           */
+
+    }
     iconcolorname[pixel] = strdup(nam);
   }
   fclose(file);
@@ -181,7 +238,17 @@ void set_custom_palette(char *fn)
 #else
     fn2 = malloc(strlen(fn)+strlen(AMIWM_HOME)+2);
 #endif
-    sprintf(fn2, "%s/%s", AMIWM_HOME, fn);
+
+  /* We'll use snprintf() for …OpenBSD and sprintf() for others */
+  
+  #if __BSD_VISIBLE
+        snprintf(fn2, strlen(fn)+strlen(AMIWM_HOME)+2, "%s/%s", AMIWM_HOME, fn);
+  #else
+        sprintf(fn2, "%s/%s", AMIWM_HOME, fn);
+  #endif
+   
+  /*                                                           */
+
     r=custom_palette_from_file(fn2, 1);
 #ifndef HAVE_ALLOCA
     free(fn2);
