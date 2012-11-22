--- icon.c	Sun Mar 15 22:06:35 1998
+++ ../../amiwm-mod/icon.c	Sun Dec  2 14:32:00 2007
@@ -371,7 +371,17 @@ Icon *createappicon(struct module *m, Window p, char *
   XSelectInput(dpy, i->labelwin, ExposureMask);
 
   if((i->label.value=malloc((i->label.nitems=strlen(name))+1))!=NULL) {
-    strcpy((char *)i->label.value, name);
+
+    /* We'll use strlcpy() for OpenBSD and strcpy() for others */
+
+    #if __BSD_VISIBLE
+        strlcpy((char *)i->label.value, name, sizeof((char *)i->label.value));
+    #else
+        strcpy((char *)i->label.value, name);
+    #endif
+
+    /*                                                       */
+
     i->labelwidth=XTextWidth(labelfont, (char *)i->label.value,
 			     i->label.nitems);
     if(i->labelwidth)
