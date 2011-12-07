!IF "$(CFG)" == ""
CFG=Debug32
!MESSAGE Default configuration is Debug.
!ENDIF 

!IF "$(CFG)" != "Release32" && "$(CFG)" != "Debug32" && "$(CFG)" != "Release64" && "$(CFG)" != "Debug64"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE CFG="Debug32" or NMAKE CFG="Release32"
!MESSAGE or
!MESSAGE NMAKE CFG="Debug64" or NMAKE CFG="Release64"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Release32" 
!MESSAGE "Debug32"
!MESSAGE "Release64" 
!MESSAGE "Debug64"
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

VCINSTALLDIR	=	C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\

VSINSTALLDIR	=	C:\Program Files (x86)\Microsoft Visual Studio 9.0\

WindowsSdkDir	=	C:\Program Files\Microsoft SDKs\Windows\v7.0\

INCLUDES	=	-I"$(VCINSTALLDIR)\include" \
			-I"$(WindowsSdkDir)\Include" \
			-I"C:\SDK\curl\include"

LIB_PATH_64	=	/LIBPATH:"C:\SDK\curl\lib\x64"
LIB_PATH_32	=	/LIBPATH:"C:\SDK\curl\lib\win32"

!IF "$(CFG)" == "Debug64"
CPP		= "$(VCINSTALLDIR)\bin\x86_amd64\cl.exe"
DEBUGFLAGS	= -D_DEBUG /RTC1 /EHsc /Zi /FD
INSTALL_DIR	= ../../install/Debug-x64/plugins/
OBJS_DIR	= ../../temp/x64/Debug/plugins/
LIB_PATH	= /LIBPATH:"../../temp/x64/Debug/" /LIBPATH:"$(WindowsSdkDir)\lib\x64" /LIBPATH:"$(VCINSTALLDIR)\lib\amd64" $(LIB_PATH_64)
LINKER_OPT	= /LDd /nologo 
LINKER_FLAGS	= $(LIB_PATH) /DEBUG /Profile
CPPFLAGS	= $(CPPFLAGS) /MDd /Zp16
!ENDIF

!IF "$(CFG)" == "Release64"
# Release version
CPP		= "$(VCINSTALLDIR)\bin\x86_amd64\cl.exe"
DEBUGFLAGS	= 
INSTALL_DIR	= ../../install/Release-x64/plugins/
OBJS_DIR	= ../../temp/x64/Release/plugins/
LIB_PATH	= /LIBPATH:"../../temp/x64/Release/" /LIBPATH:"$(WindowsSdkDir)\lib\x64" /LIBPATH:"$(VCINSTALLDIR)\lib\amd64" $(LIB_PATH_64)
LINKER_OPT	= /LD /nologo 
LINKER_FLAGS	= $(LIB_PATH)
CPPFLAGS	= $(CPPFLAGS) /MD /Zp16
!ENDIF

!IF "$(CFG)" == "Debug32"
DEBUGFLAGS	= -D_DEBUG /RTC1 /EHsc /Zi /FD
INSTALL_DIR	= ../../install/Debug-win32/plugins/
OBJS_DIR	= ../../temp/win32/Debug/plugins/
LIB_PATH	= /LIBPATH:"../../temp/win32/Debug/" /LIBPATH:"$(WindowsSdkDir)\lib" /LIBPATH:"$(VCINSTALLDIR)\lib" $(LIB_PATH_32)
LINKER_OPT	= /LDd /nologo 
LINKER_FLAGS	= $(LIB_PATH) /DEBUG /Profile
CPPFLAGS	= $(CPPFLAGS) /MDd /Zp16
!ENDIF


!IF "$(CFG)" == "Release32"
# Release version
DEBUGFLAGS	= 
INSTALL_DIR	= ../../install/Release-win32/plugins/
OBJS_DIR	= ../../temp/win32/Release/plugins/
LIB_PATH	= /LIBPATH:"../../temp/win32/Release/" /LIBPATH:"$(WindowsSdkDir)\lib" /LIBPATH:"$(VCINSTALLDIR)\lib" $(LIB_PATH_32)
LINKER_OPT	= /LD /nologo 
LINKER_FLAGS	= $(LIB_PATH)
CPPFLAGS	= $(CPPFLAGS) /MD /Zp16
!ENDIF


# compiler setting
CPPFLAGS	= $(CPPFLAGS) -TP
INCLUDE_DIR	= -I. -I../vz $(INCLUDES)
LINKER		= /incremental:no $(LINKER_FLAGS)

# created modules
TARGET = "$(OBJS_DIR)" "$(INSTALL_DIR)"		\
	pixeltransfer.dll			\
	fillarc.dll				\
	url_frame_notify.dll			\
	text_seq.dll				\
	ctrl_director.dll			\
	trajectory_live.dll			\
        blend.dll                               \
	ttfont_hcrawl.dll			\
	ttfont.dll 				\
	liveinput.dll				\
	ctrl_container.dll			\
	avifile.dll				\
	trajectory_table.dll			\
	image.dll 				\
	equipe_timer.dll			\
	systime.dll				\
	timer.dll				\
	mask.dll				\
	translate.dll				\
	rotate.dll				\
	alpha.dll				\
	rectangle.dll				\
	scale.dll

all: $(TARGET)

"$(OBJS_DIR)":
	@echo Creating directory ".\$(OBJS_DIR)"
	if not exist "$(OBJS_DIR)" mkdir ".\$(OBJS_DIR)"

"$(INSTALL_DIR)":
	@echo Creating directory ".\$(INSTALL_DIR)"
	if not exist "$(INSTALL_DIR)" mkdir ".\$(INSTALL_DIR)"	

.cpp.dll:
	@echo $(INCLUDE)
	@SET INCLUDE=$(INCLUDE)
	@echo Compiling:$< ... $(<:.cpp=.obj)
	$(CPP) \
		-c -nologo $(DEBUGFLAGS) $(CPPFLAGS) $(INCLUDE_DIR) \
		-Fo$(OBJS_DIR)$(<:.cpp=.obj) \
		-Fd$(INSTALL_DIR)$(<:.cpp=.pdb) $<

	@echo Linking:$(<:.cpp=.obj) ... $@
	$(CPP) $(LINKER_OPT) \
		-Fe$(INSTALL_DIR)$@ $(OBJS_DIR)$(<:.cpp=.obj) \
		/link $(LINKER) /implib:$(OBJS_DIR)$(<:.cpp=.lib) \
		/PDB:$(INSTALL_DIR)$(<:.cpp=.pdb)

	@SET PATH=$(PATH);$(INSTALL_DIR)../
	@vzPluginsMan.exe \
		$(INSTALL_DIR)$(<:.cpp=.dll) \
		$(INSTALL_DIR)$(<:.cpp=.man)
