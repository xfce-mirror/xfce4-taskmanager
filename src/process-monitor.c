/*
 * Copyright (c) 2024 Jehan-Antoine Vayssade, <javayss@sleek-think.ovh>
 * Copyright (c) 2010 Mike Massonnet, <mmassonnet@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "process-monitor.h"
#include "task-manager.h"

#include <cairo.h>

#define MAX_GRAPH 3

enum
{
	PROP_STEP_SIZE = 1,
	PROP_TYPE,
};
typedef struct _XtmProcessMonitorClass XtmProcessMonitorClass;
struct _XtmProcessMonitorClass
{
	GtkDrawingAreaClass parent_class;
};
struct _XtmProcessMonitor
{
	GtkDrawingArea parent;
	/*<private>*/
	gfloat step_size;
	gint type;
	GArray *history[MAX_GRAPH];
	gfloat max[MAX_GRAPH];
	gboolean dark_mode;
};
G_DEFINE_TYPE (XtmProcessMonitor, xtm_process_monitor, GTK_TYPE_DRAWING_AREA)

static void	xtm_process_monitor_finalize (GObject *object);
static void	xtm_process_monitor_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void xtm_process_monitor_cairo_set_color (XtmProcessMonitor *monitor, cairo_t *cr, gfloat alpha, guint variant);
static void	xtm_process_monitor_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean	xtm_process_monitor_draw (GtkWidget *widget, cairo_t *cr);
static void	xtm_process_monitor_paint (XtmProcessMonitor *monitor, cairo_t *cr);

static void
xtm_process_monitor_class_init (XtmProcessMonitorClass *klass)
{
	GObjectClass *class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	xtm_process_monitor_parent_class = g_type_class_peek_parent (klass);
	class->finalize = xtm_process_monitor_finalize;
	class->get_property = xtm_process_monitor_get_property;
	class->set_property = xtm_process_monitor_set_property;
	widget_class->draw = xtm_process_monitor_draw;

	g_object_class_install_property (class, PROP_STEP_SIZE,
		g_param_spec_float ("step-size", "StepSize", "Step size", 0.1f, G_MAXFLOAT, 1, G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
	g_object_class_install_property (class, PROP_TYPE,
		g_param_spec_int ("type", "Type", "Type of graph to render", 0, G_MAXINT, 0, G_PARAM_READWRITE));
}

static void
xtm_process_monitor_on_notify_theme_name (XtmProcessMonitor *monitor)
{
	monitor->dark_mode = xtm_gtk_widget_is_dark_mode (GTK_WIDGET (monitor));
}

static void
xtm_process_monitor_init (XtmProcessMonitor *monitor)
{
	GtkSettings *settings = gtk_settings_get_default ();

	for(int graph=0; graph<MAX_GRAPH; ++graph)
	{
		monitor->history[graph] = g_array_new (FALSE, TRUE, sizeof (gfloat));
		monitor->max[graph] = 1.0;
	}

	monitor->dark_mode = xtm_gtk_widget_is_dark_mode (GTK_WIDGET (monitor));

	g_signal_connect_swapped (settings, "notify::gtk-theme-name", G_CALLBACK (xtm_process_monitor_on_notify_theme_name), monitor);
	g_signal_connect (monitor, "realize", G_CALLBACK (xtm_process_monitor_on_notify_theme_name), NULL);
}

static void
xtm_process_monitor_finalize (GObject *object)
{
	XtmProcessMonitor *monitor = XTM_PROCESS_MONITOR (object);

	for(int graph=0; graph<MAX_GRAPH; ++graph)
		g_array_free (monitor->history[graph], TRUE);

	G_OBJECT_CLASS (xtm_process_monitor_parent_class)->finalize (object);
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

static gboolean
xtm_process_monitor_draw (GtkWidget *widget, cairo_t *cr)
{
	XtmProcessMonitor *monitor = XTM_PROCESS_MONITOR (widget);
	guint minimum_history_length;

	minimum_history_length = (guint)(gtk_widget_get_allocated_width(widget) / monitor->step_size);

	for(int graph=0; graph<MAX_GRAPH; ++graph)
		if (monitor->history[graph]->len < minimum_history_length)
			g_array_set_size (monitor->history[graph], minimum_history_length + 1);

	xtm_process_monitor_paint (monitor, cr);
	return FALSE;
}

static void
xtm_process_monitor_cairo_set_source_rgba (cairo_t *cr, double red, double green, double blue, double alpha, gboolean invert)
{
	if (invert)
		cairo_set_source_rgba (cr, 1.0 - red, 1.0 - green, 1.0 - blue, alpha);
	else
		cairo_set_source_rgba (cr, red, green, blue, alpha);
}

static void
xtm_process_monitor_cairo_set_source_rgb (cairo_t *cr, double red, double green, double blue, gboolean invert)
{
	if (invert)
		cairo_set_source_rgb (cr, 1.0 - red, 1.0 - green, 1.0 - blue);
	else
		cairo_set_source_rgb (cr, red, green, blue);
}

static void
xtm_process_monitor_cairo_set_color(XtmProcessMonitor *monitor, cairo_t *cr, gfloat alpha, guint variant) {
	if (monitor->type == 0)
		xtm_process_monitor_cairo_set_source_rgba (cr, 1.0, 0.43, 0.0, alpha, monitor->dark_mode);
	else if (monitor->type == 1)
    {
		//xtm_process_monitor_cairo_set_source_rgba (cr, 0.67, 0.09, 0.32, alpha, monitor->dark_mode);
        if(variant == 0)
		    xtm_process_monitor_cairo_set_source_rgba (cr, 0.80, 0.22, 0.42, alpha, monitor->dark_mode);
        else
		    xtm_process_monitor_cairo_set_source_rgba (cr, 0.50, 0.50, 0.50, alpha, monitor->dark_mode);
    }
	else
    {
        if(variant == 0)
		    xtm_process_monitor_cairo_set_source_rgba (cr, 0.00, 0.42, 0.64, alpha, monitor->dark_mode);
        else if(variant == 1)
		    xtm_process_monitor_cairo_set_source_rgba (cr, 0.02, 0.71, 0.81, alpha, monitor->dark_mode);
        else
		    xtm_process_monitor_cairo_set_source_rgba (cr, 0.00, 1.00, 1.00, alpha, monitor->dark_mode);
    }
}

static cairo_surface_t *
xtm_process_monitor_graph_surface_create (XtmProcessMonitor *monitor, gint width, gint height)
{
	cairo_t *cr;
	cairo_surface_t *graph_surface;
	gdouble peak, step_size;
	gint i;

	step_size = (gdouble)monitor->step_size;

	graph_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	cr = cairo_create (graph_surface);

	/* Draw line and area below the line, distinguish between CPU (0) and Mem (1) color-wise */
	cairo_set_line_width (cr, 0.85);
	cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
	cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
	cairo_set_antialias (cr, CAIRO_ANTIALIAS_DEFAULT);

	for(int graph=0; graph<MAX_GRAPH; ++graph)
	{
    	if (monitor->history[graph]->len <= 1)
           continue;

		cairo_move_to (cr, width, height);
	    cairo_translate (cr, step_size, 0);
		xtm_process_monitor_cairo_set_color(monitor, cr, 0.3, graph);

		for (i = 0; (step_size * (i - 1)) <= width; i++)
		{
			peak = g_array_index (monitor->history[graph], gfloat, i) / monitor->max[graph];
			cairo_translate (cr, -step_size, 0);
			cairo_line_to (cr, width, (1.0 - peak) * height);
		}

		cairo_line_to (cr, width, height);
		cairo_fill_preserve (cr);
		xtm_process_monitor_cairo_set_color(monitor, cr, 1.0, graph);
		cairo_stroke (cr);

		cairo_translate (cr, width, 0);
	}

	cairo_destroy (cr);

	return graph_surface;
}

static void
xtm_process_monitor_paint (XtmProcessMonitor *monitor, cairo_t *cr)
{
	cairo_surface_t *graph_surface;
	gint width, height;
	static const double dashed[] = { 1.5 };
	gint i;
	width = gtk_widget_get_allocated_width (GTK_WIDGET (monitor));
	height = gtk_widget_get_allocated_height (GTK_WIDGET (monitor));

	/* Don't draw anything if the graph is too small */
	if (height < 3)
		return;

	/* Paint the graph's background box */
	cairo_rectangle (cr, 0.0, 0.0, width, height);
	xtm_process_monitor_cairo_set_source_rgb (cr, 0.96, 0.96, 0.96, monitor->dark_mode);
	cairo_fill_preserve (cr);
	xtm_process_monitor_cairo_set_source_rgb (cr, 0.0, 0.0, 0.0, monitor->dark_mode);
	cairo_set_line_width (cr, 0.75);
	cairo_stroke (cr);

	/* Paint dashed lines at 25%, 50% and 75% */
	xtm_process_monitor_cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.3, monitor->dark_mode);
	cairo_set_line_width (cr, 1.0);
	cairo_set_dash (cr, dashed, 1.0, 0);
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
xtm_process_monitor_add_peak (XtmProcessMonitor *monitor, gfloat peak, gint index)
{
	g_return_if_fail (XTM_IS_PROCESS_MONITOR (monitor));
	//g_return_if_fail (peak >= 0.0f && peak <= 1.0f);

	if (peak > monitor->max[index])
		monitor->max[index] = peak;

	g_array_prepend_val (monitor->history[index], peak);
	if (monitor->history[index]->len > 1)
		g_array_remove_index (monitor->history[index], monitor->history[index]->len - 1);

	if (GDK_IS_WINDOW (gtk_widget_get_window (GTK_WIDGET (monitor))))
		gdk_window_invalidate_rect (gtk_widget_get_window (GTK_WIDGET (monitor)), NULL, FALSE);
}

void
xtm_process_monitor_set_step_size (XtmProcessMonitor *monitor, gfloat step_size)
{
	g_return_if_fail (XTM_IS_PROCESS_MONITOR (monitor));
	g_object_set (monitor, "step_size", step_size, NULL);
	if (GDK_IS_WINDOW (gtk_widget_get_window (GTK_WIDGET (monitor))))
		gdk_window_invalidate_rect (gtk_widget_get_window (GTK_WIDGET (monitor)), NULL, FALSE);
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
	for(int graph=0; graph<MAX_GRAPH; ++graph)
		g_array_set_size (monitor->history[graph], 0);
	if (GDK_IS_WINDOW (gtk_widget_get_window (GTK_WIDGET(monitor))))
		gdk_window_invalidate_rect (gtk_widget_get_window (GTK_WIDGET(monitor)), NULL, FALSE);
}
