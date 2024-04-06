ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS =								\
	data								\
	po								\
	src

desktopdir = $(datadir)/applications
desktop_in_files = xfce4-taskmanager.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

xfce4-taskmanager.desktop: xfce4-taskmanager.desktop.in
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

ChangeLog: Makefile
	ChangeLog-git.sh > $(srcdir)/ChangeLog || touch $(srcdir)/ChangeLog

EXTRA_DIST =								\
	$(desktop_in_files)

DISTCLEANFILES =							\
	$(desktop_DATA)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
