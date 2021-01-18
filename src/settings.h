/*
 * Copyright (c) 2010 Mike Massonnet, <mmassonnet@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib-object.h>

#define XTM_TYPE_SETTINGS			(xtm_settings_get_type ())
#define XTM_SETTINGS(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), XTM_TYPE_SETTINGS, XtmSettings))
#define XTM_SETTINGS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), XTM_TYPE_SETTINGS, XtmSettingsClass))
#define XTM_IS_SETTINGS(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), XTM_TYPE_SETTINGS))
#define XTM_IS_SETTINGS_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass), XTM_TYPE_SETTINGS))
#define XTM_SETTINGS_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), XTM_TYPE_SETTINGS, XtmSettingsClass))

typedef struct _XtmSettings XtmSettings;

void		xtm_settings_save_settings	(XtmSettings *settings);
GType		xtm_settings_get_type				(void);
XtmSettings *	xtm_settings_get_default			(void);



#endif /* !SETTINGS_H */
