EXTRA_DIST =								\
	AUTHORS								\
	COPYING								\
	ChangeLog							\
	INSTALL								\
	NEWS								\
	README								\
	TODO			\
	intltool-extract.in			\
	intltool-merge.in			\
	intltool-update.in
	
DISTCLEANFILES =							\
	intltool-extract						\
	intltool-merge							\
	intltool-update

SUBDIRS =  po src
DIST_SUBDIRS = src po

dist-bz2: dist
	zcat $(PACKAGE)-$(VERSION).tar.gz | bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2
