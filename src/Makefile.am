NULL = 

INCLUDES =								\
	-I$(top_srcdir)/include						\
	-DG_LOG_DOMAIN=\"xfce4-taskmanager\"				\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

bin_PROGRAMS =								\
	xfce4-taskmanager

xfce4_taskmanager_CFLAGS =						\
	$(GTK_CFLAGS)
	
xfce4_taskmanager_LDADD = 						\
	$(GTK_LIBS)

xfce4_taskmanager_SOURCES =						\
	main.c								\
	process-window.c		process-window.h		\
	process-tree-view.c		process-tree-view.h		\
	process-statusbar.c		process-statusbar.h		\
	settings.c			settings.h			\
	$(NULL)

if MAINTAINER_MODE
BUILT_SOURCES = process-window_ui.h
process-window_ui.h: process-window.ui
	$(AM_V_GEN) exo-csource --static --strip-comments --strip-content --name=process_window_ui $< >$@
endif

