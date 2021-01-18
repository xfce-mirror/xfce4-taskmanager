/*
 * Copyright (c) 2010 Mike Massonnet, <mmassonnet@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <X11/Xmu/WinUtil.h>
#include <X11/cursorfont.h>
#include <X11/Xproto.h>

#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <libxfce4ui/libxfce4ui.h>

#include "settings.h"
#include "task-manager.h"
#include "process-window.h"
#include "process-window_ui.h"
#include "process-monitor.h"
#include "process-tree-view.h"
#include "process-statusbar.h"
#include "settings-dialog.h"



typedef struct _XtmProcessWindowClass XtmProcessWindowClass;
struct _XtmProcessWindowClass
{
	GtkWidgetClass		parent_class;
};

struct _XtmProcessWindow
{
	GtkWidget		parent;
	/*<private>*/
	GtkBuilder *		builder;
	GtkWidget *		window;
	GtkWidget *		filter_entry;
	GtkWidget *		filter_searchbar;
	GtkWidget *		cpu_monitor;
	GtkWidget *		mem_monitor;
	GtkWidget *		vpaned;
	GtkWidget *		treeview;
	GtkWidget *		statusbar;
	GtkWidget *		settings_button;
	XtmSettings *		settings;
};
G_DEFINE_TYPE (XtmProcessWindow, xtm_process_window, GTK_TYPE_WIDGET)

static void	xtm_process_window_finalize			(GObject *object);
static void	xtm_process_window_hide				(GtkWidget *widget);

static void	emit_destroy_signal				(XtmProcessWindow *window);
static gboolean	xtm_process_window_configure_event		(XtmProcessWindow *window, GdkEvent *event);
static gboolean	xtm_process_vpaned_move_event			(XtmProcessWindow *window, GdkEventButton *event);
static gboolean xtm_process_window_key_pressed	(XtmProcessWindow *window, GdkEventKey *event);
static void	monitor_update_step_size			(XtmProcessWindow *window);


static void
filter_entry_icon_pressed_cb (GtkEntry *entry,
                              gint position,
                              GdkEventButton *event __unused,
                              gpointer data __unused)
{
	if (position == GTK_ENTRY_ICON_SECONDARY) {
		gtk_entry_set_text (entry, "");
		gtk_widget_grab_focus (GTK_WIDGET(entry));
	}
}

static Window
Select_Window (Display *dpy, int screen)
{
	int status;
	Cursor cursor;
	XEvent event;
	Window target_win = None, root = RootWindow(dpy,screen);
	int buttons = 0;

	/* Make the target cursor */
	cursor = XCreateFontCursor(dpy, XC_crosshair);

	/* Grab the pointer using target cursor, letting it roam all over */
	status = XGrabPointer(dpy, root, False,
	        ButtonPressMask|ButtonReleaseMask, GrabModeSync,
	        GrabModeAsync, root, cursor, CurrentTime);
	if (status != GrabSuccess) {
	    fprintf (stderr, "Can't grab the mouse.\n");
	    return None;
	}

	/* Let the user select a window... */
	while ((target_win == None) || (buttons != 0)) {
	    /* allow one more event */
	    XAllowEvents(dpy, SyncPointer, CurrentTime);
	    XWindowEvent(dpy, root, ButtonPressMask|ButtonReleaseMask, &event);
	    switch (event.type) {
	        case ButtonPress:
	            if (target_win == None) {
	                target_win = event.xbutton.subwindow; /* window selected */
	                if (target_win == None) target_win = root;
	            }
	            buttons++;
	            break;
	        case ButtonRelease:
	            if (buttons > 0) /* there may have been some down before we started */
	                buttons--;
	            break;
	    }
	}

	XUngrabPointer(dpy, CurrentTime);      /* Done with pointer */

	return target_win;
}

static void
xwininfo_clicked_cb (GtkButton *button __unused, gpointer user_data) {
	XtmProcessWindow *window = (XtmProcessWindow *) user_data;
	Window selected_window;
	Display *dpy;
	Atom atom_NET_WM_PID;
	unsigned long _nitems;
	Atom actual_type;
	int actual_format;
	unsigned char *prop;
	int status;
	unsigned long bytes_after;
	GPid pid = 0;

	dpy = XOpenDisplay (NULL);
	selected_window = Select_Window (dpy, 0);
	if (selected_window) {
		selected_window = XmuClientWindow (dpy, selected_window);
	}

	atom_NET_WM_PID = XInternAtom(dpy, "_NET_WM_PID", False);

	status = XGetWindowProperty(dpy, selected_window, atom_NET_WM_PID, 0, (~0L),
	                            False, AnyPropertyType, &actual_type,
	                            &actual_format, &_nitems, &bytes_after,
	                            &prop);
	if (status == BadWindow) {
		XTM_SHOW_MESSAGE(GTK_MESSAGE_INFO,
                                    _("Bad Window"), _("Window id 0x%lx does not exist!"), selected_window);
	} if (status != Success) {
		XTM_SHOW_MESSAGE(GTK_MESSAGE_ERROR,
                                    _("XGetWindowProperty failed"), _("XGetWindowProperty failed!"));
	} else {
		if (_nitems > 0) {
			memcpy(&pid, prop, sizeof(pid));
			xtm_process_tree_view_highlight_pid(XTM_PROCESS_TREE_VIEW (window->treeview), pid);
		} else {
			XTM_SHOW_MESSAGE(GTK_MESSAGE_INFO,
                                            _("No PID found"), _("No PID found for window 0x%lx."), selected_window);
		}
		g_free(prop);
	}

}

static void
filter_entry_keyrelease_handler(GtkEntry *entry,
                                XtmProcessTreeView *treeview)
{
	const gchar *text;
	gboolean has_text;

	text = gtk_editable_get_chars (GTK_EDITABLE(entry), 0, -1);
	xtm_process_tree_view_set_filter(treeview, text);

	has_text = gtk_entry_get_text_length (GTK_ENTRY(entry)) > 0;
	gtk_entry_set_icon_sensitive (GTK_ENTRY(entry),
	                              GTK_ENTRY_ICON_SECONDARY,
	                              has_text);
}

static void
xtm_process_window_class_init (XtmProcessWindowClass *klass)
{
	GObjectClass *class;
	GtkWidgetClass *widget_class;

	xtm_process_window_parent_class = g_type_class_peek_parent (klass);
	class = G_OBJECT_CLASS (klass);
	class->finalize = xtm_process_window_finalize;
	widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->show = xtm_process_window_show;
	widget_class->hide = xtm_process_window_hide;
}

static void
show_settings_dialog (GtkButton *button, GtkWidget *parent)
{
	xtm_settings_dialog_run (parent);
}

static void
xtm_show_legend (XtmProcessWindow *window)
{
	gboolean show_legend;

	g_object_get (window->settings,
				  "show-legend", &show_legend,
				  NULL);
	gtk_widget_set_visible (GTK_WIDGET (gtk_builder_get_object (window->builder, "legend")), show_legend);
}

static void
show_filter_toggled_cb (GtkToggleButton *button, gpointer user_data)
{
	XtmProcessWindow *window = user_data;
	gboolean active;

	active = gtk_toggle_button_get_active (button);
	gtk_revealer_set_reveal_child (GTK_REVEALER (gtk_bin_get_child (GTK_BIN (window->filter_searchbar))), active);
}

static void
xtm_process_window_init (XtmProcessWindow *window)
{
	GtkWidget *button;
	GtkWidget *icon;
	gint width, height;
	gboolean show_legend;

	window->settings = xtm_settings_get_default ();

	window->builder = gtk_builder_new ();
	gtk_builder_add_from_string (window->builder, process_window_ui, process_window_ui_length, NULL);

	window->window = GTK_WIDGET (gtk_builder_get_object (window->builder, "process-window"));
	g_object_get (window->settings, "window-width", &width, "window-height", &height, NULL);
	if (width >= 1 && height >= 1)
		gtk_window_resize (GTK_WINDOW (window->window), width, height);
	g_signal_connect_swapped (window->window, "destroy", G_CALLBACK (emit_destroy_signal), window);
	g_signal_connect_swapped (window->window, "key-press-event", G_CALLBACK(xtm_process_window_key_pressed), window);
	g_signal_connect_swapped(window->window, "configure-event",
	    G_CALLBACK(xtm_process_window_configure_event), window);

	g_signal_connect_swapped (window->settings, "notify::show-legend", G_CALLBACK (xtm_show_legend), window);
	g_object_notify (G_OBJECT (window->settings), "show-legend");

	button = GTK_WIDGET (gtk_builder_get_object (window->builder, "button-settings"));
	g_signal_connect (G_OBJECT (button), "clicked",
										G_CALLBACK (show_settings_dialog), GTK_WIDGET (window->window));

	button = GTK_WIDGET (gtk_builder_get_object (window->builder, "button-identify"));
	g_signal_connect (G_OBJECT (button), "clicked",
										G_CALLBACK (xwininfo_clicked_cb), window);

	window->filter_searchbar = GTK_WIDGET (gtk_builder_get_object (window->builder, "filter-searchbar"));
	button = GTK_WIDGET (gtk_builder_get_object (window->builder, "button-show-filter"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
																gtk_revealer_get_reveal_child (GTK_REVEALER (gtk_bin_get_child (GTK_BIN (window->filter_searchbar)))));
	g_signal_connect (G_OBJECT (button), "toggled",
										G_CALLBACK (show_filter_toggled_cb), window);
	g_object_bind_property (G_OBJECT (gtk_bin_get_child (GTK_BIN (window->filter_searchbar))), "reveal-child",
													G_OBJECT (button), "active", G_BINDING_BIDIRECTIONAL);

	{
		GtkWidget *toolitem;
		guint refresh_rate;
		gint handle_position;

		g_object_get (window->settings,
					  "refresh-rate", &refresh_rate,
					  "handle-position", &handle_position,
					  NULL);

		window->vpaned = GTK_WIDGET (gtk_builder_get_object (window->builder, "mainview-vpaned"));
		if (handle_position > -1)
			gtk_paned_set_position (GTK_PANED (window->vpaned), handle_position);
		g_signal_connect_swapped(window->vpaned, "button-release-event",
		    G_CALLBACK(xtm_process_vpaned_move_event), window);

		toolitem = GTK_WIDGET (gtk_builder_get_object (window->builder, "graph-cpu"));
		window->cpu_monitor = xtm_process_monitor_new ();
		xtm_process_monitor_set_step_size (XTM_PROCESS_MONITOR (window->cpu_monitor), refresh_rate / 1000.0f);
		xtm_process_monitor_set_type (XTM_PROCESS_MONITOR (window->cpu_monitor), 0);
		gtk_widget_show (window->cpu_monitor);
		gtk_container_add (GTK_CONTAINER (toolitem), window->cpu_monitor);

		toolitem = GTK_WIDGET (gtk_builder_get_object (window->builder, "graph-mem"));
		window->mem_monitor = xtm_process_monitor_new ();
		xtm_process_monitor_set_step_size (XTM_PROCESS_MONITOR (window->mem_monitor), refresh_rate / 1000.0f);
		xtm_process_monitor_set_type (XTM_PROCESS_MONITOR (window->mem_monitor), 1);
		gtk_widget_show (window->mem_monitor);
		gtk_container_add (GTK_CONTAINER (toolitem), window->mem_monitor);

		g_signal_connect_swapped (window->settings, "notify::refresh-rate", G_CALLBACK (monitor_update_step_size), window);
	}

	window->statusbar = xtm_process_statusbar_new ();
	gtk_widget_show (window->statusbar);
	gtk_box_pack_start (GTK_BOX (gtk_builder_get_object (window->builder, "graph-vbox")), window->statusbar, FALSE, FALSE, 0);

	if (geteuid () == 0)
	{
		GtkCssProvider *css_provider;
		css_provider = gtk_css_provider_new ();
		gtk_css_provider_load_from_data (css_provider,
										"#root-warning { background-color: #e53935; color: #ffffff; }",
										-1, NULL);
		gtk_style_context_add_provider_for_screen (gdk_screen_get_default (), GTK_STYLE_PROVIDER (css_provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		g_object_unref (css_provider);
		gtk_widget_set_name (GTK_WIDGET (gtk_builder_get_object (window->builder, "root-warning-ebox")), "root-warning");
		gtk_widget_show_all (GTK_WIDGET (gtk_builder_get_object (window->builder, "root-warning-box")));
	}

	window->treeview = xtm_process_tree_view_new ();
	gtk_widget_show (window->treeview);
	gtk_container_add (GTK_CONTAINER (gtk_builder_get_object (window->builder, "scrolledwindow")), window->treeview);

	g_object_get (window->settings,
				  "show-legend", &show_legend,
				  NULL);
	gtk_widget_set_visible (GTK_WIDGET (gtk_builder_get_object (window->builder, "legend")), show_legend);

	window->filter_entry = GTK_WIDGET(gtk_builder_get_object (window->builder, "filter-entry"));
	g_signal_connect (G_OBJECT(window->filter_entry), "icon-press", G_CALLBACK(filter_entry_icon_pressed_cb), NULL);
	g_signal_connect (G_OBJECT(window->filter_entry), "changed", G_CALLBACK(filter_entry_keyrelease_handler), window->treeview);
	gtk_widget_set_tooltip_text (window->filter_entry, _("Filter on process name"));

	gtk_widget_grab_focus (GTK_WIDGET (window->filter_entry));
}

static void
xtm_process_window_finalize (GObject *object)
{
	XtmProcessWindow *window = XTM_PROCESS_WINDOW (object);

	if (GTK_IS_TREE_VIEW (window->treeview))
		gtk_widget_destroy (window->treeview);

	if (GTK_IS_BOX (window->statusbar))
		gtk_widget_destroy (window->statusbar);

	if (XTM_IS_SETTINGS (window->settings))
		g_object_unref (window->settings);

	g_object_unref (window->builder);
	window->builder = NULL;
}

/**
 * Helper functions
 */

static void
emit_destroy_signal (XtmProcessWindow *window)
{
	g_signal_emit_by_name (window, "destroy", G_TYPE_NONE);
}

static gboolean
xtm_process_window_configure_event(XtmProcessWindow *window,
    GdkEvent *event) {

	if (NULL != window &&
	    NULL != event && GDK_CONFIGURE == event->configure.type) {
		g_object_set (window->settings,
		    "window-width", event->configure.width,
		    "window-height", event->configure.height,
		    NULL);
	}

	return (FALSE);
}

static gboolean
xtm_process_vpaned_move_event(XtmProcessWindow *window,
    GdkEventButton *event __unused) {
	gint handle_position;

	if (NULL != window) {
		handle_position = gtk_paned_get_position(GTK_PANED(window->vpaned));
		g_object_set (window->settings,
		    "handle-position", handle_position,
		    NULL);
	}

	return (FALSE);
}

static gboolean
xtm_process_window_key_pressed (XtmProcessWindow *window, GdkEventKey *event)
{
	gboolean ret = FALSE;

	if (event->keyval == GDK_KEY_Escape ||
		(event->keyval == GDK_KEY_q && (event->state & GDK_CONTROL_MASK))) {
		xtm_settings_save_settings (window->settings);
		g_signal_emit_by_name (window, "delete-event", event, &ret, G_TYPE_BOOLEAN);
		ret = TRUE;
	}
	else if (event->keyval == GDK_KEY_f && (event->state & GDK_CONTROL_MASK)) {
		gtk_widget_grab_focus (GTK_WIDGET(window->filter_entry));
		ret = TRUE;
	}

	return ret;
}

static void
monitor_update_step_size (XtmProcessWindow *window)
{
	guint refresh_rate;
	g_object_get (window->settings, "refresh-rate", &refresh_rate, NULL);
	g_object_set (window->cpu_monitor, "step-size", refresh_rate / 1000.0, NULL);
	g_object_set (window->mem_monitor, "step-size", refresh_rate / 1000.0, NULL);
}

/**
 * Class functions
 */

GtkWidget *
xtm_process_window_new (void)
{
	return g_object_new (XTM_TYPE_PROCESS_WINDOW, NULL);
}

void
xtm_process_window_show (GtkWidget *widget)
{
	g_return_if_fail (GTK_IS_WIDGET (widget));
	g_return_if_fail (GTK_IS_WIDGET (XTM_PROCESS_WINDOW (widget)->window));
	gtk_widget_show (XTM_PROCESS_WINDOW (widget)->window);
	gtk_window_present (GTK_WINDOW (XTM_PROCESS_WINDOW (widget)->window));
	GTK_WIDGET_CLASS (xtm_process_window_parent_class)->show(widget);
}

static void
xtm_process_window_hide (GtkWidget *widget)
{
	gint winx, winy;
	g_return_if_fail (GTK_IS_WIDGET (widget));
	if (!GTK_IS_WIDGET (XTM_PROCESS_WINDOW (widget)->window))
		return;
	gtk_window_get_position (GTK_WINDOW (XTM_PROCESS_WINDOW (widget)->window), &winx, &winy);
	gtk_widget_hide (XTM_PROCESS_WINDOW (widget)->window);
	gtk_window_move (GTK_WINDOW (XTM_PROCESS_WINDOW (widget)->window), winx, winy);
	GTK_WIDGET_CLASS (xtm_process_window_parent_class)->hide(widget);
}

GtkTreeModel *
xtm_process_window_get_model (XtmProcessWindow *window)
{
	g_return_val_if_fail (XTM_IS_PROCESS_WINDOW (window), NULL);
	g_return_val_if_fail (XTM_IS_PROCESS_TREE_VIEW (window->treeview), NULL);
	return xtm_process_tree_view_get_model (XTM_PROCESS_TREE_VIEW (window->treeview));
}

void
xtm_process_window_set_system_info (XtmProcessWindow *window, guint num_processes, gfloat cpu, gfloat memory, gchar* memory_str, gfloat swap __unused, gchar* swap_str)
{
	gchar text[100];
	gchar value[4];

	g_return_if_fail (XTM_IS_PROCESS_WINDOW (window));
	g_return_if_fail (GTK_IS_BOX (window->statusbar));

	g_object_set (window->statusbar, "num-processes", num_processes, "cpu", cpu, "memory", memory_str, "swap", swap_str, NULL);

	xtm_process_monitor_add_peak (XTM_PROCESS_MONITOR (window->cpu_monitor), cpu / 100.0f);
	g_snprintf (value, sizeof(value), "%.0f", cpu);
	g_snprintf (text, sizeof(text), _("CPU: %s%%"), value);
	gtk_widget_set_tooltip_text (window->cpu_monitor, text);

	xtm_process_monitor_add_peak (XTM_PROCESS_MONITOR (window->mem_monitor), memory / 100.0f);
	g_snprintf (text, sizeof(text), _("Memory: %s"), memory_str);
	gtk_widget_set_tooltip_text (window->mem_monitor, text);
}

void
xtm_process_window_show_swap_usage (XtmProcessWindow *window, gboolean show_swap_usage)
{
	g_return_if_fail (XTM_IS_PROCESS_WINDOW (window));
	g_return_if_fail (GTK_IS_BOX (window->statusbar));
	g_object_set (window->statusbar, "show-swap", show_swap_usage, NULL);
}
