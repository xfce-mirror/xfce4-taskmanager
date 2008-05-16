/* $Id$
 *
 * Copyright (c) 2006  Johannes Zellner <webmaster@nebulon.de>
 *               2008  Mike Massonnet <mmassonnet@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
	GtkWidget *button_preferences;
	GtkWidget *button_information;
	GtkWidget *button_quit;

	GtkWidget *system_info_box;

	tooltips = gtk_tooltips_new();
	gtk_tooltips_enable(tooltips);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), _("Xfce4 Taskmanager"));
	gtk_window_set_icon_name (GTK_WINDOW (window), "xfce-system");
	gtk_window_set_default_size (GTK_WINDOW (window), win_width, win_height);

	vbox1 = gtk_vbox_new (FALSE, 10);
	gtk_widget_show (vbox1);
	gtk_container_add (GTK_CONTAINER (window), vbox1);
	gtk_container_set_border_width (GTK_CONTAINER (vbox1), 10);

	system_info_box = gtk_hbox_new (FALSE, 10);
	gtk_widget_show (system_info_box);
	gtk_box_pack_start (GTK_BOX (vbox1), system_info_box, FALSE, TRUE, 0);

	cpu_usage_progress_bar_box = gtk_event_box_new ();
	cpu_usage_progress_bar = gtk_progress_bar_new ();
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (cpu_usage_progress_bar), _("Cpu usage"));
	gtk_widget_show (cpu_usage_progress_bar);
	gtk_widget_show (cpu_usage_progress_bar_box);
	gtk_container_add (GTK_CONTAINER (cpu_usage_progress_bar_box), cpu_usage_progress_bar);
	gtk_box_pack_start (GTK_BOX (system_info_box), cpu_usage_progress_bar_box, TRUE, TRUE, 0);

	mem_usage_progress_bar_box = gtk_event_box_new ();
	mem_usage_progress_bar = gtk_progress_bar_new ();
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (mem_usage_progress_bar), _("Memory usage"));
	gtk_widget_show (mem_usage_progress_bar);
	gtk_widget_show (mem_usage_progress_bar_box);
	gtk_container_add (GTK_CONTAINER (mem_usage_progress_bar_box), mem_usage_progress_bar);
	gtk_box_pack_start (GTK_BOX (system_info_box), mem_usage_progress_bar_box, TRUE, TRUE, 0);

	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow1);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (vbox1), scrolledwindow1, TRUE, TRUE, 0);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_SHADOW_IN);

	treeview = gtk_tree_view_new ();
    gtk_tree_view_set_show_expanders (GTK_TREE_VIEW (treeview), FALSE);
	gtk_widget_show (treeview);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), treeview);

	create_list_store();

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

	gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(list_store));

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(list_store), sort_column, sort_type);

	bbox1 = gtk_hbutton_box_new();
	gtk_box_pack_start(GTK_BOX(vbox1), bbox1, FALSE, TRUE, 0);
	gtk_widget_show (bbox1);

	button_preferences = gtk_button_new_from_stock ("gtk-preferences");
	gtk_button_set_focus_on_click (GTK_BUTTON (button_preferences), FALSE);
	gtk_widget_show (button_preferences);
	gtk_box_pack_start (GTK_BOX (bbox1), button_preferences, FALSE, FALSE, 0);

	button_information = gtk_button_new_from_stock ("gtk-info");
	gtk_button_set_focus_on_click (GTK_BUTTON (button_information), FALSE);
	gtk_widget_show (button_information);
	gtk_box_pack_start (GTK_BOX (bbox1), button_information, FALSE, FALSE, 0);

	button_quit = gtk_button_new_from_stock ("gtk-quit");
	gtk_widget_show (button_quit);
	gtk_box_pack_start (GTK_BOX (bbox1), button_quit, FALSE, FALSE, 0);

	g_signal_connect_swapped (treeview, "button-press-event", G_CALLBACK(on_treeview1_button_press_event), NULL);
	g_signal_connect (button_preferences, "clicked",  G_CALLBACK (on_preferences),  NULL);
	g_signal_connect (button_information, "clicked",  G_CALLBACK (on_information),  NULL);
	g_signal_connect (button_quit, "clicked",  G_CALLBACK (on_quit),  NULL);
	g_signal_connect (window, "delete-event", G_CALLBACK (on_quit), NULL);

	return window;
}

void create_list_store(void)
{
	GtkCellRenderer *cell_renderer;
	GtkCellRenderer *cell_renderer_right_align;
	GtkCellRenderer *cell_renderer_command;

	/* my change 8->9 */
	list_store = gtk_tree_store_new(9, G_TYPE_STRING, G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING);

	cell_renderer = gtk_cell_renderer_text_new();
	cell_renderer_right_align = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(cell_renderer_right_align), "xalign", 1.0, NULL); 
	cell_renderer_command = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(cell_renderer_command), "ellipsize", PANGO_ELLIPSIZE_END, NULL); 

	column = gtk_tree_view_column_new_with_attributes(_("Command"), cell_renderer_command, "text", 0, NULL);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, 0);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 0, compare_string_list_item, (void *)0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	column = gtk_tree_view_column_new_with_attributes(_("PID"), cell_renderer_right_align, "text", 1, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, 1);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 1, compare_int_list_item, (void *)1, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	column = gtk_tree_view_column_new_with_attributes(_("PPID"), cell_renderer_right_align, "text", 2, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, 2);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 2, compare_string_list_item, (void *)2, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	column = gtk_tree_view_column_new_with_attributes(_("State"), cell_renderer, "text", 3, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, 3);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 3, compare_int_list_item, (void *)3, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	column = gtk_tree_view_column_new_with_attributes(_("VM-Size"), cell_renderer_right_align, "text", 4, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, 4);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 4, compare_int_list_item, (void *)4, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	column = gtk_tree_view_column_new_with_attributes(_("RSS"), cell_renderer_right_align, "text", 5, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, 5);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 5, compare_int_list_item, (void *)5, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	column = gtk_tree_view_column_new_with_attributes(_("User"), cell_renderer, "text", 6, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, 6);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 6, compare_string_list_item, (void *)6, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	column = gtk_tree_view_column_new_with_attributes(_("CPU%"), cell_renderer_right_align, "text", 7, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, 7);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 7, compare_int_list_item, (void *)7, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);

	/* my change */
	column = gtk_tree_view_column_new_with_attributes(_("Prio"), cell_renderer_right_align, "text", 8, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, 8);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), 8, compare_int_list_item, (void *)8, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column);
	/* /my change */

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

	menu_item = gtk_menu_item_new_with_mnemonic ( _("Priority") );
	gtk_menu_item_set_submenu((gpointer) menu_item, create_prio_submenu());
	gtk_widget_show (menu_item);
	gtk_container_add (GTK_CONTAINER (taskpopup), menu_item);

	return taskpopup;
}

GtkWidget *create_prio_submenu(void)
{
	GtkWidget *prio_submenu = gtk_menu_new ();
	GtkWidget *menu_item;

	menu_item = gtk_menu_item_new_with_label ("-10");
	gtk_misc_set_alignment (GTK_MISC (GTK_BIN (menu_item)->child), 1.0, 0.5);
	gtk_widget_show (menu_item);
	gtk_container_add (GTK_CONTAINER (prio_submenu), menu_item);
	g_signal_connect ((gpointer) menu_item, "activate", G_CALLBACK (handle_prio_menu), "-10");
	
	menu_item = gtk_menu_item_new_with_label ("-5");
	gtk_misc_set_alignment (GTK_MISC (GTK_BIN (menu_item)->child), 1.0, 0.5);
	gtk_widget_show (menu_item);
	gtk_container_add (GTK_CONTAINER (prio_submenu), menu_item);
	g_signal_connect ((gpointer) menu_item, "activate", G_CALLBACK (handle_prio_menu), "-5");

	menu_item = gtk_menu_item_new_with_label ("0");
	gtk_misc_set_alignment (GTK_MISC (GTK_BIN (menu_item)->child), 1.0, 0.5);
	gtk_widget_show (menu_item);
	gtk_container_add (GTK_CONTAINER (prio_submenu), menu_item);
	g_signal_connect ((gpointer) menu_item, "activate", G_CALLBACK (handle_prio_menu), "0");

	menu_item = gtk_menu_item_new_with_label ("5");
	gtk_misc_set_alignment (GTK_MISC (GTK_BIN (menu_item)->child), 1.0, 0.5);
	gtk_widget_show (menu_item);
	gtk_container_add (GTK_CONTAINER (prio_submenu), menu_item);
	g_signal_connect ((gpointer) menu_item, "activate", G_CALLBACK (handle_prio_menu), "5");

	menu_item = gtk_menu_item_new_with_label ("10");
	gtk_misc_set_alignment (GTK_MISC (GTK_BIN (menu_item)->child), 1.0, 0.5);
	gtk_widget_show (menu_item);
	gtk_container_add (GTK_CONTAINER (prio_submenu), menu_item);
	g_signal_connect ((gpointer) menu_item, "activate", G_CALLBACK (handle_prio_menu), "10");
	
	return prio_submenu;
}

GtkWidget* create_mainmenu (void)
{
	GtkWidget *mainmenu;
	GtkWidget *info1;
	GtkWidget *trennlinie1;
	GtkWidget *show_user_tasks1;
	GtkWidget *show_root_tasks1;
	GtkWidget *show_other_tasks1;
	GtkWidget *show_cached_as_free1;
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

	show_cached_as_free1 = gtk_check_menu_item_new_with_mnemonic (_("Show memory used by cache as free"));
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(show_cached_as_free1), show_cached_as_free);
	gtk_widget_show (show_cached_as_free1);
	gtk_menu_shell_append(GTK_MENU_SHELL(mainmenu), show_cached_as_free1);

	g_signal_connect ((gpointer) info1, "activate", G_CALLBACK (on_info1_activate), NULL);
	g_signal_connect ((gpointer) show_user_tasks1, "toggled", G_CALLBACK (on_show_tasks_toggled), GINT_TO_POINTER(own_uid));
	g_signal_connect ((gpointer) show_root_tasks1, "toggled", G_CALLBACK (on_show_tasks_toggled), GINT_TO_POINTER(0));
	g_signal_connect ((gpointer) show_other_tasks1, "toggled", G_CALLBACK (on_show_tasks_toggled), GINT_TO_POINTER(-1));
	g_signal_connect ((gpointer) show_cached_as_free1, "toggled", G_CALLBACK (on_show_cached_as_free_toggled), GINT_TO_POINTER(0));

	gtk_menu_set_accel_group (GTK_MENU (mainmenu), accel_group);

	return mainmenu;
}

GtkWidget* create_infomenu (void)
{
	GtkWidget *infomenu;
	GtkWidget *title;
	GtkWidget *separator;
	GtkWidget *col_items[N_COLUMNS] = { 0 };
	gint i;

	infomenu = gtk_menu_new ();

	title = gtk_image_menu_item_new_from_stock("gtk-info", NULL);
	gtk_widget_show(title);
	gtk_menu_shell_append(GTK_MENU_SHELL(infomenu), title);
	gtk_widget_set_sensitive(title, FALSE);

	separator = gtk_separator_menu_item_new();
	gtk_widget_show(separator);
	gtk_menu_shell_append(GTK_MENU_SHELL(infomenu), separator);
	gtk_widget_set_sensitive(separator, FALSE);

	col_items[COLUMN_PID] = gtk_check_menu_item_new_with_label (_("PID"));
	col_items[COLUMN_PPID] = gtk_check_menu_item_new_with_label (_("PPID"));
	col_items[COLUMN_STATE] = gtk_check_menu_item_new_with_label (_("State"));
	col_items[COLUMN_MEM] = gtk_check_menu_item_new_with_label (_("VM-Size"));
	col_items[COLUMN_RSS] = gtk_check_menu_item_new_with_label (_("RSS"));
	col_items[COLUMN_UNAME] = gtk_check_menu_item_new_with_label (_("User"));
	col_items[COLUMN_TIME] = gtk_check_menu_item_new_with_label (_("CPU%"));
	col_items[COLUMN_PRIO] = gtk_check_menu_item_new_with_label (_("Priority"));

	for (i = 0; i < N_COLUMNS; i++)
	{
		if (GTK_IS_WIDGET (col_items[i]))
		{
			gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (col_items[i]), show_col[i]);
			gtk_widget_show(col_items[i]);
			gtk_menu_shell_append(GTK_MENU_SHELL(infomenu), col_items[i]);
			g_signal_connect (col_items[i], "toggled",
				G_CALLBACK (on_show_info_toggled), GINT_TO_POINTER(i));
		}
	}

	return infomenu;
}

void show_about_dialog(void)
{
	GtkWidget *about_dialog;
	const gchar *authors[] = {
	  _("Original Author:"),
	  "Johannes Zellner <webmaster@nebulon.de>",
	  _("Contributors:"),
	  "Mike Massonnet <mmassonnet@xfce.org>",
	  NULL };

	about_dialog = gtk_about_dialog_new();
	gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(about_dialog),
		PACKAGE_NAME);
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about_dialog),
		PACKAGE_VERSION);
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dialog),
		_("Xfce4-Taskmanager is an easy to use taskmanager"));
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about_dialog),
		"http://goodies.xfce.org/projects/applications/xfce4-taskmanager");
	gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(about_dialog),
		"xfce-system");
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about_dialog),
		authors);
	gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(about_dialog),
		_("translator-credits"));
	gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(about_dialog),
		xfce_get_license_text(XFCE_LICENSE_TEXT_GPL));
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about_dialog),
		"Copyright \302\251 2005-2008 Johannes Zellner");

	gtk_window_set_icon_name(GTK_WINDOW(about_dialog), GTK_STOCK_ABOUT);

	gtk_dialog_run(GTK_DIALOG(about_dialog));

	gtk_widget_destroy(about_dialog);
}

void change_list_store_view(void)
{
	gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(treeview), COLUMN_PID), show_col[COLUMN_PID]);
	gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(treeview), COLUMN_PPID), show_col[COLUMN_PPID]);
	gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(treeview), COLUMN_STATE), show_col[COLUMN_STATE]);
	gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(treeview), COLUMN_MEM), show_col[COLUMN_MEM]);
	gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(treeview), COLUMN_RSS), show_col[COLUMN_RSS]);
	gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(treeview), COLUMN_UNAME), show_col[COLUMN_UNAME]);
	gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(treeview), COLUMN_TIME), show_col[COLUMN_TIME]);
	gtk_tree_view_column_set_visible(gtk_tree_view_get_column(GTK_TREE_VIEW(treeview), COLUMN_PRIO), show_col[COLUMN_PRIO]);
}

void fill_list_item(gint i, GtkTreeIter *iter)
{
	static gint pagesize = 0;
	if (pagesize == 0)
		pagesize = getpagesize();

	if(iter != NULL)
	{
		struct task *task = &g_array_index(task_array, struct task, i);

		gchar *pid = g_strdup_printf("%i", task->pid);
		gchar *ppid = g_strdup_printf("%i", task->ppid);
		gchar *state = g_strdup_printf("%s", task->state);
		gchar *vsize = g_strdup_printf("%i MB", task->vsize/1024/1024);
		gchar *rss = g_strdup_printf("%i MB", task->rss*pagesize/1024/1024);
		gchar *name = g_strdup_printf("%s", task->name);
		gchar *uname = g_strdup_printf("%s", task->uname);
		gchar *time = g_strdup_printf("%0d%%", (guint)task->time_percentage);
		gchar *prio = g_strdup_printf("%i", task->prio);	/* my change */

		gtk_tree_store_set(GTK_TREE_STORE(list_store), iter, COLUMN_NAME, name, -1);
		gtk_tree_store_set(GTK_TREE_STORE(list_store), iter, COLUMN_PID, pid, -1);
		gtk_tree_store_set(GTK_TREE_STORE(list_store), iter, COLUMN_PPID, ppid, -1);
		gtk_tree_store_set(GTK_TREE_STORE(list_store), iter, COLUMN_STATE, state, -1);
		gtk_tree_store_set(GTK_TREE_STORE(list_store), iter, COLUMN_MEM, vsize, -1);
		gtk_tree_store_set(GTK_TREE_STORE(list_store), iter, COLUMN_RSS, rss, -1);
		gtk_tree_store_set(GTK_TREE_STORE(list_store), iter, COLUMN_UNAME, uname, -1);
		gtk_tree_store_set(GTK_TREE_STORE(list_store), iter, COLUMN_TIME, time, -1);
		gtk_tree_store_set(GTK_TREE_STORE(list_store), iter, COLUMN_PRIO, prio, -1);	/* my change */

		g_free(pid);
		g_free(ppid);
		g_free(state);
		g_free(vsize);
		g_free(rss);
		g_free(name);
		g_free(uname);
		g_free(time);
		g_free(prio);	/* my change */
	}
}

void add_new_list_item(gint i)
{
		GtkTreeIter iter;

		gtk_tree_store_append(GTK_TREE_STORE(list_store), &iter, NULL);

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
			g_free(str_data);
			gtk_tree_store_remove(GTK_TREE_STORE(list_store), &iter);
			break;
		}

		g_free(str_data);
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(list_store), &iter);
	}
}

gint compare_int_list_item(GtkTreeModel *model, GtkTreeIter *iter1, GtkTreeIter *iter2, gpointer column)
{
	gchar *s1 = "";
	gchar *s2 = "";

	gint ret = 0;

	gtk_tree_model_get(model, iter1, column, &s1, -1);
	gtk_tree_model_get(model, iter2, column, &s2, -1);

	gint i1 = 0;
	gint i2 = 0;

	if(s1 != NULL)
		i1 = atoi(s1);

	if(s2 != NULL)
		i2 = atoi(s2);

	ret = i2 - i1;

	if(s1 != NULL)
		g_free(s1);
	if(s2 != NULL)
		g_free(s2);

	return ret;
}

gint compare_string_list_item(GtkTreeModel *model, GtkTreeIter *iter1, GtkTreeIter *iter2, gpointer column)
{
	gchar *s1 = "";
	gchar *s2 = "";

	gint ret = 0;

	gtk_tree_model_get(model, iter1, GPOINTER_TO_INT(column), &s1, -1);
	gtk_tree_model_get(model, iter2, GPOINTER_TO_INT(column), &s2, -1);

	if(s1 != NULL && s2 != NULL)
		ret = strcmp(s2, s1);
	else
		ret = 0;

	if(s1 != NULL)
		g_free(s1);
	if(s2 != NULL)
		g_free(s2);

	return ret;
}

/* change the task view (user, root, other) */
void change_task_view(void)
{
	gtk_tree_store_clear(GTK_TREE_STORE(list_store));
	gint i = 0;

	for(i = 0; i < tasks; i++)
	{
		struct task task = g_array_index(task_array, struct task, i);

		if((task.uid == own_uid && show_user_tasks) || (task.uid == 0 && show_root_tasks) || (task.uid != own_uid && task.uid != 0 && show_other_tasks))
			add_new_list_item(i);
	}

	refresh_task_list();
}


