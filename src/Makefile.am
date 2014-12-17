AM_CPPFLAGS =								\
	-I$(top_srcdir)							\
	-DG_LOG_DOMAIN=\"xfce4-taskmanager\"				\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

bin_PROGRAMS =								\
	xfce4-taskmanager

xfce4_taskmanager_CFLAGS =						\
	$(CAIRO_CFLAGS)							\
	$(GKSU_CFLAGS)							\
	$(NULL)

xfce4_taskmanager_LDADD = 						\
	$(CAIRO_LIBS)							\
	$(GKSU_LIBS)							\
	$(NULL)

xfce4_taskmanager_SOURCES =						\
	main.c								\
	process-window_ui.h		process-window-gtk3_ui.h	\
	process-window.c		process-window.h		\
	process-monitor.c		process-monitor.h		\
	process-tree-model.c		process-tree-model.h		\
	process-tree-view.c		process-tree-view.h		\
	process-statusbar.c		process-statusbar.h		\
	exec-tool-button.c		exec-tool-button.h		\
	settings-tool-button.c		settings-tool-button.h		\
	settings-dialog_ui.h		settings-dialog-gtk3_ui.h	\
	settings-dialog.c		settings-dialog.h		\
	settings.c			settings.h			\
	task-manager.c			task-manager.h			\
	$(NULL)

if USE_GTK3
xfce4_taskmanager_LDADD +=	$(GTK3_LIBS) $(WNCK_LIBS)
xfce4_taskmanager_CFLAGS +=	$(GTK3_CFLAGS) $(WNCK_CFLAGS)
else
xfce4_taskmanager_LDADD +=	$(GTK_LIBS) $(WNCK_LIBS)
xfce4_taskmanager_CFLAGS +=	$(GTK_CFLAGS) $(WNCK_CFLAGS)
endif

if HAVE_WNCK
xfce4_taskmanager_SOURCES += app-manager.c app-manager.h
endif

if OS_FREEBSD
xfce4_taskmanager_SOURCES += task-manager-freebsd.c
endif
if OS_BSD
xfce4_taskmanager_SOURCES += task-manager-bsd.c
endif
if OS_SOLARIS
xfce4_taskmanager_SOURCES += task-manager-solaris.c
endif
if OS_LINUX
xfce4_taskmanager_SOURCES += task-manager-linux.c
endif
if OS_SKEL
xfce4_taskmanager_SOURCES += task-manager-skel.c
endif

if MAINTAINER_MODE
BUILT_SOURCES = process-window_ui.h settings-dialog_ui.h process-window-gtk3_ui.h settings-dialog-gtk3_ui.h
process-window_ui.h: process-window.ui
	$(AM_V_GEN) exo-csource --static --strip-comments --strip-content --name=process_window_ui $< >$@
settings-dialog_ui.h: settings-dialog.ui
	$(AM_V_GEN) exo-csource --static --strip-comments --strip-content --name=settings_dialog_ui $< >$@
process-window-gtk3_ui.h: process-window-gtk3.ui
	$(AM_V_GEN) exo-csource --static --strip-comments --strip-content --name=process_window_ui $< >$@
settings-dialog-gtk3_ui.h: settings-dialog-gtk3.ui
	$(AM_V_GEN) exo-csource --static --strip-comments --strip-content --name=settings_dialog_ui $< >$@
endif

EXTRA_DIST = process-window.ui settings-dialog.ui process-window-gtk3.ui settings-dialog-gtk3.ui

