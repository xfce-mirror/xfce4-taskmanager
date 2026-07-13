/*
 * Copyright (c) 2010 Mike Massonnet, <mmassonnet@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

#include <glib-object.h>
#include <gtk/gtk.h>

#define XTM_TYPE_PROCESS_MONITOR (xtm_process_monitor_get_type ())
G_DECLARE_FINAL_TYPE (XtmProcessMonitor, xtm_process_monitor, XTM, PROCESS_MONITOR, GtkDrawingArea)

GtkWidget *xtm_process_monitor_new (void);
void xtm_process_monitor_add_peak (XtmProcessMonitor *monitor, gfloat peak, gfloat peak_swap);
void xtm_process_monitor_set_step_size (XtmProcessMonitor *monitor, gfloat step_size);
void xtm_process_monitor_set_type (XtmProcessMonitor *monitor, gint type);
void xtm_process_monitor_clear (XtmProcessMonitor *monitor);

#endif /* !PROCESS_MONITOR_H */
