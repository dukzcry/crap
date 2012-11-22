--- module.c	Sun Mar 15 22:23:07 1998
+++ ../../amiwm-mod/module.c	Sun Dec  2 14:32:00 2007
@@ -202,10 +202,30 @@ void create_module(Scrn *screen, char *module_name, ch
 #else
   if((temppath = malloc(strlen(prefs.module_path)+2))) {
 #endif
-    strcpy(temppath, prefs.module_path);
+
+    /* We'll use strlcpy() for OpenBSD and strcpy() for others */
+    
+    #if __BSD_VISIBLE
+        strlcpy(temppath, prefs.module_path, sizeof(temppath));
+    #else
+        strcpy(temppath, prefs.module_path);
+    #endif
+
+    /*                                                         */    
+
     for(pathelt=strtok(temppath, ":"); pathelt;
 	pathelt=strtok(NULL, ":")) {
-      sprintf(destpath, "%s/%s", pathelt, module_name);
+    
+      /* We'll use snprintf() for OpenBSD and sprintf() for others */
+  
+      #if __BSD_VISIBLE
+        snprintf(destpath, sizeof(destpath), "%s/%s", pathelt, module_name);
+      #else
+        sprintf(destpath, "%s/%s", pathelt, module_name);
+      #endif
+   
+      /*                                                           */
+
       if(access(destpath, X_OK)>=0)
 	break;
     }
@@ -247,9 +267,21 @@ void create_module(Scrn *screen, char *module_name, ch
 	}
 	close(fds1[1]);
 	close(fds2[0]);
-	sprintf(fd1num, "%d", fds1[0]);
-	sprintf(fd2num, "%d", fds2[1]);
-	sprintf(scrnum, "0x%08lx", (screen? (screen->back):None));
+
+        /* We'll use snprintf() for OpenBSD and sprintf() for others */
+  
+        #if __BSD_VISIBLE
+                snprintf(fd1num, sizeof(fd1num), "%d", fds1[0]);
+                snprintf(fd2num, sizeof(fd2num), "%d", fds2[1]);
+                snprintf(scrnum, sizeof(scrnum), "0x%08lx", (screen? (screen->back):None));
+        #else
+                sprintf(fd1num, "%d", fds1[0]);
+                sprintf(fd2num, "%d", fds2[1]);
+                sprintf(scrnum, "0x%08lx", (screen? (screen->back):None));
+        #endif
+   
+        /*                                                           */
+
 	execl(destpath, module_name, fd1num, fd2num, scrnum, module_arg, NULL);
 	perror(destpath);
 	_exit(1);
