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
 
#include "gui.h"

/* create the GUI */
void create_gui(void)
{	
	GtkWidget *eventbox, *box, *button, *scrolled_window, *tree_view, *main_popup_menu, *task_popup_menu;
	GtkCellRenderer *cell_renderer;
	GtkTreeViewColumn *column;
	
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "xfce4-taskmanager");
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
	gtk_widget_set_size_request(GTK_WIDGET(window), 450, 400);
	
	/* popupmenus */
	task_popup_menu = create_task_popup_menu();
	main_popup_menu = create_main_popup_menu();

	/* eventbox */
	eventbox = gtk_event_box_new();
	gtk_event_box_set_visible_window(GTK_EVENT_BOX(eventbox), FALSE);
	gtk_widget_add_events(GTK_WIDGET(eventbox), "BUTTON_PRESS");
	g_signal_connect_swapped(G_OBJECT(eventbox), "button-press-event", G_CALLBACK(handle_mouse_events), G_OBJECT(main_popup_menu));
	gtk_container_set_border_width(GTK_CONTAINER(eventbox), 0);
	
	gtk_container_add(GTK_CONTAINER(window), eventbox);
	gtk_widget_show(eventbox);
	
	box = gtk_vbox_new(FALSE, 10);
	gtk_container_set_border_width(GTK_CONTAINER(box), 10);
	gtk_container_add(GTK_CONTAINER(eventbox), box);
	gtk_widget_show(box);
	
	/* tasklist */
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(box), scrolled_window, TRUE, TRUE, 0);
	gtk_widget_show(scrolled_window);
	
	tree_view = gtk_tree_view_new();
	g_signal_connect_swapped(G_OBJECT(tree_view), "button-press-event", G_CALLBACK(handle_mouse_events), G_OBJECT(task_popup_menu));
	gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);
	gtk_widget_show(tree_view);
	
	selection = gtk_tree_view_get_selection(tree_view);
	
	list_store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree_view), list_store);
	
	cell_renderer = gtk_cell_renderer_text_new();
	
	column = gtk_tree_view_column_new_with_attributes("User", cell_renderer, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	
	column = gtk_tree_view_column_new_with_attributes("PID", cell_renderer, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	
	column = gtk_tree_view_column_new_with_attributes("PPID", cell_renderer, "text", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	
	column = gtk_tree_view_column_new_with_attributes("Command", cell_renderer, "text", 3, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
	
	/* load the tasklist */
   refresh_task_list();
	
	/* Quit-button */
	button = gtk_button_new_from_stock(GTK_STOCK_QUIT);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(gtk_main_quit), NULL);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);
	gtk_widget_show(button);
	
	gtk_widget_show(window);
}

/* add new tasks to the list */
gboolean add_new_list_item(struct task task)
{
	GtkTreeIter iter;
	
	/* Append new line in the list */
	gtk_list_store_append(GTK_LIST_STORE(list_store), &iter);
	
	/* Fill the appended line with data */
	gchar *list_value_1 = g_strdup_printf("%s", task.uid);
	gtk_list_store_set(GTK_LIST_STORE(list_store), &iter, 0, list_value_1, -1);
	g_free(list_value_1);

	gchar *list_value_2 = g_strdup_printf("%s", task.pid);
	gtk_list_store_set(GTK_LIST_STORE(list_store), &iter, 1, list_value_2, -1);
	g_free(list_value_2);
	
	gchar *list_value_3 = g_strdup_printf("%s", task.ppid);
	gtk_list_store_set(GTK_LIST_STORE(list_store), &iter, 2, list_value_3, -1);
	g_free(list_value_3);
	
	gchar *list_value_4 = g_strdup_printf("%s", task.name);
	gtk_list_store_set(GTK_LIST_STORE(list_store), &iter, 3, list_value_4, -1);
	g_free(list_value_4);
	
	return(TRUE);
}

void remove_list_item(struct task task)
{
	gboolean valid;
	GtkTreeIter iter;
	
	valid = gtk_tree_model_get_iter_first(list_store, &iter);

	while(valid)
	{
		gchar *str_data;
		gtk_tree_model_get(list_store, &iter, 1, &str_data, -1);

      if(strcmp(task.pid,str_data) == 0)
      {
      	gtk_list_store_remove(list_store, &iter);
      	g_free(str_data);
      	break;
      }
      g_free(str_data);

		valid = gtk_tree_model_iter_next(list_store, &iter);
	}
}

GtkWidget *create_task_popup_menu()
{
	GtkWidget *menu, *menuitem;
	
	menu = gtk_menu_new();
	
	menuitem = gtk_menu_item_new_with_label("Stop");
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(handle_task_menu), "STOP");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_widget_show(menuitem);
	
	menuitem = gtk_menu_item_new_with_label("Continue");
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(handle_task_menu), "CONT");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_widget_show(menuitem);
	
	menuitem = gtk_menu_item_new_with_label("Term");
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(handle_task_menu), "TERM");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_widget_show(menuitem);
	
	menuitem = gtk_menu_item_new_with_label("Kill");
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(handle_task_menu), "KILL");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_widget_show(menuitem);
	
	return(menu);
}

GtkWidget *create_main_popup_menu()
{
	GtkWidget *menu, *menuitem;
	
	menu = gtk_menu_new();
	
	menuitem = gtk_menu_item_new_with_label("About");
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(show_about_dialog), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_widget_show(menuitem);
	
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_widget_show(menuitem);
	
	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(gtk_main_quit), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_widget_show(menuitem);
	
	return(menu);
}

gboolean handle_mouse_events(GtkWidget *widget, GdkEventButton *event)
{
	if(event->button == 3)
	{
		GdkEventButton *mouseevent = (GdkEventButton *)event; 
		gtk_menu_popup(GTK_MENU(widget), NULL, NULL, NULL, NULL, mouseevent->button, mouseevent->time);
		return FALSE;
	}
	return(FALSE);
}

void handle_task_menu(GtkWidget *widget, gchar *signal)
{
	if(signal != NULL)
	{
		if(strcmp(signal, "TERM") == 0)
		{
			if(xfce_confirm("Really TERM the Task?", GTK_STOCK_YES, NULL))
			{
				gchar *task_id = "";
				GtkTreeModel *model;
				GtkTreeIter iter;
			
				if(gtk_tree_selection_get_selected(selection, &model, &iter))
				{
					gtk_tree_model_get(model, &iter, 1, &task_id, -1);
					send_signal_to_task(task_id, signal);
					refresh_task_list();
				}
			}
		}
		else if(strcmp(signal, "KILL") == 0)
		{
			if(xfce_confirm("Really KILL the Task?", GTK_STOCK_YES, NULL))
			{
				gchar *task_id = "";
				GtkTreeModel *model;
				GtkTreeIter iter;
			
				if(gtk_tree_selection_get_selected(selection, &model, &iter))
				{
					gtk_tree_model_get(model, &iter, 1, &task_id, -1);
					send_signal_to_task(task_id, signal);
					refresh_task_list();
				}
			}
		}
		else
		{
			gchar *task_id = "";
			GtkTreeModel *model;
			GtkTreeIter iter;
			
			if(gtk_tree_selection_get_selected(selection, &model, &iter))
			{
				gtk_tree_model_get(model, &iter, 1, &task_id, -1);
				send_signal_to_task(task_id, signal);
			}
		}		
	}
}

void show_about_dialog()
{
	GtkWidget *about_dialog;
	XfceAboutInfo *about_info;
	
	about_info = xfce_about_info_new("xfce4-taskmanager", VERSION, "Xfce4-Taskmanager is a easy to use Taskmanager.",XFCE_COPYRIGHT_TEXT("2005", "Johannes Zellner"), XFCE_LICENSE_GPL);
	xfce_about_info_set_homepage(about_info, "http://developer.berlios.de/projects/xfce-goodies/");
	xfce_about_info_add_credit(about_info, "Johannes Zellner", "webmaster@nebulon.de", "Original Author");
    
	about_dialog = xfce_about_dialog_new(GTK_WINDOW(window), about_info, NULL);
	g_signal_connect(G_OBJECT(about_dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);
	gtk_widget_show(about_dialog);
	
}
