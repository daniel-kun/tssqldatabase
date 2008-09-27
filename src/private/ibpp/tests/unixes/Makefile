###############################################################################
#
#	File    : $Id: Makefile 57 2006-04-02 17:44:00Z epocman $
#	Subject : IBPP 'tests' - Multi-Platform GNU Makefile
#
###############################################################################
#
#	(C) Copyright 2000-2006 T.I.P. Group S.A. and the IBPP Team (www.ibpp.org)
#
#	The contents of this file are subject to the IBPP License (the "License");
#	you may not use this file except in compliance with the License.  You may
#	obtain a copy of the License at http://www.ibpp.org or in the 'license.txt'
#	file which must have been distributed along with this file.
#
#	This software, distributed under the License, is distributed on an "AS IS"
#	basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the
#	License for the specific language governing rights and limitations
#	under the License.
#	
#	Contributor(s):	Matt Hortman, initial developer of this Makefile
#	                Olivier Mascia, maintainer
#
###############################################################################
#
#	COMMENTS
#
#	* Tabulations should be set every four characters when editing this file.
#
###############################################################################

#
#	Detect Platform
#

ifeq ($(shell uname),Darwin)
	#	Darwin (OS X) Target Platform
	PLATFORM=darwin
else
ifeq ($(shell uname),Linux)
	#	Linux Target Platform
	PLATFORM=linux
else
	#	Win32 Target Platform
	ifeq ($(findstring mingw32,$(shell gcc -v 2>&1)),mingw32)
		PLATFORM=windows_mingw
	else	
	ifeq ($(findstring cygwin,$(shell gcc -v 2>&1)),cygwin)
		PLATFORM=windows_cygwin
	endif
	endif
endif	
endif

# set target directories
ROOTDIR:=$(shell pwd)
ifeq ($(DEBUG),)
	DEBUG=0
endif
ifneq ($(DEBUG),0)
	TARGETDIR=debug
	DS=d
else
	TARGETDIR=release
	DS=
endif

# *************************************************
# Source Files
# *************************************************

HDRS =	../../core/ibpp.h

HDRS +=	../../core/_ibpp.h
HDRS +=	../../core/ibase.h
HDRS +=	../../core/iberror.h

APP_SRCS =		tests.cpp

CORE_SRCS =		_ibpp.cpp
CORE_SRCS +=	_dpb.cpp
CORE_SRCS +=	_ibs.cpp
CORE_SRCS +=	_rb.cpp
CORE_SRCS +=	_spb.cpp
CORE_SRCS +=	_tpb.cpp
CORE_SRCS +=	array.cpp
CORE_SRCS +=	blob.cpp
CORE_SRCS +=	database.cpp
CORE_SRCS +=	dbkey.cpp
CORE_SRCS +=	events.cpp
CORE_SRCS +=	exception.cpp
CORE_SRCS +=	service.cpp
CORE_SRCS +=	row.cpp
CORE_SRCS +=	statement.cpp
CORE_SRCS +=	transaction.cpp
CORE_SRCS +=	date.cpp
CORE_SRCS +=	time.cpp
CORE_SRCS +=	user.cpp

# *************************************************
# Conditional Compile Options
# *************************************************

# building on darwin (OS X)
ifeq ($(PLATFORM),darwin)
	CXXOUTPUT =	-o
	CXXFLAGS =-DIBPP_DARWIN
	CXXFLAGS+=-W -Wall -fPIC
	#
	LIBS+=-framework Firebird -lm -ldl -lpthread
	ifeq ($(TARGETDIR),release)
		CXXFLAGS+= -O3
	else
		CXXFLAGS+= -g -DDEBUG
	endif
	#
	TARGETS =	$(TARGETDIR)/tests
endif

# building on linux (any flavour, I suppose)
ifeq ($(PLATFORM),linux)
	CXXOUTPUT =	-o
	CXXFLAGS =-DIBPP_LINUX
	CXXFLAGS+=-W -Wall -fPIC
	#
	# change -lfbclient by -lgds if building with Firebird 1.x or InterBase
	# -lfbclient (or -lgds) is for Firebird 1.5 SS and higher
	# -lfbembed is for Firebird 1.5 CS and higher
	#
	LIBS+=-lfbclient -lcrypt -lm -ldl -lpthread # dependancies for shared library
	ifeq ($(TARGETDIR),release)
		CXXFLAGS+= -O3
	else
		CXXFLAGS+= -g -DDEBUG
	endif
	#
	TARGETS =	$(TARGETDIR)/tests
endif

# building with mingw (MinGW 3.0)
ifeq ($(PLATFORM),windows_mingw)
	CXXOUTPUT =	-o
	CXXFLAGS =-DIBPP_WINDOWS
	CXXFLAGS+=-Wall -mthreads -D_MT -DWIN32_LEAN_AND_MEAN
	ifeq ($(TARGETDIR),release)
		CXXFLAGS+= -O3
	else
		CXXFLAGS+= -ggdb -DDEBUG -Wall
	endif
	#
	TARGETS =	$(TARGETDIR)/tests.exe
endif

# building with cygwin
ifeq ($(PLATFORM),windows_cygwin)
	CXXOUTPUT =	-o
	CXXFLAGS =-DIBPP_WINDOWS
	CXXFLAGS+=-Wall -D_MT -DWIN32_LEAN_AND_MEAN
	ifeq ($(TARGETDIR),release)
		CXXFLAGS+= -O3
	else
		CXXFLAGS+= -ggdb -DDEBUG -Wall
	endif
	#
	TARGETS =	$(TARGETDIR)/tests.exe
endif

# *************************************************
# Derived Definitions
# *************************************************

# make an object from each source file
APP_OBJS:=$(addprefix $(TARGETDIR)/,$(addsuffix .o,$(basename $(APP_SRCS))))
CORE_OBJS:=$(addprefix $(TARGETDIR)/core/,$(addsuffix .o,$(basename $(CORE_SRCS))))

# *************************************************
# Build Rules
# *************************************************

# don't check for existance of files named:
.PHONY: checks tests clean reallyclean runtests

#don't delete when generated indirectly
.SECONDARY: $(HDRS) $(APP_SRCS) $(CORE_SRCS)

all: checks $(TARGETS)

checks:
ifeq ($(PLATFORM),)
	$(warning Unsupported or unrecognized platform.)
	$(error Please read the howtobuild.txt file)
endif
	@if [ ! -d $(TARGETDIR) ]; then mkdir -p $(TARGETDIR); fi
	@if [ ! -d $(TARGETDIR)/core ]; then mkdir -p $(TARGETDIR)/core; fi
	@echo ""
	@echo "Platform : $(PLATFORM)"
	@echo "Target   : $(TARGETDIR)"
	@echo ""

#
#	PRODUCTION RULES : TRULY COMMON TO ALL PLATFORMS
#

# disable internal rules
%.o: %.cpp

# our own specific compilation rule
$(TARGETDIR)/core/%.o: ../../core/%.cpp $(HDRS)
		$(CXX) $(CXXFLAGS) $(CXXOUTPUT)$@ -c $<

$(TARGETDIR)/%.o: ../%.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) $(CXXOUTPUT)$@ -c $<

#
#	PRODUCTION RULES : SHARED BETWEEN LINUX, DARWIN
#

ifeq ($(findstring $(PLATFORM),linux darwin),$(PLATFORM))
$(TARGETDIR)/tests : $(APP_OBJS) $(CORE_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(APP_OBJS) $(CORE_OBJS) $(LIBS)
endif

#
#	PRODUCTION RULES : SHARED BETWEEN MINGW, CYGWIN
#

ifeq ($(findstring $(PLATFORM),windows_mingw windows_cygwin),$(PLATFORM))
$(TARGETDIR)/tests.exe : $(APP_OBJS) $(CORE_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ_SRCS) $(CORE_OBJS)
endif

#
#	PRODUCTION RULES : SPECIAL TARGETS
#

clean: checks
	@rm -rf $(TARGETDIR)
	@echo "CLEANED ./$(TARGETDIR)"

reallyclean:
	@rm -rf debug 
	@rm -rf release
	@echo "REALLY CLEANED ./debug and ./release"

runtests: checks $(TARGETS)
	@echo ""
	@echo "Now running tests programs..."
	@cd $(TARGETDIR); ./tests

#
#	EOF
#
