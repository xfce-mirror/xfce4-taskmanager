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
	gushort			cpu_user;
	gushort			cpu_system;
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
	get_owner_uid (&(manager->owner_uid), &(manager->owner_uid_name));
	manager->hostname = get_hostname ();
}

static void
xtm_task_manager_finalize (GObject *object)
{
	XtmTaskManager *manager = XTM_TASK_MANAGER (object);
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

GArray *
xtm_task_manager_get_tasklist (XtmTaskManager *manager)
{
}

void
xtm_task_manager_get_system_info (XtmTaskManager *manager, guint *num_processes, gushort *cpu, gushort *memory, gushort *swap)
{
}

void
xtm_task_manager_send_signal_to_pid (XtmTaskManager *manager)
{
}

void
xtm_task_manager_set_priority_to_pid (XtmTaskManager *manager)
{
}

