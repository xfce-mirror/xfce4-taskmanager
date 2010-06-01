/*
 * Copyright (c) 2010 Mike Massonnet, <mmassonnet@xfce.org>
 *
 * Based on ThunarPreferences:
 * Copyright (c) Benedikt Meurer <benny@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <glib-object.h>
#include <glib.h>

#include "settings.h"



enum
{
	PROP_SHOW_ALL_PROCESSES = 1,
	PROP_MORE_PRECISION,
	PROP_FULL_COMMAND_LINE,
	PROP_SHOW_STATUS_ICON,
	PROP_REFRESH_RATE,
	PROP_COLUMNS_POSITIONS,
	PROP_COLUMN_UID,
	PROP_COLUMN_PID,
	PROP_COLUMN_PPID,
	PROP_COLUMN_STATE,
	PROP_COLUMN_VSZ,
	PROP_COLUMN_RSS,
	PROP_COLUMN_CPU,
	PROP_COLUMN_PRIORITY,
	PROP_SORT_COLUMN_ID,
	PROP_SORT_TYPE,
	PROP_WINDOW_WIDTH,
	PROP_WINDOW_HEIGHT,
	N_PROPS,
};
typedef struct _XtmSettingsClass XtmSettingsClass;
struct _XtmSettingsClass
{
	GObjectClass		parent_class;
};
struct _XtmSettings
{
	GObject			parent;
	/*<private>*/
	GValue			values[N_PROPS];
};
#define GET_PRIV(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), XTM_TYPE_SETTINGS, XtmSettingsPriv))
G_DEFINE_TYPE (XtmSettings, xtm_settings, G_TYPE_OBJECT)

static void	xtm_settings_get_property			(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void	xtm_settings_set_property			(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static void	xtm_settings_load_settings			(XtmSettings *settings);
static void	xtm_settings_save_settings			(XtmSettings *settings);



static void
xtm_settings_class_init (XtmSettingsClass *klass)
{
	GObjectClass *class = G_OBJECT_CLASS (klass);
	xtm_settings_parent_class = g_type_class_peek_parent (klass);
	class->get_property = xtm_settings_get_property;
	class->set_property = xtm_settings_set_property;
	g_object_class_install_property (class, PROP_SHOW_ALL_PROCESSES,
		g_param_spec_boolean ("show-all-processes", "ShowAllProcesses", "Show all processes", FALSE, G_PARAM_READWRITE));
	g_object_class_install_property (class, PROP_MORE_PRECISION,
		g_param_spec_boolean ("more-precision", "MorePrecision", "More precision", FALSE, G_PARAM_READWRITE));
	g_object_class_install_property (class, PROP_FULL_COMMAND_LINE,
		g_param_spec_boolean ("full-command-line", "FullCommandLine", "Full command line", FALSE, G_PARAM_READWRITE));
	g_object_class_install_property (class, PROP_SHOW_STATUS_ICON,
		g_param_spec_boolean ("show-status-icon", "ShowStatusIcon", "Show/hide the status icon", TRUE, G_PARAM_READWRITE));
	g_object_class_install_property (class, PROP_REFRESH_RATE,
		g_param_spec_uint ("refresh-rate", "RefreshRate", "Refresh rate in milliseconds", 0, G_MAXUINT, 750, G_PARAM_READWRITE));
	g_object_class_install_property (class, PROP_COLUMNS_POSITIONS,
		g_param_spec_string ("columns-positions", "ColumnsPositions", "Positions of the tree view columns", NULL, G_PARAM_READWRITE));
	g_object_class_install_property (class, PROP_COLUMN_UID,
		g_param_spec_boolean ("column-uid", "ColumnUID", "Show column UID", FALSE, G_PARAM_READWRITE));
	g_object_class_install_property (class, PROP_COLUMN_PID,
		g_param_spec_boolean ("column-pid", "ColumnPID", "Show column PID", TRUE, G_PARAM_READWRITE));
	g_object_class_install_property (class, PROP_COLUMN_PPID,
		g_param_spec_boolean ("column-ppid", "ColumnPPID", "Show column PPID", FALSE, G_PARAM_READWRITE));
	g_object_class_install_property (class, PROP_COLUMN_STATE,
		g_param_spec_boolean ("column-state", "ColumnState", "Show column state", FALSE, G_PARAM_READWRITE));
	g_object_class_install_property (class, PROP_COLUMN_VSZ,
		g_param_spec_boolean ("column-vsz", "ColumnVSZ", "Show column VSZ", FALSE, G_PARAM_READWRITE));
	g_object_class_install_property (class, PROP_COLUMN_RSS,
		g_param_spec_boolean ("column-rss", "ColumnRSS", "Show column RSS", TRUE, G_PARAM_READWRITE));
	g_object_class_install_property (class, PROP_COLUMN_CPU,
		g_param_spec_boolean ("column-cpu", "ColumnCPU", "Show column CPU", TRUE, G_PARAM_READWRITE));
	g_object_class_install_property (class, PROP_COLUMN_PRIORITY,
		g_param_spec_boolean ("column-priority", "ColumnPriority", "Show column priority", FALSE, G_PARAM_READWRITE));
	g_object_class_install_property (class, PROP_SORT_COLUMN_ID,
		g_param_spec_uint ("sort-column-id", "SortColumn", "Sort by column id", 0, G_MAXUINT, 0, G_PARAM_READWRITE));
	g_object_class_install_property (class, PROP_SORT_TYPE,
		g_param_spec_uint ("sort-type", "SortType", "Sort type (asc/dsc)", 0, 1, 0, G_PARAM_READWRITE));
	g_object_class_install_property (class, PROP_WINDOW_WIDTH,
		g_param_spec_int ("window-width", "WindowWidth", "Window width", -1, G_MAXINT, -1, G_PARAM_READWRITE));
	g_object_class_install_property (class, PROP_WINDOW_HEIGHT,
		g_param_spec_int ("window-height", "WindowHeight", "Window height", -1, G_MAXINT, -1, G_PARAM_READWRITE));
}

static void
xtm_settings_init (XtmSettings *settings)
{
	xtm_settings_load_settings (settings);
}

static void
xtm_settings_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GValue *src = XTM_SETTINGS (object)->values + property_id;
	if (G_IS_VALUE (src))
		g_value_copy (src, value);
	else
		g_param_value_set_default (pspec, value);
}

static void
xtm_settings_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GValue *dest = XTM_SETTINGS (object)->values + property_id;
	if (!G_IS_VALUE(dest))
	{
		g_value_init (dest, pspec->value_type);
		g_param_value_set_default (pspec, dest);
	}
	if (g_param_values_cmp (pspec, value, dest) != 0)
	{
		g_value_copy (value, dest);
		xtm_settings_save_settings (XTM_SETTINGS (object));
	}
}



static void
transform_string_to_boolean (const GValue *src, GValue *dst)
{
	g_value_set_boolean (dst, (gboolean)strcmp (g_value_get_string (src), "FALSE") != 0);
}

static void
transform_string_to_int (const GValue *src, GValue *dst)
{
	g_value_set_int (dst, (gint)strtol (g_value_get_string (src), NULL, 10));
}

static void
transform_string_to_uint (const GValue *src, GValue *dst)
{
	g_value_set_uint (dst, (gint)strtoul (g_value_get_string (src), NULL, 10));
}

static void
register_transformable (void)
{
	if (!g_value_type_transformable (G_TYPE_STRING, G_TYPE_BOOLEAN))
		g_value_register_transform_func (G_TYPE_STRING, G_TYPE_BOOLEAN, transform_string_to_boolean);

	if (!g_value_type_transformable (G_TYPE_STRING, G_TYPE_INT))
		g_value_register_transform_func (G_TYPE_STRING, G_TYPE_INT, transform_string_to_int);

	if (!g_value_type_transformable (G_TYPE_STRING, G_TYPE_UINT))
		g_value_register_transform_func (G_TYPE_STRING, G_TYPE_UINT, transform_string_to_uint);
}

static void
xtm_settings_load_settings (XtmSettings *settings)
{
	GKeyFile *rc;
	gchar *filename;

	register_transformable ();

	g_object_freeze_notify (G_OBJECT (settings));

	rc = g_key_file_new ();
	filename = g_strdup_printf ("%s/xfce4/xfce4-taskmanager.rc", g_get_user_config_dir ());

	if (g_key_file_load_from_file (rc, filename, G_KEY_FILE_NONE, NULL))
	{
		gchar *string;
		GValue dst = {0};
		GValue src = {0};
		GParamSpec **specs;
		GParamSpec *spec;
		guint nspecs;
		guint n;

		specs = g_object_class_list_properties (G_OBJECT_GET_CLASS (settings), &nspecs);
		for (n = 0; n < nspecs; ++n)
		{
			spec = specs[n];
			string = g_key_file_get_string (rc, "Settings", g_param_spec_get_nick (spec), NULL);
			if (string == NULL)
				continue;

			g_value_init (&src, G_TYPE_STRING);
			g_value_set_string (&src, string);
			g_free (string);

			if (spec->value_type == G_TYPE_STRING)
			{
				g_object_set_property (G_OBJECT (settings), spec->name, &src);
			}
			else if (g_value_type_transformable (G_TYPE_STRING, spec->value_type))
			{
				g_value_init (&dst, spec->value_type);
				if (g_value_transform (&src, &dst))
					g_object_set_property (G_OBJECT (settings), spec->name, &dst);
				g_value_unset (&dst);
			}
			else
			{
				g_warning ("Failed to load property \"%s\"", spec->name);
			}

			g_value_unset (&src);
		}
		g_free (specs);
	}

	g_free (filename);
	g_key_file_free (rc);

	g_object_thaw_notify (G_OBJECT (settings));
}

static void
xtm_settings_save_settings (XtmSettings *settings)
{
	GKeyFile *rc;
	gchar *filename;

	rc = g_key_file_new ();
	filename = g_strdup_printf ("%s/xfce4/xfce4-taskmanager.rc", g_get_user_config_dir ());

	{
		const gchar *string;
		GValue dst = {0};
		GValue src = {0};
		GParamSpec **specs;
		GParamSpec *spec;
		guint nspecs;
		guint n;

		specs = g_object_class_list_properties (G_OBJECT_GET_CLASS (settings), &nspecs);
		for (n = 0; n < nspecs; ++n)
		{
			spec = specs[n];
			g_value_init (&dst, G_TYPE_STRING);
			if (spec->value_type == G_TYPE_STRING)
			{
				g_object_get_property (G_OBJECT (settings), spec->name, &dst);
			}
			else
			{
				g_value_init (&src, spec->value_type);
				g_object_get_property (G_OBJECT (settings), spec->name, &src);
				g_value_transform (&src, &dst);
				g_value_unset (&src);
			}
			string = g_value_get_string (&dst);

			if (string != NULL)
				g_key_file_set_string (rc, "Settings", g_param_spec_get_nick (spec), string);

			g_value_unset (&dst);
		}
		g_free (specs);
	}

	{
		GError *error = NULL;
		gchar *data;
		gsize length;

		if (!g_file_test (filename, G_FILE_TEST_EXISTS))
		{
			gchar *path = g_strdup_printf ("%s/xfce4", g_get_user_config_dir ());
			g_mkdir_with_parents (path, 0700);
			g_free (path);
		}

		data = g_key_file_to_data (rc, &length, NULL);
		if (!g_file_set_contents (filename, data, length, &error))
		{
			g_warning ("Unable to save settings: %s", error->message);
			g_error_free (error);
		}

		g_free (data);
	}

	g_free (filename);
	g_key_file_free (rc);
}



XtmSettings *
xtm_settings_get_default (void)
{
	static XtmSettings *settings = NULL;
	if (settings == NULL)
	{
		settings = g_object_new (XTM_TYPE_SETTINGS, NULL);
		g_object_add_weak_pointer (G_OBJECT (settings), (gpointer)&settings);
	}
	else
	{
		g_object_ref (settings);
	}
	return settings;
}

