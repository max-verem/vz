# Microsoft Developer Studio Generated NMAKE File, Based on vz.dsp
!IF "$(CFG)" == ""
CFG=vz - Win32 Debug
!MESSAGE No configuration specified. Defaulting to vz - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "vz - Win32 Release" && "$(CFG)" != "vz - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vz.mak" CFG="vz - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vz - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "vz - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "vz - Win32 Release"

OUTDIR=.\../tmp/Release
INTDIR=.\../tmp/Release/vz

!IF "$(RECURSE)" == "0" 

ALL : "..\install\Release\vz.exe"

!ELSE 

ALL : "vzTTFont - Win32 Release" "vzMain - Win32 Release" "vzOutput - Win32 Release" "vzImage - Win32 Release" "..\install\Release\vz.exe"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"vzImage - Win32 ReleaseCLEAN" "vzOutput - Win32 ReleaseCLEAN" "vzMain - Win32 ReleaseCLEAN" "vzTTFont - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\tcpserver.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "..\install\Release\vz.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\vz.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\vz.pdb" /machine:I386 /out:"../install/Release/vz.exe" 
LINK32_OBJS= \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\tcpserver.obj" \
	"$(OUTDIR)\vzImage.lib" \
	"$(OUTDIR)\vzOutput.lib" \
	"$(OUTDIR)\vzMain.lib" \
	"$(OUTDIR)\vzTTFont.lib"

"..\install\Release\vz.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "vz - Win32 Debug"

OUTDIR=.\../tmp/Debug
INTDIR=.\../tmp/Debug/vz

!IF "$(RECURSE)" == "0" 

ALL : "..\install\Debug\vz.exe"

!ELSE 

ALL : "vzTTFont - Win32 Debug" "vzMain - Win32 Debug" "vzOutput - Win32 Debug" "vzImage - Win32 Debug" "..\install\Debug\vz.exe"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"vzImage - Win32 DebugCLEAN" "vzOutput - Win32 DebugCLEAN" "vzMain - Win32 DebugCLEAN" "vzTTFont - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\tcpserver.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\vz.pdb"
	-@erase "..\install\Debug\vz.exe"
	-@erase "..\install\Debug\vz.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Zp16 /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\vz.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\vz.pdb" /debug /machine:I386 /out:"../install/Debug/vz.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\tcpserver.obj" \
	"$(OUTDIR)\vzImage.lib" \
	"$(OUTDIR)\vzOutput.lib" \
	"$(OUTDIR)\vzMain.lib" \
	"$(OUTDIR)\vzTTFont.lib"

"..\install\Debug\vz.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("vz.dep")
!INCLUDE "vz.dep"
!ELSE 
!MESSAGE Warning: cannot find "vz.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "vz - Win32 Release" || "$(CFG)" == "vz - Win32 Debug"
SOURCE=.\main.cpp

"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\tcpserver.cpp

"$(INTDIR)\tcpserver.obj" : $(SOURCE) "$(INTDIR)"


!IF  "$(CFG)" == "vz - Win32 Release"

"vzImage - Win32 Release" : 
   cd "\work\m1-soft\graph-renderers\vz\vzImage"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzImage.mak" CFG="vzImage - Win32 Release" 
   cd "..\vz"

"vzImage - Win32 ReleaseCLEAN" : 
   cd "\work\m1-soft\graph-renderers\vz\vzImage"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzImage.mak" CFG="vzImage - Win32 Release" RECURSE=1 CLEAN 
   cd "..\vz"

!ELSEIF  "$(CFG)" == "vz - Win32 Debug"

"vzImage - Win32 Debug" : 
   cd "\work\m1-soft\graph-renderers\vz\vzImage"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzImage.mak" CFG="vzImage - Win32 Debug" 
   cd "..\vz"

"vzImage - Win32 DebugCLEAN" : 
   cd "\work\m1-soft\graph-renderers\vz\vzImage"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzImage.mak" CFG="vzImage - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\vz"

!ENDIF 

!IF  "$(CFG)" == "vz - Win32 Release"

"vzOutput - Win32 Release" : 
   cd "\work\m1-soft\graph-renderers\vz\vzOutput"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzOutput.mak" CFG="vzOutput - Win32 Release" 
   cd "..\vz"

"vzOutput - Win32 ReleaseCLEAN" : 
   cd "\work\m1-soft\graph-renderers\vz\vzOutput"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzOutput.mak" CFG="vzOutput - Win32 Release" RECURSE=1 CLEAN 
   cd "..\vz"

!ELSEIF  "$(CFG)" == "vz - Win32 Debug"

"vzOutput - Win32 Debug" : 
   cd "\work\m1-soft\graph-renderers\vz\vzOutput"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzOutput.mak" CFG="vzOutput - Win32 Debug" 
   cd "..\vz"

"vzOutput - Win32 DebugCLEAN" : 
   cd "\work\m1-soft\graph-renderers\vz\vzOutput"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzOutput.mak" CFG="vzOutput - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\vz"

!ENDIF 

!IF  "$(CFG)" == "vz - Win32 Release"

"vzMain - Win32 Release" : 
   cd "\work\m1-soft\graph-renderers\vz\vzMain"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzMain.mak" CFG="vzMain - Win32 Release" 
   cd "..\vz"

"vzMain - Win32 ReleaseCLEAN" : 
   cd "\work\m1-soft\graph-renderers\vz\vzMain"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzMain.mak" CFG="vzMain - Win32 Release" RECURSE=1 CLEAN 
   cd "..\vz"

!ELSEIF  "$(CFG)" == "vz - Win32 Debug"

"vzMain - Win32 Debug" : 
   cd "\work\m1-soft\graph-renderers\vz\vzMain"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzMain.mak" CFG="vzMain - Win32 Debug" 
   cd "..\vz"

"vzMain - Win32 DebugCLEAN" : 
   cd "\work\m1-soft\graph-renderers\vz\vzMain"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzMain.mak" CFG="vzMain - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\vz"

!ENDIF 

!IF  "$(CFG)" == "vz - Win32 Release"

"vzTTFont - Win32 Release" : 
   cd "\work\m1-soft\graph-renderers\vz\vzTTFont"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzTTFont.mak" CFG="vzTTFont - Win32 Release" 
   cd "..\vz"

"vzTTFont - Win32 ReleaseCLEAN" : 
   cd "\work\m1-soft\graph-renderers\vz\vzTTFont"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzTTFont.mak" CFG="vzTTFont - Win32 Release" RECURSE=1 CLEAN 
   cd "..\vz"

!ELSEIF  "$(CFG)" == "vz - Win32 Debug"

"vzTTFont - Win32 Debug" : 
   cd "\work\m1-soft\graph-renderers\vz\vzTTFont"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzTTFont.mak" CFG="vzTTFont - Win32 Debug" 
   cd "..\vz"

"vzTTFont - Win32 DebugCLEAN" : 
   cd "\work\m1-soft\graph-renderers\vz\vzTTFont"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzTTFont.mak" CFG="vzTTFont - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\vz"

!ENDIF 


!ENDIF 

