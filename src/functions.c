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

#include "functions.h"

gint task_count = 0;

/* function to kill the current task */
void send_signal_to_task(gchar *task_id, gchar *signal)
{
	if(task_id != "" && signal != NULL)
	{
		gchar command[64] = "kill -";
		g_strlcat(command,signal, 64);
		g_strlcat(command," ", 64);
		g_strlcat(command,task_id, 64);
		
		if(system(command) != 0)
			xfce_err("Couldn't %s the task with ID %s", signal, task_id);
	}
}

void refresh_task_list(gboolean first_time)
{
	/* markes all tasks to "not checked" */
	gint i;
	
	for(i = 0; i < task_count; i++)
		all_tasks[i].checked = FALSE;
	
	/* load the current taskdetails */
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
			struct passwd *passwdp;
		
			if((task_file = fopen(task_file_name,"r")) != NULL)
			{
				while(fgets(buffer, 256, task_file) != NULL)
				{
					if(line_count == 0)
						strcpy(task.name,g_strstrip(g_strsplit(buffer, ":", 2)[1]));
					else if(line_count == 3)
						task.pid = atoi(g_strstrip(g_strsplit(buffer, ":", 2)[1]));
					else if(line_count == 5)
						task.ppid = atoi(g_strstrip(g_strsplit(buffer, ":", 2)[1]));
					else if(line_count == 7)
					{
						task.uid = atoi(g_strsplit(g_strstrip(g_strsplit(buffer, ":", 2)[1]), "\t", 2)[0]);
						passwdp = getpwuid(task.uid); 
						strcpy(task.uname, passwdp->pw_name);
					}
					
					line_count++;
				}
			
				line_count = 0;
				fclose(task_file);
				
				/* check if task is new and marks the task that its checked*/
				gboolean new_task = TRUE;
				
				for(i = 0; i < task_count; i++)
				{
					if(all_tasks[i].pid == task.pid)
					{
						all_tasks[i].checked = TRUE;
						new_task = FALSE;
					}
				}
	
				if(new_task)
				{
					task.checked = TRUE;
					all_tasks[task_count] = task;
					task_count++;
					
					if(!first_time)
						add_tree_item(task);
				}
			}
		}
	}
	closedir(dir);
	
	if(!first_time)
	{
		/* removing all tasks which are not marked */
		i = 0;
	
		while(i < task_count)
		{
			if(!all_tasks[i].checked)
			{
				remove_tree_item(all_tasks[i]);
				remove_task_from_array(i);
			}
			i++;
		}
	}
}

/* removes a task from the taskarray */
void remove_task_from_array(gint count)
{
	gint i;
	
	for(i = count; i < task_count; i++)
	{
		all_tasks[i] = all_tasks[i+1];
	}
	task_count--;
}

/* checks if there is a parent task (ppid) */
struct task *get_parent_task(struct task task)
{
	gint i;
	
	for(i = 0; i < task_count; i++)
	{
		if(task.ppid == all_tasks[i].pid && all_tasks[i].pid != task.pid)
			return(&all_tasks[i]);
	}
	
	return(NULL);
}

void show_user_tasks(void)
{
	gint i;
	gint user_id = getuid();
	
	for(i = 0; i < task_count; i++)
	{
		if(all_tasks[i].uid == user_id)
			add_tree_item(all_tasks[i]);
	}
}

void hide_user_tasks(void)
{
	gint i;
	gint user_id = getuid();
	
	for(i = 0; i < task_count; i++)
	{
		if(all_tasks[i].uid == user_id)
			remove_tree_item(all_tasks[i]);
	}
}

void show_root_tasks(void)
{
	gint i;
	
	for(i = 0; i < task_count; i++)
	{
		if(all_tasks[i].uid == 0)
			add_tree_item(all_tasks[i]);
	}
}

void hide_root_tasks(void)
{
	gint i;
	
	for(i = 0; i < task_count; i++)
	{
		if(all_tasks[i].uid == 0)
			remove_tree_item(all_tasks[i]);
	}
}

void show_other_tasks(void)
{
	gint i;
	gint user_id = getuid();
	
	for(i = 0; i < task_count; i++)
	{
		if(all_tasks[i].uid != user_id && all_tasks[i].uid != 0)
			add_tree_item(all_tasks[i]);
	}
}

void hide_other_tasks(void)
{
	gint i;
	gint user_id = getuid();
	
	for(i = 0; i < task_count; i++)
	{
		if(all_tasks[i].uid != user_id && all_tasks[i].uid != 0)
			remove_tree_item(all_tasks[i]);
	}
}

void load_config(void)
{
	gchar * homedir;
	
	homedir = xfce_resource_save_location(XFCE_RESOURCE_CONFIG, "xfce4", FALSE);
	XfceRc *config_obj = xfce_rc_simple_open(g_build_filename(homedir, "taskmanager.rc", NULL), TRUE);
	g_free(homedir);
	
	if(config_obj == NULL)
	{
		config_show_user_tasks = TRUE;
		config_show_root_tasks = FALSE;
		config_show_other_tasks = FALSE;
		printf("Could not open configfile\n");
	}
	else
	{
		config_show_user_tasks = xfce_rc_read_bool_entry(config_obj, "show_user_tasks", TRUE);
		config_show_root_tasks = xfce_rc_read_bool_entry(config_obj, "show_root_tasks", FALSE);
		config_show_other_tasks = xfce_rc_read_bool_entry(config_obj, "show_other_tasks", FALSE);	
	}
}
