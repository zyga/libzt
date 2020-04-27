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

pvs.sources ?= $(error define pvs.sources)

PLOG_CONVERTER_FLAGS ?=
_pvs_studio_path := $(shell command -v pvs-studio 2>/dev/null)

# If we have pvs-studio then run it during static checks.
ifneq (,$(_pvs_studio_path))
static-check:: static-check-pvs
endif

.PHONY: static-check-pvs
static-check-pvs: $(addsuffix .PVS-Studio.log,$(pvs.sources))
	$(strip plog-converter \
		--settings $(srcdir)/.pvs-studio.cfg \
		$(PLOG_CONVERTER_FLAGS) \
		--srcRoot $(srcdir) \
		--renderTypes errorfile $^ | srcdir=$(srcdir) abssrcdir=$(abspath $(srcdir)) awk -f $(Core.Path)/pvs-filter.awk)

pvs-report: $(addsuffix .PVS-Studio.log,$(pvs.sources))
	$(strip plog-converter \
		--settings $(srcdir)/.pvs-studio.cfg \
		$(PLOG_CONVERTER_FLAGS) \
		--srcRoot $(srcdir) \
		--projectName $(NAME) \
		--projectVersion $(VERSION) \
		--renderTypes fullhtml \
		--output $@ \
		$^)

%.c.PVS-Studio.log: %.c.i ~/.config/PVS-Studio/PVS-Studio.lic | %.c
	$(strip pvs-studio \
		--cfg $(srcdir)/.pvs-studio.cfg \
		--i-file $< \
		--source-file $(firstword $|) \
		--output-file $@)

%.c.i: %.c
	$(strip $(CC) $(CPPFLAGS) $< -E -o $@)

clean::
	rm -f *.i
	rm -f *.PVS-Studio.log
	rm -rf pvs-report
