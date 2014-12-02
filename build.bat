@echo off
REM Requires Visual Studio ***32-bit*** command line environment!
REM Works on 2013; may work on earlier versions - no guarantees
ML /c fastfunctions.asm
CL waveparse.c /GS /GL /analyze- /W3 /Gy /Zc:wchar_t /Zi /Gm- /O2 /sdl /fp:precise /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_LIB" /D "_UNICODE" /D "UNICODE" /WX- /Zc:forScope /Gd /Oy- /Oi /MD /EHsc  /link fastfunctions.obj /OUT:"convolve.exe" /SAFESEH:NO