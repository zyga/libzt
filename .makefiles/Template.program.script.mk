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

$(eval $(call import,Module.directories))

# Shellcheck can be used to check scripts
ifneq ($(shell command -v shellcheck 2>/dev/null),)
static-check-shellcheck:
	shellcheck $^
static-check:: static-check-shellcheck
endif

Template.program.script.variables=interp install_dir
define Template.program.script.spawn
$1.interp ?= $$(error define $1.interp)
$1.install_dir ?= $$(bindir)

ifneq ($$($1.install_dir),noinst)
install:: $$(DESTDIR)$$($1.install_dir)/$1
uninstall::
	rm -f $$(DESTDIR)$$($1.install_dir)/$1
$$(DESTDIR)$$($1.install_dir)/$1: $1 | $$(DESTDIR)$$($1.install_dir)
	install $$^ $$@
endif

ifneq ($$(findstring $$($1.interp),sh bash),)
static-check-shellcheck: $1
endif
endef


