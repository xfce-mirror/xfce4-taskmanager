/*
 *  xfce4-taskmanager - very simple taskmanger
 *
 *  Copyright (c) 2004 Johannes Zellner, <webmaster@nebulon.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <libxfcegui4/libxfcegui4.h>

#include "types.h"
#include "functions.h"

GtkWidget *window;
GtkTreeSelection *selection;
GtkTreeStore *tree_store;

void gui_create(void);
gboolean add_tree_item(struct task task);
void remove_tree_item(struct task task);
GtkWidget *create_task_popup_menu(void);
GtkWidget *create_main_popup_menu(void);
gboolean handle_mouse_events(GtkWidget *widget, GdkEventButton *event);
void handle_task_menu(GtkWidget *widget, gchar *signal);
void handle_toggled_checkbox(GtkCheckMenuItem *widget, gchar *signal);
void show_about_dialog(void);
