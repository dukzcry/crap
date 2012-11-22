--- main.c	Sun Mar 15 22:00:48 1998
+++ ../../amiwm-mod/main.c	Sun Dec  2 14:32:00 2007
@@ -6,9 +6,9 @@
 #include <X11/Xresource.h>
 #include <X11/keysym.h>
 #include <X11/Xmu/Error.h>
-#ifdef HAVE_X11_EXTENSIONS_SHAPE_H
+//#ifdef HAVE_X11_EXTENSIONS_SHAPE_H
 #include <X11/extensions/shape.h>
-#endif
+//#endif
 #ifdef AMIGAOS
 #include <x11/xtrans.h>
 #endif
@@ -42,6 +42,10 @@
 #include <pragmas/xlib_pragmas.h>
 extern struct Library *XLibBase;
 
+pthread_t thread;
+
+void *thread_func( void *vptr_args );
+
 struct timeval {
   long tv_sec;
   long tv_usec;
@@ -514,7 +518,7 @@ void aborticondragging()
 void badicondrop()
 {
   wberror(dragiconlist[0].icon->scr,
-	  "Icons cannot be moved into this window");
+	  "Иконки не могут быть перемещены в это окно");
   aborticondragging();
 }
 
@@ -762,6 +766,8 @@ void internal_broker(XEvent *e)
   }
 }
 
+void *panel(void *vptr_args);
+
 int main(int argc, char *argv[])
 {
   int x_fd, sc;
@@ -789,7 +795,17 @@ int main(int argc, char *argv[])
 
   if(array[1].ptr) {
     char *env=malloc(strlen((char *)array[1].ptr)+10);
-    sprintf(env, "DISPLAY=%s", (char *)array[1].ptr);
+
+    /* We'll use snprintf() for OpenBSD and sprintf() for others */
+  
+    #if __BSD_VISIBLE
+        snprintf(env, sizeof(env), "DISPLAY=%s", (char *)array[1].ptr);
+    #else
+        sprintf(env, "DISPLAY=%s", (char *)array[1].ptr);
+    #endif
+   
+    /*                                                           */
+
     putenv(env);
   }
 
@@ -840,17 +856,85 @@ int main(int argc, char *argv[])
 
   lookup_keysyms(dpy, &meta_mask, &switch_mask);
 
+  /* Выводим всякую фигню на панель */
+
+        /*    Память          */
+        FILE *pipe;
+        float MemoryStatus;
+        char SysInfoArray[157+1];
+        pipe = popen("sysctl -n hw.usermem", "r");
+        fscanf(pipe, "%f", &MemoryStatus);
+        pclose(pipe);
+        MemoryStatus /= 1048576;
+        /*                    */
+
+        /*   Часики           */
+        time_t seconds=time(NULL);
+        struct tm* timestruct=localtime(&seconds);
+        char HourArray[3], MinArray[3];
+        if(timestruct->tm_hour < 10)    // Добавляем нолики если одноразрядные числа
+                snprintf(HourArray, sizeof(HourArray), "0%d", timestruct->tm_hour);
+        else    
+                snprintf(HourArray, sizeof(HourArray), "%d", timestruct->tm_hour);
+        if(timestruct->tm_min < 10)
+                snprintf(MinArray, sizeof(MinArray), "0%d", timestruct->tm_min);
+        else
+                snprintf(MinArray, sizeof(MinArray), "%d", timestruct->tm_min);
+        /*                    */
+
+  /* We'll use snprintf() for OpenBSD and sprintf() for others */
+  
+  #if __BSD_VISIBLE
+
+        /*      В этом куске печатаем часики справа, заточено под 1440px,
+                под другое разрешение нужно править.
+        */
+        snprintf(SysInfoArray, sizeof(SysInfoArray), "Workbench %.3fM - память", MemoryStatus);
+        int i;
+        for(i = 0; SysInfoArray[i] != '\0'; i++)
+                ;
+        int SpaceNum = sizeof(SysInfoArray) - i - 6;
+        char SpaceArray[SpaceNum];
+        for(i = 0; i < SpaceNum; i++)
+                SpaceArray[i] = ' ';
+        SpaceArray[SpaceNum] = '\0';
+        /*                                                             */
+
+        snprintf(SysInfoArray, sizeof(SysInfoArray), "Workbench %.3fM - память%s%s:%s", MemoryStatus, SpaceArray, HourArray, MinArray);
+  #else 
+        sprintf(SysInfoArray, "Workbench %.3fM - память", MemoryStatus);
+  #endif
+   
+  /*                                                           */
+
+  /*                                */
+
   for(sc=0; sc<ScreenCount(dpy); sc++)
     if(sc==DefaultScreen(dpy) || prefs.manage_all)
       if(!getscreenbyroot(RootWindow(dpy, sc))) {
 	char buf[64];
-	sprintf(buf, "Screen.%d", sc);
-	openscreen((sc? strdup(buf):"Workbench Screen"), RootWindow(dpy, sc));
-      }
+
+        /* We'll use snprintf() for OpenBSD and sprintf() for others */
+  
+        #if __BSD_VISIBLE
+                snprintf(buf, sizeof(buf), "%d", sc);
+        #else 
+	        sprintf(buf, "%d", sc);
+        #endif
+
+        /*                                                           */
+
+        if(sc)
+               openscreen(strdup(buf), RootWindow(dpy, sc));
+        else
+               openscreen(strdup(SysInfoArray), RootWindow(dpy, sc));        
+      }   	
   /*
   if(!front)
-    openscreen("Workbench Screen", DefaultRootWindow(dpy));
+    openscreen("Workbench", DefaultRootWindow(dpy));
     */
+
+  
   realizescreens();
 
   setfocus(None);
@@ -1428,5 +1512,6 @@ int main(int argc, char *argv[])
   XFlush(dpy);
   XCloseDisplay(dpy);
   exit(signalled? 0:1);
+
 }
 
