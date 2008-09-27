TEMPLATE=app
CONFIG += debug

LIBS += -lfbclient

HEADERS += src/main.h   src/database.h   src/database_p.h
SOURCES += src/main.cpp src/database.cpp src/database_p.cpp

DEFINES += IBPP_LINUX

SOURCES += src/private/ibpp/core/all_in_one.cpp
