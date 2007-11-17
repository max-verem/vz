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
STREAMALPHA_SDK = c:\sdk\streamalpha\SDK_Code
STREAMALPHA_LIB	= $(STREAMALPHA_SDK)\Lib
STREAMALPHA_INC	= $(STREAMALPHA_SDK)\Inc

# Bluefish SDK
BLUEFISH_SDK	= c:\sdk\bluefish
BLUEFISH_LIB	= $(BLUEFISH_SDK)\Lib
BLUEFISH_INC	= $(BLUEFISH_SDK)\Inc

# Decklink SDK
DECKLINK_SDK	= c:\sdk\decklink
DECKLINK_LIB	= $(DECKLINK_SDK)\Lib
DECKLINK_INC	= $(DECKLINK_SDK)\Include

# BaseClasses (Platform SDK)
BASECLASSES_SDK = C:\Program Files\Microsoft Platform SDK for Windows Server 2003 R2
BASECLASSES_LIB	= $(BASECLASSES_SDK)\Lib\BaseClasses
BASECLASSES_INC	= $(BASECLASSES_SDK)\Samples\Multimedia\DirectShow\BaseClasses

!IF  "$(CFG)" == "Debug"
DEBUGFLAGS	= -D_DEBUG /RTC1 /EHsc /Zi /FD
INSTALL_DIR	= ../../install/Debug/outputs/
OBJS_DIR	= ../../tmp/Debug/outputs/
LIB_PATH	= /LIBPATH:"../../tmp/Debug/"
LINKER_OPT	= /LDd
LINKER_FLAGS	= $(LIB_PATH) /DEBUG
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
SDK_LIB		= \
		/LIBPATH:"$(STREAMALPHA_LIB)" \
		/LIBPATH:"$(BLUEFISH_LIB)" \
		/LIBPATH:"$(DECKLINK_LIB)" \
		/LIBPATH:"$(BASECLASSES_LIB)"

SDK_INC		= \
		-I"$(STREAMALPHA_INC)" \
		-I"$(BLUEFISH_INC)" \
		-I"$(DECKLINK_INC)" \
		-I"$(BASECLASSES_INC)"

#!MESSAGE SDK_INC=$(SDK_INC)
#!MESSAGE SDK_LIB=$(SDK_LIB)
#!ERROR

# compiler setting
CPPFLAGS	= $(CPPFLAGS) -TP
LIB_PATH	= $(LIB_PATH) $(SDK_LIB)
INCLUDE_DIR	= -I. -I../vz $(SDK_INC) -I$(OBJS_DIR)
LINKER		= /incremental:no $(LINKER_FLAGS)

# created modules
TARGET = decklink bluefish nullvideo streamalpha

all: "$(OBJS_DIR)" "$(INSTALL_DIR)" $(TARGET)

"$(OBJS_DIR)":
	@echo Creating directory ".\$(OBJS_DIR)"
	@if not exist "$(OBJS_DIR)" mkdir ".\$(OBJS_DIR)"

"$(INSTALL_DIR)":
	@echo Creating directory ".\$(INSTALL_DIR)"
	@if not exist "$(INSTALL_DIR)" mkdir ".\$(INSTALL_DIR)"	


.cpp.dll:
	@echo Compiling:$< ... $(<:.cpp=.obj)
	$(CPP) -c -nologo $(DEBUGFLAGS) $(CPPFLAGS) $(INCLUDE_DIR) \
		-Fo$(OBJS_DIR)$(<:.cpp=.obj) \
		-Fd$(INSTALL_DIR)$(<:.cpp=.pdb) \
		$<

	@echo Linking:$(<:.cpp=.obj) ... $@
	@$(CPP) $(LINKER_OPT) \
		-Fe$(INSTALL_DIR)$@ $(OBJS_DIR)$(<:.cpp=.obj) /link  \
		$(LINKER) /implib:$(OBJS_DIR)$(<:.cpp=.lib) \
		/PDB:$(INSTALL_DIR)$(<:.cpp=.pdb)

# stream alpha depends on streamalpha SDK (www.streamalpha.com)
# please modify STREAMALPHA_SDK for proper location
streamalpha: streamalpha_sdk streamalpha.dll

streamalpha_sdk: "$(STREAMALPHA_LIB)" "$(STREAMALPHA_INC)" 

"$(STREAMALPHA_LIB)":
	@if not exist "$(STREAMALPHA_LIB)" echo "STREAMALPHA_SDK incorrect"

"$(STREAMALPHA_INC)":
	@if not exist "$(STREAMALPHA_INC)" echo "STREAMALPHA_SDK incorrect"

# check bluefish sdk
bluefish: bluefish_sdk bluefish.cpp bluefish.dll

bluefish_sdk: "$(BLUEFISH_LIB)" "$(BLUEFISH_INC)"

"$(BLUEFISH_LIB)":
	@if not exist "$(BLUEFISH_LIB)" echo "BLUEFISH_LIB incorrect"

"$(BLUEFISH_INC)":
	@if not exist "$(BLUEFISH_INC)" echo "BLUEFISH_SDK incorrect"


# check decklink sdk
decklink: decklink_sdk decklink.cpp vzdssrc.h decklink.dll

decklink_sdk: "$(DECKLINK_LIB)" "$(DECKLINK_INC)" "$(BASECLASSES_LIB)" "$(BASECLASSES_INC)"

"$(DECKLINK_LIB)":
	@if not exist "$(DECKLINK_LIB)" echo "DECKLINK_LIB incorrect"

"$(DECKLINK_INC)":
	@if not exist "$(DECKLINK_INC)" echo "DECKLINK_SDK incorrect"

"$(BASECLASSES_LIB)":
	@if not exist "$(BASECLASSES_LIB)" echo "BASECLASSES_LIB incorrect"

"$(BASECLASSES_INC)":
	@if not exist "$(BASECLASSES_INC)" echo "BASECLASSES_SDK incorrect"


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
