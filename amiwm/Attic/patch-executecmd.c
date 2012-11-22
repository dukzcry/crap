--- executecmd.c	Sat Dec 13 00:37:20 1997
+++ ../../amiwm-mod/executecmd.c	Sun Dec  2 14:32:00 2007
@@ -34,9 +34,9 @@ extern struct Library *XLibBase;
 #define BUT_VSPACE 2
 #define BUT_HSPACE 8
 
-static const char ok_txt[]="Ok", cancel_txt[]="Cancel";
-static const char enter_txt[]="Enter Command and its Arguments:";
-static const char cmd_txt[]="Command:";
+static const char ok_txt[]="Ок", cancel_txt[]="Отмена";
+static const char enter_txt[]="Введите команду и ее аргументы:";
+static const char cmd_txt[]="Команда:";
 
 static int selected=0, depressed=0, stractive=1;
 static Window button[3];
@@ -331,8 +331,8 @@ int main(int argc, char *argv[])
   XSetFont(dpy, gc, dri.dri_Font->fid);
 
   size_hints.flags = PResizeInc;
-  txtprop1.value=(unsigned char *)"Execute a File";
-  txtprop2.value=(unsigned char *)"ExecuteCmd";
+  txtprop1.value=(unsigned char *)"Выполнить команду"; 
+ txtprop2.value=(unsigned char *)"ExecuteCmd";
   txtprop2.encoding=txtprop1.encoding=XA_STRING;
   txtprop2.format=txtprop1.format=8;
   txtprop1.nitems=strlen((char *)txtprop1.value);
