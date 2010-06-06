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

#include <glib-object.h>
#include <gtk/gtk.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include "app-manager.h"



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

static void	apps_add_application				(GArray *apps, WnckApplication *application);
static void	apps_remove_application				(GArray *apps, WnckApplication *application);
static App *	apps_lookup_pid					(GArray *apps, gint pid);
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
		WnckApplication *application = wnck_window_get_application (window);
		gint pid = wnck_application_get_pid (application);

		if (wnck_window_get_window_type (window) != WNCK_WINDOW_NORMAL)
			continue;

		if (apps_lookup_pid (manager->apps, pid) != NULL)
			continue;

		apps_add_application (manager->apps, application);
	}

#if DEBUG
	g_debug ("Initial applications: %d", manager->apps->len);
#endif

	/* Connect signals */
	g_signal_connect (screen, "application-opened", G_CALLBACK (application_opened), manager);
	g_signal_connect (screen, "application-closed", G_CALLBACK (application_closed), manager);
}

static void
xtm_app_manager_finalize (GObject *object)
{
	g_array_free (XTM_APP_MANAGER (object)->apps, TRUE);
}

static void
apps_add_application (GArray *apps, WnckApplication *application)
{
	App app;
	gint pid;

	pid = wnck_application_get_pid (application);
	if (pid == 0)
	{
		WnckWindow *window = WNCK_WINDOW (wnck_application_get_windows (application)->data);
		pid = wnck_window_get_pid (window);
	}

	if (apps_lookup_pid (apps, pid))
		return;

	app.application = application;
	app.pid = pid;
	g_snprintf (app.name, 1024, "%s", wnck_application_get_name (application));
	app.icon = wnck_application_get_mini_icon (application);
	g_object_ref (app.icon);

	g_array_append_val (apps, app);
}

static void
apps_remove_application (GArray *apps, WnckApplication *application)
{
	App *app = NULL;
	guint i;

	for (i = 0; i < apps->len; i++)
	{
		app = &g_array_index (apps, App, i);
		if (app->application == application)
			break;
	}

	if (app != NULL)
	{
		g_object_unref (app->icon);
		g_array_remove_index (apps, i);
	}
}

static App *
apps_lookup_pid (GArray *apps, gint pid)
{
	App *app;
	guint i;

	for (app = NULL, i = 0; i < apps->len; i++)
	{
		app = &g_array_index (apps, App, i);
		if (app->pid == (guint)pid)
			break;
		app = NULL;
	}

	return app;
}

static void
application_opened (WnckScreen *screen, WnckApplication *application, XtmAppManager *manager)
{
#if DEBUG
	g_debug ("Application opened %p %d", application, wnck_application_get_pid (application));
#endif
	apps_add_application (manager->apps, application);
}

static void
application_closed (WnckScreen *screen, WnckApplication *application, XtmAppManager *manager)
{
#if DEBUG
	g_debug ("Application closed %p", application);
#endif
	apps_remove_application (manager->apps, application);
}



XtmAppManager *
xtm_app_manager_new (void)
{
	return g_object_new (XTM_TYPE_APP_MANAGER, NULL);
}

const GArray *
xtm_app_manager_get_app_list (XtmAppManager *manager)
{
	g_return_val_if_fail (XTM_IS_APP_MANAGER (manager), NULL);
	return manager->apps;
}

App *
xtm_app_manager_get_app_from_pid (XtmAppManager *manager, gint pid)
{
	return apps_lookup_pid (manager->apps, pid);
}

