TEMPLATE=app
CONFIG += debug

unix:LIBS  += -lfbclient

HEADERS += src/main.h   src/database.h   src/database_p.h   src/sqlview.h
SOURCES += src/main.cpp src/database.cpp src/database_p.cpp src/sqlview.cpp

win32:DEFINES  += IBPP_WINDOWS
win32:DEFINES  -= UNICODE
unix:DEFINES  += IBPP_LINUX

SOURCES += src/private/ibpp/core/all_in_one.cpp
