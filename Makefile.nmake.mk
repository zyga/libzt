# Disable some pointless warnings:
# C4820 - padding inside a structure
# C4100 - function with unused argument
# C4996 - use of deprecated symbol
# C4710 - function was not inlined
# C5045 - side-channel mitigations inserted by the compiler
!if [set CL=/nologo /Wall /wd4820 /wd4100 /wd4996 /wd4710 /wd5045]
!endif

all: libzt-test.exe zt.dll
clean:
	del /F zt.obj zt-test.obj libzt-test.exe zt.dll zt.lib zt.exp
check: libzt-test.exe
	libzt-test.exe

libzt-test.exe: zt-test.obj
	cl /Fe: $@ $?
zt.dll: zt.obj
	cl /LD /DEF libzt.def /Fe: $@ $?
zt.obj: zt.c zt.h
	cl /c zt.c
zt-test.obj: zt-test.c zt.c zt.h
	cl /c zt-test.c
