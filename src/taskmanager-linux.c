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

#include <dirent.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#include "taskmanager.h"

static void get_cmdline(gint pid, gchar *cmdline, gint length, gchar *cmdline_full, gint length_full)
{
	FILE *file;
	char filename[255];
	char *p;
	int c;
	int i = 0;
	char buffer[4096];
	int idummy;

	snprintf (filename, 255, "/proc/%i/cmdline", pid);
	file = fopen (filename, "r");
	if (file == NULL) {
		return;
	}

	/* read byte per byte until EOF */
	while (EOF != (c = fgetc (file))) {
		if (c != 0) {
			cmdline_full[i++] = c;
		} else {
			cmdline_full[i++] = ' ';
		}

		if (i == length_full - 1) {
			break;
		}
	}
	if (cmdline_full[i-1] == ' ') {
		cmdline_full[i-1] = '\0';
	} else {
		cmdline_full[i] = '\0';
	}

	fclose (file);

	/* daemon processes and kernel processes don't have a cmdline */
	if (i == 0) {
		/* read from /proc/pid/stat and enclose with brakets */
		snprintf (filename, 255, "/proc/%i/stat", pid);
		file = fopen (filename, "r");
		if (file == NULL) {
			return;
		}

		fgets (buffer, sizeof (buffer), file);
		fclose (file);

		cmdline_full[0] = '[';
		sscanf (buffer, "%i (%252s", &idummy, &cmdline_full[1]);

		if (NULL != (strrchr (cmdline_full, ')'))) {
			*strrchr (cmdline_full, ')') = '\0';
		}

		i = strlen (cmdline_full);
		cmdline_full[i] = ']';
		cmdline_full[i+1] = '\0';

		strncpy (cmdline, cmdline_full, length);
		return;
	}

	/* get the short version */
	snprintf (filename, 255, "/proc/%i/cmdline", pid);
	file = fopen (filename, "r");
	fgets (cmdline, length, file);
	fclose (file);

	p = strchr (cmdline, ':');
	if (NULL != p) {
		*p = '\0';
	} else {
		p = strrchr (cmdline, '/');
		if (NULL != p) {
			strncpy (cmdline, p+1, length);
		}
	}

	if (cmdline[0] == '-') {
		for (i = 0; cmdline[i] != '\0'; i++) {
			cmdline[i] = cmdline[i+1];
		}
	}
}

static struct task get_task_details(gint pid)
{
	FILE		*task_file;
	FILE		*cmdline_file;
	gchar		dummy[255];
	gint		idummy;
	gchar		buffer_status[1024];
	struct task	task;
	struct passwd	*passwdp;
	struct stat	status;
	gchar		filename[255];
	gchar		cmdline_filename[255];
	static gint	pagesize = 0;

	gint utime = 0;
	gint stime = 0;

	sprintf(filename, "/proc/%i/stat", pid);
	sprintf(cmdline_filename, "/proc/%i/cmdline", pid);

	task.pid = -1;
	task.checked = FALSE;

	stat(filename, &status);

	if (pagesize == 0)
	  {
		pagesize = sysconf(_SC_PAGESIZE);
		if (pagesize == 0)
			pagesize = 4*1024;
	  }

	if(NULL == (task_file = fopen(filename,"r")))
		return task;

	fgets(buffer_status, sizeof(buffer_status), task_file);
	fclose(task_file);

	sscanf(buffer_status, "%i (%255s %1s %i %i %i %i %i %255s %255s %255s %255s %255s %i %i %i %i %i %i %i %i %i %i %i %255s %255s %255s %i %255s %255s %255s %255s %255s %255s %255s %255s %255s %255s %i %255s %255s",
			&task.pid,	// processid
			dummy,		// processname
			task.state,	// processstate
			&task.ppid,	// parentid
			&idummy,	// processs groupid

			&idummy,	// session id
			&idummy,	// tty id
			&idummy,	// tpgid: The process group ID of the process running on tty of the process
			dummy,		// flags
			dummy,		// minflt minor faults the process has maid

			dummy,		// cminflt
			dummy,		// majflt
			dummy,		// cmajflt
			&utime,		// utime the number of jiffies that this process has scheduled in user mode
			&stime,		// stime " kernel mode

			&idummy,	// cutime " waited for children in user
			&idummy,	// cstime " kernel mode
			&idummy,	// priority (nice value + fifteen)
			&task.prio,	// nice range from 19 to -19	/* my change */
			&idummy,	// hardcoded 0

			&idummy,	// itrealvalue time in jiffies to next SIGALRM send to this process
			&idummy,	// starttime jiffies the process startet after system boot
			&task.vsize,	// vsize in bytes
			&task.rss,	// rss (number of pages in real memory)
			dummy,		// rlim limit in bytes for rss

			dummy,		// startcode
			dummy,		// endcode
			&idummy,	// startstack
			dummy,		// kstkesp value of esp (stack pointer)
			dummy, 		// kstkeip value of EIP (instruction pointer)

			dummy,		// signal. bitmap of pending signals
			dummy,		// blocked: bitmap of blocked signals
			dummy,		// sigignore: bitmap of ignored signals
			dummy,		// sigcatch: bitmap of catched signals
			dummy,		// wchan

			dummy,		// nswap
			dummy,		// cnswap
			dummy,		// exit_signal
			&idummy,	// CPU number last executed on
			dummy,

			dummy
		);

	task.old_time = task.time;
	task.time = stime + utime;
	task.time_percentage = 0;
	task.rss *= pagesize;

	task.uid = status.st_uid;
	passwdp = getpwuid(task.uid);
	if(passwdp != NULL && passwdp->pw_name != NULL)
		g_strlcpy(task.uname, passwdp->pw_name, sizeof(task.uname));

	get_cmdline(pid, task.name, sizeof(task.name), task.fullname, sizeof(task.fullname));

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
	const gchar *file_name = "/proc/stat";
	gchar buffer[100];
	gboolean retval = FALSE;
	FILE *file;

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

	if (!g_file_test (file_name, G_FILE_TEST_EXISTS))
	{
		return FALSE;
	}


	file = fopen (file_name, "r");

	if (file)
	{
		if ( fgets (buffer, 100, file) != NULL )
		{
			if ( sscanf (buffer, "cpu\t%u %u %u %u",
				     &sys_stat->cpu_user,
				     &sys_stat->cpu_nice,
				     &sys_stat->cpu_system,
				     &sys_stat->cpu_idle
				    ) == 4 )
			{
				sys_stat->valid_proc_reading = TRUE;
				retval = TRUE;
			}
		}
		fclose( file );
	}
        return retval;
}

gboolean get_system_status (system_status *sys_stat)
{
	FILE *file;
	gchar *file_name;
	gchar *buffer;

	buffer = g_new (gchar, 100);

	file_name = g_strdup ("/proc/meminfo");

	if (!g_file_test (file_name, G_FILE_TEST_EXISTS))
	{
		g_free(file_name);
		return FALSE;
	}

	file = fopen (file_name, "r");

	if (file)
	{
		while (fgets (buffer, 100, file) != NULL)
		{
			sscanf (buffer, "MemTotal:\t%u kB", &sys_stat->mem_total);
			sscanf (buffer, "MemFree:\t%u kB", &sys_stat->mem_free);
			sscanf (buffer, "Cached:\t%u kB", &sys_stat->mem_cached);
			sscanf (buffer, "Buffers:\t%u kB", &sys_stat->mem_buffers);
		}
		fclose (file);
	}
	g_free (buffer);
	g_free (file_name);

	buffer = g_new (gchar, 100);

	file_name = g_strdup ("/proc/cpuinfo");

	if (!g_file_test (file_name, G_FILE_TEST_EXISTS))
	{
		g_free(file_name);
		return FALSE;
	}

	file = fopen (file_name, "r");

	sys_stat->cpu_count = -1;

	if (file)
	{

		while (fgets (buffer, 100, file) != NULL)
		{
			sscanf (buffer, "processor : %i", &sys_stat->cpu_count);
		}
		fclose (file);
		sys_stat->cpu_count++;
	}
	g_free (buffer);
	g_free (file_name);

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

