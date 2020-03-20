@echo off
set CL=/nologo /Wall /wd4820 /wd4100 /wd4996 /wd4710 /wd5045
cl /c zt.c
cl /c zt-test.c
cl zt-test.obj /Fe: libzt-test.exe
cl /LD /DEF libzt.def zt.obj /Fe: zt.dll
