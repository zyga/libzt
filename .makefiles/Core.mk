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

NAME ?= $(error define NAME)
VERSION ?= $(error define VERSION)

# Location of include files used by the makefile system
Core.Path = $(srcdir)/.makefiles
# Modules present in the makefile system
Core.Modules = \
				Module.configure Module.directories Module.git-version \
				Module.OS Module.toolchain Module.pvs Module.coverity \
				Template.header Template.library.a Template.library.dylib \
				Template.library.so Template.manpage Template.program.test \
				Template.program.script Template.tarball
# Files bundled by distribution tarballs
Core.DistFiles = $(addprefix $(Core.Path)/,Core.mk $(foreach m,$(Core.Modules),$m.mk) pvs-filter.awk configure)

# Meta-targets that don't have specific specific commands
.PHONY: $(sort all clean coverage fmt static-check check install uninstall dist distclean)

# Run static checkers when checking
check:: static-check

# Default goal is to build everything, regardless of declaration order
.DEFAULT_GOAL = all

# Display diagnostic messages when DEBUG has specific items.
_comma=,
DEBUG ?= 
DEBUG := $(subst $(_comma), ,$(DEBUG))

# List of imported modules.
MODULES ?=

# Define the module and template system.

define import
ifeq (,$1)
$$(error import, expected module name)
endif

ifeq (,$$(findstring $1,$$(MODULES)))
$$(if $$(findstring import,$$(DEBUG)),$$(info DEBUG: importing »$1«))
include $$(srcdir)/.makefiles/$1.mk
MODULES += $1
endif
endef

define spawn
ifeq (,$1)
$$(error spawn, expected module name)
endif
ifeq (,$2)
$$(error spawn, expected variable name)
endif

$$(eval $$(call import,$1))
$$(if $$(findstring spawn,$$(DEBUG)),$$(info DEBUG: spawning »$1« as »$2«))
$$(eval $$(call $1.spawn,$2))
$$(if $$(findstring spawn,$$(DEBUG)),$$(foreach n,$$($1.variables),$$(info DEBUG:     instance variable »$2«.$$n=$$($2.$$n))))
endef
