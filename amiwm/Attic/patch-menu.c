--- menu.c	Sun Mar 15 22:24:39 1998
+++ ../../amiwm-mod/menu.c	Sun Dec  2 14:32:00 2007
@@ -76,7 +76,17 @@ void spawn(const char *cmd)
 {
   char *line=malloc(strlen(cmd)+12);
   if(line) {
-    sprintf(line, "RUN <>NIL: %s", cmd);
+
+    /* We'll use snprintf() for OpenBSD and sprintf() for others */
+  
+    #if __BSD_VISIBLE
+        snprintf(line, sizeof(line), "RUN <>NIL: %s", cmd);
+    #else
+        sprintf(line, "RUN <>NIL: %s", cmd);
+    #endif
+   
+    /*                                                           */
+
     system(line);
     free(line);
   }
@@ -92,10 +102,30 @@ void spawn(const char *cmd)
   if(line) {
 #endif
   char *dot;
-  sprintf(line, "DISPLAY='%s", x_server);
+
+  /* We'll use snprintf() for OpenBSD and sprintf() for others */
+  
+  #if __BSD_VISIBLE
+        snprintf(line, strlen(x_server)+strlen(cmd)+28, "DISPLAY='%s", x_server);
+  #else
+        sprintf(line, "DISPLAY='%s", x_server);
+  #endif
+   
+  /*                                                           */
+
   if(!(dot=strrchr(line, '.')) || strchr(dot, ':'))
     dot=line+strlen(line);
-  sprintf(dot, ".%d' %s &", scr->number, cmd);
+
+    /* We'll use snprintf() for OpenBSD and sprintf() for others */
+  
+    #if __BSD_VISIBLE
+        snprintf(dot, strlen(x_server)+strlen(cmd)+28+strlen(line), ".%d' %s &", scr->number, cmd);
+    #else
+        sprintf(dot, ".%d' %s &", scr->number, cmd);
+    #endif
+   
+    /*                                                           */
+
 #ifdef __ultrix
   {
     int pid, status;
@@ -361,21 +391,21 @@ void createmenubar()
   scr->subspace=scr->hotkeyspace-scr->dri.dri_Font->ascent;
   scr->menuleft=4;
   m=add_menu("Workbench", 0);
-  add_item(m,"Backdrop",'B',CHECKIT|CHECKED|DISABLED);
-  add_item(m,"Execute Command...",'E',0);
-  add_item(m,"Redraw All",0,0);
-  add_item(m,"Update All",0,DISABLED);
-  add_item(m,"Last Message",0,DISABLED);
-  add_item(m,"About...",'?',0);
-  add_item(m,"Quit...",'Q',0);
+  add_item(m,"Задний план",'B',CHECKIT|CHECKED|DISABLED);
+  add_item(m,"Выполнить команду...",'E',0);
+  add_item(m,"Отрисовать заново",0,0);
+  add_item(m,"Обновить все",0,DISABLED);
+  add_item(m,"Последнее уведомление",0,DISABLED);
+  add_item(m,"О Workbench...",'?',0);
+  add_item(m,"Выход...",'Q',0);
   menu_layout(m);
-  m=add_menu("Window", 0);
+  m=add_menu("Окно", 0);
   add_item(m,"New Drawer",'N',DISABLED);
   add_item(m,"Open Parent",0,DISABLED);
   add_item(m,"Close",'K',DISABLED);
   add_item(m,"Update",0,DISABLED);
-  add_item(m,"Select Contents",'A',0);
-  add_item(m,"Clean Up",'.',0);
+  add_item(m,"Выбрать все",'A',0);
+  add_item(m,"Разложить",'.',0);
   sm1=sub_menu(add_item(m,"Snapshot",0,DISABLED),0);
   add_item(sm1, "Window",0,DISABLED);
   add_item(sm1, "All",0,DISABLED);
@@ -405,11 +435,11 @@ void createmenubar()
   add_item(m,"Format Disk...",0,DISABLED);
   add_item(m,"Empty Trash",0,DISABLED);
   menu_layout(m);
-  m=add_menu("Tools", 0);
+  m=add_menu("Утилиты", 0);
 #ifdef AMIGAOS
-  add_item(m,"ResetWB",0,DISABLED);
+  add_item(m,"Перезапустить Workbench",0,DISABLED);
 #else
-  add_item(m,"ResetWB",0,0);
+  add_item(m,"Перезапустить Workbench",0,0);
 #endif
 
   for(ti=firsttoolitem; ti; ti=ti->next)
@@ -661,7 +691,7 @@ void menuaction(struct Item *i, struct Item *si)
 	     "<marcus@lysator.liu.se>"
 	     "' Ok");
 #endif
-      break;      
+      break;     
     case 6:
 #ifndef AMIGAOS
       if(prefs.fastquit) {
@@ -678,9 +708,22 @@ void menuaction(struct Item *i, struct Item *si)
 #else
 	char buf[256];
 #endif
-	sprintf(buf, "; export DISPLAY; ( if [ `"BIN_PREFIX"requestchoice "
-		"Workbench 'Do you really want\nto quit workbench?' Ok Cancel`"
-		" = 1 ]; then kill %d; fi; )", (int)getpid());
+
+        /* We'll use snprintf() for OpenBSD and sprintf() for others */
+  
+        #if __BSD_VISIBLE
+                snprintf(buf, 256, "; export DISPLAY; ( if [ `"BIN_PREFIX"requestchoice "
+                "Workbench 'Вы действительно \nхотите закрыть workbench?' Ок Отмена`"
+                " = 1 ]; then kill %d; fi; )", (int)getpid());
+        #else
+                sprintf(buf, "; export DISPLAY; ( if [ `"BIN_PREFIX"requestchoice "
+                "Workbench 'Вы действительно \nхотите закрыть workbench?' Ок Отмена`"
+                " = 1 ]; then kill %d; fi; )", (int)getpid());
+
+        #endif
+   
+        /*                                                           */
+
 	spawn(buf);
       }
 #endif
@@ -704,7 +747,7 @@ void menuaction(struct Item *i, struct Item *si)
     if(item==0)
       restart_amiwm();
 #endif
-    if(item>0) {
+    if(item>=1) {
       int it=0, si=-1;
       for(ti=firsttoolitem; ti; ti=ti->next) {
 	if(ti->level>0)
