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
	PROP_TYPE,
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
	gint			type;
	GArray *		history;
};
G_DEFINE_TYPE (XtmProcessMonitor, xtm_process_monitor, GTK_TYPE_DRAWING_AREA)

static void	xtm_process_monitor_get_property	(GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void	xtm_process_monitor_set_property	(GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
#if GTK_CHECK_VERSION(3, 0, 0)
static gboolean	xtm_process_monitor_draw		(GtkWidget *widget, cairo_t *cr);
#else
static gboolean	xtm_process_monitor_expose		(GtkWidget *widget, GdkEventExpose *event);
#endif
static void	xtm_process_monitor_paint		(XtmProcessMonitor *monitor, cairo_t *cr);



static void
xtm_process_monitor_class_init (XtmProcessMonitorClass *klass)
{
	GObjectClass *class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	xtm_process_monitor_parent_class = g_type_class_peek_parent (klass);
	class->get_property = xtm_process_monitor_get_property;
	class->set_property = xtm_process_monitor_set_property;
#if GTK_CHECK_VERSION(3, 0, 0)
	widget_class->draw = xtm_process_monitor_draw;
#else
	widget_class->expose_event = xtm_process_monitor_expose;
#endif
	g_object_class_install_property (class, PROP_STEP_SIZE,
		g_param_spec_float ("step-size", "StepSize", "Step size", 0.1, G_MAXFLOAT, 1, G_PARAM_CONSTRUCT|G_PARAM_READWRITE));
	g_object_class_install_property (class, PROP_TYPE,
		g_param_spec_int ("type", "Type", "Type of graph to render", 0, G_MAXINT, 0, G_PARAM_READWRITE));
}

static void
xtm_process_monitor_init (XtmProcessMonitor *monitor)
{
	monitor->history = g_array_new (FALSE, TRUE, sizeof (gfloat));
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

		case PROP_TYPE:
		g_value_set_int (value, monitor->type);
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

		case PROP_TYPE:
		monitor->type = g_value_get_int (value);
		break;

		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

#if GTK_CHECK_VERSION(3, 0, 0)
static gboolean
xtm_process_monitor_draw (GtkWidget *widget, cairo_t *cr)
{
	XtmProcessMonitor *monitor = XTM_PROCESS_MONITOR (widget);
	guint minimum_history_length;

	minimum_history_length = gtk_widget_get_allocated_width(widget) / monitor->step_size;
	if (monitor->history->len < minimum_history_length)
		g_array_set_size (monitor->history, minimum_history_length + 1);

	xtm_process_monitor_paint (monitor, cr);
	return FALSE;
}
#else
static gboolean
xtm_process_monitor_expose (GtkWidget *widget, GdkEventExpose *event)
{
	XtmProcessMonitor *monitor = XTM_PROCESS_MONITOR (widget);
	guint minimum_history_length;
	cairo_t *cr;

	minimum_history_length = widget->allocation.width / monitor->step_size;
	if (monitor->history->len < minimum_history_length)
		g_array_set_size (monitor->history, minimum_history_length + 1);

	cr = gdk_cairo_create (gtk_widget_get_window(GTK_WIDGET(monitor)));
	xtm_process_monitor_paint (monitor, cr);
	cairo_destroy (cr);
	return FALSE;
}
#endif

static cairo_surface_t *
xtm_process_monitor_graph_surface_create (XtmProcessMonitor *monitor, gint width, gint height)
{
	cairo_t *cr;
	cairo_surface_t *graph_surface;
	gfloat *peak;
	gfloat step_size;
	gint i;

	if (monitor->history->len <= 1)
	{
		g_warning ("Cannot paint graph with n_peak <= 1");
		return NULL;
	}
	step_size = monitor->step_size;

	graph_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	cr = cairo_create (graph_surface);

	/* Draw area below the line, distinguish between CPU (0) and Mem (1) color-wise */
	if (monitor->type == 0)
		cairo_set_source_rgba (cr, 1.0, 0.43, 0.0, 0.3);
	else
		cairo_set_source_rgba (cr, 0.67, 0.09, 0.32, 0.3);
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

	/* Draw line */
	cairo_translate (cr, step_size * i, 0);

	if (monitor->type == 0)
		cairo_set_source_rgba (cr, 1.0, 0.43, 0.0, 1.0);
	else
		cairo_set_source_rgba (cr, 0.67, 0.09, 0.32, 1.0);
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
xtm_process_monitor_paint (XtmProcessMonitor *monitor, cairo_t *cr)
{
	cairo_surface_t *graph_surface;
	gint width, height;
	static const double dashed[] = {1.5};
	gint i;
#if GTK_CHECK_VERSION(3, 0, 0)
	width = gtk_widget_get_allocated_width(GTK_WIDGET(monitor));
	height = gtk_widget_get_allocated_height(GTK_WIDGET(monitor));
#else
	width = GTK_WIDGET (monitor)->allocation.width;
	height = GTK_WIDGET (monitor)->allocation.height;
#endif
	/* Don't draw anything if the graph is too small */
	if (height < 3)
		return;

	/* Paint the graph's background box */
	cairo_rectangle (cr, 0.0, 0.0, width, height);
	cairo_set_source_rgb (cr, 0.96, 0.96, 0.96);
	cairo_fill_preserve (cr);
	cairo_set_source_rgb (cr, 0.0, 0.0, 0.0); 
	cairo_set_line_width (cr, 0.75);
	cairo_stroke (cr);
  
	/* Paint dashed lines at 25%, 50% and 75% */
	cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.3); 
	cairo_set_line_width (cr, 1.0);
	cairo_set_dash(cr, dashed, 1.0, 0);
	for (i = 25; i <= 75; i += 25)
	{
		cairo_move_to (cr, 1.5, i * height / 100 + 0.5);
		cairo_line_to (cr, width - 0.5, i * height / 100 + 0.5);
		cairo_stroke (cr);
	}

	/* Paint the graph on a slightly smaller surface not to overlap with the background box */
	graph_surface = xtm_process_monitor_graph_surface_create (monitor, width - 1, height - 1);
	if (graph_surface != NULL)
	{
		cairo_set_source_surface (cr, graph_surface, 0.0, 0.0);
		cairo_paint (cr);
		cairo_surface_destroy (graph_surface);
	}
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

	if (GDK_IS_WINDOW (gtk_widget_get_window (GTK_WIDGET(monitor))))
		gdk_window_invalidate_rect (gtk_widget_get_window (GTK_WIDGET(monitor)), NULL, FALSE);
}

void
xtm_process_monitor_set_step_size (XtmProcessMonitor *monitor, gfloat step_size)
{
	g_return_if_fail (XTM_IS_PROCESS_MONITOR (monitor));
	g_object_set (monitor, "step_size", step_size, NULL);
	if (GDK_IS_WINDOW (gtk_widget_get_window (GTK_WIDGET(monitor))))
		gdk_window_invalidate_rect (gtk_widget_get_window (GTK_WIDGET(monitor)), NULL, FALSE);
}

void
xtm_process_monitor_set_type (XtmProcessMonitor *monitor, gint type)
{
	g_return_if_fail (XTM_IS_PROCESS_MONITOR (monitor));
	g_object_set (monitor, "type", type, NULL);
}

void
xtm_process_monitor_clear (XtmProcessMonitor *monitor)
{
	g_return_if_fail (XTM_IS_PROCESS_MONITOR (monitor));
	g_array_set_size (monitor->history, 0);
	if (GDK_IS_WINDOW (gtk_widget_get_window (GTK_WIDGET(monitor))))
		gdk_window_invalidate_rect (gtk_widget_get_window (GTK_WIDGET(monitor)), NULL, FALSE);
}
