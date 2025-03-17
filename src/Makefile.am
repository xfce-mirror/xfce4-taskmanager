AM_CPPFLAGS =								\
	-I$(top_srcdir)							\
	-DG_LOG_DOMAIN=\"xfce4-taskmanager\"				\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

bin_PROGRAMS =								\
	xfce4-taskmanager

xfce4_taskmanager_CFLAGS =						\
	$(CAIRO_CFLAGS)							\
	$(LIBX11_CFLAGS)						\
	$(LIBXMU_CFLAGS)						\
	$(GTK3_CFLAGS)							\
	$(WNCK_CFLAGS)							\
	$(LIBXFCE4UI_CFLAGS)				\
	$(LIBXFCE4UTIL_CFLAGS)				\
	$(XFCONF_CFLAGS)						\
	$(NULL)

xfce4_taskmanager_LDADD = 						\
	$(CAIRO_LIBS)							\
	$(LIBX11_LIBS)						\
	$(LIBXMU_LIBS)						\
	$(GTK3_LIBS) 						\
	$(WNCK_LIBS)						\
	$(LIBXFCE4UI_LIBS)			\
	$(LIBXFCE4UTIL_LIBS)			\
	$(XFCONF_LIBS)					\
	$(NULL)

xfce4_taskmanager_built_sources =	\
	task-manager-resources.c		\
	task-manager-resources.h

xfce4_taskmanager_SOURCES =						\
	$(xfce4_taskmanager_built_sources)			\
	main.c								\
	process-window.c		process-window.h		\
	process-monitor.c		process-monitor.h		\
	process-tree-model.c		process-tree-model.h		\
	process-tree-view.c		process-tree-view.h		\
	process-statusbar.c		process-statusbar.h		\
	settings-dialog.c		settings-dialog.h		\
	settings.c			settings.h			\
	task-manager.c			task-manager.h			\
	$(NULL)

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
if OS_DARWIN
xfce4_taskmanager_SOURCES += task-manager-darwin.c
endif
if OS_SKEL
xfce4_taskmanager_SOURCES += task-manager-skel.c
endif

xfce4_taskmanager_glade_files = $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir $(srcdir) $(srcdir)/task-manager.gresource.xml)

task-manager-resources.c: $(srcdir)/task-manager.gresource.xml $(xfce4_taskmanager_glade_files) Makefile
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source $<

DISTCLEANFILES =						\
	$(xfce4_taskmanager_built_sources)

EXTRA_DIST =							\
	$(xfce4_taskmanager_glade_files)	\
	task-manager.gresource.xml			\
	meson.build
