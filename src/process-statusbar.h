/*
 * Copyright (c) 2010 Mike Massonnet, <mmassonnet@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef PROCESS_STATUSBAR_H
#define PROCESS_STATUSBAR_H

#include <glib-object.h>
#include <gtk/gtk.h>

#define XTM_TYPE_PROCESS_STATUSBAR (xtm_process_statusbar_get_type ())
G_DECLARE_FINAL_TYPE (XtmProcessStatusbar, xtm_process_statusbar, XTM, PROCESS_STATUSBAR, GtkBox)

GtkWidget *xtm_process_statusbar_new (void);

#endif /* !PROCESS_STATUSBAR_H */
