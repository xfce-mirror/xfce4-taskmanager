/*
 * Copyright (c) 2010 Mike Massonnet, <mmassonnet@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "task-manager.h"



typedef struct _XtmTaskManagerClass XtmTaskManagerClass;
struct _XtmTaskManagerClass
{
	GObjectClass		parent_class;
};
struct _XtmTaskManager
{
	GObject			parent;
	/*<private>*/
	GArray *		tasks;
	guint			owner_uid;
	gchar *			owner_uid_name;
	gchar *			hostname;
	gushort			cpu_count;
	gfloat			cpu_user;
	gfloat			cpu_system;
	guint64			memory_total;
	guint64			memory_free;
	guint64			memory_cache;
	guint64			memory_buffers;
	guint64			swap_total;
	guint64			swap_free;
};
G_DEFINE_TYPE (XtmTaskManager, xtm_task_manager, G_TYPE_OBJECT)

static void	xtm_task_manager_finalize			(GObject *object);

static void	get_owner_uid					(guint *owner_uid, gchar **owner_uid_name);
static gchar *	get_hostname					();



static void
xtm_task_manager_class_init (XtmTaskManagerClass *klass)
{
	GObjectClass *class = G_OBJECT_CLASS (klass);
	xtm_task_manager_parent_class = g_type_class_peek_parent (klass);
	class->finalize = xtm_task_manager_finalize;
}

static void
xtm_task_manager_init (XtmTaskManager *manager)
{
	manager->tasks = g_array_new (FALSE, FALSE, sizeof (Task));
	get_owner_uid (&(manager->owner_uid), &(manager->owner_uid_name));
	manager->hostname = get_hostname ();
}

static void
xtm_task_manager_finalize (GObject *object)
{
	XtmTaskManager *manager = XTM_TASK_MANAGER (object);
	g_array_free (manager->tasks, TRUE);
	g_free (manager->owner_uid_name);
	g_free (manager->hostname);
}

static void
get_owner_uid (guint *owner_uid, gchar **owner_uid_name)
{
	uid_t uid;
	struct passwd *pw;
	gchar *username = NULL;

	uid = getuid ();
	pw = getpwuid (uid);

	username = g_strdup ((pw != NULL) ? pw->pw_name : "nobody");

	*owner_uid = (guint) uid;
	*owner_uid_name = username;
}

static gchar *
get_hostname ()
{
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif
	char hostname[HOST_NAME_MAX];
	if (gethostname (hostname, HOST_NAME_MAX))
		return g_strdup ("(unknown)");
	return g_strdup_printf ("%s", hostname);
}



XtmTaskManager *
xtm_task_manager_new ()
{
	return g_object_new (XTM_TYPE_TASK_MANAGER, NULL);
}

const gchar *
xtm_task_manager_get_username (XtmTaskManager *manager)
{
	g_return_val_if_fail (G_LIKELY (XTM_IS_TASK_MANAGER (manager)), NULL);
	return manager->owner_uid_name;
}

const gchar *
xtm_task_manager_get_hostname (XtmTaskManager *manager)
{
	g_return_val_if_fail (G_LIKELY (XTM_IS_TASK_MANAGER (manager)), NULL);
	return manager->hostname;
}

void
xtm_task_manager_get_system_info (XtmTaskManager *manager, guint *num_processes, gfloat *cpu, gfloat *memory, gfloat *swap)
{
	guint64 memory_used, swap_used;

	/* Set number of processes */
	*num_processes = manager->tasks->len;

	/* Set memory and swap usage */
	get_memory_usage (&manager->memory_total, &manager->memory_free, &manager->memory_cache, &manager->memory_buffers,
			&manager->swap_total, &manager->swap_free);

	memory_used = manager->memory_total - manager->memory_free - manager->memory_cache - manager->memory_buffers;
	swap_used = manager->swap_total - manager->swap_free;

	*memory = (manager->memory_total != 0) ? memory_used * 100 / (gdouble)manager->memory_total : 0;
	*swap = (manager->swap_total != 0) ? swap_used * 100 / (gdouble)manager->swap_total : 0;

	/* Set CPU usage */
	get_cpu_usage (&manager->cpu_count, &manager->cpu_user, &manager->cpu_system);
	*cpu = manager->cpu_user + manager->cpu_system;
}

const GArray *
xtm_task_manager_get_task_list (XtmTaskManager *manager)
{
	GArray *array;
	guint i;

	if (manager->tasks->len == 0)
	{
		get_task_list (manager->tasks);
#if 1|DEBUG
		{
			gint i;
			for (i = 0; i < manager->tasks->len; i++)
			{
				Task *task = &g_array_index (manager->tasks, Task, i);
				g_print ("%5d %5s %15s %.50s\n", task->pid, task->uid_name, task->name, task->cmdline);
			}
		}
#endif
		return manager->tasks;
	}

	/* Retrieve new task list */
	array = g_array_new (FALSE, FALSE, sizeof (Task));
	get_task_list (array);

	/* Remove terminated tasks */
	for (i = 0; i < manager->tasks->len; i++)
	{
		guint j;
		Task *task = &g_array_index (manager->tasks, Task, i);
		gboolean found = FALSE;

		for (j = 0; j < array->len; j++)
		{
			Task *tasktmp = &g_array_index (array, Task, j);
			if (task->pid != tasktmp->pid)
				continue;
			found = TRUE;
			break;
		}

		if (found == FALSE)
			g_array_remove_index (manager->tasks, i);
	}

	/* Append started tasks and update existing ones */
	for (i = 0; i < array->len; i++)
	{
		guint j;
		Task *tasktmp = &g_array_index (array, Task, i);
		gboolean found = FALSE;

		for (j = 0; j < manager->tasks->len; j++)
		{
			Task *task = &g_array_index (manager->tasks, Task, j);
			if (task->pid != tasktmp->pid)
				continue;

			found = TRUE;

			task->ppid = tasktmp->ppid;
			if (g_strcmp0 (task->state, tasktmp->state))
				g_strlcpy (task->state, tasktmp->state, sizeof (task->state));
			task->cpu_user = tasktmp->cpu_user;
			task->cpu_system = tasktmp->cpu_system;
			task->rss = tasktmp->rss;
			task->vsz = tasktmp->vsz;
			task->prio = tasktmp->prio;
			break;
		}

		if (found == FALSE)
			g_array_append_val (manager->tasks, tasktmp);
	}

	g_array_free (array, TRUE);

	return manager->tasks;
}

void
xtm_task_manager_send_signal_to_pid (XtmTaskManager *manager)
{
}

void
xtm_task_manager_set_priority_to_pid (XtmTaskManager *manager)
{
}

