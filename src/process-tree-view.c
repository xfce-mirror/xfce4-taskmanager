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

enum
{
	COLUMN_COMMAND,
	COLUMN_PID,
	COLUMN_PPID,
	COLUMN_STATE,
	COLUMN_VSZ,
	COLUMN_RSS,
	COLUMN_UID,
	COLUMN_CPU,
	COLUMN_PRIORITY,
	N_COLUMNS,
};

static gboolean	treeview_clicked				(XtmProcessTreeView *treeview, GdkEventButton *event);
static void	settings_changed				(GObject *object, GParamSpec *pspec, XtmProcessTreeView *treeview);
static int	sort_by_string					(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data);



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

	treeview->model = gtk_list_store_new (9, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
		G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	g_object_set (treeview, "search-column", COLUMN_COMMAND, "model", treeview->model, NULL);

	cell_text = gtk_cell_renderer_text_new();

	cell_right_aligned = gtk_cell_renderer_text_new ();
	g_object_set (cell_right_aligned, "xalign", 1.0, NULL);

	cell_cmdline = gtk_cell_renderer_text_new ();
	g_object_set (cell_cmdline, "ellipsize", PANGO_ELLIPSIZE_END, NULL);

	column = gtk_tree_view_column_new_with_attributes (_("Task"), cell_cmdline, "text", COLUMN_COMMAND, NULL);
	g_object_set (column, "expand", TRUE, "reorderable", FALSE, "resizable", TRUE, "visible", TRUE, NULL);
	gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column), COLUMN_COMMAND);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	g_object_get (treeview->settings, "column-pid", &visible, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("PID"), cell_right_aligned, "text", COLUMN_PID, NULL);
	g_object_set (column, "expand", FALSE, "reorderable", FALSE, "resizable", TRUE, "visible", visible, NULL);
	gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column), COLUMN_PID);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	g_object_get (treeview->settings, "column-ppid", &visible, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("PPID"), cell_right_aligned, "text", COLUMN_PPID, NULL);
	g_object_set (column, "expand", FALSE, "reorderable", FALSE, "resizable", TRUE, "visible", visible, NULL);
	gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column), COLUMN_PPID);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	g_object_get (treeview->settings, "column-state", &visible, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("State"), cell_text, "text", COLUMN_STATE, NULL);
	g_object_set (column, "expand", FALSE, "reorderable", FALSE, "resizable", TRUE, "visible", visible, NULL);
	gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column), COLUMN_STATE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	g_object_get (treeview->settings, "column-vsz", &visible, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("VSZ"), cell_right_aligned, "text", COLUMN_VSZ, NULL);
	g_object_set (column, "expand", FALSE, "reorderable", FALSE, "resizable", TRUE, "visible", visible, NULL);
	gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column), COLUMN_VSZ);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	g_object_get (treeview->settings, "column-rss", &visible, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("RSS"), cell_right_aligned, "text", COLUMN_RSS, NULL);
	g_object_set (column, "expand", FALSE, "reorderable", FALSE, "resizable", TRUE, "visible", visible, NULL);
	gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column), COLUMN_RSS);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	g_object_get (treeview->settings, "column-uid", &visible, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("UID"), cell_text, "text", COLUMN_UID, NULL);
	g_object_set (column, "expand", FALSE, "reorderable", FALSE, "resizable", TRUE, "visible", visible, NULL);
	gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column), COLUMN_UID);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	g_object_get (treeview->settings, "column-cpu", &visible, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("CPU"), cell_right_aligned, "text", COLUMN_CPU, NULL);
	g_object_set (column, "expand", FALSE, "reorderable", FALSE, "resizable", TRUE, "visible", visible, NULL);
	gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column), COLUMN_CPU);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	g_object_get (treeview->settings, "column-priority", &visible, NULL);
	/* TRANSLATORS: “Prio.” is short for Priority, it appears in the tree view header. */
	column = gtk_tree_view_column_new_with_attributes (_("Prio."), cell_right_aligned, "text", COLUMN_PRIORITY, NULL);
	g_object_set (column, "expand", FALSE, "reorderable", FALSE, "resizable", TRUE, "visible", visible, NULL);
	gtk_tree_view_column_set_sort_column_id (GTK_TREE_VIEW_COLUMN (column), COLUMN_PRIORITY);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (treeview->model), sort_by_string, NULL, NULL);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (treeview->model), sort_column_id, sort_type);

	g_signal_connect (treeview, "button-press-event", G_CALLBACK (treeview_clicked), NULL);
}

/**
 * Helper functions
 */

static gboolean
treeview_clicked (XtmProcessTreeView *treeview, GdkEventButton *event)
{
	if (event->button != 3)
		return FALSE;

	g_debug ("popup menu");

	return TRUE;
}

static void
settings_changed (GObject *object, GParamSpec *pspec, XtmProcessTreeView *treeview)
{
	if (g_strstr_len (pspec->name, -1, "column-") != NULL)
	{
		gboolean visible;
		gushort column_id;

		if (!g_strcmp0 (pspec->name, "column-uid"))
			column_id = COLUMN_UID;
		else if (!g_strcmp0 (pspec->name, "column-pid"))
			column_id = COLUMN_PID;
		else if (!g_strcmp0 (pspec->name, "column-ppid"))
			column_id = COLUMN_PPID;
		else if (!g_strcmp0 (pspec->name, "column-state"))
			column_id = COLUMN_STATE;
		else if (!g_strcmp0 (pspec->name, "column-vsz"))
			column_id = COLUMN_VSZ;
		else if (!g_strcmp0 (pspec->name, "column-rss"))
			column_id = COLUMN_RSS;
		else if (!g_strcmp0 (pspec->name, "column-cpu"))
			column_id = COLUMN_CPU;
		else if (!g_strcmp0 (pspec->name, "column-priority"))
			column_id = COLUMN_PRIORITY;

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

static int
sort_by_string (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data)
{
	gint sort_column_id;
	GtkSortType order;
	gchar *str1 = NULL, *str2 = NULL;
	gchar *cstr1 = NULL, *cstr2 = NULL;
	gint ret = 0;

	g_debug (__func__);

	if (!gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (model), &sort_column_id, &order))
	{
		g_debug ("sort column default or unsorted: %d", sort_column_id);
		return ret;
	}

	gtk_tree_model_get(model, a, sort_column_id, &str1, -1);
	gtk_tree_model_get(model, b, sort_column_id, &str2, -1);

	cstr1 = g_utf8_collate_key_for_filename (str1, -1);
	cstr2 = g_utf8_collate_key_for_filename (str2, -1);

	if (cstr1 != NULL && cstr2 != NULL)
	{
		ret = g_utf8_collate (cstr1, cstr2);
	}
	else if ((cstr1 == NULL && cstr2 != NULL) || (cstr1 != NULL && cstr2 == NULL))
	{
		ret = (cstr1 == NULL) ? -1 : 1;
	}

	g_free (str1);
	g_free (str2);
	g_free (cstr1);
	g_free (cstr2);

	return ret;
}



GtkWidget *
xtm_process_tree_view_new ()
{
	return g_object_new (XTM_TYPE_PROCESS_TREE_VIEW, NULL);
}

