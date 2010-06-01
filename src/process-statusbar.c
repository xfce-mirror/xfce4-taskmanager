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

#include "process-statusbar.h"
#include "settings.h"



enum
{
	PROP_CPU = 1,
	PROP_MEMORY,
	PROP_SWAP,
	PROP_SHOW_SWAP,
	PROP_NUM_PROCESSES,
};
typedef struct _XtmProcessStatusbarClass XtmProcessStatusbarClass;
struct _XtmProcessStatusbarClass
{
	GtkStatusbarClass	parent_class;
};
struct _XtmProcessStatusbar
{
	GtkStatusbar		parent;
	/*<private>*/
	XtmSettings *		settings;

	GtkWidget *		label_num_processes;
	GtkWidget *		label_cpu;
	GtkWidget *		label_memory;
	GtkWidget *		label_swap;

	gfloat			cpu;
	gfloat			memory;
	gfloat			swap;
	guint			num_processes;
};
G_DEFINE_TYPE (XtmProcessStatusbar, xtm_process_statusbar, GTK_TYPE_STATUSBAR)

static void	xtm_process_statusbar_finalize			(GObject *object);
static void	xtm_process_statusbar_set_property		(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);



static void
xtm_process_statusbar_class_init (XtmProcessStatusbarClass *klass)
{
	GObjectClass *class = G_OBJECT_CLASS (klass);
	xtm_process_statusbar_parent_class = g_type_class_peek_parent (klass);
	class->finalize = xtm_process_statusbar_finalize;
	class->set_property = xtm_process_statusbar_set_property;
	g_object_class_install_property (class, PROP_CPU,
		g_param_spec_float ("cpu", "CPU", "CPU usage", 0, 100, 0, G_PARAM_CONSTRUCT|G_PARAM_WRITABLE));
	g_object_class_install_property (class, PROP_MEMORY,
		g_param_spec_float ("memory", "Memory", "Memory usage", 0, 100, 0, G_PARAM_CONSTRUCT|G_PARAM_WRITABLE));
	g_object_class_install_property (class, PROP_SWAP,
		g_param_spec_float ("swap", "Swap", "Swap usage", 0, 100, 0, G_PARAM_CONSTRUCT|G_PARAM_WRITABLE));
	g_object_class_install_property (class, PROP_SHOW_SWAP,
		g_param_spec_boolean ("show-swap", "ShowSwap", "Show or hide swap usage", TRUE, G_PARAM_WRITABLE));
	g_object_class_install_property (class, PROP_NUM_PROCESSES,
		g_param_spec_uint ("num-processes", "NumProcesses", "Number of processes", 0, G_MAXUINT, 0, G_PARAM_CONSTRUCT|G_PARAM_WRITABLE));
}

static void
xtm_process_statusbar_init (XtmProcessStatusbar *statusbar)
{
	GtkWidget *area, *hbox;

	statusbar->settings = xtm_settings_get_default ();

#if GTK_CHECK_VERSION(2,20,0)
	area = gtk_statusbar_get_message_area (GTK_STATUSBAR (statusbar));
#else
	{
		GtkShadowType shadow_type;
		GtkWidget *frame;

		gtk_widget_style_get (GTK_WIDGET (statusbar), "shadow-type", &shadow_type, NULL);
		frame = gtk_frame_new (NULL);
		gtk_frame_set_shadow_type (GTK_FRAME (frame), shadow_type);
		gtk_box_pack_start (GTK_BOX (statusbar), frame, TRUE, TRUE, 0);

		area = gtk_hbox_new (FALSE, 0);
		gtk_container_add (GTK_CONTAINER (frame), area);
		gtk_widget_show_all (frame);
	}
#endif

	hbox = gtk_hbox_new (FALSE, 16);
	gtk_box_pack_start (GTK_BOX (area), hbox, TRUE, TRUE, 6);

	statusbar->label_num_processes = gtk_label_new (NULL);
	gtk_box_pack_start (GTK_BOX (hbox), statusbar->label_num_processes, FALSE, FALSE, 0);

	statusbar->label_cpu = gtk_label_new (NULL);
	gtk_box_pack_start (GTK_BOX (hbox), statusbar->label_cpu, FALSE, FALSE, 0);

	statusbar->label_memory = gtk_label_new (NULL);
	gtk_box_pack_start (GTK_BOX (hbox), statusbar->label_memory, FALSE, FALSE, 0);

	statusbar->label_swap = gtk_label_new (NULL);
	gtk_box_pack_start (GTK_BOX (hbox), statusbar->label_swap, FALSE, FALSE, 0);

	gtk_widget_show_all (hbox);
}

static void
xtm_process_statusbar_finalize (GObject *object)
{
	g_object_unref (XTM_PROCESS_STATUSBAR (object)->settings);
	G_OBJECT_CLASS (xtm_process_statusbar_parent_class)->finalize (object);
}

static gchar *
rounded_float_value (gfloat value, XtmSettings *settings)
{
	gboolean precision;
	g_object_get (settings, "more-precision", &precision, NULL);
	return g_strdup_printf ((precision) ? "%.2f" : "%.0f", value);
}

static void
xtm_process_statusbar_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	XtmProcessStatusbar *statusbar = XTM_PROCESS_STATUSBAR (object);
	gchar *text;
	gchar *float_value;

	switch (property_id)
	{
		case PROP_CPU:
		statusbar->cpu = g_value_get_float (value);
		float_value = rounded_float_value (statusbar->cpu, statusbar->settings);
		text = g_strdup_printf (_("CPU: %s%%"), float_value);
		gtk_label_set_text (GTK_LABEL (statusbar->label_cpu), text);
		g_free (float_value);
		g_free (text);
		break;

		case PROP_MEMORY:
		statusbar->memory = g_value_get_float (value);
		float_value = rounded_float_value (statusbar->memory, statusbar->settings);
		text = g_strdup_printf (_("Memory: %s%%"), float_value);
		gtk_label_set_text (GTK_LABEL (statusbar->label_memory), text);
		g_free (float_value);
		g_free (text);
		break;

		case PROP_SWAP:
		statusbar->swap = g_value_get_float (value);
		float_value = rounded_float_value (statusbar->swap, statusbar->settings);
		text = g_strdup_printf (_("Swap: %s%%"), float_value);
		gtk_label_set_text (GTK_LABEL (statusbar->label_swap), text);
		g_free (float_value);
		g_free (text);
		break;

		case PROP_SHOW_SWAP:
		if (g_value_get_boolean (value))
			gtk_widget_show (statusbar->label_swap);
		else
			gtk_widget_hide (statusbar->label_swap);
		break;

		case PROP_NUM_PROCESSES:
		statusbar->num_processes = g_value_get_uint (value);
		text = g_strdup_printf (_("Processes: %d"), statusbar->num_processes);
		gtk_label_set_text (GTK_LABEL (statusbar->label_num_processes), text);
		g_free (text);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}



GtkWidget *
xtm_process_statusbar_new (void)
{
	return g_object_new (XTM_TYPE_PROCESS_STATUSBAR, NULL);
}

