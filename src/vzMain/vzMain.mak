# Microsoft Developer Studio Generated NMAKE File, Based on vzMain.dsp
!IF "$(CFG)" == ""
CFG=vzMain - Win32 Debug
!MESSAGE No configuration specified. Defaulting to vzMain - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "vzMain - Win32 Release" && "$(CFG)" != "vzMain - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
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
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "vzMain - Win32 Release"

OUTDIR=.\../tmp/Release
INTDIR=.\../tmp/Release/vzMain

!IF "$(RECURSE)" == "0" 

ALL : "..\install\Release\vzMain.dll"

!ELSE 

ALL : "vzOutput - Win32 Release" "vzImage - Win32 Release" "..\install\Release\vzMain.dll"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"vzImage - Win32 ReleaseCLEAN" "vzOutput - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\unicode.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vzConfig.obj"
	-@erase "$(INTDIR)\vzContainer.obj"
	-@erase "$(INTDIR)\vzContainerFunction.obj"
	-@erase "$(INTDIR)\vzFunction.obj"
	-@erase "$(INTDIR)\vzFunctions.obj"
	-@erase "$(INTDIR)\vzMain.obj"
	-@erase "$(INTDIR)\vzMotion.obj"
	-@erase "$(INTDIR)\vzMotionControl.obj"
	-@erase "$(INTDIR)\vzMotionControlKey.obj"
	-@erase "$(INTDIR)\vzMotionDirector.obj"
	-@erase "$(INTDIR)\vzMotionParameter.obj"
	-@erase "$(INTDIR)\vzMotionTimeline.obj"
	-@erase "$(INTDIR)\vzScene.obj"
	-@erase "$(INTDIR)\vzXMLAttributes.obj"
	-@erase "$(INTDIR)\vzXMLParams.obj"
	-@erase "$(OUTDIR)\vzMain.exp"
	-@erase "$(OUTDIR)\vzMain.lib"
	-@erase "..\install\Release\vzMain.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Zp16 /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VZMAIN_EXPORTS" /Fp"$(INTDIR)\vzMain.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\vzMain.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\vzMain.pdb" /machine:I386 /out:"../install/Release/vzMain.dll" /implib:"$(OUTDIR)\vzMain.lib" 
LINK32_OBJS= \
	"$(INTDIR)\unicode.obj" \
	"$(INTDIR)\vzConfig.obj" \
	"$(INTDIR)\vzContainer.obj" \
	"$(INTDIR)\vzContainerFunction.obj" \
	"$(INTDIR)\vzFunction.obj" \
	"$(INTDIR)\vzFunctions.obj" \
	"$(INTDIR)\vzMain.obj" \
	"$(INTDIR)\vzMotion.obj" \
	"$(INTDIR)\vzMotionControl.obj" \
	"$(INTDIR)\vzMotionControlKey.obj" \
	"$(INTDIR)\vzMotionDirector.obj" \
	"$(INTDIR)\vzMotionParameter.obj" \
	"$(INTDIR)\vzMotionTimeline.obj" \
	"$(INTDIR)\vzScene.obj" \
	"$(INTDIR)\vzXMLAttributes.obj" \
	"$(INTDIR)\vzXMLParams.obj" \
	"$(OUTDIR)\vzImage.lib" \
	"$(OUTDIR)\vzOutput.lib"

"..\install\Release\vzMain.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "vzMain - Win32 Debug"

OUTDIR=.\../tmp/Debug
INTDIR=.\../tmp/Debug/vzMain

!IF "$(RECURSE)" == "0" 

ALL : "..\install\Debug\vzMain.dll"

!ELSE 

ALL : "vzOutput - Win32 Debug" "vzImage - Win32 Debug" "..\install\Debug\vzMain.dll"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"vzImage - Win32 DebugCLEAN" "vzOutput - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\unicode.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\vzConfig.obj"
	-@erase "$(INTDIR)\vzContainer.obj"
	-@erase "$(INTDIR)\vzContainerFunction.obj"
	-@erase "$(INTDIR)\vzFunction.obj"
	-@erase "$(INTDIR)\vzFunctions.obj"
	-@erase "$(INTDIR)\vzMain.obj"
	-@erase "$(INTDIR)\vzMotion.obj"
	-@erase "$(INTDIR)\vzMotionControl.obj"
	-@erase "$(INTDIR)\vzMotionControlKey.obj"
	-@erase "$(INTDIR)\vzMotionDirector.obj"
	-@erase "$(INTDIR)\vzMotionParameter.obj"
	-@erase "$(INTDIR)\vzMotionTimeline.obj"
	-@erase "$(INTDIR)\vzScene.obj"
	-@erase "$(INTDIR)\vzXMLAttributes.obj"
	-@erase "$(INTDIR)\vzXMLParams.obj"
	-@erase "$(OUTDIR)\vzMain.exp"
	-@erase "$(OUTDIR)\vzMain.lib"
	-@erase "$(OUTDIR)\vzMain.pdb"
	-@erase "..\install\Debug\vzMain.dll"
	-@erase "..\install\Debug\vzMain.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

"$(INTDIR)" :
    if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /Zp16 /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VZMAIN_EXPORTS" /Fp"$(INTDIR)\vzMain.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\vzMain.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\vzMain.pdb" /debug /machine:I386 /out:"../install/Debug/vzMain.dll" /implib:"$(OUTDIR)\vzMain.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\unicode.obj" \
	"$(INTDIR)\vzConfig.obj" \
	"$(INTDIR)\vzContainer.obj" \
	"$(INTDIR)\vzContainerFunction.obj" \
	"$(INTDIR)\vzFunction.obj" \
	"$(INTDIR)\vzFunctions.obj" \
	"$(INTDIR)\vzMain.obj" \
	"$(INTDIR)\vzMotion.obj" \
	"$(INTDIR)\vzMotionControl.obj" \
	"$(INTDIR)\vzMotionControlKey.obj" \
	"$(INTDIR)\vzMotionDirector.obj" \
	"$(INTDIR)\vzMotionParameter.obj" \
	"$(INTDIR)\vzMotionTimeline.obj" \
	"$(INTDIR)\vzScene.obj" \
	"$(INTDIR)\vzXMLAttributes.obj" \
	"$(INTDIR)\vzXMLParams.obj" \
	"$(OUTDIR)\vzImage.lib" \
	"$(OUTDIR)\vzOutput.lib"

"..\install\Debug\vzMain.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("vzMain.dep")
!INCLUDE "vzMain.dep"
!ELSE 
!MESSAGE Warning: cannot find "vzMain.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "vzMain - Win32 Release" || "$(CFG)" == "vzMain - Win32 Debug"
SOURCE=..\vz\unicode.cpp

"$(INTDIR)\unicode.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\vz\vzConfig.cpp

"$(INTDIR)\vzConfig.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\vz\vzContainer.cpp

"$(INTDIR)\vzContainer.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\vz\vzContainerFunction.cpp

"$(INTDIR)\vzContainerFunction.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\vz\vzFunction.cpp

"$(INTDIR)\vzFunction.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\vz\vzFunctions.cpp

"$(INTDIR)\vzFunctions.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\vz\vzMain.cpp

"$(INTDIR)\vzMain.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\vz\vzMotion.cpp

"$(INTDIR)\vzMotion.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\vz\vzMotionControl.cpp

"$(INTDIR)\vzMotionControl.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\vz\vzMotionControlKey.cpp

"$(INTDIR)\vzMotionControlKey.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\vz\vzMotionDirector.cpp

"$(INTDIR)\vzMotionDirector.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\vz\vzMotionParameter.cpp

"$(INTDIR)\vzMotionParameter.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\vz\vzMotionTimeline.cpp

"$(INTDIR)\vzMotionTimeline.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\vz\vzScene.cpp

"$(INTDIR)\vzScene.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\vz\vzXMLAttributes.cpp

"$(INTDIR)\vzXMLAttributes.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\vz\vzXMLParams.cpp

"$(INTDIR)\vzXMLParams.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!IF  "$(CFG)" == "vzMain - Win32 Release"

"vzImage - Win32 Release" : 
   cd "\work\m1-soft\graph-renderers\vz\vzImage"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzImage.mak" CFG="vzImage - Win32 Release" 
   cd "..\vzMain"

"vzImage - Win32 ReleaseCLEAN" : 
   cd "\work\m1-soft\graph-renderers\vz\vzImage"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzImage.mak" CFG="vzImage - Win32 Release" RECURSE=1 CLEAN 
   cd "..\vzMain"

!ELSEIF  "$(CFG)" == "vzMain - Win32 Debug"

"vzImage - Win32 Debug" : 
   cd "\work\m1-soft\graph-renderers\vz\vzImage"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzImage.mak" CFG="vzImage - Win32 Debug" 
   cd "..\vzMain"

"vzImage - Win32 DebugCLEAN" : 
   cd "\work\m1-soft\graph-renderers\vz\vzImage"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzImage.mak" CFG="vzImage - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\vzMain"

!ENDIF 

!IF  "$(CFG)" == "vzMain - Win32 Release"

"vzOutput - Win32 Release" : 
   cd "\work\m1-soft\graph-renderers\vz\vzOutput"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzOutput.mak" CFG="vzOutput - Win32 Release" 
   cd "..\vzMain"

"vzOutput - Win32 ReleaseCLEAN" : 
   cd "\work\m1-soft\graph-renderers\vz\vzOutput"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzOutput.mak" CFG="vzOutput - Win32 Release" RECURSE=1 CLEAN 
   cd "..\vzMain"

!ELSEIF  "$(CFG)" == "vzMain - Win32 Debug"

"vzOutput - Win32 Debug" : 
   cd "\work\m1-soft\graph-renderers\vz\vzOutput"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzOutput.mak" CFG="vzOutput - Win32 Debug" 
   cd "..\vzMain"

"vzOutput - Win32 DebugCLEAN" : 
   cd "\work\m1-soft\graph-renderers\vz\vzOutput"
   $(MAKE) /$(MAKEFLAGS) /F ".\vzOutput.mak" CFG="vzOutput - Win32 Debug" RECURSE=1 CLEAN 
   cd "..\vzMain"

!ENDIF 


!ENDIF 

