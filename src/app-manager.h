/*
 * Copyright (c) 2010 Mike Massonnet, <mmassonnet@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef APP_MANAGER_H
#define APP_MANAGER_H

#include <glib-object.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

typedef struct _App App;
struct _App
{
	WnckApplication *application;
	GPid pid;
	gchar name[1024];
	cairo_surface_t *surface;
};

#define XTM_TYPE_APP_MANAGER (xtm_app_manager_get_type ())
G_DECLARE_FINAL_TYPE (XtmAppManager, xtm_app_manager, XTM, APP_MANAGER, GObject)

XtmAppManager *xtm_app_manager_new (void);
App *xtm_app_manager_get_app_from_pid (XtmAppManager *manager, GPid pid);

#endif /* !APP_MANAGER_H */
