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

include $(srcdir)/.makefiles/Makefile.UNIX.mk

# Build the libzt-test executable as a position-independent-executable (hardening).
# In addition inject the dependency on libzt.so.1
# TODO: add code coverage support
libzt-test: LDFLAGS += -fPIE

# Build and install the dynamic library.
all:: libzt.so.1
clean::
	rm -f libzt.so libzt.so.1
install:: $(DESTDIR)$(libdir)/libzt.so
install:: $(DESTDIR)$(libdir)/libzt.so.1
uninstall::
	rm -f $(DESTDIR)$(libdir)/libzt.so
	rm -f $(DESTDIR)$(libdir)/libzt.so.1
# On Linux compile all object files with -fPIC to allow them being
# used in shared libraries.
%.o: CFLAGS += -fPIC
libzt.so.1: LDFLAGS += -shared -fvisibility=hidden
libzt.so.1: LDFLAGS += -Wl,-soname=libzt.so.1 -Wl,--version-script=$(srcdir)/libzt.map
libzt.so.1: zt.o
	$(strip $(LINK.o) $^ $(LIBS) -o $@)
libzt.so: | libzt.so.1
	ln -s $| $@
$(DESTDIR)$(libdir)/libzt.so.1: libzt.so.1 | $(DESTDIR)$(libdir)
	install $^ $@
$(DESTDIR)$(libdir)/libzt.so: | $(DESTDIR)$(libdir)/libzt.so.1
	ln -s $(notdir $|) $@
