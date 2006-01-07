# Microsoft Developer Studio Generated NMAKE File, Based on vzImage.dsp
!IF "$(CFG)" == ""
CFG=vzImage - Win32 Debug
!MESSAGE No configuration specified. Defaulting to vzImage - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "vzImage - Win32 Release" && "$(CFG)" != "vzImage - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vzImage.mak" CFG="vzImage - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "vzImage - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "vzImage - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "vzImage - Win32 Release"

OUTDIR=.\../tmp/Release
INTDIR=.\../tmp/Release/vzImage

ALL : "..\install\Release\vzImage.dll"


CLEAN :
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vzImage.obj"
	-@erase "$(OUTDIR)\vzImage.exp"
	-@erase "$(OUTDIR)\vzImage.lib"
	-@erase "..\install\Release\vzImage.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VZIMAGE_EXPORTS" /Fp"$(INTDIR)\vzImage.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\vzImage.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\vzImage.pdb" /machine:I386 /out:"../install/Release/vzImage.dll" /implib:"$(OUTDIR)\vzImage.lib" 
LINK32_OBJS= \
	"$(INTDIR)\vzImage.obj"

"..\install\Release\vzImage.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "vzImage - Win32 Debug"

OUTDIR=.\../tmp/Debug
INTDIR=.\../tmp/Debug/vzImage

ALL : "..\install\Debug\vzImage.dll"


CLEAN :
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\vzImage.obj"
	-@erase "$(OUTDIR)\vzImage.exp"
	-@erase "$(OUTDIR)\vzImage.lib"
	-@erase "$(OUTDIR)\vzImage.pdb"
	-@erase "..\install\Debug\vzImage.dll"
	-@erase "..\install\Debug\vzImage.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VZIMAGE_EXPORTS" /Fp"$(INTDIR)\vzImage.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\vzImage.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\vzImage.pdb" /debug /machine:I386 /out:"../install/Debug/vzImage.dll" /implib:"$(OUTDIR)\vzImage.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\vzImage.obj"

"..\install\Debug\vzImage.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("vzImage.dep")
!INCLUDE "vzImage.dep"
!ELSE 
!MESSAGE Warning: cannot find "vzImage.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "vzImage - Win32 Release" || "$(CFG)" == "vzImage - Win32 Debug"
SOURCE=..\vz\vzImage.cpp

"$(INTDIR)\vzImage.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

