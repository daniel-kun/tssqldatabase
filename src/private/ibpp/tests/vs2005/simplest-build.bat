@REM $Id: simplest-build.bat 46 2006-03-15 15:03:34Z epocman $
@MKDIR bin
@cl /DIBPP_WINDOWS /O2 /Oi /W4 /EHsc /I..\..\core /Fobin\ ..\tests.cpp ..\..\core\all_in_one.cpp advapi32.lib
