/*
 * Copyright (c) 2010 Mike Massonnet, <mmassonnet@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib-object.h>

typedef struct _Task Task;
struct _Task
{
	guint uid;
	gchar uid_name[64];
	guint pid;
	guint ppid;
	gchar program_name[64];
	gchar full_cmdline[255];
	gchar state[16];
	gushort cpu;
	guint64 memory_vsz;
	guint64 memory_rss;
	gushort priority;
};

#define XTM_TYPE_TASK_MANAGER			(xtm_task_manager_get_type ())
#define XTM_TASK_MANAGER(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), XTM_TYPE_TASK_MANAGER, XtmTaskManager))
#define XTM_TASK_MANAGER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), XTM_TYPE_TASK_MANAGER, XtmTaskManagerClass))
#define XTM_IS_TASK_MANAGER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), XTM_TYPE_TASK_MANAGER))
#define XTM_IS_TASK_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), XTM_TYPE_TASK_MANAGER))
#define XTM_TASK_MANAGER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), XTM_TYPE_TASK_MANAGER, XtmTaskManagerClass))

typedef struct _XtmTaskManager XtmTaskManager;

GType			xtm_task_manager_get_type			(void);
XtmTaskManager *	xtm_task_manager_new				();
const gchar *		xtm_task_manager_get_username			(XtmTaskManager *manager);
const gchar *		xtm_task_manager_get_hostname			(XtmTaskManager *manager);

#endif /* !TASK_MANAGER_H */
