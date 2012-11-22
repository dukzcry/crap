--- ppmtoinfo.c	Sun Mar 15 22:43:06 1998
+++ ../../amiwm-mod/ppmtoinfo.c	Sun Dec  2 14:32:00 2007
@@ -113,7 +113,17 @@ char *makelibfilename(char *oldname)
   char *newname;
   if(*oldname=='/') return oldname;
   newname=myalloc(strlen(oldname)+strlen(AMIWM_HOME)+2);
-  sprintf(newname, AMIWM_HOME"/%s", oldname);
+
+  /* We'll use snprintf() for OpenBSD and sprintf() for others */
+  
+  #if __BSD_VISIBLE
+        snprintf(newname, strlen(oldname)+strlen(AMIWM_HOME)+2, AMIWM_HOME"/%s", oldname);
+  #else
+        sprintf(newname, AMIWM_HOME"/%s", oldname);
+  #endif
+   
+  /*                                                           */
+
   return newname;
 }
 
