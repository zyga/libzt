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


# Use git to augment version.
$(eval $(call import,Module.git-version))

# The release tarball.
$(NAME)_$(VERSION).tar.gz.files += zt.h zt.c zt-test.c
$(NAME)_$(VERSION).tar.gz.files += libzt.map libzt.export_list libzt.def
$(NAME)_$(VERSION).tar.gz.files += configure GNUmakefile build.bat Makefile.nmake.mk
$(NAME)_$(VERSION).tar.gz.files += examples/demo.c examples/test-root-user.c examples/GNUmakefile
$(NAME)_$(VERSION).tar.gz.files += .pvs-studio.cfg
$(NAME)_$(VERSION).tar.gz.files += README.md LICENSE NEWS
$(NAME)_$(VERSION).tar.gz.files += $(foreach m,$(manpages),man/$m.in)
$(eval $(call spawn,Template.tarball.src,$(NAME)_$(VERSION).tar.gz))

# Compiling and configuration.
$(eval $(call import,Module.toolchain))
$(eval $(call import,Module.configure))
$(eval $(call import,Module.OS))

# The custom configuration script sets the variable CONFIGURED.
# In its absence provide defaults appropriate for each compiler.
ifeq (,$(CONFIGURED))
$(info NOTE: Build tree is not configured, using curated compiler options.)
$(info NOTE: Use ./configure to disable this mechanism)
# Optimize a little by default.
CFLAGS += -O2
# -Werror is enabled when building without a configuration file created
# by the configure script. This is explicitly meant to find bugs, break the
# build and be noticed.
CFLAGS += -Werror
# Extra careful when using gcc or clang.
ifneq (,$(or $(is_gcc),$(is_clang)))
CFLAGS += -Wall -Wextra -Wpedantic -Wconversion -Wchar-subscripts
endif # gcc || clang
# More careful than default when using watcom
ifneq (,$(is_watcom))
CFLAGS += -Wall -Wextra
endif
endif # !configured

# The configure script
configure.interp = sh
configure.install_dir = noinst
$(eval $(call spawn,Template.program.script,configure))

# Public header file
$(eval $(call spawn,Template.header,zt.h))

# Libraries (.a, .so and .dylib)
ifneq (,$(OS.has_a))
libzt.a.sources = zt.c
$(eval $(call spawn,Template.library.a,libzt.a))
endif

ifneq (,$(OS.has_elf))
libzt.so.sources = zt.c
libzt.so.soname = libzt.so.1
libzt.so.version-script = $(srcdir)/libzt.map
$(eval $(call spawn,Template.library.so,libzt.so))
endif

ifneq (,$(OS.has_macho))
libzt.dylib.sources = zt.c
libzt.dylib.soname = libzt.1.dylib
libzt.dylib.export-list = $(srcdir)/libzt.export_list
$(eval $(call spawn,Template.library.dylib,libzt.dylib))
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
$(foreach m,$(manpages),$(eval $(call spawn,Template.manpage,man/$m)))

# The test program
libzt-test.sources = zt-test.c
libzt-test.sources_coverage = zt.c
$(eval $(call spawn,Template.program.test,libzt-test))
ifneq (,$(is_gcc))
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
pvs.sources=zt.c zt-test.c
PLOG_CONVERTER_FLAGS += -d V1042 # libzt is FOSS, no need to tell us.
$(eval $(call import,Module.pvs))

# Static analysis using Coverity
coverity.sources=zt.c zt.h zt-test.c
$(eval $(call import,Module.coverity))
