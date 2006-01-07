# Microsoft Developer Studio Generated NMAKE File, Based on vzOutput.dsp
!IF "$(CFG)" == ""
CFG=vzOutput - Win32 Debug
!MESSAGE No configuration specified. Defaulting to vzOutput - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "vzOutput - Win32 Release" && "$(CFG)" != "vzOutput - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vzOutput.mak" CFG="vzOutput - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vzOutput - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "vzOutput - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "vzOutput - Win32 Release"

OUTDIR=.\../tmp/Release
INTDIR=.\../tmp/Release/vzOutput

!IF "$(RECURSE)" == "0" 

ALL : "..\install\Release\vzOutput.dll"

!ELSE 

ALL : "vzImage - Win32 Release" "..\install\Release\vzOutput.dll"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"vzImage - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vzOutput.obj"
	-@erase "$(OUTDIR)\vzOutput.exp"
	-@erase "$(OUTDIR)\vzOutput.lib"
	-@erase "..\install\Release\vzOutput.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VZOUTPUT_EXPORTS" /Fp"$(INTDIR)\vzOutput.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\vzOutput.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\vzOutput.pdb" /machine:I386 /out:"../install/Release/vzOutput.dll" /implib:"$(OUTDIR)\vzOutput.lib" 
LINK32_OBJS= \
	"$(INTDIR)\vzOutput.obj" \
	"$(OUTDIR)\vzImage.lib"

"..\install\Release\vzOutput.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "vzOutput - Win32 Debug"

OUTDIR=.\../tmp/Debug
INTDIR=.\../tmp/Debug/vzOutput

!IF "$(RECURSE)" == "0" 

ALL : "..\install\Debug\vzOutput.dll"

!ELSE 

ALL : "vzImage - Win32 Debug" "..\install\Debug\vzOutput.dll"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"vzImage - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\vzOutput.obj"
	-@erase "$(OUTDIR)\vzOutput.exp"
	-@erase "$(OUTDIR)\vzOutput.lib"
	-@erase "$(OUTDIR)\vzOutput.pdb"
	-@erase "..\install\Debug\vzOutput.dll"
	-@erase "..\install\Debug\vzOutput.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VZOUTPUT_EXPORTS" /Fp"$(INTDIR)\vzOutput.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\vzOutput.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\vzOutput.pdb" /debug /machine:I386 /out:"../install/Debug/vzOutput.dll" /implib:"$(OUTDIR)\vzOutput.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\vzOutput.obj" \
	"$(OUTDIR)\vzImage.lib"

"..\install\Debug\vzOutput.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("vzOutput.dep")
!INCLUDE "vzOutput.dep"
!ELSE 
!MESSAGE Warning: cannot find "vzOutput.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "vzOutput - Win32 Release" || "$(CFG)" == "vzOutput - Win32 Debug"
SOURCE=..\vz\vzOutput.cpp

"$(INTDIR)\vzOutput.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!IF  "$(CFG)" == "vzOutput - Win32 Release"

"vzImage - Win32 Release" : 
   cd "\work\m1-soft\graph-renderers\vz\vzImage"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzImage.mak" CFG="vzImage - Win32 Release" 
   cd "..\vzOutput"

"vzImage - Win32 ReleaseCLEAN" : 
   cd "\work\m1-soft\graph-renderers\vz\vzImage"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzImage.mak" CFG="vzImage - Win32 Release" RECURSE=1 CLEAN 
   cd "..\vzOutput"

!ELSEIF  "$(CFG)" == "vzOutput - Win32 Debug"

"vzImage - Win32 Debug" : 
   cd "\work\m1-soft\graph-renderers\vz\vzImage"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzImage.mak" CFG="vzImage - Win32 Debug" 
   cd "..\vzOutput"

"vzImage - Win32 DebugCLEAN" : 
   cd "\work\m1-soft\graph-renderers\vz\vzImage"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzImage.mak" CFG="vzImage - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\vzOutput"

!ENDIF 


!ENDIF 

