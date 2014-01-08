/*
 * Copyright (c) 2010 Mike Massonnet, <mmassonnet@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SETTINGS_TOOL_BUTTON_H
#define SETTINGS_TOOL_BUTTON_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib-object.h>
#include <gtk/gtk.h>

#define XTM_TYPE_SETTINGS_TOOL_BUTTON			(xtm_settings_tool_button_get_type ())
#define XTM_SETTINGS_TOOL_BUTTON(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), XTM_TYPE_SETTINGS_TOOL_BUTTON, XtmSettingsToolButton))
#define XTM_SETTINGS_TOOL_BUTTON_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), XTM_TYPE_SETTINGS_TOOL_BUTTON, XtmSettingsToolButtonClass))
#define XTM_IS_SETTINGS_TOOL_BUTTON(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), XTM_TYPE_SETTINGS_TOOL_BUTTON))
#define XTM_IS_SETTINGS_TOOL_BUTTON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), XTM_TYPE_SETTINGS_TOOL_BUTTON))
#define XTM_SETTINGS_TOOL_BUTTON_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), XTM_TYPE_SETTINGS_TOOL_BUTTON, XtmSettingsToolButtonClass))

typedef struct _XtmSettingsToolButton XtmSettingsToolButton;

GType		xtm_settings_tool_button_get_type		(void);
GtkWidget *	xtm_settings_tool_button_new			(void);

#endif /* !SETTINGS_TOOL_BUTTON_H */
