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

#include "exec-tool-button.h"



typedef struct _XtmExecToolButtonClass XtmExecToolButtonClass;

struct _XtmExecToolButtonClass
{
	GtkMenuToolButtonClass	parent_class;
};

struct _XtmExecToolButton
{
	GtkMenuToolButton	parent;
};
G_DEFINE_TYPE (XtmExecToolButton, xtm_exec_tool_button, GTK_TYPE_MENU_TOOL_BUTTON)

static GtkWidget *	construct_menu					(void);
static void		execute_default_command				(void);



static void
xtm_exec_tool_button_class_init (XtmExecToolButtonClass *klass)
{
	xtm_exec_tool_button_parent_class = g_type_class_peek_parent (klass);
}

static void
xtm_exec_tool_button_init (XtmExecToolButton *button)
{
	GtkWidget *menu;

	gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (button), "system-run");
	gtk_tool_button_set_use_underline (GTK_TOOL_BUTTON (button), TRUE);

	menu = construct_menu ();
	gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (button), menu);
	g_signal_connect (button, "clicked", G_CALLBACK (execute_default_command), NULL);

	gtk_widget_show_all (GTK_WIDGET (button));
}

static void
execute_command (const gchar *command)
{
	GError *error = NULL;
	GdkScreen *screen;
	GdkDisplay *display;
	GdkAppLaunchContext *launch_context;
	GAppInfo *app_info;
	app_info = g_app_info_create_from_commandline (command, NULL, G_APP_INFO_CREATE_NONE, &error);
	if (!error) {
		screen = gdk_screen_get_default();
		display = gdk_screen_get_display (screen);
		launch_context = gdk_display_get_app_launch_context (display);
		gdk_app_launch_context_set_screen (launch_context, screen);
		g_app_info_launch (app_info, NULL, G_APP_LAUNCH_CONTEXT
			(launch_context), &error);
		g_object_unref (launch_context);
	}
	g_object_unref (app_info);

	if (error != NULL)
	{
		GtkWidget *dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
			_("Execution error"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error->message);
		gtk_window_set_title (GTK_WINDOW (dialog), _("Task Manager"));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_error_free (error);
	}
}

static gboolean
program_exists (gchar *program)
{
	gchar *program_path = g_find_program_in_path (program);
	if (program_path == NULL)
		return FALSE;
	g_free (program_path);
	return TRUE;
}

static void
execute_default_command (void)
{
	static gchar *command = NULL;

	if (command == NULL)
	{
		/* Find a runner program */
		if (program_exists ("xfrun4"))
			command = g_strdup ("xfrun4");
		else if (program_exists ("gmrun"))
			command = g_strdup ("gmrun");
		else if (program_exists ("gexec"))
			command = g_strdup ("gexec");
		/* Find an applications-listing program */
		else if (program_exists ("xfce4-appfinder"))
			command = g_strdup ("xfce4-appfinder");
		/* Find a terminal emulator */
		else if (program_exists ("exo-open"))
			command = g_strdup ("exo-open --launch TerminalEmulator");
		else if (program_exists ("xterm"))
			command = g_strdup ("xterm -fg grey -bg black");
		else
		{
			GtkWidget *dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
				_("Execution error"));
			gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
				_("Couldn't find any default command to run."));
			gtk_window_set_title (GTK_WINDOW (dialog), _("Task Manager"));
			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
			return;
		}
	}

	execute_command (command);
}

static void
menu_append_item (GtkMenu *menu, gchar *title, gchar *command, gchar *icon_name)
{
	GtkWidget *image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU);
	GtkWidget *mi = gtk_image_menu_item_new_with_label (title);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
	g_signal_connect_swapped (mi, "activate", G_CALLBACK (execute_command), command);
}

static GtkWidget *
construct_menu (void)
{
	GtkWidget *menu = gtk_menu_new ();

	/* Find a runner program */
	if (program_exists ("xfrun4"))
		menu_append_item (GTK_MENU (menu), _("Run Program..."), "xfrun4", "system-run");
	else if (program_exists ("gmrun"))
		menu_append_item (GTK_MENU (menu), _("Run Program..."), "gmrun", "system-run");
	else if (program_exists ("gexec"))
		menu_append_item (GTK_MENU (menu), _("Run Program..."), "gexec", "system-run");
	/* Find an applications-listing program */
	if (program_exists ("xfce4-appfinder"))
		menu_append_item (GTK_MENU (menu), _("Application Finder"), "xfce4-appfinder", "edit-find");
	/* Find a terminal emulator */
	if (program_exists ("exo-open"))
		menu_append_item (GTK_MENU (menu), _("Terminal emulator"), "exo-open --launch TerminalEmulator", "utilities-terminal");
	else if (program_exists ("xterm"))
		menu_append_item (GTK_MENU (menu), _("XTerm"), "xterm -fg grey -bg black", "utilities-terminal");

	gtk_widget_show_all (menu);

	return menu;
}

GtkWidget *
xtm_exec_tool_button_new (void)
{
	return g_object_new (XTM_TYPE_EXEC_TOOL_BUTTON, NULL);
}
