/*
 * Copyright (c) 2010 Mike Massonnet, <mmassonnet@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib-object.h>
#include <gtk/gtk.h>

#define XTM_TYPE_SETTINGS_DIALOG		(xtm_settings_dialog_get_type ())
#define XTM_SETTINGS_DIALOG(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), XTM_TYPE_SETTINGS_DIALOG, XtmSettingsDialog))
#define XTM_SETTINGS_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), XTM_TYPE_SETTINGS_DIALOG, XtmSettingsDialogClass))
#define XTM_IS_SETTINGS_DIALOG(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), XTM_TYPE_SETTINGS_DIALOG))
#define XTM_IS_SETTINGS_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), XTM_TYPE_SETTINGS_DIALOG))
#define XTM_SETTINGS_DIALOG_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), XTM_TYPE_SETTINGS_DIALOG, XtmSettingsDialogClass))

typedef struct _XtmSettingsDialog XtmSettingsDialog;

GType		xtm_settings_dialog_get_type			(void);
GtkWidget *	xtm_settings_dialog_new				(GtkWindow *parent_window);
void		xtm_settings_dialog_run				(XtmSettingsDialog *dialog);

#endif /* !SETTINGS_DIALOG_H */
