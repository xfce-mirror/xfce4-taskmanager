/*
 * Copyright (c) 2005-2006 Johannes Zellner, <webmaster@nebulon.de>
 * Copyright (c) 2010 Mike Massonnet, <mmassonnet@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "settings.h"
#include "process-window.h"
#include "task-manager.h"

static XtmSettings *settings;
static GtkWidget *window;
static GtkStatusIcon *status_icon;
static XtmTaskManager *task_manager;
static gboolean timeout = 0;

static void
status_icon_activated ()
{
	if (!(GTK_WIDGET_VISIBLE (window)))
		gtk_widget_show (window);
	else
		gtk_widget_hide (window);
}

static gboolean
init_timeout (void)
{
	guint num_processes;
	gfloat cpu, memory, swap;
	guint64 swap_free, swap_total;

	xtm_task_manager_get_system_info (task_manager, &num_processes, &cpu, &memory, &swap);
	xtm_process_window_set_system_info (XTM_PROCESS_WINDOW (window), num_processes, cpu, memory, swap);

	xtm_task_manager_get_swap_usage (task_manager, &swap_free, &swap_total);
	xtm_process_window_show_swap_usage (XTM_PROCESS_WINDOW (window), (swap_total > 0));

	xtm_task_manager_update_model (task_manager);

	if (timeout == 0)
	{
		guint refresh_rate;
		g_object_get (settings, "refresh-rate", &refresh_rate, NULL);
		timeout = g_timeout_add (refresh_rate, (GSourceFunc)init_timeout, NULL);
	}

	return TRUE;
}

static void
force_timeout_update (void)
{
	init_timeout ();
}

static void
refresh_rate_changed (XtmSettings *settings)
{
	if (!g_source_remove (timeout))
	{
		g_critical ("Unable to remove source");
		return;
	}
	timeout = 0;
	init_timeout ();
}

int main (int argc, char *argv[])
{
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	gtk_init (&argc, &argv);

	settings = xtm_settings_get_default ();

	status_icon = gtk_status_icon_new_from_icon_name ("utilities-system-monitor");
	g_signal_connect (status_icon, "activate", G_CALLBACK (status_icon_activated), NULL);

	window = xtm_process_window_new ();
	gtk_widget_show (window);

	task_manager = xtm_task_manager_new (xtm_process_window_get_model (XTM_PROCESS_WINDOW (window)));
	g_message ("Running as %s on %s", xtm_task_manager_get_username (task_manager), xtm_task_manager_get_hostname (task_manager));

	init_timeout ();
	g_signal_connect (settings, "notify::refresh-rate", G_CALLBACK (refresh_rate_changed), NULL);
	g_signal_connect_after (settings, "notify::more-precision", G_CALLBACK (force_timeout_update), NULL);
	g_signal_connect_after (settings, "notify::full-command-line", G_CALLBACK (force_timeout_update), NULL);

	g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

	gtk_main ();

	if (timeout > 0)
		g_source_remove (timeout);
	g_object_unref (window);

	return 0;
}

