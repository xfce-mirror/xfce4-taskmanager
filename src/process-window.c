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

#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#ifdef HAVE_GKSU
#include <libgksu/libgksu.h>
#endif

#include "settings.h"
#include "process-window.h"
#include "process-window_ui.h"
#include "process-monitor.h"
#include "process-tree-view.h"
#include "process-statusbar.h"



typedef struct _XtmProcessWindowClass XtmProcessWindowClass;
typedef struct _XtmProcessWindowPriv XtmProcessWindowPriv;
struct _XtmProcessWindowClass
{
	GtkWidgetClass		parent_class;
};
struct _XtmProcessWindow
{
	GtkWidget		parent;
	/*<private>*/
	XtmProcessWindowPriv *	priv;
};
struct _XtmProcessWindowPriv
{
	GtkBuilder *		builder;
	GtkWidget *		window;
	GtkWidget *		cpu_monitor;
	GtkWidget *		mem_monitor;
	GtkWidget *		treeview;
	GtkWidget *		statusbar;
	XtmSettings *		settings;
};
#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XTM_TYPE_PROCESS_WINDOW, XtmProcessWindowPriv))
G_DEFINE_TYPE (XtmProcessWindow, xtm_process_window, GTK_TYPE_WIDGET)

static void	xtm_process_window_finalize			(GObject *object);
static void	xtm_process_window_show				(GtkWidget *widget);
static void	xtm_process_window_hide				(GtkWidget *widget);

static void	emit_destroy_signal				(XtmProcessWindow *window);
static gboolean	emit_delete_event_signal			(XtmProcessWindow *window, GdkEvent *event);
static void	monitor_update_step_size			(XtmProcessWindow *window);
static void	show_menu_execute_task				(XtmProcessWindow *window, GtkButton *button);
static void	show_menu_preferences				(XtmProcessWindow *window, GtkButton *button);
static void	show_about_dialog				(XtmProcessWindow *window);



static void
xtm_process_window_class_init (XtmProcessWindowClass *klass)
{
	GObjectClass *class;
	GtkWidgetClass *widget_class;

	g_type_class_add_private (klass, sizeof (XtmProcessWindowPriv));
	xtm_process_window_parent_class = g_type_class_peek_parent (klass);
	class = G_OBJECT_CLASS (klass);
	class->finalize = xtm_process_window_finalize;
	widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->show = xtm_process_window_show;
	widget_class->hide = xtm_process_window_hide;
}

static void
xtm_process_window_init (XtmProcessWindow *window)
{
	GtkWidget *button;
	gint width, height;

	window->priv = GET_PRIV (window);

	window->priv->settings = xtm_settings_get_default ();

	window->priv->builder = gtk_builder_new ();
	gtk_builder_add_from_string (window->priv->builder, process_window_ui, process_window_ui_length, NULL);

	window->priv->window = GTK_WIDGET (gtk_builder_get_object (window->priv->builder, "process-window"));
	g_object_get (window->priv->settings, "window-width", &width, "window-height", &height, NULL);
	if (width >= 1 && height >= 1)
		gtk_window_resize (GTK_WINDOW (window->priv->window), width, height);
	g_signal_connect_swapped (window->priv->window, "destroy", G_CALLBACK (emit_destroy_signal), window);
	g_signal_connect_swapped (window->priv->window, "delete-event", G_CALLBACK (emit_delete_event_signal), window);

	{
		GtkWidget *toolitem;
		guint refresh_rate;

		g_object_get (window->priv->settings, "refresh-rate", &refresh_rate, NULL);

		toolitem = GTK_WIDGET (gtk_builder_get_object (window->priv->builder, "cpu-toolitem"));
		window->priv->cpu_monitor = xtm_process_monitor_new ();
		xtm_process_monitor_set_step_size (XTM_PROCESS_MONITOR (window->priv->cpu_monitor), refresh_rate / 1000.0);
		gtk_widget_show (window->priv->cpu_monitor);
		gtk_container_add (GTK_CONTAINER (toolitem), window->priv->cpu_monitor);

		toolitem = GTK_WIDGET (gtk_builder_get_object (window->priv->builder, "mem-toolitem"));
		window->priv->mem_monitor = xtm_process_monitor_new ();
		xtm_process_monitor_set_step_size (XTM_PROCESS_MONITOR (window->priv->mem_monitor), refresh_rate / 1000.0);
		gtk_widget_show (window->priv->mem_monitor);
		gtk_container_add (GTK_CONTAINER (toolitem), window->priv->mem_monitor);

		g_signal_connect_swapped (window->priv->settings, "notify::refresh-rate", G_CALLBACK (monitor_update_step_size), window);
	}

	if (geteuid () == 0)
	{
		gtk_rc_parse_string ("style\"root-warning-style\"{bg[NORMAL]=\"#b4254b\"\nfg[NORMAL]=\"#fefefe\"}\n"
				"widget\"GtkWindow.*.root-warning\"style\"root-warning-style\"\n"
				"widget\"GtkWindow.*.root-warning.GtkLabel\"style\"root-warning-style\"");
		gtk_widget_set_name (GTK_WIDGET (gtk_builder_get_object (window->priv->builder, "root-warning-ebox")), "root-warning");
		gtk_widget_show_all (GTK_WIDGET (gtk_builder_get_object (window->priv->builder, "root-warning-box")));
	}

	window->priv->treeview = xtm_process_tree_view_new ();
	gtk_widget_show (window->priv->treeview);
	gtk_container_add (GTK_CONTAINER (gtk_builder_get_object (window->priv->builder, "scrolledwindow")), window->priv->treeview);

	window->priv->statusbar = xtm_process_statusbar_new ();
	gtk_widget_show (window->priv->statusbar);
	gtk_box_pack_start (GTK_BOX (gtk_builder_get_object (window->priv->builder, "process-vbox")), window->priv->statusbar, FALSE, FALSE, 0);

	button = GTK_WIDGET (gtk_builder_get_object (window->priv->builder, "toolbutton-execute"));
	g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_menu_execute_task), window);

	button = GTK_WIDGET (gtk_builder_get_object (window->priv->builder, "toolbutton-preferences"));
	g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_menu_preferences), window);

	button = GTK_WIDGET (gtk_builder_get_object (window->priv->builder, "toolbutton-about"));
	g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_about_dialog), window);

	button = GTK_WIDGET (gtk_builder_get_object (window->priv->builder, "toolbutton-quit"));
	g_signal_connect_swapped (button, "clicked", G_CALLBACK (emit_destroy_signal), window);

	g_object_unref (window->priv->builder);
	window->priv->builder = NULL;
}

static void
xtm_process_window_finalize (GObject *object)
{
	XtmProcessWindowPriv *priv = XTM_PROCESS_WINDOW (object)->priv;

	if (GTK_IS_WINDOW (priv->window))
	{
		gint width, height;
		guint sort_column_id;
		GtkSortType sort_type;

		gtk_window_get_size (GTK_WINDOW (priv->window), &width, &height);
		xtm_process_tree_view_get_sort_column_id (XTM_PROCESS_TREE_VIEW (priv->treeview), (gint*)&sort_column_id, &sort_type);

		g_object_set (priv->settings, "window-width", width, "window-height", height,
				"sort-column-id", sort_column_id, "sort-type", sort_type, NULL);
	}

	if (GTK_IS_TREE_VIEW (priv->treeview))
		gtk_widget_destroy (priv->treeview);

	if (GTK_IS_STATUSBAR (priv->statusbar))
		gtk_widget_destroy (priv->statusbar);

	if (XTM_IS_SETTINGS (priv->settings))
		g_object_unref (priv->settings);
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
emit_delete_event_signal (XtmProcessWindow *window, GdkEvent *event)
{
	gboolean ret;
	g_signal_emit_by_name (window, "delete-event", event, &ret, G_TYPE_BOOLEAN);
	return ret;
}

static void
monitor_update_step_size (XtmProcessWindow *window)
{
	guint refresh_rate;
	g_object_get (window->priv->settings, "refresh-rate", &refresh_rate, NULL);
	g_object_set (window->priv->cpu_monitor, "step-size", refresh_rate / 1000.0, NULL);
	g_object_set (window->priv->mem_monitor, "step-size", refresh_rate / 1000.0, NULL);
}

static void
menu_position_func (GtkMenu *menu, gint *x, gint *y, gboolean *push_in, GtkWidget *widget)
{
	gdk_window_get_origin (widget->window, x, y);
	*x += widget->allocation.x;
	*y += widget->allocation.height;
	*push_in = TRUE;
}

#ifdef HAVE_GKSU
static void
run_as_root (XtmProcessWindow *window)
{
	gtk_widget_hide (window->priv->window);
	gksu_run (g_get_prgname (), NULL);
	gtk_widget_show (window->priv->window);
}
#endif

static void
execute_command (const gchar *command)
{
	GError *error = NULL;

	gdk_spawn_command_line_on_screen (gdk_screen_get_default (), command, &error);
	if (error != NULL)
	{
		GtkWidget *dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
			_("Execution error"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error->message);
		gtk_window_set_title (GTK_WINDOW (dialog), _("Task Manager"));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_error_free (error);
	}
}

static void
menu_execute_append_item (GtkMenu *menu, gchar *title, gchar *command, gchar *icon_name)
{
	GtkWidget *image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU);
	GtkWidget *mi = gtk_image_menu_item_new_with_label (title);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
	g_signal_connect_swapped (mi, "activate", G_CALLBACK (execute_command), command);
}

static gboolean
program_exists (gchar *program)
{
	gchar *program_path = g_find_program_in_path (program);
	if (program_path == NULL)
		return FALSE;
	g_free (program_path);
	return TRUE;
}

static void
show_menu_execute_task (XtmProcessWindow *window, GtkButton *button)
{
	static GtkWidget *menu = NULL;

	if (menu == NULL)
	{
		menu = gtk_menu_new ();

#ifdef HAVE_GKSU
		/* Run task manager as root */
		if (geteuid () != 0)
		{
			GtkWidget *image = gtk_image_new_from_icon_name ("utilities-system-monitor", GTK_ICON_SIZE_MENU);
			GtkWidget *mi = gtk_image_menu_item_new_with_label (_("Run Task Manager as root"));
			gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
			g_signal_connect_swapped (mi, "activate", G_CALLBACK (run_as_root), window);

			mi = gtk_separator_menu_item_new ();
			gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
		}
#endif

		/* Find a runner program */
		if (program_exists ("xfrun4"))
			menu_execute_append_item (GTK_MENU (menu), _("Run Program..."), "xfrun4", "system-run");
		else if (program_exists ("gmrun"))
			menu_execute_append_item (GTK_MENU (menu), _("Run Program..."), "gmrun", "system-run");
		else if (program_exists ("gexec"))
			menu_execute_append_item (GTK_MENU (menu), _("Run Program..."), "gexec", "system-run");
		/* Find an applications list program */
		if (program_exists ("xfce4-appfinder"))
			menu_execute_append_item (GTK_MENU (menu), _("Application Finder"), "xfce4-appfinder", "xfce4-appfinder");
		/* Find a terminal emulator */
		if (program_exists ("exo-open"))
			menu_execute_append_item (GTK_MENU (menu), _("Terminal emulator"), "exo-open --launch TerminalEmulator", "terminal");
		else if (program_exists ("xterm"))
			menu_execute_append_item (GTK_MENU (menu), _("XTerm"), "xterm -fg grey -bg black", "terminal");
		gtk_widget_show_all (menu);
	}

	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, (GtkMenuPositionFunc)menu_position_func, button, 0, gtk_get_current_event_time ());
}

static void
refresh_rate_toggled (GtkCheckMenuItem *mi, XtmSettings *settings)
{
	guint refresh_rate = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (mi), "refresh-rate"));
	g_object_set (settings, "refresh-rate", refresh_rate, NULL);
}

static void
menu_refresh_rate_append_item (GtkMenu *menu, gchar *title, guint refresh_rate, XtmSettings *settings)
{
	GtkWidget *mi;
	guint cur_refresh_rate;

	g_object_get (settings, "refresh-rate", &cur_refresh_rate, NULL);

	if (cur_refresh_rate == refresh_rate)
	{
		mi = gtk_radio_menu_item_new_with_label (NULL, title);
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mi), TRUE);
	}
	else
	{
		mi = gtk_menu_item_new_with_label (title);
	}
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
	g_object_set_data (G_OBJECT (mi), "refresh-rate", GUINT_TO_POINTER (refresh_rate));
	g_signal_connect (mi, "activate", G_CALLBACK (refresh_rate_toggled), settings);
}

static GtkWidget *
build_refresh_rate_menu (XtmSettings *settings)
{
	GtkWidget *menu;

	menu = gtk_menu_new ();

	/* TRANSLATORS: The next values are in seconds or milliseconds */
	menu_refresh_rate_append_item (GTK_MENU (menu), _("500ms"), 500, settings);
	menu_refresh_rate_append_item (GTK_MENU (menu), _("750ms"), 750, settings);
	menu_refresh_rate_append_item (GTK_MENU (menu), _("1s"), 1000, settings);
	menu_refresh_rate_append_item (GTK_MENU (menu), _("2s"), 2000, settings);
	menu_refresh_rate_append_item (GTK_MENU (menu), _("5s"), 5000, settings);
	menu_refresh_rate_append_item (GTK_MENU (menu), _("10s"), 10000, settings);

	return menu;
}

static void
preferences_toggled (GtkCheckMenuItem *mi, XtmSettings *settings)
{
	gboolean active = gtk_check_menu_item_get_active (mi);
	gchar *setting_name = g_object_get_data (G_OBJECT (mi), "setting-name");
	g_object_set (settings, setting_name, active, NULL);
}

static void
menu_preferences_append_item (GtkMenu *menu, gchar *title, gchar *setting_name, XtmSettings *settings)
{
	GtkWidget *mi;
	gboolean active = FALSE;

	g_object_get (settings, setting_name, &active, NULL);

	mi = gtk_check_menu_item_new_with_label (title);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mi), active);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
	g_object_set_data (G_OBJECT (mi), "setting-name", setting_name);
	g_signal_connect (mi, "toggled", G_CALLBACK (preferences_toggled), settings);
}

static void
show_menu_preferences (XtmProcessWindow *window, GtkButton *button)
{
	static GtkWidget *menu = NULL;
	GtkWidget *refresh_rate_menu;
	GtkWidget *mi;

	if (menu != NULL)
	{
		gtk_widget_destroy (menu);
	}

	menu = gtk_menu_new ();
	menu_preferences_append_item (GTK_MENU (menu), _("Show all processes"), "show-all-processes", window->priv->settings);
	menu_preferences_append_item (GTK_MENU (menu), _("More precision"), "more-precision", window->priv->settings);
	menu_preferences_append_item (GTK_MENU (menu), _("Full command line"), "full-command-line", window->priv->settings);
	menu_preferences_append_item (GTK_MENU (menu), _("Show status icon"), "show-status-icon", window->priv->settings);

	refresh_rate_menu = build_refresh_rate_menu (window->priv->settings);
	mi = gtk_menu_item_new_with_label (_("Refresh rate"));
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (mi), refresh_rate_menu);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

	mi = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

	menu_preferences_append_item (GTK_MENU (menu), _("PID"), "column-pid", window->priv->settings);
	menu_preferences_append_item (GTK_MENU (menu), _("PPID"), "column-ppid", window->priv->settings);
	menu_preferences_append_item (GTK_MENU (menu), _("State"), "column-state", window->priv->settings);
	menu_preferences_append_item (GTK_MENU (menu), _("Virtual Bytes"), "column-vsz", window->priv->settings);
	menu_preferences_append_item (GTK_MENU (menu), _("Private Bytes"), "column-rss", window->priv->settings);
	menu_preferences_append_item (GTK_MENU (menu), _("UID"), "column-uid", window->priv->settings);
	menu_preferences_append_item (GTK_MENU (menu), _("CPU"), "column-cpu", window->priv->settings);
	menu_preferences_append_item (GTK_MENU (menu), _("Priority"), "column-priority", window->priv->settings);

	gtk_widget_show_all (menu);
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, (GtkMenuPositionFunc)menu_position_func, button, 0, gtk_get_current_event_time ());
}

#if !GTK_CHECK_VERSION(2,18,0)
static void
url_hook_about_dialog (GtkAboutDialog *dialog, const gchar *uri, gpointer user_data)
{
	gchar *command = g_strdup_printf ("exo-open %s", uri);
	if (!g_spawn_command_line_async (command, NULL))
	{
		g_free (command);
		command = g_strdup_printf ("firefox %s", uri);
		g_spawn_command_line_async (command, NULL);
	}
	g_free (command);
}
#endif

static void
show_about_dialog (XtmProcessWindow *window)
{
	const gchar *authors[] = {
		"(c) 2008-2010 Mike Massonnet",
		"(c) 2005-2008 Johannes Zellner",
		"",
		"FreeBSD",
		"  \342\200\242 Mike Massonnet",
		"  \342\200\242 Oliver Lehmann",
		"",
		"OpenBSD",
		"  \342\200\242 Landry Breuil",
		"",
		"Linux",
		"  \342\200\242 Johannes Zellner",
		"  \342\200\242 Mike Massonnet",
		"",
		"OpenSolaris",
		"  \342\200\242 Mike Massonnet",
		"  \342\200\242 Peter Tribble",
		NULL };
	const gchar *license =
		"This program is free software; you can redistribute it and/or modify\n"
		"it under the terms of the GNU General Public License as published by\n"
		"the Free Software Foundation; either version 2 of the License, or\n"
		"(at your option) any later version.\n";

#if !GTK_CHECK_VERSION(2,18,0)
	gtk_about_dialog_set_url_hook (url_hook_about_dialog, NULL, NULL);
#endif
	gtk_show_about_dialog (GTK_WINDOW (window->priv->window),
		"program-name", _("Task Manager"),
		"version", PACKAGE_VERSION,
		"copyright", "Copyright \302\251 2005-2010 The Xfce development team",
		"logo-icon-name", "utilities-system-monitor",
		"icon-name", GTK_STOCK_ABOUT,
		"comments", _("Easy to use task manager"),
		"license", license,
		"authors", authors,
		"translator-credits", _("translator-credits"),
		"website", "http://goodies.xfce.org/projects/applications/xfce4-taskmanager",
		"website-label", "goodies.xfce.org",
		NULL);
}



/**
 * Class functions
 */

GtkWidget *
xtm_process_window_new (void)
{
	return g_object_new (XTM_TYPE_PROCESS_WINDOW, NULL);
}

static void
xtm_process_window_show (GtkWidget *widget)
{
	g_return_if_fail (GTK_IS_WIDGET (widget));
	g_return_if_fail (GTK_IS_WIDGET (XTM_PROCESS_WINDOW (widget)->priv->window));
	gtk_widget_show (XTM_PROCESS_WINDOW (widget)->priv->window);
	gtk_window_present (GTK_WINDOW (XTM_PROCESS_WINDOW (widget)->priv->window));
	GTK_WIDGET_SET_FLAGS (widget, GTK_VISIBLE);
}

static void
xtm_process_window_hide (GtkWidget *widget)
{
	gint winx, winy;
	g_return_if_fail (GTK_IS_WIDGET (widget));
	if (!GTK_IS_WIDGET (XTM_PROCESS_WINDOW (widget)->priv->window))
		return;
	gtk_window_get_position (GTK_WINDOW (XTM_PROCESS_WINDOW (widget)->priv->window), &winx, &winy);
	gtk_widget_hide (XTM_PROCESS_WINDOW (widget)->priv->window);
	gtk_window_move (GTK_WINDOW (XTM_PROCESS_WINDOW (widget)->priv->window), winx, winy);
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_VISIBLE);
}

GtkTreeModel *
xtm_process_window_get_model (XtmProcessWindow *window)
{
	g_return_val_if_fail (XTM_IS_PROCESS_WINDOW (window), NULL);
	g_return_val_if_fail (GTK_IS_TREE_VIEW (window->priv->treeview), NULL);
	return gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model (GTK_TREE_VIEW (window->priv->treeview))));
}

void
xtm_process_window_set_system_info (XtmProcessWindow *window, guint num_processes, gfloat cpu, gfloat memory, gfloat swap)
{
	gchar text[100];
	gchar value[4];

	g_return_if_fail (XTM_IS_PROCESS_WINDOW (window));
	g_return_if_fail (GTK_IS_STATUSBAR (window->priv->statusbar));

	g_object_set (window->priv->statusbar, "num-processes", num_processes, "cpu", cpu, "memory", memory, "swap", swap, NULL);

	xtm_process_monitor_add_peak (XTM_PROCESS_MONITOR (window->priv->cpu_monitor), cpu / 100.0);
	g_snprintf (value, 4, "%.0f", cpu);
	g_snprintf (text, 100, _("CPU: %s%%"), value);
	gtk_widget_set_tooltip_text (window->priv->cpu_monitor, text);

	xtm_process_monitor_add_peak (XTM_PROCESS_MONITOR (window->priv->mem_monitor), memory / 100.0);
	g_snprintf (value, 4, "%.0f", memory);
	g_snprintf (text, 100, _("Memory: %s%%"), value);
	gtk_widget_set_tooltip_text (window->priv->mem_monitor, text);
}

void
xtm_process_window_show_swap_usage (XtmProcessWindow *window, gboolean show_swap_usage)
{
	g_return_if_fail (XTM_IS_PROCESS_WINDOW (window));
	g_return_if_fail (GTK_IS_STATUSBAR (window->priv->statusbar));
	g_object_set (window->priv->statusbar, "show-swap", show_swap_usage, NULL);
}

