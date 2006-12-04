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

# StreamAlpha SDK path
STREAMALPHA_LIB	= c:\sdk\streamalpha\SDK_Code\Lib
STREAMALPHA_INC	= c:\sdk\streamalpha\SDK_Code\Inc

!IF  "$(CFG)" == "Debug"
DEBUGFLAGS	= -D_DEBUG /Z7 /Yd /GZ
INSTALL_DIR	= ../../install/Debug/outputs/
OBJS_DIR	= ../../tmp/Debug/outputs/
LIB_PATH	= /LIBPATH:"../../tmp/Debug/"
LINKER_OPT	= /LDd
LINKER_FLAGS	= $(LIB_PATH)
CPPFLAGS	= $(CPPFLAGS) /MDd /Zm256
!ENDIF

!IF  "$(CFG)" == "Release"
# Release version
DEBUGFLAGS	= 
INSTALL_DIR	= ../../install/Release/outputs/
OBJS_DIR	= ../../tmp/Release/outputs/
LIB_PATH	= /LIBPATH:"../../tmp/Release/"
LINKER_OPT	= /LD
LINKER_FLAGS	= $(LIB_PATH)
CPPFLAGS	= $(CPPFLAGS) /MD /Zm256
!ENDIF

# additional sdks
SDK_LIB		=  /LIBPATH:"$(STREAMALPHA_LIB)"
SDK_INC		=  -I"$(STREAMALPHA_INC)"

# compiler setting
CPPFLAGS	= $(CPPFLAGS) /G6 -TP -GD 
LIB_PATH	= $(LIB_PATH) $(SDK_LIB)
INCLUDE_DIR	= -I. -I../vz $(SDK_INC) -I$(OBJS_DIR)
LINKER		= /incremental:no $(LINKER_FLAGS)

# created modules
TARGET = nullvideo streamalpha

all: "$(OBJS_DIR)" "$(INSTALL_DIR)" $(TARGET)

"$(OBJS_DIR)":
	@echo Creating directory ".\$(OBJS_DIR)"
	@if not exist "$(OBJS_DIR)" mkdir ".\$(OBJS_DIR)"

"$(INSTALL_DIR)":
	@echo Creating directory ".\$(INSTALL_DIR)"
	@if not exist "$(INSTALL_DIR)" mkdir ".\$(INSTALL_DIR)"	


.cpp.dll:
	@echo Compiling:$< ... $(<:.cpp=.obj)
	@$(CPP) -c -nologo $(DEBUGFLAGS) $(CPPFLAGS) $(INCLUDE_DIR) -Fo$(OBJS_DIR)$(<:.cpp=.obj) $<
	@echo Linking:$(<:.cpp=.obj) ... $@
	@$(CPP) $(LINKER_OPT) -Fe$(INSTALL_DIR)$@ $(OBJS_DIR)$(<:.cpp=.obj) /link $(LINKER) /implib:$(OBJS_DIR)$(<:.cpp=.lib)

# stream alpha depends on streamalpha SDK (www.streamalpha.com)
# please modify STREAMALPHA_LIB and STREAMALPHA_INC for proper
# location
streamalpha: streamalpha_sdk streamalpha.dll

streamalpha_sdk: "$(STREAMALPHA_LIB)" "$(STREAMALPHA_INC)" 

"$(STREAMALPHA_LIB)":
	@if not exist "$(STREAMALPHA_LIB)" echo "STREAMALPHA_LIB incorrect"

"$(STREAMALPHA_INC)":
	@if not exist "$(STREAMALPHA_INC)" echo "STREAMALPHA_INC incorrect"

# NULL output module - fake for testing....
nullvideo: nullvideo.cpp nullvideo.dll

nullvideo.cpp: nullvideo.tp_bars.h nullvideo.tp_grid.h nullvideo.tp_lines.h

nullvideo.tp_bars.h: nullvideo.tp_bars.tga
	@echo Converting $**
	@SET PATH=$(PATH);$(INSTALL_DIR:outputs/=)
	@tga2h $** $(OBJS_DIR)$(**:.tga=.h) tp_bars

nullvideo.tp_grid.h: nullvideo.tp_grid.tga
	@echo Converting $**
	@SET PATH=$(PATH);$(INSTALL_DIR:outputs/=)
	@tga2h $** $(OBJS_DIR)$(**:.tga=.h) tp_grid

nullvideo.tp_lines.h: nullvideo.tp_lines.tga
	@echo Converting $**
	@SET PATH=$(PATH);$(INSTALL_DIR:outputs/=)
	@tga2h $** $(OBJS_DIR)$(**:.tga=.h) tp_lines
