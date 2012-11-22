--- rc.c	Sun Mar 15 22:45:40 1998
+++ ../../amiwm-mod/rc.c	Sun Dec  2 14:32:00 2007
@@ -66,7 +66,17 @@ void read_rc_file(char *filename, int manage_all)
 #else
   if((fn=malloc(strlen(home)+strlen(RC_FILENAME)+4))) {
 #endif
-    sprintf(fn, "%s/"RC_FILENAME, home);
+
+    /* We'll use snprintf() for OpenBSD and sprintf() for others */
+  
+    #if __BSD_VISIBLE
+        snprintf(fn, 256, "%s/"RC_FILENAME, home);
+    #else
+        sprintf(fn, "%s/"RC_FILENAME, home);
+    #endif
+   
+    /*                                                           */
+
     if((rcfile=fopen(fn, "r"))) {
       yyparse();
       fclose(rcfile);
