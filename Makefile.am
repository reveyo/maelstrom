ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects

EXTRA_DIST = \
AUTHORS \
COPYING-AZY \
COPYING-EMAIL \
COPYING-SHOTGUN \
COPYING-SHOTGUN-ICONS \
README-AZY \
README-EMAIL \
README-SHOTGUN \
THANKS-EMAIL \
THANKS-SHOTGUN \
TODO-AZY \
TODO-SHOTGUN \
azy.pc.in \
email.pc.in \
shotgun.pc.in \
rpm/maelstrom.spec.in \
autogen.sh

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA =

bin_PROGRAMS =
noinst_PROGRAMS =

header_install_HEADERS =
header_installdir = $(pkgincludedir)-$(VMAJ)
include src/include/Makefile.mk

if BUILD_AZY
pkgconfig_DATA += azy.pc
endif

if BUILD_SHOTGUN
pkgconfig_DATA += shotgun.pc
endif

if BUILD_EMAIL
pkgconfig_DATA += email.pc
endif

CLEANFILES = \
maelstrom-*.tar.bz2 \
maelstrom-*.tar.gz \
src/bin/lemon

DISTCLEANFILES =

MAINTAINERCLEANFILES = \
Makefile.in \
aclocal.m4 \
config.guess \
config.h* \
config.sub \
configure \
depcomp \
install-sh \
ltmain.sh \
missing \
compile \
m4/l* \
rpm/maelstrom.spec \
ylwrap

PHONY = doc
INTERMEDIATE_S =

include src/lib/Makefile.mk
include src/bin/Makefile.mk
include src/modules/Makefile.mk
include src/tests/Makefile.mk
include data/Makefile.mk

maintainer-clean-local:
	rm -rf autom4te.cache

.PHONY: $(PHONY)

# Documentation

doc: all
	@echo "entering doc/"
	$(MAKE) -C doc doc

clean-local:
	rm -f src/modules/sawed-off/e-module-sawed-off.edj
	rm -f data/theme/default.edj

.INTERMEDIATE: $(INTERMEDIATE_S)
.SECONDARY: $(INTERMEDIATE_S)

