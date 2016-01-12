SUBDIRS = src test
ACLOCAL_AMFLAGS = -I build-aux

pkgconfigdir = $(libdir)/pkgconfig
nodist_pkgconfig_DATA = src/eixx.pc

clean-local:
	-rm -fr @default_prefix@

CLEANFILES = stamp-h1
DISTCLEANFILES = config.h.in~ config.log
MAINTAINERCLEANFILES = configure build-aux/l*.m4 build-aux/*.sh \
	build-aux/config.* build-aux/missing build-aux/depcomp \
	build-aux/install-sh \
	aclocal.m4 Makefile.in src/Makefile.in test/Makefile.in \
    src/eixx.pc
MOSTLYCLEANFILES = $(DX_CLEANFILES)

EXTRA_DIST = README LICENSE LICENSE.header license.sh \
			 build-aux/boost.m4  \
             bootstrap \
             build-aux/ax_prog_doxygen.m4 $(DX_CONFIG) $(DX_DOCDIR)/html

dist_doc_DATA = README.md LICENSE

@DX_RULES@

docs: doxygen-doc

.PHONY: docs
