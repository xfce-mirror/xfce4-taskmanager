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

#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "process-tree-view.h"
#include "task-manager.h"
#include "settings.h"



typedef struct _XtmProcessTreeViewClass XtmProcessTreeViewClass;
struct _XtmProcessTreeViewClass
{
	GtkTreeViewClass	parent_class;
};
struct _XtmProcessTreeView
{
	GtkTreeView		parent;
	/*<private>*/
	GtkListStore *		model;
	XtmSettings *		settings;
};
G_DEFINE_TYPE (XtmProcessTreeView, xtm_process_tree_view, GTK_TYPE_TREE_VIEW)

static gboolean		treeview_clicked				(XtmProcessTreeView *treeview, GdkEventButton *event);
static void		settings_changed				(GObject *object, GParamSpec *pspec, XtmProcessTreeView *treeview);
static int		sort_by_string					(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data);



static void
xtm_process_tree_view_class_init (XtmProcessTreeViewClass *klass)
{
	GObjectClass *class = G_OBJECT_CLASS (klass);
	xtm_process_tree_view_parent_class = g_type_class_peek_parent (klass);
}

static void
xtm_process_tree_view_init (XtmProcessTreeView *treeview)
{
	GtkCellRenderer *cell_text, *cell_right_aligned, *cell_cmdline;
	GtkTreeViewColumn *column;
	gboolean visible;
	guint sort_column_id;
	GtkSortType sort_type;

	treeview->settings = xtm_settings_get_default ();
	g_object_get (treeview->settings, "sort-column-id", &sort_column_id, "sort-type", &sort_type, NULL);
	g_signal_connect (treeview->settings, "notify", G_CALLBACK (settings_changed), treeview);

	treeview->model = gtk_list_store_new (XTM_PTV_N_COLUMNS, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_UINT64,
		G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_STRING, G_TYPE_INT);

	g_object_set (treeview, "search-column", XTM_PTV_COLUMN_COMMAND, "model", treeview->model, NULL);

	cell_text = gtk_cell_renderer_text_new();

	cell_right_aligned = gtk_cell_renderer_text_new ();
	g_object_set (cell_right_aligned, "xalign", 1.0, NULL);

	cell_cmdline = gtk_cell_renderer_text_new ();
	g_object_set (cell_cmdline, "ellipsize", PANGO_ELLIPSIZE_END, NULL);

	column = gtk_tree_view_column_new_with_attributes (_("Task"), cell_cmdline, "text", XTM_PTV_COLUMN_COMMAND, NULL);
	g_object_set (column, "expand", TRUE, "reorderable", FALSE, "resizable", TRUE, "visible", TRUE, NULL);
	gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column), XTM_PTV_COLUMN_COMMAND);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	g_object_get (treeview->settings, "column-pid", &visible, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("PID"), cell_right_aligned, "text", XTM_PTV_COLUMN_PID, NULL);
	g_object_set (column, "expand", FALSE, "reorderable", FALSE, "resizable", TRUE, "visible", visible, NULL);
	gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column), XTM_PTV_COLUMN_PID);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	g_object_get (treeview->settings, "column-ppid", &visible, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("PPID"), cell_right_aligned, "text", XTM_PTV_COLUMN_PPID, NULL);
	g_object_set (column, "expand", FALSE, "reorderable", FALSE, "resizable", TRUE, "visible", visible, NULL);
	gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column), XTM_PTV_COLUMN_PPID);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	g_object_get (treeview->settings, "column-state", &visible, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("State"), cell_text, "text", XTM_PTV_COLUMN_STATE, NULL);
	g_object_set (column, "expand", FALSE, "reorderable", FALSE, "resizable", TRUE, "visible", visible, NULL);
	gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column), XTM_PTV_COLUMN_STATE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	g_object_get (treeview->settings, "column-vsz", &visible, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("VSZ"), cell_right_aligned, "text", XTM_PTV_COLUMN_VSZ_STR, NULL);
	g_object_set (column, "expand", FALSE, "reorderable", FALSE, "resizable", TRUE, "visible", visible, NULL);
	gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column), XTM_PTV_COLUMN_VSZ);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	g_object_get (treeview->settings, "column-rss", &visible, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("RSS"), cell_right_aligned, "text", XTM_PTV_COLUMN_RSS_STR, NULL);
	g_object_set (column, "expand", FALSE, "reorderable", FALSE, "resizable", TRUE, "visible", visible, NULL);
	gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column), XTM_PTV_COLUMN_RSS);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	g_object_get (treeview->settings, "column-uid", &visible, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("UID"), cell_text, "text", XTM_PTV_COLUMN_UID, NULL);
	g_object_set (column, "expand", FALSE, "reorderable", FALSE, "resizable", TRUE, "visible", visible, NULL);
	gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column), XTM_PTV_COLUMN_UID);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	g_object_get (treeview->settings, "column-cpu", &visible, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("CPU"), cell_right_aligned, "text", XTM_PTV_COLUMN_CPU_STR, NULL);
	g_object_set (column, "expand", FALSE, "reorderable", FALSE, "resizable", TRUE, "visible", visible, NULL);
	gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column), XTM_PTV_COLUMN_CPU);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	g_object_get (treeview->settings, "column-priority", &visible, NULL);
	/* TRANSLATORS: “Prio.” is short for Priority, it appears in the tree view header. */
	column = gtk_tree_view_column_new_with_attributes (_("Prio."), cell_right_aligned, "text", XTM_PTV_COLUMN_PRIORITY, NULL);
	g_object_set (column, "expand", FALSE, "reorderable", FALSE, "resizable", TRUE, "visible", visible, NULL);
	gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column), XTM_PTV_COLUMN_PRIORITY);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (treeview->model), sort_column_id, sort_type);

	g_signal_connect (treeview, "button-press-event", G_CALLBACK (treeview_clicked), NULL);
}

/**
 * Helper functions
 */

static void
cb_send_signal (GtkMenuItem *mi, gpointer user_data)
{
	guint pid = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (mi), "pid"));
	gint signal = GPOINTER_TO_INT (user_data);

	if (!send_signal_to_pid (pid, signal))
	{
		GtkWidget *dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
			_("Error sending signal"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), _("An error was encountered by sending a signal to the PID %d. "
			"It is likely you don't have the required privileges."), pid);
		gtk_window_set_title (GTK_WINDOW (dialog), _("Task Manager"));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}
}

static void
cb_set_priority (GtkMenuItem *mi, gpointer user_data)
{
	guint pid = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (mi), "pid"));
	gint priority = GPOINTER_TO_INT (user_data);

	if (!set_priority_to_pid (pid, priority))
	{
		GtkWidget *dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
			_("Error setting priority"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), _("An error was encountered by setting a priority to the PID %d. "
			"It is likely you don't have the required privileges."), pid);
		gtk_window_set_title (GTK_WINDOW (dialog), _("Task Manager"));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}
}

static GtkWidget *
build_context_menu (guint pid)
{
	GtkWidget *menu, *menu_priority, *mi;

	menu = gtk_menu_new ();

	mi = gtk_menu_item_new_with_label (_("Terminate"));
	g_object_set_data (G_OBJECT (mi), "pid", GUINT_TO_POINTER (pid));
	gtk_container_add (GTK_CONTAINER (menu), mi);
	g_signal_connect (mi, "activate", G_CALLBACK (cb_send_signal), GINT_TO_POINTER (XTM_SIGNAL_TERMINATE));

	if (!pid_is_sleeping (pid))
	{
		mi = gtk_menu_item_new_with_label (_("Stop"));
		g_object_set_data (G_OBJECT (mi), "pid", GUINT_TO_POINTER (pid));
		gtk_container_add (GTK_CONTAINER (menu), mi);
		g_signal_connect (mi, "activate", G_CALLBACK (cb_send_signal), GINT_TO_POINTER (XTM_SIGNAL_STOP));
	}
	else
	{
		mi = gtk_menu_item_new_with_label (_("Continue"));
		g_object_set_data (G_OBJECT (mi), "pid", GUINT_TO_POINTER (pid));
		gtk_container_add (GTK_CONTAINER (menu), mi);
		g_signal_connect (mi, "activate", G_CALLBACK (cb_send_signal), GINT_TO_POINTER (XTM_SIGNAL_CONTINUE));
	}

	mi = gtk_menu_item_new_with_label (_("Kill"));
	g_object_set_data (G_OBJECT (mi), "pid", GUINT_TO_POINTER (pid));
	gtk_container_add (GTK_CONTAINER (menu), mi);
	g_signal_connect (mi, "activate", G_CALLBACK (cb_send_signal), GINT_TO_POINTER (XTM_SIGNAL_KILL));

	menu_priority = gtk_menu_new ();

	mi = gtk_menu_item_new_with_label (_("Very low"));
	g_object_set_data (G_OBJECT (mi), "pid", GUINT_TO_POINTER (pid));
	gtk_container_add (GTK_CONTAINER (menu_priority), mi);
	g_signal_connect (mi, "activate", G_CALLBACK (cb_set_priority), GINT_TO_POINTER (XTM_PRIORITY_VERY_LOW));

	mi = gtk_menu_item_new_with_label (_("Low"));
	g_object_set_data (G_OBJECT (mi), "pid", GUINT_TO_POINTER (pid));
	gtk_container_add (GTK_CONTAINER (menu_priority), mi);
	g_signal_connect (mi, "activate", G_CALLBACK (cb_set_priority), GINT_TO_POINTER (XTM_PRIORITY_LOW));

	mi = gtk_menu_item_new_with_label (_("Normal"));
	g_object_set_data (G_OBJECT (mi), "pid", GUINT_TO_POINTER (pid));
	gtk_container_add (GTK_CONTAINER (menu_priority), mi);
	g_signal_connect (mi, "activate", G_CALLBACK (cb_set_priority), GINT_TO_POINTER (XTM_PRIORITY_NORMAL));

	mi = gtk_menu_item_new_with_label (_("High"));
	g_object_set_data (G_OBJECT (mi), "pid", GUINT_TO_POINTER (pid));
	gtk_container_add (GTK_CONTAINER (menu_priority), mi);
	g_signal_connect (mi, "activate", G_CALLBACK (cb_set_priority), GINT_TO_POINTER (XTM_PRIORITY_HIGH));

	mi = gtk_menu_item_new_with_label (_("Very high"));
	g_object_set_data (G_OBJECT (mi), "pid", GUINT_TO_POINTER (pid));
	gtk_container_add (GTK_CONTAINER (menu_priority), mi);
	g_signal_connect (mi, "activate", G_CALLBACK (cb_set_priority), GINT_TO_POINTER (XTM_PRIORITY_VERY_HIGH));

	mi = gtk_menu_item_new_with_label (_("Priority"));
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (mi), menu_priority);
	gtk_container_add (GTK_CONTAINER (menu), mi);

	gtk_widget_show_all (menu);

	return menu;
}

static gboolean
treeview_clicked (XtmProcessTreeView *treeview, GdkEventButton *event)
{
	static GtkWidget *menu = NULL;
	guint pid;

	if (event->button != 3)
		return FALSE;

	if (menu != NULL)
		gtk_widget_destroy (menu);

	{
		GtkTreeModel *model;
		GtkTreeSelection *selection;
		GtkTreePath *path;
		GtkTreeIter iter;

		model = GTK_TREE_MODEL (treeview->model);
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
		gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (treeview), (gint)event->x, (gint)event->y, &path, NULL, NULL, NULL);

		if (path == NULL)
			return;

		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_model_get (model, &iter, XTM_PTV_COLUMN_PID, &pid, -1);
		gtk_tree_selection_select_path (selection, path);
		gtk_tree_path_free (path);

#if DEBUG
		g_debug ("Found iter with pid %d", pid);
#endif
	}

	menu = build_context_menu (pid);
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, event->button, event->time);

	return TRUE;
}

static void
settings_changed (GObject *object, GParamSpec *pspec, XtmProcessTreeView *treeview)
{
	if (g_str_has_prefix (pspec->name, "column-"))
	{
		gboolean visible;
		gushort column_id;

		if (!g_strcmp0 (pspec->name, "column-uid"))
			column_id = XTM_PTV_COLUMN_UID;
		else if (!g_strcmp0 (pspec->name, "column-pid"))
			column_id = XTM_PTV_COLUMN_PID;
		else if (!g_strcmp0 (pspec->name, "column-ppid"))
			column_id = XTM_PTV_COLUMN_PPID;
		else if (!g_strcmp0 (pspec->name, "column-state"))
			column_id = XTM_PTV_COLUMN_STATE;
		else if (!g_strcmp0 (pspec->name, "column-vsz"))
			column_id = XTM_PTV_COLUMN_VSZ_STR;
		else if (!g_strcmp0 (pspec->name, "column-rss"))
			column_id = XTM_PTV_COLUMN_RSS_STR;
		else if (!g_strcmp0 (pspec->name, "column-cpu"))
			column_id = XTM_PTV_COLUMN_CPU_STR;
		else if (!g_strcmp0 (pspec->name, "column-priority"))
			column_id = XTM_PTV_COLUMN_PRIORITY;

		g_object_get (object, pspec->name, &visible, NULL);
		gtk_tree_view_column_set_visible (gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), column_id), visible);
	}
	else if (!g_strcmp0 (pspec->name, "show-system-processes"))
	{
		gboolean visible;
		g_object_get (object, pspec->name, &visible, NULL);
		// TODO show/hide system processes from treeview
	}
}



GtkWidget *
xtm_process_tree_view_new ()
{
	return g_object_new (XTM_TYPE_PROCESS_TREE_VIEW, NULL);
}

