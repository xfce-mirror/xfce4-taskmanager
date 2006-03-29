/*
 *  xfce4-taskmanager - very simple taskmanger
 *
 *  Copyright (c) 2005 Johannes Zellner, <webmaster@nebulon.de>
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
	GtkWidget *window1;
	GtkWidget *vbox1;
	GtkWidget *bbox1;
	GtkWidget *scrolledwindow1;
	GtkWidget *button1;
	GtkWidget *button2;
	GtkWidget *button3;

	window1 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window1), _("xfce4-taskmanager"));
	gtk_window_set_default_size (GTK_WINDOW (window1), win_width, win_height);

	vbox1 = gtk_vbox_new (FALSE, 10);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (window1), vbox1);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), 10);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow1);
	gtk_scrolled_window_set_policy (scrolledwindow1, GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (vbox1), scrolledwindow1, TRUE, TRUE, 0);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_SHADOW_IN);

	treeview1 = gtk_tree_view_new ();
	gtk_widget_show (treeview1);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), treeview1);

	create_list_store();
	
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview1));

	gtk_tree_view_set_model(GTK_TREE_VIEW(treeview1), GTK_TREE_MODEL(list_store));
	
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

	g_signal_connect ((gpointer) window1, "destroy", G_CALLBACK (on_quit), NULL);
	g_signal_connect_swapped ((gpointer) treeview1, "button-press-event", G_CALLBACK(on_treeview1_button_press_event), NULL);
	g_signal_connect ((gpointer) button1, "clicked",  G_CALLBACK (on_quit),  NULL);
	g_signal_connect ((gpointer) button2, "button_release_event",  G_CALLBACK (on_button1_button_press_event),  NULL);
	g_signal_connect ((gpointer) button3, "toggled",  G_CALLBACK (on_button3_toggled_event),  NULL);
	
	return window1;
}

void change_list_store_view(void)
{
	gtk_tree_view_column_set_visible (column4, full_view);
	gtk_tree_view_column_set_visible (column5, full_view);
	gtk_tree_view_column_set_visible (column6, full_view);
}

void create_list_store(void)
{
	GtkCellRenderer *cell_renderer;

	list_store = gtk_list_store_new(8, G_TYPE_STRING, G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);
	
	cell_renderer = gtk_cell_renderer_text_new();

	column1 = gtk_tree_view_column_new_with_attributes(_("Command"), cell_renderer, "text", 0, NULL);
	gtk_tree_view_column_set_resizable(column1, TRUE);
	gtk_tree_view_column_set_sort_column_id(column1, 0);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 0, compare_list_item, (void *)0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview1), column1);
	
	column2 = gtk_tree_view_column_new_with_attributes(_("PID"), cell_renderer, "text", 1, NULL);
	gtk_tree_view_column_set_resizable(column2, TRUE);
	gtk_tree_view_column_set_sort_column_id(column2, 1);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 1, compare_list_item, (void *)1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview1), column2);
	
	column3 = gtk_tree_view_column_new_with_attributes(_("PPID"), cell_renderer, "text", 2, NULL);
	gtk_tree_view_column_set_resizable(column3, TRUE);
	gtk_tree_view_column_set_sort_column_id(column3, 2);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 2, compare_list_item, (void *)2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview1), column3);
	
	column4 = gtk_tree_view_column_new_with_attributes(_("State"), cell_renderer, "text", 3, NULL);
	gtk_tree_view_column_set_resizable(column4, TRUE);
	gtk_tree_view_column_set_sort_column_id(column4, 3);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 3, compare_list_item, (void *)3, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview1), column4);
	
	column5 = gtk_tree_view_column_new_with_attributes(_("VM-Size"), cell_renderer, "text", 4, NULL);
	gtk_tree_view_column_set_resizable(column5, TRUE);
	gtk_tree_view_column_set_sort_column_id(column5, 4);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 4, compare_list_item, (void *)4, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview1), column5);
		
	column6 = gtk_tree_view_column_new_with_attributes(_("RSS"), cell_renderer, "text", 5, NULL);
	gtk_tree_view_column_set_resizable(column6, TRUE);
	gtk_tree_view_column_set_sort_column_id(column6, 5);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 5, compare_list_item, (void *)5, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview1), column6);
		
	column7 = gtk_tree_view_column_new_with_attributes(_("User"), cell_renderer, "text", 6, NULL);
	gtk_tree_view_column_set_resizable(column7, TRUE);
	gtk_tree_view_column_set_sort_column_id(column7, 6);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 6, compare_list_item, (void *)6, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview1), column7);
	
	change_list_store_view();
}

GtkWidget* create_taskpopup (void)
{
	GtkWidget *taskpopup;
	GtkWidget *stop1;
	GtkWidget *continue1;
	GtkWidget *term1;
	GtkWidget *kill1;

	taskpopup = gtk_menu_new ();

	stop1 = gtk_menu_item_new_with_mnemonic (_("Stop"));
	gtk_widget_show (stop1);
	gtk_container_add (GTK_CONTAINER (taskpopup), stop1);

	continue1 = gtk_menu_item_new_with_mnemonic (_("Continue"));
	gtk_widget_show (continue1);
	gtk_container_add (GTK_CONTAINER (taskpopup), continue1);

	term1 = gtk_menu_item_new_with_mnemonic (_("Term"));
	gtk_widget_show (term1);
	gtk_container_add (GTK_CONTAINER (taskpopup), term1);

	kill1 = gtk_menu_item_new_with_mnemonic (_("Kill"));
	gtk_widget_show (kill1);
	gtk_container_add (GTK_CONTAINER (taskpopup), kill1);

	g_signal_connect ((gpointer) stop1, "activate", G_CALLBACK (handle_task_menu), "STOP");
	g_signal_connect ((gpointer) continue1, "activate", G_CALLBACK (handle_task_menu), "CONT");
	g_signal_connect ((gpointer) term1, "activate", G_CALLBACK (handle_task_menu), "TERM");
	g_signal_connect ((gpointer) kill1, "activate", G_CALLBACK (handle_task_menu), "KILL");

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



