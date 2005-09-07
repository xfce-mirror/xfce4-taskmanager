@SET_MAKE@

INCLUDES = -I$(top_srcdir)/include

bin_PROGRAMS = xfce4-taskmanager

xfce4_taskmanager_SOURCES = \
	main.c \
	callbacks.c callbacks.h \
	functions.c functions.h \
	interface.c interface.h \
	types.h

xfce4_taskmanager_CFLAGS =							\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"                           \
	@XFCE4_GUI_CFLAGS@ \
	@GTK_CFLAGS@ 
	
xfce4_taskmanager_LDADD = 							\
	@XFCE4_GUI_LIBS@ \
	@GTK_LIBS@ 

ACLOCAL_AMFLAGS = -I m4

dist-bz2: dist
	zcat $(PACKAGE)-$(VERSION).tar.gz | bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2
