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
	gtk_widget_add_events(GTK_WIDGET(eventbox), GDK_BUTTON_PRESS);
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
	gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(tree_view));
	gtk_widget_show(tree_view);
	
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	
	tree_store = gtk_tree_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree_view), GTK_TREE_MODEL(tree_store));
	
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
   refresh_task_list(TRUE);
   if(config_show_root_tasks)
   	show_root_tasks();
   if(config_show_user_tasks)
   	show_user_tasks();
  	if(config_show_other_tasks)
   	show_other_tasks();
   
	/* Quit-button */
	button = gtk_button_new_from_stock(GTK_STOCK_QUIT);
	g_signal_connect_swapped(G_OBJECT(button), "button-press-event", G_CALLBACK(handle_mouse_events), G_OBJECT(main_popup_menu));
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(gtk_main_quit), NULL);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);
	gtk_widget_show(button);
	
	gtk_widget_show(window);
}


GtkTreeIter get_iter_by_task(struct task *task)
{
	gboolean valid;
	GtkTreeIter iter;
	GtkTreeIter parent_iter;
	parent_iter.stamp = -1;
	gint count = 0;
	
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tree_store), &iter);

	while(valid)
	{
		gchar *str_data;
		gtk_tree_model_get(GTK_TREE_MODEL(tree_store), &iter, 1, &str_data, -1);

      if(task->ppid == atoi(str_data))
      {
      	g_free(str_data);
      	return(iter);
      }
      g_free(str_data);

		if(gtk_tree_model_iter_has_child(GTK_TREE_MODEL(tree_store), &iter))
		{
			parent_iter = *gtk_tree_iter_copy(&iter);
			valid = gtk_tree_model_iter_children(GTK_TREE_MODEL(tree_store), &iter, &parent_iter);
		}
		else
		{
			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(tree_store), &iter);
			while(!valid)
			{
				if(parent_iter.stamp != -1)
				{
					iter = *gtk_tree_iter_copy(&parent_iter);

					if(!gtk_tree_model_iter_parent(GTK_TREE_MODEL(tree_store), &parent_iter, &iter))
						parent_iter.stamp = -1;
					
					valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(tree_store), &iter);
				}
				else
					break;
			}
		}
	}
	
	iter.stamp = -1;
	return(iter);
}

/* add new tasks to the list */
gboolean add_tree_item(struct task task)
{
	GtkTreeIter iter;
	
	/* check if there is a parent task */
	struct task *parent_task = get_parent_task(task);
	
	/* Append new line in the list */
	if(parent_task == NULL)
	{
		gtk_tree_store_append(GTK_TREE_STORE(tree_store), &iter, NULL);
	}
	else
	{
		GtkTreeIter parent_iter = get_iter_by_task(&task);

		if(parent_iter.stamp == -1)
			gtk_tree_store_append(GTK_TREE_STORE(tree_store), &iter, NULL);
		else
			gtk_tree_store_append(GTK_TREE_STORE(tree_store), &iter, &parent_iter);
	}
	
	/* Fill the appended line with data */
	gchar *list_value_1 = g_strdup_printf("%s", task.uname);
	gtk_tree_store_set(GTK_TREE_STORE(tree_store), &iter, 0, list_value_1, -1);
	g_free(list_value_1);

	gchar *list_value_2 = g_strdup_printf("%i", task.pid);
	gtk_tree_store_set(GTK_TREE_STORE(tree_store), &iter, 1, list_value_2, -1);
	g_free(list_value_2);
	
	gchar *list_value_3 = g_strdup_printf("%i", task.ppid);
	gtk_tree_store_set(GTK_TREE_STORE(tree_store), &iter, 2, list_value_3, -1);
	g_free(list_value_3);
	
	gchar *list_value_4 = g_strdup_printf("%s", task.name);
	gtk_tree_store_set(GTK_TREE_STORE(tree_store), &iter, 3, list_value_4, -1);
	g_free(list_value_4);
	
	return(TRUE);
}

void remove_tree_item(struct task task)
{
	gboolean valid;
	GtkTreeIter iter;
	
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(tree_store), &iter);

	while(valid)
	{
		gchar *str_data;
		gtk_tree_model_get(GTK_TREE_MODEL(tree_store), &iter, 1, &str_data, -1);

      if(task.pid == atoi(str_data))
      {
      	gtk_tree_store_remove(GTK_TREE_STORE(tree_store), &iter);
      	g_free(str_data);
      	break;
      }
      g_free(str_data);

		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(tree_store), &iter);
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
	
	menuitem = gtk_check_menu_item_new_with_label("Show user tasks");
	if(config_show_user_tasks)
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
	g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(handle_toggled_checkbox), "show_user_task");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_widget_show(menuitem);
	
	menuitem = gtk_check_menu_item_new_with_label("Show root tasks");
	if(config_show_root_tasks)
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
	g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(handle_toggled_checkbox), "show_root_task");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_widget_show(menuitem);
	
	menuitem = gtk_check_menu_item_new_with_label("Show other tasks");
	if(config_show_other_tasks)
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
	g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(handle_toggled_checkbox), "show_other_task");
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
	gchar *task_id = "";
	GtkTreeModel *model;
	GtkTreeIter iter;
	
	if(signal != NULL && gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		if(strcmp(signal, "TERM") == 0)
		{
			if(xfce_confirm("Really TERM the Task?", GTK_STOCK_YES, NULL))
			{
				gtk_tree_model_get(model, &iter, 1, &task_id, -1);
				send_signal_to_task(task_id, signal);
				refresh_task_list(FALSE);
			}
		}
		else if(strcmp(signal, "KILL") == 0)
		{
			if(xfce_confirm("Really KILL the Task?", GTK_STOCK_YES, NULL))
			{
				gtk_tree_model_get(model, &iter, 1, &task_id, -1);
				send_signal_to_task(task_id, signal);
				refresh_task_list(FALSE);
			}
		}
		else
		{
			gtk_tree_model_get(model, &iter, 1, &task_id, -1);
			send_signal_to_task(task_id, signal);
		}		
	}
}

void handle_toggled_checkbox(GtkCheckMenuItem *widget, gchar *checkbox_id)
{
	if(strcmp(checkbox_id, "show_user_task") == 0)
	{
		if(widget->active)
		{
			config_show_user_tasks = TRUE;
			show_user_tasks();
		}
		else
		{
			config_show_user_tasks = FALSE;
			hide_user_tasks();
		}
	}
	else if(strcmp(checkbox_id, "show_root_task") == 0)
	{
		if(widget->active)
		{
			config_show_root_tasks = TRUE;
			show_root_tasks();
		}
		else
		{
			config_show_root_tasks = FALSE;
			hide_root_tasks();
		}
	}
	else if(strcmp(checkbox_id, "show_other_task") == 0)
	{
		if(widget->active)
		{
			config_show_other_tasks = TRUE;
			show_other_tasks();
		}
		else
		{
			config_show_other_tasks = FALSE;
			hide_other_tasks();
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
