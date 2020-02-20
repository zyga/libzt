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

# Compiler defaults unless changed by GNUmakefile.configure.mk
CPPFLAGS ?=
CFLAGS ?= -Wall -Wextra -Wconversion -Wpedantic -Wchar-subscripts -Werror -O2
# On older compilers assert() in zt-test.c generates large strings but it's
# not something we care about strongly.
CFLAGS += -Wno-overlength-strings
ARFLAGS = -cr
TARGET_ARCH ?=
LIBS ?=

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


# Install developer manual pages
install:: $(sort \
	$(addprefix $(DESTDIR)$(man1dir)/,$(notdir $(wildcard man/*.1))) \
	$(addprefix $(DESTDIR)$(man2dir)/,$(notdir $(wildcard man/*.2))) \
	$(addprefix $(DESTDIR)$(man3dir)/,$(notdir $(wildcard man/*.3))) \
	$(addprefix $(DESTDIR)$(man4dir)/,$(notdir $(wildcard man/*.4))) \
	$(addprefix $(DESTDIR)$(man5dir)/,$(notdir $(wildcard man/*.5))) \
	$(addprefix $(DESTDIR)$(man6dir)/,$(notdir $(wildcard man/*.6))) \
	$(addprefix $(DESTDIR)$(man7dir)/,$(notdir $(wildcard man/*.7))) \
	$(addprefix $(DESTDIR)$(man8dir)/,$(notdir $(wildcard man/*.8))) \
	$(addprefix $(DESTDIR)$(man9dir)/,$(notdir $(wildcard man/*.9))))
uninstall::
	rm -f $(sort \
		$(addprefix $(DESTDIR)$(man1dir)/,$(notdir $(wildcard man/*.1))) \
		$(addprefix $(DESTDIR)$(man2dir)/,$(notdir $(wildcard man/*.2))) \
		$(addprefix $(DESTDIR)$(man3dir)/,$(notdir $(wildcard man/*.3))) \
		$(addprefix $(DESTDIR)$(man4dir)/,$(notdir $(wildcard man/*.4))) \
		$(addprefix $(DESTDIR)$(man5dir)/,$(notdir $(wildcard man/*.5))) \
		$(addprefix $(DESTDIR)$(man6dir)/,$(notdir $(wildcard man/*.6))) \
		$(addprefix $(DESTDIR)$(man7dir)/,$(notdir $(wildcard man/*.7))) \
		$(addprefix $(DESTDIR)$(man8dir)/,$(notdir $(wildcard man/*.8))) \
		$(addprefix $(DESTDIR)$(man9dir)/,$(notdir $(wildcard man/*.9))))
$(DESTDIR)$(man1dir)/%.1: man/%.1 | $(DESTDIR)$(man1dir)
	install $^ $@
$(DESTDIR)$(man2dir)/%.2: man/%.2 | $(DESTDIR)$(man2dir)
	install $^ $@
$(DESTDIR)$(man3dir)/%.3: man/%.3 | $(DESTDIR)$(man3dir)
	install $^ $@
$(DESTDIR)$(man4dir)/%.4: man/%.4 | $(DESTDIR)$(man4dir)
	install $^ $@
$(DESTDIR)$(man5dir)/%.5: man/%.5 | $(DESTDIR)$(man5dir)
	install $^ $@
$(DESTDIR)$(man6dir)/%.6: man/%.6 | $(DESTDIR)$(man6dir)
	install $^ $@
$(DESTDIR)$(man7dir)/%.7: man/%.7 | $(DESTDIR)$(man7dir)
	install $^ $@
$(DESTDIR)$(man8dir)/%.8: man/%.8 | $(DESTDIR)$(man8dir)
	install $^ $@
$(DESTDIR)$(man9dir)/%.9: man/%.9 | $(DESTDIR)$(man9dir)
	install $^ $@


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
check:: libzt-test
	./$<
install:: $(DESTDIR)$(bindir)/libzt-test
uninstall::
	rm -f $(DESTDIR)$(bindir)/libzt-test
libzt-test: zt-test.o
	$(strip $(LINK.o) $^ $(LIBS) -o $@)
$(DESTDIR)$(bindir)/libzt-test: libzt-test | $(DESTDIR)$(bindir)
	install $^ $@
zt-test.o: zt-test.c zt.c zt.h
zt-test.o: CFLAGS += -g

# We may need to add some options to BSD tar, this is where we store them.
bsd_tar_options ?=

# Create the release archive. Instructions on how to make it
# differ based on flavor of the tools used.
dist:: $(NAME)_$(VERSION).tar.gz
$(NAME)_$(VERSION).tar.gz: $(sort $(addprefix $(srcdir)/, \
	zt.c zt.h zt-test.c libzt.map libzt.export_list \
	man/ZT_CMP_BOOL.3 man/ZT_CMP_INT.3 man/ZT_CMP_RUNE.3 man/ZT_CMP_UINT.3 \
	man/ZT_CURRENT_LOCATION.3 man/ZT_FALSE.3 man/ZT_NOT_NULL.3 \
	man/ZT_NULL.3 man/ZT_TRUE.3 man/libzt-test.1 man/libzt.3 man/zt_check.3 \
	man/zt_claim.3 man/zt_location.3 man/zt_location_at.3 man/zt_main.3 \
	man/zt_pack_boolean.3 man/zt_pack_integer.3 man/zt_pack_nothing.3 \
	man/zt_pack_pointer.3 man/zt_pack_rune.3 man/zt_pack_string.3 \
	man/zt_pack_unsigned.3 man/zt_test.3 man/zt_test_case_func.3 \
	man/zt_test_suite_func.3 man/zt_value.3 man/zt_visit_test_case.3 \
	man/zt_visitor.3 examples/demo.c examples/test-root-user.c \
	examples/GNUmakefile configure GNUmakefile .makefiles/Makefile.Darwin.mk \
	.makefiles/Makefile.Linux.mk .makefiles/Makefile.UNIX.mk .pvs-filter.awk \
	.pvs-studio.cfg README.md LICENSE NEWS))

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

%.c.PVS-Studio.log: %.c.i | %.c
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
endif

# GNU man can be used to perform rudimentary validation of manual pages.
ifneq ($(and $(shell command -v man 2>/dev/null),$(shell man --help 2>&1 | grep -F -- --warning)),)
static-check:: $(wildcard $(srcdir)/man/*.3)
	LC_ALL=en_US.UTF-8 MANROFFSEQ='' MANWIDTH=80 man --warnings -E UTF-8 -l -Tutf8 -Z $^ 2>&1 >/dev/null | diff -u - /dev/null
endif

# Run static checkers when checking.
check:: static-check
