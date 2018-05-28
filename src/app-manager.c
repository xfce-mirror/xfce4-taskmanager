/*
 * Copyright (c) 2010 Mike Massonnet, <mmassonnet@xfce.org>
 * Copyright (c) 2018 Rozhuk Ivan <rozhuk.im@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include "app-manager.h"
#include "task-manager.h"



typedef struct _XtmAppManagerClass XtmAppManagerClass;
struct _XtmAppManagerClass
{
	GObjectClass		parent_class;
};
struct _XtmAppManager
{
	GObject			parent;
	/*<private>*/
	GArray *		apps;
};
G_DEFINE_TYPE (XtmAppManager, xtm_app_manager, G_TYPE_OBJECT)

static void	xtm_app_manager_finalize			(GObject *object);

static GPid	app_get_pid					(WnckApplication *application);
static gint	app_pid_compare_fn				(gconstpointer a, gconstpointer b);

static void	apps_add_application				(GArray *apps, WnckApplication *application, GPid pid);
static void	apps_remove_application				(GArray *apps, WnckApplication *application);
static App *	apps_lookup_pid					(GArray *apps, GPid pid);
static void	application_opened				(WnckScreen *screen, WnckApplication *application, XtmAppManager *manager);
static void	application_closed				(WnckScreen *screen, WnckApplication *application, XtmAppManager *manager);



static void
xtm_app_manager_class_init (XtmAppManagerClass *klass)
{
	GObjectClass *class = G_OBJECT_CLASS (klass);
	xtm_app_manager_parent_class = g_type_class_peek_parent (klass);
	class->finalize = xtm_app_manager_finalize;
}

static void
xtm_app_manager_init (XtmAppManager *manager)
{
	WnckApplication *application;
	WnckScreen *screen = wnck_screen_get_default ();
	GList *windows, *l;

	/* Retrieve initial applications */
	while (gtk_events_pending ())
		gtk_main_iteration ();

	manager->apps = g_array_new (FALSE, FALSE, sizeof (App));
	windows = wnck_screen_get_windows (screen);
	for (l = windows; l != NULL; l = l->next)
	{
		WnckWindow *window = WNCK_WINDOW (l->data);
		if (wnck_window_get_window_type (window) != WNCK_WINDOW_NORMAL)
			continue;

		application = wnck_window_get_application (window);
		apps_add_application (manager->apps, application, app_get_pid (application));
	}

	G_DEBUG_FMT ("Initial applications: %d", manager->apps->len);

	/* Connect signals */
	g_signal_connect (screen, "application-opened", G_CALLBACK (application_opened), manager);
	g_signal_connect (screen, "application-closed", G_CALLBACK (application_closed), manager);
}

static void
xtm_app_manager_finalize (GObject *object)
{
	g_array_free (XTM_APP_MANAGER (object)->apps, TRUE);
}

static GPid
app_get_pid(WnckApplication *application)
{
	GPid pid;
	if (NULL == application)
		return (0);
	pid = wnck_application_get_pid (application);
	if (pid != 0)
		return (pid);
	return (wnck_window_get_pid (WNCK_WINDOW (wnck_application_get_windows (application)->data)));
}

static gint
app_pid_compare_fn(gconstpointer a, gconstpointer b)
{
	return (((const App*)a)->pid - ((const App*)b)->pid);
}

static void
apps_add_application (GArray *apps, WnckApplication *application, GPid pid)
{
	App app;

	if (apps_lookup_pid (apps, pid))
		return;

	app.application = application;
	app.pid = pid;
	g_snprintf (app.name, sizeof(app.name), "%s", wnck_application_get_name (application));
	app.icon = wnck_application_get_mini_icon (application);
	g_object_ref (app.icon);

	g_array_append_val (apps, app);
	g_array_sort (apps, app_pid_compare_fn);
}

static void
apps_remove_application (GArray *apps, WnckApplication *application)
{
	App *app = apps_lookup_pid(apps, app_get_pid (application));

	if (app == NULL)
		return;
	g_object_unref (app->icon);
	g_array_remove_index (apps, (guint)(((size_t)app - (size_t)apps->data) / sizeof(App)));
}

static App *
apps_lookup_pid (GArray *apps, GPid pid)
{
	App tapp;

	tapp.pid = pid;

	return (bsearch(&tapp, apps->data, apps->len, sizeof(App), app_pid_compare_fn));
}

static void
application_opened (WnckScreen *screen, WnckApplication *application, XtmAppManager *manager)
{
	GPid pid = app_get_pid (application);
	G_DEBUG_FMT ("Application opened %p %d", (void*)application, pid);
	apps_add_application (manager->apps, application, pid);
}

static void
application_closed (WnckScreen *screen, WnckApplication *application, XtmAppManager *manager)
{
	G_DEBUG_FMT ("Application closed %p", (void*)application);
	apps_remove_application (manager->apps, application);
}



XtmAppManager *
xtm_app_manager_new (void)
{
	return g_object_new (XTM_TYPE_APP_MANAGER, NULL);
}

App *
xtm_app_manager_get_app_from_pid (XtmAppManager *manager, GPid pid)
{
	return apps_lookup_pid (manager->apps, pid);
}
