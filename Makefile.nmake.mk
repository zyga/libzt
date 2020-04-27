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