!IF "$(CFG)" == ""
CFG=Debug
!MESSAGE Default configuration is Debug.
!ENDIF 

!IF "$(CFG)" != "Release" && "$(CFG)" != "Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE CFG="Debug" or NMAKE CFG="Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Release" 
!MESSAGE "Debug"
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 


!IF  "$(CFG)" == "Debug"
DEBUGFLAGS	= -D_DEBUG /RTC1 /EHsc /Zi /FD
INSTALL_DIR	= ../../install/Debug/plugins/
OBJS_DIR	= ../../tmp/Debug/plugins/
LIB_PATH	= /LIBPATH:"../../tmp/Debug/"
LINKER_OPT	= /LDd /nologo 
LINKER_FLAGS	= $(LIB_PATH) /DEBUG
CPPFLAGS	= $(CPPFLAGS) /MDd /Zp16
!ENDIF


!IF "$(CFG)" == "Release"
# Release version
DEBUGFLAGS	= 
INSTALL_DIR	= ../../install/Release/plugins/
OBJS_DIR	= ../../tmp/Release/plugins/
LIB_PATH	= /LIBPATH:"../../tmp/Release/"
LINKER_OPT	= /LD /nologo 
LINKER_FLAGS	= $(LIB_PATH)
CPPFLAGS	= $(CPPFLAGS) /MD /Zp16
!ENDIF

# compiler setting
CPPFLAGS	= $(CPPFLAGS) -TP
INCLUDE_DIR	= -I. -I../vz
LINKER		= /incremental:no $(LINKER_FLAGS)

# created modules
TARGET = "$(OBJS_DIR)" "$(INSTALL_DIR)"		\
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
