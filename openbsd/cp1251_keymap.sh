#!/bin/sh                                                                                                                    
                                                                                                                             
$home: cp1251_keymap.sh,v 1.2 2009/07/22 08:36:45 dukzcry Exp $                                                              
                                                                                                                             
# This was an CP1251 keymap for NetBSD.                                                                                      
#                                                                                                                            
# Created Evgeny Levinskiy (aka Scr1pt).                                                                                     
# Please send your remarks to Scr1pt at rambler.ru (at -> @) and send netmail                                                
# to 2:5014/25.1 (Fido).                                                                                                     
#                                                                                                                            
# Now it is an actually CP1251 keymap script for OpenBSD.                                                                    
# Modified by slonotop @ linux.org.ru                                                                                        
# Symbols like: copyright, registered, section, guillemot, plusminus                                                         
# were pulled out to match Windows-1251 and as useless, as there are                                                         
# should be binded via AltGr key.                                                                                            
#                                                                                                                            
#                       +==========================================================================+                         
#                       |             |  English Language  <--[ Caps Lock ]-->  Russian Language   |                         
#                       |keycode ##   | -              +shift     | -                       +shift |                         
#                       +==========================================================================+                         
wsconsctl \                                                                                                                  
        keyboard.map+="keycode   30 =   1              exclam       1                       exclam" \                        
        keyboard.map+="keycode   31 =   2              at           2                     quotedbl" \                        
        keyboard.map+="keycode   32 =   3              numbersign   3                  onesuperior" \                        
        keyboard.map+="keycode   33 =   4              dollar       4                    semicolon" \                        
        keyboard.map+="keycode   34 =   5              percent      5                      percent" \                        
        keyboard.map+="keycode   35 =   6              asciicircum  6                        colon" \                        
        keyboard.map+="keycode   36 =   7              ampersand    7                     question" \                        
        keyboard.map+="keycode   37 =   8              asterisk     8                     asterisk" \                        
        keyboard.map+="keycode   38 =   9              parenleft    9                    parenleft" \                        
        keyboard.map+="keycode   39 =   0              parenright   0                   parenright" \                        
        keyboard.map+="keycode   46 =   equal          plus         equal                     plus" \                        
        keyboard.map+="keycode   20 =   q              Q            eacute                  Eacute" \                        
        keyboard.map+="keycode   26 =   w              W            odiaeresis          Odiaeresis" \                        
        keyboard.map+="keycode   8  =   e              E            oacute                  Oacute" \                        
        keyboard.map+="keycode   21 =   r              R            ecircumflex        Ecircumflex" \                        
        keyboard.map+="keycode   23 =   t              T            aring                    Aring" \                        
        keyboard.map+="keycode   28 =   y              Y            iacute                  Iacute" \                        
        keyboard.map+="keycode   24 =   u              U            atilde                  Atilde" \                        
        keyboard.map+="keycode   12 =   i              I            oslash                Ooblique" \                        
        keyboard.map+="keycode   18 =   o              O            ugrave                  Ugrave" \                        
        keyboard.map+="keycode   19 =   p              P            ccedilla              Ccedilla" \                        
        keyboard.map+="keycode   47 =   bracketleft    braceleft    otilde                  Otilde" \                        
        keyboard.map+="keycode   48 =   bracketright   braceright   uacute                  Uacute" \                        
        keyboard.map+="keycode   4  =   a              A            ocircumflex        Ocircumflex" \                        
        keyboard.map+="keycode   22 =   s              S            ucircumflex        Ucircumflex" \                        
        keyboard.map+="keycode   7  =   d              D            acircumflex        Acircumflex" \                        
        keyboard.map+="keycode   9  =   f              F            agrave                  Agrave" \                        
        keyboard.map+="keycode   10 =   g              G            idiaeresis          Idiaeresis" \                        
        keyboard.map+="keycode   11 =   h              H            eth                        ETH" \                        
        keyboard.map+="keycode   13 =   j              J            icircumflex        Icircumflex" \                        
        keyboard.map+="keycode   14 =   k              K            ediaeresis          Ediaeresis" \                        
        keyboard.map+="keycode   15 =   l              L            adiaeresis          Adiaeresis" \                        
        keyboard.map+="keycode   51 =   semicolon      colon        ae                          AE" \                        
        keyboard.map+="keycode   52 =   apostrophe     quotedbl     yacute                  Yacute" \                        
        keyboard.map+="keycode   53 =   grave          asciitilde   cedilla              diaeresis" \                        
        keyboard.map+="keycode   49 =   backslash      bar          backslash            brokenbar" \                        
        keyboard.map+="keycode   29 =   z              Z            ydiaeresis              ssharp" \                        
        keyboard.map+="keycode   27 =   x              X            division              multiply" \                        
        keyboard.map+="keycode   6  =   c              C            ntilde                  Ntilde" \                        
        keyboard.map+="keycode   25 =   v              V            igrave                  Igrave" \                        
        keyboard.map+="keycode   5  =   b              B            egrave                  Egrave" \    
        keyboard.map+="keycode   17 =   n              N            ograve                  Ograve" \                        
        keyboard.map+="keycode   16 =   m              M            udiaeresis          Udiaeresis" \                        
        keyboard.map+="keycode   54 =   comma          less         aacute                  Aacute" \                        
        keyboard.map+="keycode   55 =   period         greater      thorn                    THORN" \                        
        keyboard.map+="keycode   56 =   slash          question     period                   comma"         