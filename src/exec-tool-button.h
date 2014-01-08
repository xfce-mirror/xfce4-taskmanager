/*
 * Copyright (c) 2010 Mike Massonnet, <mmassonnet@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef EXEC_TOOL_BUTTON_H
#define EXEC_TOOL_BUTTON_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib-object.h>
#include <gtk/gtk.h>

#define XTM_TYPE_EXEC_TOOL_BUTTON		(xtm_exec_tool_button_get_type ())
#define XTM_EXEC_TOOL_BUTTON(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), XTM_TYPE_EXEC_TOOL_BUTTON, XtmExecToolButton))
#define XTM_EXEC_TOOL_BUTTON_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), XTM_TYPE_EXEC_TOOL_BUTTON, XtmExecToolButtonClass))
#define XTM_IS_EXEC_TOOL_BUTTON(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), XTM_TYPE_EXEC_TOOL_BUTTON))
#define XTM_IS_EXEC_TOOL_BUTTON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), XTM_TYPE_EXEC_TOOL_BUTTON))
#define XTM_EXEC_TOOL_BUTTON_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), XTM_TYPE_EXEC_TOOL_BUTTON, XtmExecToolButtonClass))

typedef struct _XtmExecToolButton XtmExecToolButton;

GType		xtm_exec_tool_button_get_type			(void);
GtkWidget *	xtm_exec_tool_button_new			(void);

#endif /* !EXEC_TOOL_BUTTON_H */
