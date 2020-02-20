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
VERSION = 0.2

# Meta-targets that don't have specific specific commands
.PHONY: $(sort all clean coverage fmt static-check check install uninstall dist distclean)
# Default goal is to build everything, regardless of declaration order
.DEFAULT_GOAL = all

# Include optional generated makefile from the configuration system.
srcdir ?= .
-include GNUmakefile.configure.mk

# Include platform specific makefile.
include $(srcdir)/.makefiles/Makefile.$(shell uname -s).mk
