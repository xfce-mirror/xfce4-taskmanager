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

#include "process-window.h"
#include "process-tree-view.h"
#include "task-manager.h"

static GtkWidget *window;
static XtmTaskManager *task_manager;
static gboolean timeout = 0;

static void
update_tree_iter (GtkTreeModel *model, GtkTreeIter *iter, Task *task)
{
	gchar vsz[64], rss[64], cpu[16];

	// TODO add precision for values < 1 MB
	g_snprintf (vsz, 64, _("%lu MB"), task->vsz / 1024 / 1024);
	g_snprintf (rss, 64, _("%lu MB"), task->rss / 1024 / 1024);
	// TODO make precision optional
	g_snprintf (cpu, 16, _("%.2f%%"), task->cpu_user + task->cpu_system);

	gtk_list_store_set (GTK_LIST_STORE (model), iter,
		XTM_PTV_COLUMN_PPID, task->ppid,
		XTM_PTV_COLUMN_STATE, task->state,
		XTM_PTV_COLUMN_VSZ, task->vsz,
		XTM_PTV_COLUMN_VSZ_STR, vsz,
		XTM_PTV_COLUMN_RSS, task->rss,
		XTM_PTV_COLUMN_RSS_STR, rss,
		XTM_PTV_COLUMN_CPU, task->cpu_user + task->cpu_system,
		XTM_PTV_COLUMN_CPU_STR, cpu,
		XTM_PTV_COLUMN_PRIORITY, task->prio,
		-1);
}

static void
add_tree_iter (GtkTreeModel *model, Task *task)
{
	GtkTreeIter iter;
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		XTM_PTV_COLUMN_COMMAND, task->cmdline,
		XTM_PTV_COLUMN_PID, task->pid,
		XTM_PTV_COLUMN_STATE, task->state,
		XTM_PTV_COLUMN_UID, task->uid_name,
		-1);
	update_tree_iter (model, &iter, task);
}

static void
update_tree_model (const GArray *task_list)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	Task *task;
	guint i;
	gboolean valid;

	model = xtm_process_window_get_model (XTM_PROCESS_WINDOW (window));

	// TODO pick a timestamp for started/terminated tasks to keep them momentary (red/green color, italic, ...)
	/* Remove terminated tasks */
	valid = gtk_tree_model_get_iter_first (model, &iter);
	while (valid)
	{
		guint pid;
		gboolean found = FALSE;

		gtk_tree_model_get (model, &iter, XTM_PTV_COLUMN_PID, &pid, -1);
		for (i = 0; i < task_list->len; i++)
		{
			task = &g_array_index (task_list, Task, i);
			if (pid != task->pid)
				continue;
			found = TRUE;
			break;
		}

		if (found == FALSE)
			gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

		valid = gtk_tree_model_iter_next (model, &iter);
	}

	/* Append started tasks and update existing ones */
	for (i = 0; i < task_list->len; i++)
	{
		guint pid;
		gboolean found = FALSE;

		task = &g_array_index (task_list, Task, i);
		valid = gtk_tree_model_get_iter_first (model, &iter);
		while (valid)
		{
			gtk_tree_model_get (model, &iter, XTM_PTV_COLUMN_PID, &pid, -1);

			if (pid == task->pid)
			{
				// TODO check if elements have to be updated, updating everything always is a CPU hog
				update_tree_iter (model, &iter, task);
				found = TRUE;
				break;
			}

			valid = gtk_tree_model_iter_next (model, &iter);
		}

		if (found == FALSE)
		{
			add_tree_iter (model, task);
		}
	}
}

static gboolean
init_timeout ()
{
	guint num_processes;
	gfloat cpu, memory, swap;
	const GArray *task_list;

	xtm_task_manager_get_system_info (task_manager, &num_processes, &cpu, &memory, &swap);
	xtm_process_window_set_system_info (XTM_PROCESS_WINDOW (window), num_processes, cpu, memory, swap);

	task_list = xtm_task_manager_get_task_list (task_manager);
	update_tree_model (task_list);

	if (timeout == 0)
		timeout = g_timeout_add (1000, init_timeout, NULL);

	return TRUE;
}

int main (int argc, char *argv[])
{
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	gtk_init (&argc, &argv);

	window = xtm_process_window_new ();
	gtk_widget_show (window);

	task_manager = xtm_task_manager_new ();
	g_message ("Running as %s on %s", xtm_task_manager_get_username (task_manager), xtm_task_manager_get_hostname (task_manager));

	init_timeout ();

	g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

	gtk_main ();

	if (timeout > 0)
		g_source_remove (timeout);
	g_object_unref (window);

	return 0;
}

