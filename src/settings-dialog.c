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

#include "settings.h"
#include "settings-dialog.h"
#include "settings-dialog_ui.h"



typedef struct _XtmSettingsDialogClass XtmSettingsDialogClass;
struct _XtmSettingsDialogClass
{
	GtkWidgetClass		parent_class;
};
struct _XtmSettingsDialog
{
	GtkWidget		parent;
	/*<private>*/
	GtkWidget *		window;
	XtmSettings *		settings;
};
G_DEFINE_TYPE (XtmSettingsDialog, xtm_settings_dialog, GTK_TYPE_WIDGET)

static void	xtm_settings_dialog_finalize				(GObject *object);
static void	xtm_settings_dialog_show				(GtkWidget *widget);
static void	xtm_settings_dialog_hide				(GtkWidget *widget);



static void
xtm_settings_dialog_class_init (XtmSettingsDialogClass *klass)
{
	GObjectClass *class;
	GtkWidgetClass *widget_class;
	xtm_settings_dialog_parent_class = g_type_class_peek_parent (klass);
	class = G_OBJECT_CLASS (klass);
	class->finalize = xtm_settings_dialog_finalize;
	widget_class = GTK_WIDGET_CLASS (klass);
	widget_class->show = xtm_settings_dialog_show;
	widget_class->hide = xtm_settings_dialog_hide;
}

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
combobox_changed (GtkComboBox *combobox, XtmSettings *settings)
{
	gint active = gtk_combo_box_get_active (combobox);
	gchar *setting_name = g_object_get_data (G_OBJECT (combobox), "setting-name");
	g_object_set (settings, setting_name, active, NULL);
}

static void
xtm_settings_dialog_init (XtmSettingsDialog *dialog)
{
	GtkBuilder *builder;

	g_object_ref_sink (dialog);

	dialog->settings = xtm_settings_get_default ();

	builder = gtk_builder_new ();
	gtk_builder_add_from_string (builder, settings_dialog_ui, settings_dialog_ui_length, NULL);

	dialog->window = GTK_WIDGET (gtk_builder_get_object (builder, "settings-dialog"));

#ifndef HAVE_WNCK
	gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (builder, "button-show-application-icons")));
#endif

	builder_bind_toggle_button (builder, "button-show-application-icons", dialog->settings, "show-application-icons");
	builder_bind_toggle_button (builder, "button-full-command-line", dialog->settings, "full-command-line");
	builder_bind_toggle_button (builder, "button-more-precision", dialog->settings, "more-precision");
	builder_bind_toggle_button (builder, "button-prompt-terminate-task", dialog->settings, "prompt-terminate-task");
	builder_bind_toggle_button (builder, "button-show-status-icon", dialog->settings, "show-status-icon");
	builder_bind_toggle_button (builder, "button-process-tree", dialog->settings, "process-tree");

	g_object_unref (builder);
}

static void
xtm_settings_dialog_finalize (GObject *object)
{
	XtmSettingsDialog *dialog = XTM_SETTINGS_DIALOG (object);
	gtk_widget_destroy (dialog->window);
	g_object_unref (dialog->settings);
}



GtkWidget *
xtm_settings_dialog_new (GtkWindow *parent_window)
{
	GtkWidget *dialog = g_object_new (XTM_TYPE_SETTINGS_DIALOG, NULL);
	gtk_window_set_transient_for (GTK_WINDOW (XTM_SETTINGS_DIALOG (dialog)->window), parent_window);
	return dialog;
}

static void
xtm_settings_dialog_show (GtkWidget *widget)
{
	g_return_if_fail (GTK_IS_WIDGET (widget));
	g_return_if_fail (GTK_IS_WIDGET (XTM_SETTINGS_DIALOG (widget)->window));
	gtk_widget_show (XTM_SETTINGS_DIALOG (widget)->window);
	gtk_window_present (GTK_WINDOW (XTM_SETTINGS_DIALOG (widget)->window));
}

static void
xtm_settings_dialog_hide (GtkWidget *widget)
{
	gint winx, winy;
	g_return_if_fail (GTK_IS_WIDGET (widget));
	if (!GTK_IS_WIDGET (XTM_SETTINGS_DIALOG (widget)->window))
		return;
	gtk_window_get_position (GTK_WINDOW (XTM_SETTINGS_DIALOG (widget)->window), &winx, &winy);
	gtk_widget_hide (XTM_SETTINGS_DIALOG (widget)->window);
	gtk_window_move (GTK_WINDOW (XTM_SETTINGS_DIALOG (widget)->window), winx, winy);
}

void
xtm_settings_dialog_run (XtmSettingsDialog *dialog)
{
	gtk_dialog_run (GTK_DIALOG (dialog->window));
}
