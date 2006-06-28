/*
 *  xfce4-taskmanager - very simple taskmanger
 *
 *  Copyright (c) 2006 Johannes Zellner, <webmaster@nebulon.de>
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

#include "interface.h"

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)

GtkWidget* create_main_window (void)
{
	GtkWidget *window;
	GtkWidget *vbox1;
	GtkWidget *bbox1;
	GtkWidget *scrolledwindow1;
	GtkWidget *button1;
	GtkWidget *button2;
	GtkWidget *button3;

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), _("xfce4-taskmanager"));
	gtk_window_set_default_size (GTK_WINDOW (window), win_width, win_height);

	vbox1 = gtk_vbox_new (FALSE, 10);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (window), vbox1);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), 10);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow1);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (vbox1), scrolledwindow1, TRUE, TRUE, 0);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_SHADOW_IN);

	treeview = gtk_tree_view_new ();
	gtk_widget_show (treeview);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), treeview);

	create_list_store();
	
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

	gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(list_store));
	
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(list_store), 1, GTK_SORT_ASCENDING);

	bbox1 = gtk_hbutton_box_new();
	gtk_box_pack_start(GTK_BOX(vbox1), bbox1, FALSE, TRUE, 0);
	gtk_widget_show (bbox1);
  
	button2 = gtk_button_new_from_stock ("gtk-preferences");
	gtk_widget_show (button2);
	gtk_box_pack_start (GTK_BOX (bbox1), button2, FALSE, FALSE, 0);
	
	button3 = gtk_toggle_button_new_with_label (_("more details"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button3), full_view);
	gtk_widget_show (button3);
	gtk_box_pack_start (GTK_BOX (bbox1), button3, FALSE, FALSE, 0);
	
	button1 = gtk_button_new_from_stock ("gtk-quit");
	gtk_widget_show (button1);
	gtk_box_pack_start (GTK_BOX (bbox1), button1, FALSE, FALSE, 0);

	g_signal_connect ((gpointer) window, "destroy", G_CALLBACK (on_quit), NULL);
	g_signal_connect_swapped ((gpointer) treeview, "button-press-event", G_CALLBACK(on_treeview1_button_press_event), NULL);
	g_signal_connect ((gpointer) button1, "clicked",  G_CALLBACK (on_quit),  NULL);
	g_signal_connect ((gpointer) button2, "button_release_event",  G_CALLBACK (on_button1_button_press_event),  NULL);
	g_signal_connect ((gpointer) button3, "toggled",  G_CALLBACK (on_button3_toggled_event),  NULL);
	
	return window;
}

void create_list_store(void)
{
	GtkCellRenderer *cell_renderer;

	list_store = gtk_list_store_new(8, G_TYPE_STRING, G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);
	
	cell_renderer = gtk_cell_renderer_text_new();

	column = gtk_tree_view_column_new_with_attributes(_("Command"), cell_renderer, "text", 0, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, 0);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 0, compare_list_item, (void *)0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	
	column = gtk_tree_view_column_new_with_attributes(_("PID"), cell_renderer, "text", 1, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, 1);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 1, compare_list_item, (void *)1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	
	column = gtk_tree_view_column_new_with_attributes(_("PPID"), cell_renderer, "text", 2, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, 2);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 2, compare_list_item, (void *)2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	
	column = gtk_tree_view_column_new_with_attributes(_("State"), cell_renderer, "text", 3, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, 3);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 3, compare_list_item, (void *)3, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	
	column = gtk_tree_view_column_new_with_attributes(_("VM-Size"), cell_renderer, "text", 4, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, 4);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 4, compare_list_item, (void *)4, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
		
	column = gtk_tree_view_column_new_with_attributes(_("RSS"), cell_renderer, "text", 5, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, 5);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 5, compare_list_item, (void *)5, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
		
	column = gtk_tree_view_column_new_with_attributes(_("User"), cell_renderer, "text", 6, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, 6);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 6, compare_list_item, (void *)6, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	
	change_list_store_view();
}

GtkWidget* create_taskpopup (void)
{
	GtkWidget *taskpopup;
	GtkWidget *menu_item;

	taskpopup = gtk_menu_new ();

	menu_item = gtk_menu_item_new_with_mnemonic (_("Stop"));
	gtk_widget_show (menu_item);
	gtk_container_add (GTK_CONTAINER (taskpopup), menu_item);
	g_signal_connect ((gpointer) menu_item, "activate", G_CALLBACK (handle_task_menu), "STOP");

	menu_item = gtk_menu_item_new_with_mnemonic (_("Continue"));
	gtk_widget_show (menu_item);
	gtk_container_add (GTK_CONTAINER (taskpopup), menu_item);
	g_signal_connect ((gpointer) menu_item, "activate", G_CALLBACK (handle_task_menu), "CONT");

	menu_item = gtk_menu_item_new_with_mnemonic (_("Term"));
	gtk_widget_show (menu_item);
	gtk_container_add (GTK_CONTAINER (taskpopup), menu_item);
	g_signal_connect ((gpointer) menu_item, "activate", G_CALLBACK (handle_task_menu), "TERM");

	menu_item = gtk_menu_item_new_with_mnemonic (_("Kill"));
	gtk_widget_show (menu_item);
	gtk_container_add (GTK_CONTAINER (taskpopup), menu_item);
	g_signal_connect ((gpointer) menu_item, "activate", G_CALLBACK (handle_task_menu), "KILL");

	return taskpopup;
}

GtkWidget* create_mainmenu (void)
{
	GtkWidget *mainmenu;
	GtkWidget *info1;
	GtkWidget *trennlinie1;
	GtkWidget *show_user_tasks1;
	GtkWidget *show_root_tasks1;
	GtkWidget *show_other_tasks1;
	GtkAccelGroup *accel_group;

	accel_group = gtk_accel_group_new ();

	mainmenu = gtk_menu_new ();

	info1 = gtk_image_menu_item_new_from_stock ("gtk-about", accel_group);
	gtk_widget_show (info1);
	gtk_menu_shell_append(GTK_MENU_SHELL(mainmenu), info1);

	trennlinie1 = gtk_separator_menu_item_new ();
	gtk_widget_show (trennlinie1);
	gtk_menu_shell_append(GTK_MENU_SHELL(mainmenu), trennlinie1);
	gtk_widget_set_sensitive (trennlinie1, FALSE);

	show_user_tasks1 = gtk_check_menu_item_new_with_mnemonic (_("Show user tasks"));
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(show_user_tasks1), show_user_tasks);
	gtk_widget_show (show_user_tasks1);
	gtk_menu_shell_append(GTK_MENU_SHELL(mainmenu), show_user_tasks1);

	show_root_tasks1 = gtk_check_menu_item_new_with_mnemonic (_("Show root tasks"));
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(show_root_tasks1), show_root_tasks);
	gtk_widget_show (show_root_tasks1);
	gtk_menu_shell_append(GTK_MENU_SHELL(mainmenu), show_root_tasks1);

	show_other_tasks1 = gtk_check_menu_item_new_with_mnemonic (_("Show other tasks"));
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(show_other_tasks1), show_other_tasks);
	gtk_widget_show (show_other_tasks1);
	gtk_menu_shell_append(GTK_MENU_SHELL(mainmenu), show_other_tasks1);

	g_signal_connect ((gpointer) info1, "activate", G_CALLBACK (on_info1_activate), NULL);
	g_signal_connect ((gpointer) show_user_tasks1, "toggled", G_CALLBACK (on_show_tasks_toggled), (void *)own_uid);
	g_signal_connect ((gpointer) show_root_tasks1, "toggled", G_CALLBACK (on_show_tasks_toggled), (void *)0);
	g_signal_connect ((gpointer) show_other_tasks1, "toggled", G_CALLBACK (on_show_tasks_toggled), (void *)-1);

	gtk_menu_set_accel_group (GTK_MENU (mainmenu), accel_group);

	return mainmenu;
}

void show_about_dialog(void)
{
	GtkWidget *about_dialog;
	XfceAboutInfo *about_info;
	
	about_info = xfce_about_info_new("xfce4-taskmanager", VERSION, "Xfce4-Taskmanager is a easy to use Taskmanager.",XFCE_COPYRIGHT_TEXT("2005", "Johannes Zellner"), XFCE_LICENSE_GPL);
	xfce_about_info_set_homepage(about_info, "http://developer.berlios.de/projects/xfce-goodies/");
	xfce_about_info_add_credit(about_info, "Johannes Zellner", "webmaster@nebulon.de", "Original Author");
    
	about_dialog = xfce_about_dialog_new(GTK_WINDOW(main_window), about_info, NULL);
	g_signal_connect(G_OBJECT(about_dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);
	gtk_window_set_title (GTK_WINDOW (about_dialog), _("xfce4-taskmanager"));
	gtk_widget_show(about_dialog);
	
	xfce_about_info_free(about_info);
}

void change_list_store_view(void)
{
	gtk_tree_view_column_set_visible (gtk_tree_view_get_column(GTK_TREE_VIEW(treeview), COLUMN_PPID), full_view);
	gtk_tree_view_column_set_visible (gtk_tree_view_get_column(GTK_TREE_VIEW(treeview), COLUMN_MEM), full_view);
	gtk_tree_view_column_set_visible (gtk_tree_view_get_column(GTK_TREE_VIEW(treeview), COLUMN_RSS), full_view);
}

void fill_list_item(gint i, GtkTreeIter *iter)
{
	if(iter != NULL)
	{
		struct task *task = &g_array_index(task_array, struct task, i);
		
		gchar *pid = g_strdup_printf("%i", task->pid);
		gchar *ppid = g_strdup_printf("%i", task->ppid);
		gchar *state = g_strdup_printf("%s", task->state);
		gchar *size = g_strdup_printf("%i kB", task->size);
		gchar *rss = g_strdup_printf("%i kB", task->rss);
		gchar *name = g_strdup_printf("%s", task->name);
		gchar *uname = g_strdup_printf("%s", task->uname);
	
		gtk_list_store_set(GTK_LIST_STORE(list_store), iter, COLUMN_NAME, name, -1);
		gtk_list_store_set(GTK_LIST_STORE(list_store), iter, COLUMN_PID, pid, -1);
		gtk_list_store_set(GTK_LIST_STORE(list_store), iter, COLUMN_PPID, ppid, -1);
		gtk_list_store_set(GTK_LIST_STORE(list_store), iter, COLUMN_STATE, state, -1);
		gtk_list_store_set(GTK_LIST_STORE(list_store), iter, COLUMN_MEM, size, -1);
		gtk_list_store_set(GTK_LIST_STORE(list_store), iter, COLUMN_RSS, rss, -1);
		gtk_list_store_set(GTK_LIST_STORE(list_store), iter, COLUMN_UNAME, uname, -1);
		
		free(pid);
		free(ppid);
		free(state);
		free(size);
		free(rss);
		free(name);
		free(uname);
	}
}


void send_signal_to_task(gchar *task_id, gchar *signal)
{
	if(task_id != "" && signal != NULL)
	{
		gchar command[64] = "kill -";
		g_strlcat(command,signal, sizeof command);
		g_strlcat(command," ", sizeof command);
		g_strlcat(command,task_id, sizeof command);
		
		if(system(command) != 0)
			xfce_err("Couldn't %s the task with ID %s", signal, task_id);
	}
}


void add_new_list_item(gint i)
{
		GtkTreeIter iter;
	
		gtk_list_store_append(GTK_LIST_STORE(list_store), &iter);
	
		fill_list_item(i, &iter);
}

void refresh_list_item(gint i)
{
	GtkTreeIter iter;
	gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list_store), &iter);
	struct task task = g_array_index(task_array, struct task, i);
		
	while(valid)
	{
		gchar *str_data = "";
		gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter, 1, &str_data, -1);

		if(task.pid == atoi(str_data))
		{
			g_free(str_data);
			fill_list_item(i, &iter);
			break;
		}

		g_free(str_data);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(list_store), &iter);
	}
}

void remove_list_item(gint pid)
{
	GtkTreeIter iter;
	gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list_store), &iter);
	
	while(valid)
	{
		gchar *str_data = "";
		gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter, 1, &str_data, -1);

		if(pid == atoi(str_data))
		{
			free(str_data);
			gtk_list_store_remove(GTK_LIST_STORE(list_store), &iter);
			break;
		}

		free(str_data);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(list_store), &iter);
	}
}

gint compare_list_item(GtkTreeModel *model, GtkTreeIter *iter1, GtkTreeIter *iter2, gpointer colnum)
{	
	gchar *s1 = "";
	gchar *s2 = "";
	gint ret = 0;

	gtk_tree_model_get(model, iter1, colnum, &s1, -1);
	gtk_tree_model_get(model, iter2, colnum, &s2, -1);

	if(GPOINTER_TO_INT(colnum) == 1 || GPOINTER_TO_INT(colnum) == 2 || GPOINTER_TO_INT(colnum) == 4 || GPOINTER_TO_INT(colnum) == 5)
	{
		gint i1 = 0;
		gint i2 = 0;
	
		if(s1 != NULL)
			i1 = atoi(s1);
		
		if(s2 != NULL)
			i2 = atoi(s2);
	
		if(i1 < i2)
			ret = -1;
		else
			ret = 1;
	}
	else
	{
		if(s1 == NULL)
			s1 = "";
		if(s2 == NULL)
			s2 = "";
	
		ret = strcmp(s1, s2);
	}
	
	free(s1);
	free(s2);
	
	return ret;
}

/* change the task view (user, root, other) */
void change_task_view(void)
{
	gtk_list_store_clear(GTK_LIST_STORE(list_store));
	gint i = 0;
	
	for(i = 0; i < tasks; i++)
	{
		struct task task = g_array_index(task_array, struct task, i);
		
		if((task.uid == own_uid && show_user_tasks) || (task.uid == 0 && show_root_tasks) || (task.uid != own_uid && task.uid != 0 && show_other_tasks))
			add_new_list_item(i);
	}
	
	refresh_task_list();
}


