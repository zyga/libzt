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

# MacOS uses xcrun helper for some toolchain binaries.
xcrun := xcrun
bsd_tar_options := --no-mac-metadata
include $(srcdir)/.makefiles/Makefile.UNIX.mk

# Build and install the dynamic library
all:: libzt.dylib
clean::
	rm -f libzt.dylib libzt.1.dylib
install:: $(DESTDIR)$(libdir)/libzt.dylib
install:: $(DESTDIR)$(libdir)/libzt.1.dylib
uninstall::
	rm -f $(DESTDIR)$(libdir)/libzt.dylib
	rm -f $(DESTDIR)$(libdir)/libzt.1.dylib
libzt.1.dylib: LDFLAGS += -dynamiclib -fvisibility=hidden
libzt.1.dylib: LDFLAGS += -exported_symbols_list=$(srcdir)/libzt.export_list
libzt.1.dylib: LDFLAGS += -compatibility_version 1.0 -current_version 1.0
libzt.1.dylib: zt.o $(srcdir)/libzt.export_list
	$(strip $(LINK.o) $(filter %.o,$^) $(LIBS) -o $@)
libzt.dylib: | libzt.1.dylib
	ln -s $| $@
$(DESTDIR)$(libdir)/libzt.1.dylib: libzt.1.dylib | $(DESTDIR)$(libdir)
	install $^ $@
$(DESTDIR)$(libdir)/libzt.dylib: | $(DESTDIR)$(libdir)/libzt.1.dylib
	ln -s $(notdir $|) $@
