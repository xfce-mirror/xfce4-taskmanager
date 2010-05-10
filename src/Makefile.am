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
	process-window_ui.h						\
	process-window.c		process-window.h		\
	process-tree-view.c		process-tree-view.h		\
	process-statusbar.c		process-statusbar.h		\
	task-manager.c			task-manager.h			\
	settings.c			settings.h			\
	$(NULL)

if OS_BSD
xfce4_taskmanager_SOURCES += task-manager-bsd.c
endif
if OS_SOLARIS
xfce4_taskmanager_SOURCES += task-manager-solaris.c
endif
if OS_LINUX
xfce4_taskmanager_SOURCES += task-manager-linux.c
endif

if MAINTAINER_MODE
BUILT_SOURCES = process-window_ui.h
process-window_ui.h: process-window.ui
	$(AM_V_GEN) exo-csource --static --strip-comments --strip-content --name=process_window_ui $< >$@
endif

EXTRA_DIST = process-window.ui

