## Haiku Generic Makefile v2.6 ## 

## Fill in this file to specify the project being created, and the referenced
## Makefile-Engine will do all of the hard work for you. This handles any
## architecture of Haiku.

# The name of the binary.
NAME = Radio
ARCH = $(shell getarch)

V_MAJOR = 0
V_MIDDLE = 0
V_MINOR = 3
V_VARIETY = B_APPV_DEVELOPMENT
V_BUILD = 13

TARGET_DIR := ./dist
PACKAGE = $(TARGET_DIR)/$(NAME)_$(VERSION)-$(ARCH).hpkg

# The type of binary, must be one of:
#	APP:	Application
#	SHARED:	Shared library or add-on
#	STATIC:	Static library archive
#	DRIVER: Kernel driver
TYPE = APP

# 	If you plan to use localization, specify the application's MIME signature.
APP_MIME_SIG = application/x-vnd.Fishpond-Radio

#	The following lines tell Pe and Eddie where the SRCS, RDEFS, and RSRCS are
#	so that Pe and Eddie can fill them in for you.
#%{
# @src->@ 

#	Specify the source files to use. Full paths or paths relative to the 
#	Makefile can be included. All files, regardless of directory, will have
#	their object files created in the common object directory. Note that this
#	means this Makefile will not work correctly if two source files with the
#	same name (source.c or source.cpp) are included from different directories.
#	Also note that spaces in folder names do not work well with this Makefile.
SRCS := About.cpp \
	Expander.cpp \
	HttpUtils.cpp \
	MainWindow.cpp \
	RadioApp.cpp \
	RadioSettings.cpp \
	Station.cpp \
	StationFinder.cpp \
	StationFinderListenLive.cpp \
	StationFinderRadioNetwork.cpp \
	StationListView.cpp \
	StationPanel.cpp \
	StreamIO.cpp \
	StreamPlayer.cpp \
    Utils.cpp
	

#	Specify the resource definition files to use. Full or relative paths can be
#	used.
RDEFS = 

#	Specify the resource files to use. Full or relative paths can be used.
#	Both RDEFS and RSRCS can be utilized in the same Makefile.
RSRCS = Radio.rsrc

# End Pe/Eddie support.
# @<-src@ 
#%}

#	Specify libraries to link against.
#	There are two acceptable forms of library specifications:
#	-	if your library follows the naming pattern of libXXX.so or libXXX.a,
#		you can simply specify XXX for the library. (e.g. the entry for
#		"libtracker.so" would be "tracker")
#
#	-	for GCC-independent linking of standard C++ libraries, you can use
#		$(STDCPPLIBS) instead of the raw "stdc++[.r4] [supc++]" library names.
#
#	- 	if your library does not follow the standard library naming scheme,
#		you need to specify the path to the library and it's name.
#		(e.g. for mylib.a, specify "mylib.a" or "path/mylib.a")
LIBS = $(STDCPPLIBS) be translation bnetapi media xml2

#	Specify additional paths to directories following the standard libXXX.so
#	or libXXX.a naming scheme. You can specify full paths or paths relative
#	to the Makefile. The paths included are not parsed recursively, so
#	include all of the paths where libraries must be found. Directories where
#	source files were specified are	automatically included.
LIBPATHS = 

#	Additional paths to look for system headers. These use the form
#	"#include <header>". Directories that contain the files in SRCS are
#	NOT auto-included here.
SYSTEM_INCLUDE_PATHS = /boot/system/develop/headers/private/media \
        /boot/system/develop/headers/private/media/experimental \
        /boot/system/develop/headers/private/shared \
        /boot/system/develop/headers/libxml2 

#	Additional paths paths to look for local headers. These use the form
#	#include "header". Directories that contain the files in SRCS are
#	automatically included.
LOCAL_INCLUDE_PATHS = 

#	Specify the level of optimization that you want. Specify either NONE (O0),
#	SOME (O1), FULL (O2), or leave blank (for the default optimization level).
OPTIMIZE = FULL

# 	Specify the codes for languages you are going to support in this
# 	application. The default "en" one must be provided too. "make catkeys"
# 	will recreate only the "locales/en.catkeys" file. Use it as a template
# 	for creating catkeys for other languages. All localization files must be
# 	placed in the "locales" subdirectory.
LOCALES = 

#	Specify all the preprocessor symbols to be defined. The symbols will not
#	have their values set automatically; you must supply the value (if any) to
#	use. For example, setting DEFINES to "DEBUG=1" will cause the compiler
#	option "-DDEBUG=1" to be used. Setting DEFINES to "DEBUG" would pass
#	"-DDEBUG" on the compiler's command line.
DEFINES = HAIKU_TARGET_PLATFORM_HAIKU

#	Specify the warning level. Either NONE (suppress all warnings),
#	ALL (enable all warnings), or leave blank (enable default warnings).
WARNINGS = 

#	With image symbols, stack crawls in the debugger are meaningful.
#	If set to "TRUE", symbols will be created.
SYMBOLS := TRUE

#	Includes debug information, which allows the binary to be debugged easily.
#	If set to "TRUE", debug info will be created.
DEBUGGER := 

#	Specify any additional compiler flags to be used.
COMPILER_FLAGS = 

#	Specify any additional linker flags to be used.
LINKER_FLAGS = 

#	Specify the version of this binary. Example:
#		-app 3 4 0 d 0 -short 340 -long "340 "`echo -n -e '\302\251'`"1999 GNU GPL"
#	This may also be specified in a resource.
VERSION = $(V_MAJOR).$(V_MIDDLE).$(V_MINOR)-$(V_BUILD)
APP_VERSION := -app $(V_MAJOR) $(V_MIDDLE) $(V_MINOR) $(V_VARIETY) $(V_BUILD) -short "$(V_MAJOR).$(V_MIDDLE).$(V_MINOR)" -long "$(NAME) v$(VERSION) ©Fishpond 2017"

#	(Only used when "TYPE" is "DRIVER"). Specify the desired driver install
#	location in the /dev hierarchy. Example:
#		DRIVER_PATH = video/usb
#	will instruct the "driverinstall" rule to place a symlink to your driver's
#	binary in ~/add-ons/kernel/drivers/dev/video/usb, so that your driver will
#	appear at /dev/video/usb when loaded. The default is "misc".
DRIVER_PATH = 

VERSION_RDEF = $(NAME)-version.rdef
RDEFS += $(VERSION_RDEF)
	
## Include the Makefile-Engine
DEVEL_DIRECTORY = \
	$(shell findpaths -e B_FIND_PATH_DEVELOP_DIRECTORY etc/makefile-engine)
include $(DEVEL_DIRECTORY)

ifeq ($(DEBUGGER),TRUE)
	DEFINES += DEBUGGING TRACING
endif

PACKAGE_DIR = $(OBJ_DIR)/package
ARCH_PACKAGE_INFO = $(OBJ_DIR)/PackageInfo_$(ARCH)

$(ARCH_PACKAGE_INFO): Makefile
	cat ./PackageInfo | sed 's/$$VERSION/$(VERSION)/' | sed 's/$$ARCH/$(ARCH)/' > $(ARCH_PACKAGE_INFO)
	
$(PACKAGE): $(TARGET_DIR)/$(NAME) $(ARCH_PACKAGE_INFO)
	mkdir -p $(PACKAGE_DIR)/apps
	mkdir -p $(PACKAGE_DIR)/data/deskbar/menu/Applications
	cp $(TARGET_DIR)/$(NAME) $(PACKAGE_DIR)/apps/
	-ln -s ../../../../apps/$(NAME) $(PACKAGE_DIR)/data/deskbar/menu/Applications/$(NAME)
	package create -C $(PACKAGE_DIR)/ -i $(OBJ_DIR)/PackageInfo_$(ARCH) $(PACKAGE)
	copyattr -n BEOS:ICON $(TARGET_DIR)/$(NAME) $(PACKAGE)
	
package: $(PACKAGE)

clean: rdefclean distclean

rdefclean:
	-rm $(VERSION_RDEF)
	
distclean:
	-rm $(TARGET_DIR)/$(NAME)

$(VERSION_RDEF): Makefile
	echo "resource app_signature \"$(APP_MIME_SIG)\";" > $(VERSION_RDEF)
	echo "resource app_version {" >> $(VERSION_RDEF)
	echo "     major = $(V_MAJOR), middle = $(V_MIDDLE), minor = $(V_MINOR)," >> $(VERSION_RDEF)
	echo "     variety = B_APPV_ALPHA, internal = $(V_BUILD)," >> $(VERSION_RDEF)
	echo "     short_info=\"$(NAME)\"," >> $(VERSION_RDEF)
	echo "	   long_info=\"$(NAME) v$(VERSION), ©Fishpond 2017\"" >> $(VERSION_RDEF)
	echo "};" >> $(VERSION_RDEF)

install:: package
	pkgman install $(PACKAGE)
	


