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
#include <gtk/gtk.h>

/**
 * Task struct used as elements of a task list GArray.
 */

typedef struct _Task Task;
struct _Task
{
	guint		uid;
	gchar		uid_name[256];
	guint		pid;
	guint		ppid;
	gchar		name[256];
	gchar		cmdline[1024];
	gchar		state[16];
	gfloat		cpu_user;
	gfloat		cpu_system;
	guint64		vsz;
	guint64		rss;
	gshort		prio;
};

/**
 * OS specific implementation.
 */

gboolean	get_memory_usage	(guint64 *memory_total, guint64 *memory_free, guint64 *memory_cache, guint64 *memory_buffers, guint64 *swap_total, guint64 *swap_free);
gboolean	get_cpu_usage		(gushort *cpu_count, gfloat *cpu_user, gfloat *cpu_system);
gboolean	get_task_list		(GArray *task_list);
gboolean	pid_is_sleeping		(guint pid);

/**
 * GObject class used to update the graphical widgets.
 */

#define XTM_TYPE_TASK_MANAGER			(xtm_task_manager_get_type ())
#define XTM_TASK_MANAGER(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), XTM_TYPE_TASK_MANAGER, XtmTaskManager))
#define XTM_TASK_MANAGER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), XTM_TYPE_TASK_MANAGER, XtmTaskManagerClass))
#define XTM_IS_TASK_MANAGER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), XTM_TYPE_TASK_MANAGER))
#define XTM_IS_TASK_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), XTM_TYPE_TASK_MANAGER))
#define XTM_TASK_MANAGER_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), XTM_TYPE_TASK_MANAGER, XtmTaskManagerClass))

typedef struct _XtmTaskManager XtmTaskManager;

GType			xtm_task_manager_get_type			(void);
XtmTaskManager *	xtm_task_manager_new				(GtkTreeModel *model);
const gchar *		xtm_task_manager_get_username			(XtmTaskManager *manager);
const gchar *		xtm_task_manager_get_hostname			(XtmTaskManager *manager);
void			xtm_task_manager_get_system_info		(XtmTaskManager *manager, guint *num_processes, gfloat *cpu, gfloat *memory, gfloat *swap);
void			xtm_task_manager_get_swap_usage			(XtmTaskManager *manager, guint64 *swap_free, guint64 *swap_total);
const GArray *		xtm_task_manager_get_task_list			(XtmTaskManager *manager);
void			xtm_task_manager_update_model			(XtmTaskManager *manager);

/**
 * Helper functions.
 */

enum
{
	XTM_SIGNAL_TERMINATE = 0,
	XTM_SIGNAL_STOP,
	XTM_SIGNAL_CONTINUE,
	XTM_SIGNAL_KILL,
};

enum
{
	XTM_PRIORITY_VERY_LOW = 0,
	XTM_PRIORITY_LOW,
	XTM_PRIORITY_NORMAL,
	XTM_PRIORITY_HIGH,
	XTM_PRIORITY_VERY_HIGH,
};

void		get_owner_uid		(guint *owner_uid, gchar **owner_uid_name);
gchar *		get_hostname		();
gboolean	send_signal_to_pid	(guint pid, gint xtm_signal);
gboolean	set_priority_to_pid	(guint pid, gint priority);

#endif /* !TASK_MANAGER_H */
