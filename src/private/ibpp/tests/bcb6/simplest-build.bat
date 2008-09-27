@REM $Id: simplest-build.bat 46 2006-03-15 15:03:34Z epocman $
@MKDIR bin
@bcc32 -DIBPP_WINDOWS -O2 -w -I..\..\core -nbin ..\tests.cpp ..\..\core\all_in_one.cpp
