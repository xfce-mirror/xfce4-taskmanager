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

#include <gtk/gtk.h>
#include <cairo.h>

#include "process-monitor.h"



enum
{
	PROP_STEP_SIZE = 1,
	PROP_COLOR_RED,
	PROP_COLOR_GREEN,
	PROP_COLOR_BLUE,
};
typedef struct _XtmProcessMonitorClass XtmProcessMonitorClass;
struct _XtmProcessMonitorClass
{
	GtkDrawingAreaClass	parent_class;
};
struct _XtmProcessMonitor
{
	GtkDrawingArea		parent;
	/*<private>*/
	gfloat			step_size;
	GArray *		history;
	gfloat			color_red;
	gfloat			color_green;
	gfloat			color_blue;
};
G_DEFINE_TYPE (XtmProcessMonitor, xtm_process_monitor, GTK_TYPE_DRAWING_AREA)

static void	xtm_process_monitor_get_property	(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void	xtm_process_monitor_set_property	(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean	xtm_process_monitor_expose		(GtkWidget *widget, GdkEventExpose *event);
static void	xtm_process_monitor_paint		(XtmProcessMonitor *monitor);



static void
xtm_process_monitor_class_init (XtmProcessMonitorClass *klass)
{
	GObjectClass *class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	xtm_process_monitor_parent_class = g_type_class_peek_parent (klass);
	class->get_property = xtm_process_monitor_get_property;
	class->set_property = xtm_process_monitor_set_property;
	widget_class->expose_event = xtm_process_monitor_expose;
	g_object_class_install_property (class, PROP_STEP_SIZE,
		g_param_spec_float ("step-size", "StepSize", "Step size", 0.1, G_MAXFLOAT, 1, G_PARAM_CONSTRUCT|G_PARAM_READWRITE));
	g_object_class_install_property (class, PROP_COLOR_RED,
		g_param_spec_float ("color-red", "ColorRed", "Color red", 0, 1, 0, G_PARAM_READWRITE));
	g_object_class_install_property (class, PROP_COLOR_GREEN,
		g_param_spec_float ("color-green", "ColorGreen", "Color green", 0, 1, 0, G_PARAM_READWRITE));
	g_object_class_install_property (class, PROP_COLOR_BLUE,
		g_param_spec_float ("color-blue", "ColorBlue", "Color blue", 0, 1, 0, G_PARAM_READWRITE));
}

static void
init_source_color (GtkWidget *widget, GtkStyle *prev_style, gpointer user_data)
{
	GdkColor *color = &widget->style->base[GTK_STATE_SELECTED];
	XTM_PROCESS_MONITOR (widget)->color_red = color->red / 65535.0;
	XTM_PROCESS_MONITOR (widget)->color_green = color->green / 65535.0;
	XTM_PROCESS_MONITOR (widget)->color_blue = color->blue / 65535.0;
}

static void
xtm_process_monitor_init (XtmProcessMonitor *monitor)
{
	monitor->history = g_array_new (FALSE, TRUE, sizeof (gfloat));
	g_signal_connect (monitor, "style-set", G_CALLBACK (init_source_color), NULL);
}

static void
xtm_process_monitor_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	XtmProcessMonitor *monitor = XTM_PROCESS_MONITOR (object);
	switch (property_id)
	{
		case PROP_STEP_SIZE:
		g_value_set_float (value, monitor->step_size);
		break;

		case PROP_COLOR_RED:
		g_value_set_float (value, monitor->color_red);
		break;

		case PROP_COLOR_GREEN:
		g_value_set_float (value, monitor->color_green);
		break;

		case PROP_COLOR_BLUE:
		g_value_set_float (value, monitor->color_blue);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
xtm_process_monitor_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	XtmProcessMonitor *monitor = XTM_PROCESS_MONITOR (object);
	switch (property_id)
	{
		case PROP_STEP_SIZE:
		monitor->step_size = g_value_get_float (value);
		break;

		case PROP_COLOR_RED:
		monitor->color_red = g_value_get_float (value);
		break;

		case PROP_COLOR_GREEN:
		monitor->color_green = g_value_get_float (value);
		break;

		case PROP_COLOR_BLUE:
		monitor->color_blue = g_value_get_float (value);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static gboolean
xtm_process_monitor_expose (GtkWidget *widget, GdkEventExpose *event)
{
	XtmProcessMonitor *monitor = XTM_PROCESS_MONITOR (widget);
	guint minimum_history_length;

	minimum_history_length = widget->allocation.width / monitor->step_size;
	if (monitor->history->len < minimum_history_length)
		g_array_set_size (monitor->history, minimum_history_length + 1);

	xtm_process_monitor_paint (monitor);
	return FALSE;
}

static cairo_surface_t *
xtm_process_monitor_graph_surface_create (XtmProcessMonitor *monitor)
{
	cairo_t *cr;
	cairo_surface_t *graph_surface;
	cairo_pattern_t *linpat;
	gfloat *peak;
	gfloat step_size;
	gint width, height;
	gint i;

	if (monitor->history->len <= 1)
	{
		g_warning ("Cannot paint graph with n_peak <= 1");
		return NULL;
	}

	width = GTK_WIDGET (monitor)->allocation.width;
	height = GTK_WIDGET (monitor)->allocation.height;
	step_size = monitor->step_size;

	graph_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	cr = cairo_create (graph_surface);

	/* Draw area */
	linpat = cairo_pattern_create_linear (0, 0, 0, height);
	cairo_pattern_add_color_stop_rgba (linpat, 0.4, monitor->color_red, monitor->color_green, monitor->color_blue, 0.3);
	cairo_pattern_add_color_stop_rgba (linpat, 0.9, monitor->color_red, monitor->color_green, monitor->color_blue, 0.5);
	cairo_pattern_add_color_stop_rgba (linpat, 1, monitor->color_red, monitor->color_green, monitor->color_blue, 0.6);
	cairo_set_source (cr, linpat);
	cairo_set_line_width (cr, 0.0);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_DEFAULT);

	cairo_move_to (cr, width, height);
	cairo_translate (cr, step_size, 0);
	for (i = 0; (step_size * (i - 1)) <= width; i++)
	{
		peak = &g_array_index (monitor->history, gfloat, i);
		cairo_translate (cr, -step_size, 0);
		cairo_line_to (cr, width, (1.0 - *peak) * height);
	}
	cairo_line_to (cr, width, height);
	cairo_fill (cr);

	cairo_pattern_destroy (linpat);

	/* Draw line */
	cairo_translate (cr, step_size * i, 0);

	cairo_set_source_rgb (cr, monitor->color_red, monitor->color_green, monitor->color_blue);
	cairo_set_line_width (cr, 0.85);
	cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_DEFAULT);
	cairo_move_to (cr, width, height);
	for (i = 0; (step_size * (i - 1)) <= width; i++)
	{
		peak = &g_array_index (monitor->history, gfloat, i);
		cairo_translate (cr, -step_size, 0);
		cairo_line_to (cr, width, (1.0 - *peak) * height);
	}
	cairo_stroke (cr);

	cairo_destroy (cr);

	return graph_surface;
}

static void
xtm_process_monitor_paint (XtmProcessMonitor *monitor)
{
	cairo_t *cr;
	cairo_surface_t *graph_surface;
	gint width, height;
	gint i;

	cr = gdk_cairo_create (GTK_WIDGET (monitor)->window);
	width = GTK_WIDGET (monitor)->allocation.width;
	height = GTK_WIDGET (monitor)->allocation.height;

	/* Paint a box */
	gtk_paint_box (GTK_WIDGET (monitor)->style, GTK_WIDGET (monitor)->window, GTK_STATE_PRELIGHT, GTK_SHADOW_IN,
			NULL, GTK_WIDGET (monitor), "trough", 0, 0, width, height);

	/* Paint the graph */
	graph_surface = xtm_process_monitor_graph_surface_create (monitor);
	if (graph_surface != NULL)
	{
		cairo_set_source_surface (cr, graph_surface, 0.0, 0.0);
		cairo_paint (cr);
		cairo_surface_destroy (graph_surface);
	}

	/* Trace some marks */
	cairo_set_line_width (cr, 0.75);
	cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
	cairo_set_line_join (cr, CAIRO_LINE_JOIN_MITER);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_DEFAULT);
	for (i = 10; i < 100; i += 10)
	{
		cairo_move_to (cr, 0.0, i * height / 100);
		cairo_line_to (cr, width, i * height / 100);
	}
	cairo_stroke (cr);

	gdk_cairo_set_source_color (cr, &GTK_WIDGET (monitor)->style->fg[GTK_STATE_NORMAL]);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
	cairo_set_line_width (cr, 1.0);
	for (i = 25; i <= 75; i += 25)
	{
		cairo_move_to (cr, 0.0, i * height / 100);
		cairo_line_to (cr, 4.0, i * height / 100);
		cairo_move_to (cr, width, i * height / 100);
		cairo_line_to (cr, width - 4.0, i * height / 100);
	}
	cairo_stroke (cr);

	/* Repaint a shadow on top of everything to clear corners */
	gtk_paint_shadow (GTK_WIDGET (monitor)->style, GTK_WIDGET (monitor)->window, GTK_STATE_PRELIGHT, GTK_SHADOW_IN,
			NULL, GTK_WIDGET (monitor), "trough", 0, 0, width, height);

	cairo_destroy (cr);
}

GtkWidget *
xtm_process_monitor_new (void)
{
	return g_object_new (XTM_TYPE_PROCESS_MONITOR, NULL);
}

void
xtm_process_monitor_add_peak (XtmProcessMonitor *monitor, gfloat peak)
{
	g_return_if_fail (XTM_IS_PROCESS_MONITOR (monitor));
	g_return_if_fail (peak >= 0.0 && peak <= 1.0);

	g_array_prepend_val (monitor->history, peak);
	if (monitor->history->len > 1)
		g_array_remove_index (monitor->history, monitor->history->len - 1);

	if (GDK_IS_WINDOW (GTK_WIDGET (monitor)->window))
		gdk_window_invalidate_rect (GTK_WIDGET (monitor)->window, NULL, FALSE);
}

void
xtm_process_monitor_set_step_size (XtmProcessMonitor *monitor, gfloat step_size)
{
	g_return_if_fail (XTM_IS_PROCESS_MONITOR (monitor));
	g_object_set (monitor, "step_size", step_size, NULL);
	if (GDK_IS_WINDOW (GTK_WIDGET (monitor)->window))
		gdk_window_invalidate_rect (GTK_WIDGET (monitor)->window, NULL, FALSE);
}

void
xtm_process_monitor_clear (XtmProcessMonitor *monitor)
{
	g_return_if_fail (XTM_IS_PROCESS_MONITOR (monitor));
	g_array_set_size (monitor->history, 0);
	if (GDK_IS_WINDOW (GTK_WIDGET (monitor)->window))
		gdk_window_invalidate_rect (GTK_WIDGET (monitor)->window, NULL, FALSE);
}

void
xtm_process_monitor_set_source_color (XtmProcessMonitor *monitor, gdouble red, gdouble green, gdouble blue)
{
	g_return_if_fail (XTM_IS_PROCESS_MONITOR (monitor));
	g_signal_handlers_disconnect_by_func (GTK_WIDGET (monitor), init_source_color, NULL);
	g_object_set (monitor, "color-red", red, "color-green", green, "color-blue", blue, NULL);
	if (GDK_IS_WINDOW (GTK_WIDGET (monitor)->window))
		gdk_window_invalidate_rect (GTK_WIDGET (monitor)->window, NULL, FALSE);
}

