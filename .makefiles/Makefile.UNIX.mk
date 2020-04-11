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

# Deduce the kind of the selected compiler. Some build rules or compiler
# options depend on the compiler used. As an alternative we could look at
# preprocessor macros but this way seems sufficient for now.
_cc := $(shell sh -c "command -v $(CC)")
ifeq ($(_cc),/usr/bin/cc)
_cc := $(realpath $(_cc))
endif
is_gcc=$(if $(findstring gcc,$(_cc)),yes)
is_clang=$(if $(findstring clang,$(_cc)),yes)
is_watcom=$(if $(findstring watcom,$(_cc)),yes)
is_tcc=$(if $(findstring tcc,$(_cc)),yes)
_cc_kind=$(or $(if $(is_gcc),gcc),$(if $(is_clang),clang),$(if $(is_watcom),watcom),$(if $(is_tcc),tcc))

# Craft a better version if we have Git.
ifneq ($(shell command -v git 2>/dev/null),)
VERSION := $(or $(shell GIT_DIR=$(srcdir)/.git git describe --abbrev=10 --tags 2>/dev/null),$(VERSION))
endif

# Compiler defaults unless changed by GNUmakefile.configure.mk
CPPFLAGS ?=
CFLAGS ?=
CFLAGS ?=
ARFLAGS = -cr
TARGET_ARCH ?=
LDLIBS ?=
LDFLAGS ?=

# Configuration system defaults, also changed by GNUMakefile.configure.mk
CONFIGURED ?=
HOST_ARCH_TRIPLET ?=
BUILD_ARCH_TRIPLET ?=

# The custom configuration script sets the variable CONFIGURED.
# In its absence provide defaults appropriate for each compiler.
ifeq ($(CONFIGURED),)
$(info Build tree is not configured, using curated compiler options.)
$(info Use ./configure to disable this mechanism)
ifneq ($(or $(is_gcc),$(is_clang)),)
CFLAGS += -Wall -Wextra -Wpedantic -Wconversion -Wchar-subscripts
endif
ifneq ($(is_watcom),)
CFLAGS += -Wall -Wextra
endif
CFLAGS += -O2
# -Werror is enabled when building without a configuration file created
# by the configure script. This is explicitly meant to find bugs, break the
# build and be noticed.
CFLAGS += -Werror
else # CONFIGURED
# If we are configured then check for cross compilation by mismatch
# of host and build triplets. When this happens set CC and CXX.
ifneq ($(BUILD_ARCH_TRIPLET),$(HOST_ARCH_TRIPLET))
ifeq ($(origin CC),default)
CC = $(HOST_ARCH_TRIPLET)-gcc
endif
ifeq ($(origin CXX),default)
CXX = $(HOST_ARCH_TRIPLET)-g++
endif
endif
endif

ifeq ($(_cc_kind),gcc)
# Older gcc complains about the assert() in zt-test.c
zt-test.o: CFLAGS += -Wno-overlength-strings
endif

# Installation location
DESTDIR ?=

# Relevant UNIX-y directories.
prefix ?= /usr/local
exec_prefix ?= $(prefix)
bindir ?= $(exec_prefix)/bin
sbindir ?= $(exec_prefix)/sbin
libexecdir ?= $(exec_prefix)/libexec
datarootdir ?= $(prefix)/share
datadir ?= $(datarootdir)
sysconfdir ?= $(prefix)/etc
sharedstatedir ?= $(prefix)/com
localstatedir ?= $(prefix)/var
runstatedir ?= $(localstatedir)/run
includedir ?= $(prefix)/include
oldincludedir ?= $(prefix)/include
docdir ?= $(datarootdir)/doc/$(NAME)
infodir ?= $(datarootdir)info
libdir ?= $(exec_prefix)/lib
localedir ?= $(datarootdir)/locale
mandir ?= $(datarootdir)/man
man1dir ?= $(mandir)/man1
man2dir ?= $(mandir)/man2
man3dir ?= $(mandir)/man3
man4dir ?= $(mandir)/man4
man5dir ?= $(mandir)/man5
man6dir ?= $(mandir)/man6
man7dir ?= $(mandir)/man7
man8dir ?= $(mandir)/man8
man9dir ?= $(mandir)/man9

# Create standard directories on demand.
$(sort $(DESTDIR) $(addprefix $(DESTDIR), \
	$(prefix) $(exec_prefix) $(bindir) $(sbindir) $(libexecdir) \
	$(datarootdir) $(datadir) $(sysconfdir) $(sharedstatedir) \
	$(localstatedir) $(runstatedir) $(includedir) $(oldincludedir) \
	$(docdir) $(infodir) $(libdir) $(localedir) $(mandir) $(man1dir) \
	$(man2dir) $(man3dir) $(man4dir) $(man5dir) $(man6dir) $(man7dir) \
	$(man8dir) $(man9dir))):
	install -d $@


# Remove the generate makefile when dist-cleaning
distclean:: clean
	rm -f GNUmakefile.configure.mk


# The zt.o object file depends on those files.
zt.o: zt.c zt.h
clean::
	rm -f *.o


# Install the public header file.
install:: $(DESTDIR)$(includedir)/zt.h
uninstall::
	rm -f $(DESTDIR)$(includedir)/zt.h
$(DESTDIR)$(includedir)/zt.h: zt.h | $(DESTDIR)$(includedir)
	install $^ $@

# Build generated manual pages
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

man_sections = 1 2 3 4 5 6 7 8 9

# Build all manual pages.
all:: $(addprefix man/,$(manpages))
# GNU man can be used to perform rudimentary validation of manual pages.
ifneq ($(and $(shell command -v man 2>/dev/null),$(shell man --help 2>&1 | grep -F -- --warning)),)
static-check:: $(addprefix man/,$(manpages))
	LC_ALL=C MANROFFSEQ='' MANWIDTH=80 man --warnings -E UTF-8 -l -Tutf8 -Z $^ 2>&1 >/dev/null | diff -u - /dev/null
endif
# Install install the built manual pages.
install:: $(sort $(foreach n,$(man_sections),$(addprefix $(DESTDIR)$(man$ndir)/,$(notdir $(filter %.$n,$(manpages))))))
# Remove the installed manual pages.
uninstall::
	rm -f $(sort $(foreach n,$(man_sections),$(addprefix $(DESTDIR)$(man$ndir)/,$(notdir $(filter %.$n,$(manpages))))))
# Remove all built manual pages.
clean::
	rm -f $(addprefix man/,$(manpages))
ifneq ($(srcdir),.)
	test -d man && rmdir man || :
endif
$(CURDIR)/man:
	install -d $@
man/%: man/%.in | $(CURDIR)/man
	sed -e 's/@VERSION@/$(VERSION)/g' $< >$@
define man_tmpl
$$(DESTDIR)$$(man$1dir)/%.$1: man/%.$1 | $$(DESTDIR)$$(man$1dir)
	install $$^ $$@
endef
$(foreach n,$(man_sections),$(eval $(call man_tmpl,$n)))

# Build and install the static library.
all:: libzt.a
clean::
	rm -f libzt.a
install:: $(DESTDIR)$(libdir)/libzt.a
uninstall::
	rm -f $(DESTDIR)$(libdir)/libzt.a
libzt.a: zt.o
	$(AR) $(ARFLAGS) $@ $^
$(DESTDIR)$(libdir)/libzt.a: libzt.a | $(DESTDIR)$(libdir)
	install $^ $@


# Build and install the test executable.
all:: libzt-test
clean::
	rm -f libzt-test
ifeq ($(BUILD_ARCH_TRIPLET),$(HOST_ARCH_TRIPLET))
check:: libzt-test
	./$<
else
check::
	echo "Not running unit tests when cross compiling"
endif

install:: $(DESTDIR)$(bindir)/libzt-test
uninstall::
	rm -f $(DESTDIR)$(bindir)/libzt-test
libzt-test: zt-test.o
	$(strip $(LINK.o) $^ $(LDLIBS) -o $@)
$(DESTDIR)$(bindir)/libzt-test: libzt-test | $(DESTDIR)$(bindir)
	install $^ $@
zt-test.o: zt-test.c zt.c zt.h
zt-test.o: CFLAGS += -g

# We may need to add some options to BSD tar, this is where we store them.
bsd_tar_options ?=

makefiles = \
	Makefile.Darwin.mk \
	Makefile.FreeBSD.mk \
	Makefile.GNU-kFreeBSD.mk \
	Makefile.GNU.mk \
	Makefile.Linux.mk \
	Makefile.NetBSD.mk \
	Makefile.OpenBSD.mk \
	Makefile.SunOS.mk \
	Makefile.UNIX.mk \
	Makefile.Windows.mk

# Create the release archive. Instructions on how to make it
# differ based on flavor of the tools used.
dist:: $(NAME)_$(VERSION).tar.gz
$(NAME)_$(VERSION).tar.gz: $(sort $(addprefix $(srcdir)/, \
	zt.c zt.h zt-test.c \
	libzt.map libzt.export_list libzt.def \
	configure GNUmakefile $(foreach f,$(makefiles),.makefiles/$f) build.bat \
	$(foreach f,$(manpages),man/$f.in) \
	examples/demo.c examples/test-root-user.c examples/GNUmakefile \
	.pvs-filter.awk .pvs-studio.cfg README.md LICENSE NEWS))

ifneq ($(shell command -v tar 2>/dev/null),)
ifneq ($(shell tar --version 2>&1 | grep GNU),)
$(NAME)_$(VERSION).tar.gz:
	# Using GNU tar options
	tar -zcf $@ --xform='s@^@$(NAME)-$(VERSION)/@g' $^
else
$(NAME)_$(VERSION).tar.gz:
	# Using BSD tar options
	tar $(bsd_tar_options) -zcf $@ -s '@.@$(NAME)-$(VERSION)/~@' $^
endif
endif

# xcrun is the helper for accessing toolchain programs on MacOS
# It is defined as empty for non-Darwin build environments.
xcrun ?=

# Support coverage analysis when building with clang and supplied with llvm
# or when using xcrun.
ifneq ($(or $(xcrun),$(and $(findstring clang,$(CC)),$(shell command -v llvm-cov 2>/dev/null),$(shell command -v llvm-profdata 2>/dev/null))),)
# Build libzt-test with code coverage measurements and show them via "coverage" target.
libzt-test: CFLAGS += -fcoverage-mapping -fprofile-instr-generate
libzt-test: LDFLAGS += -fcoverage-mapping -fprofile-instr-generate
libzt-test.profraw: %.profraw: %
	LLVM_PROFILE_FILE=$@ ./$^
libzt-test.profdata: %.profdata: %.profraw
	$(strip $(xcrun) llvm-profdata merge -sparse $< -o $@)
coverage:: libzt-test.profdata
	$(strip $(xcrun) llvm-cov show ./libzt-test -instr-profile=$< $(srcdir)/zt.c)

.PHONY: coverage-todo
coverage-todo:: libzt-test.profdata
	$(strip $(xcrun) llvm-cov show ./libzt-test -instr-profile=$< -region-coverage-lt=100 $(srcdir)/zt.c)

.PHONY: coverage-report
coverage-report:: libzt-test.profdata
	$(strip $(xcrun) llvm-cov report ./libzt-test -instr-profile=$< $(srcdir)/zt.c)
endif
clean::
	rm -f *.profdata *.profraw


# Support for static analysis using PVS Studio, if installed.
ifneq ($(shell command -v pvs-studio 2>/dev/null),)
clean::
	rm -f *.i
	rm -f *.PVS-Studio.log
	rm -rf pvs-report

%.c.i: %.c
	$(strip $(CC) $(CPPFLAGS) $< -E -o $@)

%.c.PVS-Studio.log: %.c.i ~/.config/PVS-Studio/PVS-Studio.lic | %.c
	$(strip pvs-studio \
		--cfg $(srcdir)/.pvs-studio.cfg \
		--i-file $< \
		--source-file $(firstword $|) \
		--output-file $@)

pvs-report: zt.c.PVS-Studio.log zt-test.c.PVS-Studio.log
	$(strip plog-converter \
		--settings $(srcdir)/.pvs-studio.cfg \
		--srcRoot $(srcdir) \
		--projectName $(NAME) \
		--projectVersion $(VERSION) \
		--renderTypes fullhtml \
		--output $@ \
		$^)

static-check:: zt.c.PVS-Studio.log zt-test.c.PVS-Studio.log
	$(strip plog-converter \
		--settings $(srcdir)/.pvs-studio.cfg \
		--srcRoot $(srcdir) \
		--renderTypes errorfile $^ | srcdir=$(srcdir) abssrcdir=$(abspath $(srcdir)) awk -f $(srcdir)/.pvs-filter.awk)
endif

# Support for static analysis with Coverity, if installed.
ifneq ($(shell command -v cov-build 2>/dev/null),)
clean::
	rm -rf cov-int
	rm -f libzt-coverity.tar.gz
cov-int: zt.c zt.h zt-test.c $(MAKEFILE_LIST)
	cov-build --dir $@ $(MAKE)
libzt-coverity.tar.gz: cov-int
	tar zcf $@ $<
# NOTE: coverity scan upload is not automated.
endif

# Support for static analysis using shellcheck, if installed.
ifneq ($(shell command -v shellcheck 2>/dev/null),)
static-check:: configure
	shellcheck $^
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

# Run static checkers when checking.
check:: static-check
