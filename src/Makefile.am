# $Id$

INCLUDES =								\
	-I$(top_srcdir)/include						\
	-DG_LOG_DOMAIN=\"xfce4-taskmanager\"				\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

bin_PROGRAMS =								\
	xfce4-taskmanager

xfce4_taskmanager_SOURCES =						\
	main.c								\
	callbacks.c							\
	callbacks.h							\
	menu-positions.c						\
	menu-positions.h						\
	functions.c							\
	functions.h							\
	interface.c							\
	interface.h							\
	taskmanager.h							\
	types.h

if OS_BSD_FAMILY
xfce4_taskmanager_SOURCES += taskmanager-bsd.c
endif
if OS_SOLARIS
xfce4_taskmanager_SOURCES += taskmanager-linux.c
endif
if OS_LINUX
xfce4_taskmanager_SOURCES += taskmanager-linux.c
endif

xfce4_taskmanager_CFLAGS =						\
	$(LIBXFCEGUI4_CFLAGS)						\
	$(LIBXFCE4UTIL_CFLAGS)						\
	$(GTK_CFLAGS)
	
xfce4_taskmanager_LDADD = 						\
	$(LIBXFCEGUI4_LIBS)						\
	$(LIBXFCE4UTIL_LIBS)						\
	$(GTK_LIBS)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
