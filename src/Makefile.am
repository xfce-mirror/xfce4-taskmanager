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
	functions.c							\
	functions.h							\
	interface.c							\
	interface.h							\
	xfce-taskmanager-linux.c					\
	xfce-taskmanager-linux.h					\
	types.h

xfce4_taskmanager_CFLAGS =						\
	$(LIBXFCEGUI4_CFLAGS)						\
	$(LIBXFCE4UTIL_CFLAGS)						\
	$(GTK_CFLAGS)
	
xfce4_taskmanager_LDADD = 						\
	$(LIBXFCEGUI4_LIBS)						\
	$(LIBXFCE4UTIL_LIBS)						\
	$(GTK_LIBS)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
