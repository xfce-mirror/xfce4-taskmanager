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

#include "functions.h"

gboolean refresh_task_list(void)
{
	gint i, j;
	GArray *new_task_list;

	/* gets the new task list */
	new_task_list = (GArray*) get_task_list();

	/* check if task is new and marks the task that its checked*/
	for(i = 0; i < task_array->len; i++)
	{
		struct task *tmp = &g_array_index(task_array, struct task, i);
		tmp->checked = FALSE;

		for(j = 0; j < new_task_list->len; j++)
		{
			struct task *new_tmp = &g_array_index(new_task_list, struct task, j);

			if(new_tmp->pid == tmp->pid)
			{
				tmp->old_time = tmp->time;
				
				tmp->time = new_tmp->time;
				
				
				tmp->time_percentage = (gdouble)(tmp->time - tmp->old_time) * (gdouble)(1000.0 / REFRESH_INTERVAL);
				
				if((gint)tmp->ppid != (gint)new_tmp->ppid || strcmp(tmp->state,new_tmp->state) || (unsigned int)tmp->size != (unsigned int)new_tmp->size || (unsigned int)tmp->rss != (unsigned int)new_tmp->rss || (unsigned int)tmp->time != (unsigned int)tmp->old_time)
				{
					tmp->ppid = new_tmp->ppid;
					strcpy(tmp->state, new_tmp->state);
					tmp->size = new_tmp->size;
					tmp->rss = new_tmp->rss;
					
					refresh_list_item(i);
				}
				tmp->checked = TRUE;
				new_tmp->checked = TRUE;
				break;
			}
			else
				tmp->checked = FALSE;
		}
	}

	/* check for unchecked old-tasks for deleting */
	i = 0;
	while( i < task_array->len)
	{

		struct task *tmp = &g_array_index(task_array, struct task, i);

		if(!tmp->checked)
		{
			remove_list_item((gint)tmp->pid);
			g_array_remove_index(task_array, i);
			tasks--;
		}
		else
			i++;

	}


	/* check for unchecked new tasks for inserting */
	for(i = 0; i < new_task_list->len; i++)
	{
		struct task *new_tmp = &g_array_index(new_task_list, struct task, i);

		if(!new_tmp->checked)
		{
			struct task *new_task = new_tmp;

			g_array_append_val(task_array, *new_task);
			if((show_user_tasks && new_task->uid == own_uid) || (show_root_tasks && new_task->uid == 0) ||  (show_other_tasks && new_task->uid != own_uid && new_task->uid != 0))
				add_new_list_item(tasks);
			tasks++;
		}
	}

	g_array_free(new_task_list, TRUE);
	
	system_status *sys_stat = g_new (system_status, 1);
	get_system_status (sys_stat);
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (mem_usage_progress_bar), 1.0 - ( (gdouble) sys_stat->mem_free / sys_stat->mem_total ));
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (cpu_usage_progress_bar), get_cpu_usage(sys_stat));
	g_free (sys_stat);
	
	return TRUE;
}

gdouble get_cpu_usage(system_status *sys_stat)
{
	gdouble cpu_usage = 0.0;
	gint i = 0;
	
	for(i = 0; i < task_array->len; i++)
	{
		struct task *tmp = &g_array_index(task_array, struct task, i);
		cpu_usage += tmp->time_percentage;
	}
	
	cpu_usage = cpu_usage / (sys_stat->cpu_count * 100.0);
	
	return cpu_usage;
}

/*
 * configurationfile support
 */
void load_config(void)
{
	XfceRc *rc_file = xfce_rc_simple_open(config_file, FALSE);

	xfce_rc_set_group (rc_file, "General");

	show_user_tasks = xfce_rc_read_bool_entry(rc_file, "show_user_tasks", TRUE);
	show_root_tasks = xfce_rc_read_bool_entry(rc_file, "show_root_tasks", FALSE);
	show_other_tasks = xfce_rc_read_bool_entry(rc_file, "show_other_tasks", FALSE);

	full_view = xfce_rc_read_bool_entry(rc_file, "full_view", FALSE);

	win_width = xfce_rc_read_int_entry(rc_file, "win_width", 500);
	win_height = xfce_rc_read_int_entry(rc_file, "win_height", 400);
	
	xfce_rc_close(rc_file);
}

void save_config(void)
{
	XfceRc *rc_file = xfce_rc_simple_open(config_file, FALSE);

	xfce_rc_set_group (rc_file, "General");

	xfce_rc_write_bool_entry(rc_file, "show_user_tasks", show_user_tasks);
	xfce_rc_write_bool_entry(rc_file, "show_root_tasks", show_root_tasks);
	xfce_rc_write_bool_entry(rc_file, "show_other_tasks", show_other_tasks);

	xfce_rc_write_bool_entry(rc_file, "full_view", full_view);

	gtk_window_get_size(GTK_WINDOW (main_window), &win_width, &win_height);

	xfce_rc_write_int_entry(rc_file, "win_width", win_width);
	xfce_rc_write_int_entry(rc_file, "win_height", win_height);

	xfce_rc_flush(rc_file);

	xfce_rc_close(rc_file);
}
