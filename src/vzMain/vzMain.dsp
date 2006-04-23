# Microsoft Developer Studio Project File - Name="vzMain" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=vzMain - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vzMain.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vzMain.mak" CFG="vzMain - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vzMain - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "vzMain - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "vzMain - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../tmp/Release/"
# PROP Intermediate_Dir "../../tmp/Release/vzMain"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VZMAIN_EXPORTS" /YX /FD /c
# ADD CPP /nologo /Zp16 /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VZMAIN_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x422 /d "NDEBUG"
# ADD RSC /l 0x422 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"../../install/Release/vzMain.dll"

!ELSEIF  "$(CFG)" == "vzMain - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../tmp/Debug/"
# PROP Intermediate_Dir "../../tmp/Debug/vzMain"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VZMAIN_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /Zp16 /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VZMAIN_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x422 /d "_DEBUG"
# ADD RSC /l 0x422 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /profile /debug /machine:I386 /out:"../../install/Debug/vzMain.dll"

!ENDIF 

# Begin Target

# Name "vzMain - Win32 Release"
# Name "vzMain - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\vz\unicode.cpp
# End Source File
# Begin Source File

SOURCE=..\vz\vzConfig.cpp
# End Source File
# Begin Source File

SOURCE=..\vz\vzContainer.cpp
# End Source File
# Begin Source File

SOURCE=..\vz\vzContainerFunction.cpp
# End Source File
# Begin Source File

SOURCE=..\vz\vzFunction.cpp
# End Source File
# Begin Source File

SOURCE=..\vz\vzFunctions.cpp
# End Source File
# Begin Source File

SOURCE=..\vz\vzMain.cpp
# End Source File
# Begin Source File

SOURCE=..\vz\vzMotion.cpp
# End Source File
# Begin Source File

SOURCE=..\vz\vzMotionControl.cpp
# End Source File
# Begin Source File

SOURCE=..\vz\vzMotionControlKey.cpp
# End Source File
# Begin Source File

SOURCE=..\vz\vzMotionDirector.cpp
# End Source File
# Begin Source File

SOURCE=..\vz\vzMotionParameter.cpp
# End Source File
# Begin Source File

SOURCE=..\vz\vzMotionTimeline.cpp
# End Source File
# Begin Source File

SOURCE=..\vz\vzScene.cpp
# End Source File
# Begin Source File

SOURCE=..\vz\vzXMLAttributes.cpp
# End Source File
# Begin Source File

SOURCE=..\vz\vzXMLParams.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\vz\plugin.h
# End Source File
# Begin Source File

SOURCE=..\vz\unicode.h
# End Source File
# Begin Source File

SOURCE=..\vz\vzConfig.h
# End Source File
# Begin Source File

SOURCE=..\vz\vzContainer.h
# End Source File
# Begin Source File

SOURCE=..\vz\vzContainerFunction.h
# End Source File
# Begin Source File

SOURCE=..\vz\vzFunction.h
# End Source File
# Begin Source File

SOURCE=..\vz\vzFunctions.h
# End Source File
# Begin Source File

SOURCE=..\vz\vzMain.h
# End Source File
# Begin Source File

SOURCE=..\vz\vzMotion.h
# End Source File
# Begin Source File

SOURCE=..\vz\vzMotionControl.h
# End Source File
# Begin Source File

SOURCE=..\vz\vzMotionControlKey.h
# End Source File
# Begin Source File

SOURCE=..\vz\vzMotionDirector.h
# End Source File
# Begin Source File

SOURCE=..\vz\vzMotionParameter.h
# End Source File
# Begin Source File

SOURCE=..\vz\vzMotionTimeline.h
# End Source File
# Begin Source File

SOURCE=..\vz\vzScene.h
# End Source File
# Begin Source File

SOURCE=..\vz\vzTVSpec.h
# End Source File
# Begin Source File

SOURCE=..\vz\vzXMLAttributes.h
# End Source File
# Begin Source File

SOURCE=..\vz\vzXMLParams.h
# End Source File
# Begin Source File

SOURCE=..\vz\xerces.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
