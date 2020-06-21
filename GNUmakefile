# Copyright 2019-2020 Zygmunt Krynicki.
#
# This file is part of libzt.
#
# Libzt is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License.
#
# Libzt is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with Libzt.  If not, see <https://www.gnu.org/licenses/>.

NAME = libzt
VERSION = 0.3

srcdir ?= .
# Include optional generated makefile from the configuration system.
-include GNUmakefile.configure.mk
# Use bundled ZMK
include $(srcdir)/z.mk

$(eval $(call ZMK.Import,Toolchain))
$(eval $(call ZMK.Import,Configure))
$(eval $(call ZMK.Import,OS))
$(eval $(call ZMK.Import,GitVersion))

# Manual pages
manpages = \
	libzt-test.1 \
	libzt.3 \
	zt_check.3 \
	zt_claim.3 \
	ZT_CMP_BOOL.3 \
	ZT_CMP_INT.3 \
	ZT_CMP_PTR.3 \
	ZT_CMP_RUNE.3 \
	ZT_CMP_UINT.3 \
	ZT_CURRENT_LOCATION.3 \
	ZT_FALSE.3 \
	zt_location.3 \
	zt_location_at.3 \
	zt_main.3 \
	ZT_NOT_NULL.3 \
	ZT_NULL.3 \
	zt_pack_boolean.3 \
	zt_pack_integer.3 \
	zt_pack_nothing.3 \
	zt_pack_pointer.3 \
	zt_pack_rune.3 \
	zt_pack_string.3 \
	zt_pack_unsigned.3 \
	zt_test.3 \
	zt_test_case_func.3 \
	zt_test_suite_func.3 \
	ZT_TRUE.3 \
	zt_value.3 \
	zt_visit_test_case.3 \
	zt_visitor.3


# The release tarball.
$(NAME)_$(VERSION).tar.gz.Files += zt.h zt.c zt-test.c
$(NAME)_$(VERSION).tar.gz.Files += libzt.map libzt.export_list libzt.def
$(NAME)_$(VERSION).tar.gz.Files += configure GNUmakefile build.bat Makefile.nmake.mk
$(NAME)_$(VERSION).tar.gz.Files += examples/demo.c examples/test-root-user.c examples/GNUmakefile
$(NAME)_$(VERSION).tar.gz.Files += .pvs-studio.cfg
$(NAME)_$(VERSION).tar.gz.Files += README.md LICENSE NEWS
$(NAME)_$(VERSION).tar.gz.Files += $(foreach m,$(manpages),man/$m.in)
$(eval $(call ZMK.Expand,Tarball.Src,$(NAME)_$(VERSION).tar.gz))


# The custom configuration script sets the variable Configure.Configured
# In its absence provide defaults appropriate for each compiler.
ifeq (,$(Configure.Configured))
$(info NOTE: Build tree is not configured, using curated compiler options.)
$(info NOTE: Use ./configure to disable this mechanism)
# Optimize a little by default.
CFLAGS += -O2
# -Werror is enabled when building without a configuration file created
# by the configure script. This is explicitly meant to find bugs, break the
# build and be noticed.
CFLAGS += -Werror
# Extra careful when using gcc or clang.
ifneq (,$(or $(Toolchain.CC.IsGcc),$(Toolchain.CC.IsClang)))
CFLAGS += -Wall -Wextra -Wpedantic -Wconversion -Wchar-subscripts
endif # gcc || clang
# More careful than default when using watcom
ifneq (,$(Toolchain.CC.IsWatcom))
CFLAGS += -Wall -Wextra
endif
endif # !configured

# The configure script
configure.Interpreter = sh
configure.InstallDir = noinst
$(eval $(call ZMK.Expand,Script,configure))

# Public header file
$(eval $(call ZMK.Expand,Header,zt.h))

# Libraries (.a, .so and .dylib)
libzt.a.Sources = zt.c
$(eval $(call ZMK.Expand,Library.A,libzt.a))

ifeq ($(Toolchain.CC.ImageFormat),ELF)
libzt.so.1.Sources = zt.c
libzt.so.1.VersionScript = $(srcdir)/libzt.map
$(eval $(call ZMK.Expand,Library.So,libzt.so.1))
endif

ifeq ($(Toolchain.CC.ImageFormat),Mach-O)
libzt.1.dylib.Sources = zt.c
libzt.1.dylib.ExportList = $(srcdir)/libzt.export_list
$(eval $(call ZMK.Expand,Library.DyLib,libzt.1.dylib))
endif

# Manual pages, generated from .in files
all:: $(foreach m,$(manpages),man/$m)
clean::
	rm -f $(addprefix man/,$(manpages))
ifneq ($(srcdir),.)
	test -d man && rmdir man || :
endif
$(CURDIR)/man: # For out-of-tree builds.
	install -d $@
man/%: man/%.in | $(CURDIR)/man
	sed -e 's/@VERSION@/$(VERSION)/g' $< >$@
$(foreach m,$(manpages),$(eval $(call ZMK.Expand,ManPage,man/$m)))

# The test program
libzt-test.Sources = zt-test.c
libzt-test.SourcesCoverage = zt.c
$(eval $(call ZMK.Expand,Program.Test,libzt-test))
ifneq (,$(Toolchain.CC.IsGcc))
libzt-test: CFLAGS += -Wno-overlength-strings # Old gcc complains about the assert() in zt-test.c
endif

# Support formatting using clang-format, if installed.
ifneq ($(shell command -v clang-format 2>/dev/null),)
fmt:: $(wildcard $(srcdir)/*.[ch] $(srcdir)/examples/*.[ch])
	clang-format -i -style=WebKit $^
else
fmt:
	echo "error: clang-format not found, cannot format" >&2
	exit 1
endif

# Static analysis using PVS Studio.
PVS.Sources=zt.c zt-test.c
PLOG_CONVERTER_FLAGS += -d V1042 # libzt is FOSS, no need to tell us.
$(eval $(call ZMK.Import,PVS))

# Static analysis using Coverity
Coverity.Sources=zt.c zt.h zt-test.c
$(eval $(call ZMK.Import,Coverity))
