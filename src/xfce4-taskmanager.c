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
#include <glib.h>
#include <stdio.h>
#include <libxfcegui4/libxfcegui4.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

/* config vars */
#define REFRESH_INTERVAL 3

gchar *current_task_id = "";
GtkListStore *model;
GtkWidget *window;

struct task
{
	gchar pid[256];
	gchar ppid[256];
	gchar uid[256];
	gchar name[256];
};

/* called, if the user select a listitem */
static void select_task(GtkTreeSelection *selection)
{
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		gtk_tree_model_get (model, &iter, 1, &current_task_id, -1);
	}
}

/* appends a new listitem */
static void add_new_list_item(struct task task)
{
	GtkTreeIter iter;
	struct passwd *passwdp; 
	
	/* Append new line in the list */
	gtk_list_store_append(GTK_LIST_STORE(model), &iter);
	
	/* Fill the appended line with data */
	passwdp = getpwuid(atoi(task.uid)); 
	gchar *list_value_1 = g_strdup_printf("%s", passwdp->pw_name);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, list_value_1, -1);
	g_free(list_value_1);
	
	gchar *list_value_2 = g_strdup_printf("%s", task.pid);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 1, list_value_2, -1);
	g_free(list_value_2);
	
	gchar *list_value_3 = g_strdup_printf("%s", task.ppid);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 2, list_value_3, -1);
	g_free(list_value_3);
	
	gchar *list_value_4 = g_strdup_printf("%s", task.name);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 3, list_value_4, -1);
	g_free(list_value_4);
}

/* reads all taskes from 'ps' and creates the listitems */
static void get_task_list(void)
{
	DIR *dir;
	struct dirent *dir_entry;
	
	if((dir = opendir("/proc")) == NULL)
		printf("Error: couldn't load the directory\n");
	
	while((dir_entry = readdir(dir)) != NULL)
	{
		if(atoi(dir_entry->d_name) != 0)
		{
			FILE *task_file;
			gchar task_file_name[256] = "/proc/";
			g_strlcat(task_file_name,dir_entry->d_name, 256);
			g_strlcat(task_file_name,"/status", 256);
			
			gchar buffer[256];
			gint line_count = 0;
			struct task task;
			
			if((task_file = fopen(task_file_name,"r")) != NULL)
			{
				while(fgets(buffer, 256, task_file) != NULL)
				{
					if(line_count == 0)
						strcpy(task.name,g_strstrip(g_strsplit(buffer, ":", 2)[1]));
					else if(line_count == 3)
						strcpy(task.pid,g_strstrip(g_strsplit(buffer, ":", 2)[1]));
					else if(line_count == 5)
						strcpy(task.ppid,g_strstrip(g_strsplit(buffer, ":", 2)[1]));
					else if(line_count == 7)
						strcpy(task.uid,g_strsplit(g_strstrip(g_strsplit(buffer, ":", 2)[1]), "\t", 2)[0]);
						
					line_count++;
				}
				
				line_count = 0;
				fclose(task_file);
				
				add_new_list_item(task);
			}
		}
	}
	closedir(dir);
}

/* function to kill the current task */
static void send_signal_to_task(gchar *signal)
{
	if(current_task_id != "")
	{
		gchar command[64] = "kill -";
		g_strlcat(command,signal, 64);
		g_strlcat(command," ", 64);
		g_strlcat(command,current_task_id, 64);
		
		if(system(command) != 0)
			xfce_err("Couldn't %s the task with ID %s", signal, current_task_id);
	}
}

/* handles all the button events */
static void handle_events(GtkWidget *widget, gchar *widget_action)
{
	if(strcmp(widget_action, "task_stop") == 0)
	{
		send_signal_to_task("STOP");
		gtk_list_store_clear(GTK_LIST_STORE(model));
		get_task_list();
	}
	if(strcmp(widget_action, "task_cont") == 0)
	{
		send_signal_to_task("CONT");
		gtk_list_store_clear(GTK_LIST_STORE(model));
		get_task_list();
	}
	if(strcmp(widget_action, "task_term") == 0)
	{
		if(xfce_confirm("Really TERM the task?", GTK_STOCK_OK, NULL))
		{
			send_signal_to_task("TERM");
			gtk_list_store_clear(GTK_LIST_STORE(model));
			get_task_list();
		}
	}
	if(strcmp(widget_action, "task_kill") == 0)
	{
		if(xfce_confirm("Really KILL the task?", GTK_STOCK_OK, NULL))
		{
			send_signal_to_task("KILL");
			gtk_list_store_clear(GTK_LIST_STORE(model));
			get_task_list();
		}
	}
	if(strcmp(widget_action, "button_reload") == 0)
	{
		gtk_list_store_clear(GTK_LIST_STORE(model));
		get_task_list();
	}
	if(strcmp(widget_action, "button_quit") == 0)
	{
		gtk_main_quit();
	}
}

/* handles all the mouse events */
static gboolean handle_mouse_events(GtkWidget *widget, GdkEventButton *event)
{
	printf("hallo");
	if(event->button == 3)
	{
		GdkEventButton *mouseevent = (GdkEventButton *)event; 
		gtk_menu_popup(GTK_MENU(widget), NULL, NULL, NULL, NULL, mouseevent->button, mouseevent->time);
		return FALSE;
	}

	return FALSE;
}

/* create the GUI */
static void create_gui(void)
{
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "xfce4-taskmanager");
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK (gtk_main_quit), NULL);
	gtk_container_set_border_width (GTK_CONTAINER (window), 10);
	gtk_widget_set_size_request (GTK_WIDGET (window), 450, 400);
		
	GtkWidget *button;
	GtkWidget *box1;
	
	box1 = gtk_vbox_new(FALSE, 10);

	/* list rightclickmenu */
	GtkWidget *list_popupmenu;
	GtkWidget *popupmenuitem;
	
	list_popupmenu = gtk_menu_new();
	
	popupmenuitem = gtk_menu_item_new_with_label("Stop");
	gtk_menu_shell_append(GTK_MENU_SHELL(list_popupmenu), popupmenuitem);
	g_signal_connect(G_OBJECT(popupmenuitem), "activate", G_CALLBACK(handle_events), "task_stop");
	gtk_widget_show(popupmenuitem);
	
	popupmenuitem = gtk_menu_item_new_with_label("Continue");
	gtk_menu_shell_append(GTK_MENU_SHELL(list_popupmenu), popupmenuitem);
	g_signal_connect(G_OBJECT(popupmenuitem), "activate", G_CALLBACK(handle_events), "task_cont");
	gtk_widget_show(popupmenuitem);
	
	popupmenuitem = gtk_menu_item_new_with_label("Term");
	gtk_menu_shell_append(GTK_MENU_SHELL(list_popupmenu), popupmenuitem);
	g_signal_connect(G_OBJECT(popupmenuitem), "activate", G_CALLBACK(handle_events), "task_term");
	gtk_widget_show(popupmenuitem);
	
	popupmenuitem = gtk_menu_item_new_with_label("Kill");
	gtk_menu_shell_append(GTK_MENU_SHELL(list_popupmenu), popupmenuitem);
	g_signal_connect(G_OBJECT(popupmenuitem), "activate", G_CALLBACK(handle_events), "task_kill");
	gtk_widget_show(popupmenuitem);
	
	/* tasklistgui */
	GtkWidget *scrolled_window;
	GtkTreeSelection *select;
	GtkWidget *tree_view;
	GtkTreeIter iter;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
   
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC,  GTK_POLICY_AUTOMATIC);
   
	model = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	tree_view = gtk_tree_view_new();
	gtk_container_add (GTK_CONTAINER(scrolled_window), tree_view);
	gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view), GTK_TREE_MODEL (model));
	
	g_signal_connect_swapped (G_OBJECT(tree_view), "button-press-event", G_CALLBACK(handle_mouse_events), G_OBJECT(list_popupmenu));
            
	gtk_widget_show (tree_view);
   
	cell = gtk_cell_renderer_text_new();

	column = gtk_tree_view_column_new_with_attributes("User", cell, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), GTK_TREE_VIEW_COLUMN (column));
	
	column = gtk_tree_view_column_new_with_attributes("PID", cell, "text", 1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), GTK_TREE_VIEW_COLUMN (column));
	
	column = gtk_tree_view_column_new_with_attributes("PPID", cell, "text", 2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), GTK_TREE_VIEW_COLUMN (column));
	
	column = gtk_tree_view_column_new_with_attributes("Command", cell, "text", 3, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), GTK_TREE_VIEW_COLUMN (column));

	select = gtk_tree_view_get_selection(GTK_TREE_VIEW (tree_view));
	gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
	g_signal_connect(G_OBJECT(select), "changed", G_CALLBACK(select_task), NULL);
		
	/* load the tasklist */
   get_task_list();
	
	gtk_box_pack_start(GTK_BOX(box1), scrolled_window, TRUE, TRUE, 0);
	gtk_widget_show(scrolled_window);
	
	/* QUIT-Button */
	button = gtk_button_new_from_stock(GTK_STOCK_QUIT);
	g_signal_connect(G_OBJECT (button), "clicked", G_CALLBACK(handle_events), "button_quit");
	gtk_box_pack_start(GTK_BOX(box1), button, FALSE, TRUE, 0);
	gtk_widget_show(button);
	
	gtk_container_add (GTK_CONTAINER(window), box1);
	gtk_widget_show(box1);

	gtk_widget_show(window);
}

/* handler for SIGALRM to refresh the list */
void refresh_handler(void)
{
	gtk_list_store_clear(GTK_LIST_STORE(model));
	get_task_list();
	alarm(REFRESH_INTERVAL);
}

/* main */
int main(int argc, char *argv[])
{
	gtk_init(&argc, &argv);
	
	create_gui();
	
	signal(SIGALRM, refresh_handler);
	alarm(REFRESH_INTERVAL);
	
	gtk_main();
	
	return 0;
}
