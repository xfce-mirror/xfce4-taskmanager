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
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <libxfce4ui/libxfce4ui.h>

#include "settings.h"
#include "settings-dialog.h"
#include "settings-dialog_ui.h"

static void	show_about_dialog				(GtkWidget *widget);
static GtkWidget *xtm_settings_dialog_new (GtkBuilder *builder, GtkWidget *parent_window);



static void
button_toggled (GtkToggleButton *button, XtmSettings *settings)
{
	gboolean active = gtk_toggle_button_get_active (button);
	gchar *setting_name = g_object_get_data (G_OBJECT (button), "setting-name");
	g_object_set (settings, setting_name, active, NULL);
}

static void
builder_bind_toggle_button (GtkBuilder *builder, gchar *widget_name, XtmSettings *settings, gchar *setting_name)
{
	gboolean active;
	GtkWidget *button;

	g_object_get (settings, setting_name, &active, NULL);

	button = GTK_WIDGET (gtk_builder_get_object (builder, widget_name));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), active);
	g_object_set_data (G_OBJECT (button), "setting-name", setting_name);
	g_signal_connect (button, "toggled", G_CALLBACK (button_toggled), settings);
}

static void
show_about_dialog (GtkWidget *widget)
{
	const gchar *authors[] = {
		"(c) 2018-2019 Rozhuk Ivan",
		"(c) 2014 Landry Breuil",
		"(c) 2014 Harald Judt",
		"(c) 2014 Peter de Ridder",
		"(c) 2014 Simon Steinbeiss",
		"(c) 2008-2010 Mike Massonnet",
		"(c) 2005-2008 Johannes Zellner",
		"",
		"FreeBSD",
		"  \342\200\242 Rozhuk Ivan",
		"  \342\200\242 Mike Massonnet",
		"  \342\200\242 Oliver Lehmann",
		"",
		"OpenBSD",
		"  \342\200\242 Landry Breuil",
		"",
		"Linux",
		"  \342\200\242 Johannes Zellner",
		"  \342\200\242 Mike Massonnet",
		"",
		"OpenSolaris",
		"  \342\200\242 Mike Massonnet",
		"  \342\200\242 Peter Tribble",
		NULL };
	const gchar *license =
		"This program is free software; you can redistribute it and/or modify\n"
		"it under the terms of the GNU General Public License as published by\n"
		"the Free Software Foundation; either version 2 of the License, or\n"
		"(at your option) any later version.\n";

	gtk_show_about_dialog (GTK_WINDOW (widget),
		"program-name", _("Task Manager"),
		"version", PACKAGE_VERSION,
		"copyright", "Copyright \302\251 2005-2019 The Xfce development team",
		"logo-icon-name", "org.xfce.taskmanager",
		"comments", _("Easy to use task manager"),
		"license", license,
		"authors", authors,
		"translator-credits", _("translator-credits"),
		"website", "http://goodies.xfce.org/projects/applications/xfce4-taskmanager",
		"website-label", "goodies.xfce.org",
		NULL);
}

static GtkWidget *
xtm_settings_dialog_new (GtkBuilder *builder, GtkWidget *parent_window)
{
	GtkWidget *dialog;
	GtkWidget *button;
	XtmSettings *settings;

	settings = xtm_settings_get_default ();
	dialog = GTK_WIDGET (gtk_builder_get_object (builder, "settings-dialog"));
	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent_window));

#ifndef HAVE_WNCK
	gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "button-show-application-icons")));
#endif

	builder_bind_toggle_button (builder, "button-show-application-icons", settings, "show-application-icons");
	builder_bind_toggle_button (builder, "button-full-command-line", settings, "full-command-line");
	builder_bind_toggle_button (builder, "button-more-precision", settings, "more-precision");
	builder_bind_toggle_button (builder, "button-prompt-terminate-task", settings, "prompt-terminate-task");
	builder_bind_toggle_button (builder, "button-show-status-icon", settings, "show-status-icon");
	builder_bind_toggle_button (builder, "button-process-tree", settings, "process-tree");

	button = GTK_WIDGET (gtk_builder_get_object (builder, "button-about"));
	g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_about_dialog), dialog);

	return dialog;
}

void
xtm_settings_dialog_run (GtkWidget *parent_window)
{
	GtkBuilder *builder;
	GtkWidget *dialog;

	builder = gtk_builder_new ();
	gtk_builder_add_from_string (builder, settings_dialog_ui, settings_dialog_ui_length, NULL);
	g_return_if_fail (GTK_IS_BUILDER (builder));

	dialog = xtm_settings_dialog_new (builder, parent_window);

	g_object_unref (builder);
	gtk_dialog_run (GTK_DIALOG (dialog));
}
