/*
 * Copyright (c) 2009  Peter Tribble <peter.tribble@gmail.com>
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

#include "taskmanager.h"

static void init_stats()
{
	kc = kstat_open();
}

struct task get_task_details(gint pid)
{
	struct task task;
	gchar filename[255];
	gchar pstate[2];
	int fd;
	struct passwd *passwdp;
	struct psinfo thisproc;

	sprintf(filename, "/proc/%i/psinfo", pid);

	task.pid = pid;
	task.checked = FALSE;

	if ((fd = open(filename, O_RDONLY)) > 0)
	{
	if (read(fd, &thisproc, sizeof(psinfo_t)) == sizeof(psinfo_t))
	{
	g_strlcpy(task.name,thisproc.pr_fname,64);
	sprintf(pstate, "%c", thisproc.pr_lwp.pr_sname);
	g_strlcpy(task.state, pstate, 16);
	task.ppid = (gint) thisproc.pr_ppid;
	task.size = (gint) thisproc.pr_size;
	task.rss = (gint) thisproc.pr_rssize;
	task.uid = (gint) thisproc.pr_uid;
		passwdp = getpwuid(thisproc.pr_uid);
		if(passwdp != NULL && passwdp->pw_name != NULL)
			g_strlcpy(task.uname, passwdp->pw_name, sizeof task.uname);
	/*
	 * To get the appropriate precision we need to multiply up by 10000
	 * so that when we convert to a percentage we can represent 0.01%.
	 */
	task.time = 10000*thisproc.pr_time.tv_sec + thisproc.pr_time.tv_nsec/100000;
	task.old_time = task.time;
	task.time_percentage = 0;
	}
	close(fd);
	}

	return task;
}

GArray *get_task_list(void)
{
	DIR *dir;
	struct dirent *dir_entry;
	GArray *task_list;

	task_list = g_array_new (FALSE, FALSE, sizeof (struct task));
	
	if((dir = opendir("/proc/")) == NULL)
	{
		fprintf(stderr, "Error: couldn't load the /proc directory\n");
		return NULL;
	}

	gint count = 0;
	
	while((dir_entry = readdir(dir)) != NULL)
	{
		if(atoi(dir_entry->d_name) != 0)
		{
			struct task task = get_task_details(atoi(dir_entry->d_name));
			if(task.pid != -1)
				g_array_append_val(task_list, task);
		}
		count++;
	}

	closedir(dir);

	return task_list;
}

gboolean get_cpu_usage_from_proc(system_status *sys_stat)
{
	gboolean retval = FALSE;
	kstat_t *ksp;
	kstat_named_t *knp;

	if (!kc)
	{
		init_stats();
	}

	if ( sys_stat->valid_proc_reading == TRUE ) {
		sys_stat->cpu_old_jiffies =
			sys_stat->cpu_user +
			sys_stat->cpu_nice +
			sys_stat->cpu_system+
			sys_stat->cpu_idle;
		sys_stat->cpu_old_used =
			sys_stat->cpu_user +
			sys_stat->cpu_nice +
			sys_stat->cpu_system;
	} else {
		sys_stat->cpu_old_jiffies = 0;
	}

	sys_stat->valid_proc_reading = FALSE;

	kstat_chain_update(kc);

	for (ksp = kc->kc_chain; ksp != NULL; ksp = ksp->ks_next)
	{
		if (!strcmp(ksp->ks_module, "cpu") && !strcmp(ksp->ks_name, "sys"))
		{
			kstat_read(kc, ksp, NULL);
			knp = kstat_data_lookup(ksp, "cpu_ticks_user");
			sys_stat->cpu_user = knp->value.ui64;
			knp = kstat_data_lookup(ksp, "cpu_ticks_kernel");
			sys_stat->cpu_system = knp->value.ui64;
			knp = kstat_data_lookup(ksp, "cpu_ticks_idle");
			sys_stat->cpu_idle = knp->value.ui64;
			sys_stat->cpu_nice = 0L;
			sys_stat->valid_proc_reading = TRUE;
			retval = TRUE;
		}
	}
        return retval;
}

gboolean get_system_status (system_status *sys_stat)
{
	kstat_t *ksp;
	kstat_named_t *knp;

	if (!kc)
	{
		init_stats();
	}

	if (ksp = kstat_lookup(kc, "unix", 0, "system_pages"))
	{
		kstat_read(kc, ksp, NULL);
		knp = kstat_data_lookup(ksp, "physmem");
		sys_stat->mem_total = (getpagesize()*knp->value.ui64)/1024;
		knp = kstat_data_lookup(ksp, "freemem");
		sys_stat->mem_free = (getpagesize()*knp->value.ui64)/1024;
		sys_stat->mem_cached = 0;
	}
	
	sys_stat->cpu_count = 0;
	kstat_chain_update(kc);

	for (ksp = kc->kc_chain; ksp != NULL; ksp = ksp->ks_next)
	{
		if (!strcmp(ksp->ks_module, "cpu_info"))
		{
			sys_stat->cpu_count++;
		}
	}
	
	return TRUE;
}

void send_signal_to_task(gint task_id, gint signal)
{
	if(task_id > 0 && signal != 0)
	{
		gint ret = 0;
		
		ret = kill(task_id, signal);

		if(ret != 0)
			xfce_err(_("Couldn't send signal to the task with ID %d"), signal, task_id);
	}
}


void set_priority_to_task(gint task_id, gint prio)
{
	if(task_id > 0)
	{
		gchar command[128] = "";
		g_sprintf(command, "renice %d %d > /dev/null", prio, task_id);
		
		if(system(command) != 0)
			xfce_err(_("Couldn't set priority %d to the task with ID %d"), prio, task_id);
	}
}

