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
#include <signal.h>
#include <sys/resource.h>

#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "task-manager.h"
#include "process-tree-view.h" /* for the columns of the model */



typedef struct _XtmTaskManagerClass XtmTaskManagerClass;
struct _XtmTaskManagerClass
{
	GObjectClass		parent_class;
};
struct _XtmTaskManager
{
	GObject			parent;
	/*<private>*/
	GtkTreeModel *		model;
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

static void	model_add_task					(GtkTreeModel *model, Task *task);
static void	model_update_tree_iter				(GtkTreeModel *model, GtkTreeIter *iter, Task *task);
static void	model_update_task				(GtkTreeModel *model, Task *task);
static void	model_remove_task				(GtkTreeModel *model, Task *task);



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
_xtm_task_manager_set_model (XtmTaskManager *manager, GtkTreeModel *model)
{
	g_return_if_fail (G_LIKELY (XTM_IS_TASK_MANAGER (manager)));
	g_return_if_fail (G_LIKELY (GTK_IS_TREE_MODEL (model)));
	manager->model = model;
}

static void
model_add_task (GtkTreeModel *model, Task *task)
{
	GtkTreeIter iter;
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		XTM_PTV_COLUMN_COMMAND, task->cmdline,
		XTM_PTV_COLUMN_PID, task->pid,
		XTM_PTV_COLUMN_STATE, task->state,
		XTM_PTV_COLUMN_UID, task->uid,
		XTM_PTV_COLUMN_UID_STR, task->uid_name,
		-1);
	model_update_tree_iter (model, &iter, task);
}

static void
memory_human_size (guint64 mem, gchar *mem_str)
{
	guint64 mem_tmp;

	mem_tmp = mem / 1024 / 1024;
	if (mem_tmp > 3)
	{
		g_snprintf (mem_str, 64, _("%lu MiB"), mem_tmp);
		return;
	}

	mem_tmp = mem / 1024;
	if (mem_tmp > 8)
	{
		g_snprintf (mem_str, 64, _("%lu KiB"), mem_tmp);
		return;
	}

	g_snprintf (mem_str, 64, _("%lu B"), mem);
}

static void
model_update_tree_iter (GtkTreeModel *model, GtkTreeIter *iter, Task *task)
{
	gchar vsz[64], rss[64], cpu[16];

	memory_human_size (task->vsz, vsz);
	memory_human_size (task->rss, rss);
	// TODO make precision optional
	g_snprintf (cpu, 16, _("%.2f%%"), task->cpu_user + task->cpu_system);

	gtk_list_store_set (GTK_LIST_STORE (model), iter,
		XTM_PTV_COLUMN_PPID, task->ppid,
		XTM_PTV_COLUMN_STATE, task->state,
		XTM_PTV_COLUMN_VSZ, task->vsz,
		XTM_PTV_COLUMN_VSZ_STR, vsz,
		XTM_PTV_COLUMN_RSS, task->rss,
		XTM_PTV_COLUMN_RSS_STR, rss,
		XTM_PTV_COLUMN_CPU, task->cpu_user + task->cpu_system,
		XTM_PTV_COLUMN_CPU_STR, cpu,
		XTM_PTV_COLUMN_PRIORITY, task->prio,
		-1);
}

static void
model_find_tree_iter_for_pid (GtkTreeModel *model, guint pid, GtkTreeIter *iter)
{
	gboolean valid;
	guint pid_iter;

	valid = gtk_tree_model_get_iter_first (model, iter);
	while (valid)
	{
		gtk_tree_model_get (model, iter, XTM_PTV_COLUMN_PID, &pid_iter, -1);
		if (pid == pid_iter)
			break;
		valid = gtk_tree_model_iter_next (model, iter);
	}
}

static void
model_update_task (GtkTreeModel *model, Task *task)
{
	GtkTreeIter iter;
	model_find_tree_iter_for_pid (model, task->pid, &iter);
	model_update_tree_iter (model, &iter, task);
}

static void
model_remove_task (GtkTreeModel *model, Task *task)
{
	GtkTreeIter iter;
	model_find_tree_iter_for_pid (model, task->pid, &iter);
	gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
}



XtmTaskManager *
xtm_task_manager_new (GtkTreeModel *model)
{
	XtmTaskManager *manager = g_object_new (XTM_TYPE_TASK_MANAGER, NULL);
	_xtm_task_manager_set_model (manager, model);
	return manager;
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
	xtm_task_manager_update_model (manager);
	return manager->tasks;
}

void
xtm_task_manager_update_model (XtmTaskManager *manager)
{
	GArray *array;
	GtkTreeIter iter;
	gboolean valid;
	guint i;

	/* Retrieve initial task list and return */
	if (manager->tasks->len == 0)
	{
		get_task_list (manager->tasks);
		for (i = 0; i < manager->tasks->len; i++)
		{
			Task *task = &g_array_index (manager->tasks, Task, i);
			model_add_task (manager->model, task);
#if 1|DEBUG
			g_print ("%5d %5s %15s %.50s\n", task->pid, task->uid_name, task->name, task->cmdline);
#endif
		}
		return;
	}

	/* Retrieve new task list */
	array = g_array_new (FALSE, FALSE, sizeof (Task));
	get_task_list (array);

	/* Remove terminated tasks */
	for (i = 0; i < manager->tasks->len; i++)
	{
		guint j;
		Task *tasktmp;
		Task *task = &g_array_index (manager->tasks, Task, i);
		gboolean found = FALSE;

		for (j = 0; j < array->len; j++)
		{
			tasktmp = &g_array_index (array, Task, j);
			if (task->pid != tasktmp->pid)
				continue;
			found = TRUE;
			break;
		}

		if (found == FALSE)
		{
#if DEBUG
			g_debug ("Remove old task %d %s", task->pid, task->name);
#endif
			model_remove_task (manager->model, task);
			g_array_remove_index (manager->tasks, i);
		}
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

			/* Update the model (with the rest) only if needed, this keeps the CPU cool */
			if (task->ppid != tasktmp->ppid
				|| g_strcmp0 (task->state, tasktmp->state)
				|| task->cpu_user != tasktmp->cpu_user
				|| task->cpu_system != tasktmp->cpu_system
				|| task->rss != tasktmp->rss
				|| task->vsz != tasktmp->vsz
				|| task->prio != tasktmp->prio)
			{
				task->ppid = tasktmp->ppid;
				g_strlcpy (task->state, tasktmp->state, sizeof (task->state));
				task->cpu_user = tasktmp->cpu_user;
				task->cpu_system = tasktmp->cpu_system;
				task->rss = tasktmp->rss;
				task->vsz = tasktmp->vsz;
				task->prio = tasktmp->prio;
				model_update_task (manager->model, tasktmp);
			}

			break;
		}

		if (found == FALSE)
		{
#if DEBUG
			g_debug ("Add new task %d %s", tasktmp->pid, tasktmp->name);
#endif
			model_add_task (manager->model, tasktmp);
			g_array_append_val (manager->tasks, *tasktmp);
		}
	}

	g_array_free (array, TRUE);

	return;
}



void
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

gchar *
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

gboolean
send_signal_to_pid (guint pid, gint signal)
{
	gint sig;
	gint res;
	switch (signal)
	{
		case XTM_SIGNAL_TERMINATE:
			sig = SIGTERM;
			break;
		case XTM_SIGNAL_STOP:
			sig = SIGSTOP;
			break;
		case XTM_SIGNAL_CONTINUE:
			sig = SIGCONT;
			break;
		case XTM_SIGNAL_KILL:
			sig = SIGKILL;
			break;
		default:
			return TRUE;
	}
	res = kill (pid, sig);
	return (res == 0) ? TRUE : FALSE;
}

gboolean
set_priority_to_pid (guint pid, gint priority)
{
	gint prio;
	gint res;
	switch (priority)
	{
		case XTM_PRIORITY_VERY_LOW:
			prio = 15;
			break;
		case XTM_PRIORITY_LOW:
			prio = 5;
			break;
		case XTM_PRIORITY_NORMAL:
			prio = 0;
			break;
		case XTM_PRIORITY_HIGH:
			prio = -5;
			break;
		case XTM_PRIORITY_VERY_HIGH:
			prio = -15;
			break;
		default:
			return TRUE;
	}
	res = setpriority (PRIO_PROCESS, pid, prio);
	return (res == 0) ? TRUE : FALSE;
}

