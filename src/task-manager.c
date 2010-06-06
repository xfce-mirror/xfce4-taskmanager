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
#ifdef HAVE_WNCK
#include "app-manager.h"
#endif
#include "process-tree-view.h" /* for the columns of the model */
#include "settings.h"

#define TIMESTAMP_DELTA 4



static XtmSettings *settings = NULL;
static gboolean model_update_forced = FALSE;
static gboolean more_precision;
static gboolean full_cmdline;



typedef struct _XtmTaskManagerClass XtmTaskManagerClass;
struct _XtmTaskManagerClass
{
	GObjectClass		parent_class;
};
struct _XtmTaskManager
{
	GObject			parent;
	/*<private>*/
#ifdef HAVE_WNCK
	XtmAppManager *		app_manager;
#endif
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

static void	setting_changed					(GObject *object, GParamSpec *pspec, XtmTaskManager *manager);
#ifdef HAVE_WNCK
static void	model_add_task					(GtkTreeModel *model, Task *task, App *app, glong timestamp);
static void	model_update_tree_iter				(GtkTreeModel *model, GtkTreeIter *iter, Task *task, App *app);
static void	model_update_task				(GtkTreeModel *model, Task *task, App *app);
#else
static void	model_add_task					(GtkTreeModel *model, Task *task, glong timestamp);
static void	model_update_tree_iter				(GtkTreeModel *model, GtkTreeIter *iter, Task *task);
static void	model_update_task				(GtkTreeModel *model, Task *task);
#endif
static void	model_mark_tree_iter_as_removed			(GtkTreeModel *model, GtkTreeIter *iter);
static void	model_remove_tree_iter				(GtkTreeModel *model, GtkTreeIter *iter);
static void	model_find_tree_iter_for_pid			(GtkTreeModel *model, guint pid, GtkTreeIter *iter);
static glong	__current_timestamp				();



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
#ifdef HAVE_WNCK
	manager->app_manager = xtm_app_manager_new ();
#endif
	manager->tasks = g_array_new (FALSE, FALSE, sizeof (Task));
	get_owner_uid (&(manager->owner_uid), &(manager->owner_uid_name));
	manager->hostname = get_hostname ();

	/* Listen to settings changes and force an update on the whole model */
	settings = xtm_settings_get_default ();
	g_object_get (settings, "more-precision", &more_precision, NULL);
	g_object_get (settings, "full-command-line", &full_cmdline, NULL);
	g_signal_connect (settings, "notify::more-precision", G_CALLBACK (setting_changed), manager);
	g_signal_connect (settings, "notify::full-command-line", G_CALLBACK (setting_changed), manager);
}

static void
xtm_task_manager_finalize (GObject *object)
{
	XtmTaskManager *manager = XTM_TASK_MANAGER (object);
	g_array_free (manager->tasks, TRUE);
	g_free (manager->owner_uid_name);
	g_free (manager->hostname);
#ifdef HAVE_WNCK
	g_object_unref (manager->app_manager);
#endif
	g_object_unref (settings);
}

static void
setting_changed (GObject *object, GParamSpec *pspec, XtmTaskManager *manager)
{
	g_object_get (object, "more-precision", &more_precision, NULL);
	g_object_get (object, "full-command-line", &full_cmdline, NULL);
	model_update_forced = TRUE;
}

static gchar *
pretty_cmdline (gchar *cmdline, gchar *comm)
{
	gchar *text = g_strchomp (g_strdelimit (g_strdup (cmdline), "\n\r", ' '));
	if (!full_cmdline && sizeof (text) > 3)
	{
		/* Shorten full path to commands and wine applications */
		if (text[0] == '/' || (g_ascii_isupper (text[0]) && text[1] == ':' && text[2] == '\\'))
		{
			gchar *p = g_strstr_len (text, -1, comm);
			if (p != NULL)
			{
				g_snprintf (text, g_utf8_strlen (text, -1), "%s", p);
			}
		}
	}
	return text;
}

static void
_xtm_task_manager_set_model (XtmTaskManager *manager, GtkTreeModel *model)
{
	g_return_if_fail (XTM_IS_TASK_MANAGER (manager));
	g_return_if_fail (GTK_IS_TREE_MODEL (model));
	manager->model = model;
}

static void
#ifdef HAVE_WNCK
model_add_task (GtkTreeModel *model, Task *task, App *app, glong timestamp)
#else
model_add_task (GtkTreeModel *model, Task *task, glong timestamp)
#endif
{
	GtkTreeIter iter;
	gchar *cmdline;

#ifdef HAVE_WNCK
	if (app != NULL && full_cmdline == FALSE)
		cmdline = g_strdup (app->name);
	else
		cmdline = pretty_cmdline (task->cmdline, task->name);
#else
	cmdline = pretty_cmdline (task->cmdline, task->name);
#endif

	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		XTM_PTV_COLUMN_COMMAND, cmdline,
		XTM_PTV_COLUMN_PID, task->pid,
		XTM_PTV_COLUMN_STATE, task->state,
		XTM_PTV_COLUMN_UID, task->uid,
		XTM_PTV_COLUMN_UID_STR, task->uid_name,
		XTM_PTV_COLUMN_BACKGROUND, NULL,
		XTM_PTV_COLUMN_FOREGROUND, NULL,
		XTM_PTV_COLUMN_TIMESTAMP, timestamp,
		-1);
#ifdef HAVE_WNCK
	model_update_tree_iter (model, &iter, task, app);
#else
	model_update_tree_iter (model, &iter, task);
#endif

	g_free (cmdline);
}

static void
model_mark_tree_iter_as_removed (GtkTreeModel *model, GtkTreeIter *iter)
{
	gtk_list_store_set (GTK_LIST_STORE (model), iter,
		XTM_PTV_COLUMN_CPU, 0.0,
		XTM_PTV_COLUMN_CPU_STR, "-",
		XTM_PTV_COLUMN_BACKGROUND, "#a40000",
		XTM_PTV_COLUMN_FOREGROUND, "#ffffff",
		XTM_PTV_COLUMN_TIMESTAMP, __current_timestamp (),
		-1);
}

static void
model_remove_tree_iter (GtkTreeModel *model, GtkTreeIter *iter)
{
	gtk_list_store_remove (GTK_LIST_STORE (model), iter);
}

static void
memory_human_size (guint64 mem, gchar *mem_str)
{
	guint64 mem_tmp;

	mem_tmp = mem / 1024 / 1024;
	if (mem_tmp > 3)
	{
		g_snprintf (mem_str, 64, _("%lu MiB"), (gulong)mem_tmp);
		return;
	}

	mem_tmp = mem / 1024;
	if (mem_tmp > 8)
	{
		g_snprintf (mem_str, 64, _("%lu KiB"), (gulong)mem_tmp);
		return;
	}

	g_snprintf (mem_str, 64, _("%lu B"), (gulong)mem);
}

static void
#ifdef HAVE_WNCK
model_update_tree_iter (GtkTreeModel *model, GtkTreeIter *iter, Task *task, App *app)
#else
model_update_tree_iter (GtkTreeModel *model, GtkTreeIter *iter, Task *task)
#endif
{
	gchar vsz[64], rss[64], cpu[16];
	gchar value[14];
	glong old_timestamp;
	gchar *old_state;
	gchar *background, *foreground;
#ifdef HAVE_WNCK
	GdkPixbuf *icon;
#endif

	memory_human_size (task->vsz, vsz);
	memory_human_size (task->rss, rss);

	g_snprintf (value, 14, (more_precision) ? "%.2f" : "%.0f", task->cpu_user + task->cpu_system);
	g_snprintf (cpu, 16, _("%s%%"), value);

	/* Retrieve values for tweaking background/foreground color and updating content as needed */
	gtk_tree_model_get (model, iter, XTM_PTV_COLUMN_TIMESTAMP, &old_timestamp, XTM_PTV_COLUMN_STATE, &old_state,
			XTM_PTV_COLUMN_BACKGROUND, &background, XTM_PTV_COLUMN_FOREGROUND, &foreground,
#ifdef HAVE_WNCK
			XTM_PTV_COLUMN_ICON, &icon,
#endif
			-1);

#ifdef HAVE_WNCK
	if (app != NULL && icon == NULL)
		gtk_list_store_set (GTK_LIST_STORE (model), iter, XTM_PTV_COLUMN_ICON, app->icon, -1);

	if (app != NULL && full_cmdline == FALSE)
	{
		gchar *cmdline = g_strdup (app->name);
		gtk_list_store_set (GTK_LIST_STORE (model), iter, XTM_PTV_COLUMN_COMMAND, cmdline, -1);
		g_free (cmdline);
	}
	else if (model_update_forced)
#else
	if (model_update_forced)
#endif
	{
		gchar *cmdline = pretty_cmdline (task->cmdline, task->name);
		gtk_list_store_set (GTK_LIST_STORE (model), iter, XTM_PTV_COLUMN_COMMAND, cmdline, -1);
		g_free (cmdline);
	}

	if (g_strcmp0 (task->state, old_state) != 0 && background == NULL)
	{
		/* Set yellow color for changing state */
		background = g_strdup ("#edd400");
		foreground = g_strdup ("#000000");
		old_timestamp = __current_timestamp () - TIMESTAMP_DELTA + 3;
	}

	if (__current_timestamp () - old_timestamp <= TIMESTAMP_DELTA)
	{
		/* Set green color for started task */
		background = (background == NULL) ? g_strdup ("#73d216") : background;
		foreground = (foreground == NULL) ? g_strdup ("#000000") : foreground;
	}
	else
	{
		/* Reset color */
		background = NULL;
		foreground = NULL;
	}

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
		XTM_PTV_COLUMN_BACKGROUND, background,
		XTM_PTV_COLUMN_FOREGROUND, foreground,
		XTM_PTV_COLUMN_TIMESTAMP, old_timestamp,
		-1);

	g_free (background);
	g_free (foreground);
	g_free (old_state);
}

static void
#ifdef HAVE_WNCK
model_update_task (GtkTreeModel *model, Task *task, App *app)
#else
model_update_task (GtkTreeModel *model, Task *task)
#endif
{
	GtkTreeIter iter;
	model_find_tree_iter_for_pid (model, task->pid, &iter);
#ifdef HAVE_WNCK
	model_update_tree_iter (model, &iter, task, app);
#else
	model_update_tree_iter (model, &iter, task);
#endif
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

static glong
__current_timestamp (void)
{
	GTimeVal current_time;
	g_get_current_time (&current_time);
	return current_time.tv_sec;
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
	g_return_val_if_fail (XTM_IS_TASK_MANAGER (manager), NULL);
	return manager->owner_uid_name;
}

const gchar *
xtm_task_manager_get_hostname (XtmTaskManager *manager)
{
	g_return_val_if_fail (XTM_IS_TASK_MANAGER (manager), NULL);
	return manager->hostname;
}

void
xtm_task_manager_get_system_info (XtmTaskManager *manager, guint *num_processes, gfloat *cpu, gfloat *memory, gfloat *swap)
{
	guint64 memory_used, swap_used;

	g_return_if_fail (XTM_IS_TASK_MANAGER (manager));

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

void
xtm_task_manager_get_swap_usage (XtmTaskManager *manager, guint64 *swap_free, guint64 *swap_total)
{
	g_return_if_fail (XTM_IS_TASK_MANAGER (manager));
	*swap_free = manager->swap_free;
	*swap_total = manager->swap_total;
}

const GArray *
xtm_task_manager_get_task_list (XtmTaskManager *manager)
{
	g_return_val_if_fail (XTM_IS_TASK_MANAGER (manager), NULL);
	xtm_task_manager_update_model (manager);
	return manager->tasks;
}

void
xtm_task_manager_update_model (XtmTaskManager *manager)
{
	static GArray *removed_tasks = NULL;
	GArray *array;
	guint i;

	g_return_if_fail (XTM_IS_TASK_MANAGER (manager));

	if (removed_tasks == NULL)
		removed_tasks = g_array_new (FALSE, FALSE, sizeof (GtkTreeIter));

	/* Retrieve initial task list and return */
	if (manager->tasks->len == 0)
	{
		get_task_list (manager->tasks);
		for (i = 0; i < manager->tasks->len; i++)
		{
			Task *task = &g_array_index (manager->tasks, Task, i);
#ifdef HAVE_WNCK
			App *app = xtm_app_manager_get_app_from_pid (manager->app_manager, task->pid);
			model_add_task (manager->model, task, app, 0);
#else
			model_add_task (manager->model, task, 0);
#endif
#if DEBUG
			g_print ("%5d %5s %15s %.50s\n", task->pid, task->uid_name, task->name, task->cmdline);
#endif
		}
		return;
	}

	/* Retrieve new task list */
	array = g_array_new (FALSE, FALSE, sizeof (Task));
	get_task_list (array);

	/* Remove terminated tasks */
	for (i = 0; i < removed_tasks->len; i++)
	{
		GtkTreeIter *iter = &g_array_index (removed_tasks, GtkTreeIter, i);
		glong old_timestamp;
		gtk_tree_model_get (manager->model, iter, XTM_PTV_COLUMN_TIMESTAMP, &old_timestamp, -1);
		if (__current_timestamp () - old_timestamp > TIMESTAMP_DELTA)
		{
#if DEBUG
			gint pid;
			gtk_tree_model_get (manager->model, iter, XTM_PTV_COLUMN_PID, &pid, -1);
			g_debug ("Remove old task %d", pid);
#endif
			model_remove_tree_iter (manager->model, iter);
			g_array_remove_index (removed_tasks, i);
		}
	}

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
			GtkTreeIter iter;
#if DEBUG
			g_debug ("Task %d '%s' terminated, marking as removed", task->pid, task->name);
#endif
			model_find_tree_iter_for_pid (manager->model, task->pid, &iter);
			model_mark_tree_iter_as_removed (manager->model, &iter);
			g_array_append_val (removed_tasks, iter);
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
#ifdef HAVE_WNCK
			App *app;
#endif
			gboolean updated = FALSE;

			if (task->pid != tasktmp->pid)
				continue;

#ifdef HAVE_WNCK
			app = xtm_app_manager_get_app_from_pid (manager->app_manager, task->pid);
#endif
			found = TRUE;

			/* Update the model (with the rest) only if needed, this keeps the CPU cool */
			if (model_update_forced
				|| task->ppid != tasktmp->ppid
				|| g_strcmp0 (task->state, tasktmp->state)
				|| task->cpu_user != tasktmp->cpu_user
				|| task->cpu_system != tasktmp->cpu_system
				|| task->rss != tasktmp->rss
				|| task->vsz != tasktmp->vsz
				|| task->prio != tasktmp->prio)
			{
				updated = TRUE;
				task->ppid = tasktmp->ppid;
				g_strlcpy (task->state, tasktmp->state, sizeof (task->state));
				task->cpu_user = tasktmp->cpu_user;
				task->cpu_system = tasktmp->cpu_system;
				task->rss = tasktmp->rss;
				task->vsz = tasktmp->vsz;
				task->prio = tasktmp->prio;
#ifdef HAVE_WNCK
				model_update_task (manager->model, tasktmp, app);
#else
				model_update_task (manager->model, tasktmp);
#endif
			}

			/* Update command name if needed (can happen) */
			if (!model_update_forced && g_strcmp0 (task->cmdline, tasktmp->cmdline))
			{
				GtkTreeIter iter;
				gchar *cmdline;

				cmdline = pretty_cmdline (tasktmp->cmdline, tasktmp->name);
				model_find_tree_iter_for_pid (manager->model, task->pid, &iter);
				gtk_list_store_set (GTK_LIST_STORE (manager->model), &iter, XTM_PTV_COLUMN_COMMAND, cmdline, -1);
				g_free (cmdline);
			}

			/* Update color if needed */
			if (updated == FALSE)
			{
				GtkTreeIter iter;
				gchar *color;
				glong old_timestamp;

				model_find_tree_iter_for_pid (manager->model, task->pid, &iter);
				gtk_tree_model_get (manager->model, &iter, XTM_PTV_COLUMN_BACKGROUND, &color, XTM_PTV_COLUMN_TIMESTAMP, &old_timestamp, -1);

				if (color != NULL && __current_timestamp () - old_timestamp > TIMESTAMP_DELTA)
				{
#if DEBUG
					g_debug ("Remove color from running PID %d", task->pid);
#endif
#ifdef HAVE_WNCK
					model_update_task (manager->model, tasktmp, app);
#else
					model_update_task (manager->model, tasktmp);
#endif
				}

				g_free (color);
			}

			break;
		}

		if (found == FALSE)
		{
#ifdef HAVE_WNCK
			App *app = xtm_app_manager_get_app_from_pid (manager->app_manager, tasktmp->pid);
			model_add_task (manager->model, tasktmp, app, __current_timestamp ());
#else
			model_add_task (manager->model, tasktmp, __current_timestamp ());
#endif
			g_array_append_val (manager->tasks, *tasktmp);
#if DEBUG
			g_debug ("Add new task %d %s", tasktmp->pid, tasktmp->name);
#endif
		}
	}

	g_array_free (array, TRUE);
	model_update_forced = FALSE;

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
get_hostname (void)
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
send_signal_to_pid (guint pid, gint xtm_signal)
{
	gint sig;
	gint res;
	switch (xtm_signal)
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

