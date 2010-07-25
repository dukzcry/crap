# Microsoft Developer Studio Project File - Name="ffsdrv" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ffsdrv - Win32 Checked
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ffsdrv.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ffsdrv.mak" CFG="ffsdrv - Win32 Checked"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ffsdrv - Win32 Checked" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Checked"
# PROP BASE Intermediate_Dir "Checked"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Checked"
# PROP Intermediate_Dir "Checked"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gz /W3 /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /Gz /W3 /Z7 /Oi /Gy /I "$(BASEDIR)\inc" /I "$(BASEDIR)\inc\ddk" /I "." /D _WIN32_WINNT=0x0500 /D WIN32=100 /D "_DEBUG" /D "_WINDOWS" /D "STD_CALL" /D CONDITION_HANDLING=1 /D NT_UP=1 /D NT_INST=0 /D _NT1X_=100 /D WINNT=1 /D WIN32_LEAN_AND_MEAN=1 /D DBG=1 /D DEVL=1 /D FPO=0 /D "NDEBUG" /D _DLL=1 /D _X86_=1 /FR /YX /FD /Zel -cbstring /QIfdiv- /QIf /GF /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "$(BASEDIR)\inc\Win98" /i "$(BASEDIR)\inc" /i "." /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /machine:IX86
# ADD LINK32 ntoskrnl.lib hal.lib wmilib.lib int64.lib libcntpr.lib /nologo /base:"0x10000" /version:5.0 /stack:0x40000,0x1000 /entry:"DriverEntry" /incremental:no /debug /debugtype:both /machine:IX86 /nodefaultlib /out:".\checked\ffs.sys" /libpath:"$(BASEDIR)\libchk\i386" /driver /debug:notmapped,FULL /IGNORE:4001,4037,4039,4065,4070,4078,4087,4089,4096 /MERGE:_PAGE=PAGE /MERGE:_TEXT=.text /SECTION:INIT,d /MERGE:.rdata=.text /FULLBUILD /RELEASE /FORCE:MULTIPLE /OPT:REF /OPTIDATA /align:0x20 /osversion:4.00 /subsystem:native
# SUBTRACT LINK32 /pdb:none
# Begin Target

# Name "ffsdrv - Win32 Checked"
# Begin Group "Source Files"

# PROP Default_Filter ".c;.cpp"
# Begin Source File

SOURCE=.\block.c
# End Source File
# Begin Source File

SOURCE=.\cleanup.c
# End Source File
# Begin Source File

SOURCE=.\close.c
# End Source File
# Begin Source File

SOURCE=.\cmcb.c
# End Source File
# Begin Source File

SOURCE=.\create.c
# End Source File
# Begin Source File

SOURCE=.\debug.c
# End Source File
# Begin Source File

SOURCE=.\devctl.c
# End Source File
# Begin Source File

SOURCE=.\dirctl.c
# End Source File
# Begin Source File

SOURCE=.\dispatch.c
# End Source File
# Begin Source File

SOURCE=.\except.c
# End Source File
# Begin Source File

SOURCE=.\fastio.c
# End Source File
# Begin Source File

SOURCE=.\ffs.c
# End Source File
# Begin Source File

SOURCE=.\fileinfo.c
# End Source File
# Begin Source File

SOURCE=.\flush.c
# End Source File
# Begin Source File

SOURCE=.\fsctl.c
# End Source File
# Begin Source File

SOURCE=.\init.c
# End Source File
# Begin Source File

SOURCE=.\lock.c
# End Source File
# Begin Source File

SOURCE=.\memory.c
# End Source File
# Begin Source File

SOURCE=.\misc.c
# End Source File
# Begin Source File

SOURCE=.\pnp.c
# End Source File
# Begin Source File

SOURCE=.\read.c
# End Source File
# Begin Source File

SOURCE=.\shutdown.c
# End Source File
# Begin Source File

SOURCE=.\volinfo.c
# End Source File
# Begin Source File

SOURCE=.\write.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=.\bootblock.h
# End Source File
# Begin Source File

SOURCE=.\dinode.h
# End Source File
# Begin Source File

SOURCE=.\dir.h
# End Source File
# Begin Source File

SOURCE=.\disklabel.h
# End Source File
# Begin Source File

SOURCE=.\ffsdrv.h
# End Source File
# Begin Source File

SOURCE=.\fs.h
# End Source File
# Begin Source File

SOURCE=.\ntifs.h
# End Source File
# Begin Source File

SOURCE=.\type.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ffsdrv.rc
# End Source File
# End Group
# End Target
# End Project
