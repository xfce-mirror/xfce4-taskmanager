/*
 * Copyright (c) 2008-2010  Mike Massonnet <mmassonnet@xfce.org>
 * Copyright (c) 2006  Johannes Zellner <webmaster@nebulon.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>
#include <string.h>

#include <glib.h>

#include "task-manager.h"

static gushort _cpu_count = 0;

gboolean
get_memory_usage (guint64 *memory_total, guint64 *memory_free, guint64 *memory_cache, guint64 *memory_buffers, guint64 *swap_total, guint64 *swap_free)
{
	FILE *file;
	gchar buffer[1024];
	gchar *filename = "/proc/meminfo";
	gushort found = 0;

	if ((file = fopen (filename, "r")) == NULL)
		return FALSE;

	while (found < 6 && fgets (buffer, 1024, file) != NULL)
	{
		found += sscanf (buffer, "MemTotal:\t%u kB", memory_total);
		found += sscanf (buffer, "MemFree:\t%u kB", memory_free);
		found += sscanf (buffer, "Cached:\t%u kB", memory_cache);
		found += sscanf (buffer, "Buffers:\t%u kB", memory_buffers);
		found += sscanf (buffer, "SwapTotal:\t%u kB", swap_total);
		found += sscanf (buffer, "SwapFree:\t%u kB", swap_free);
	}
	fclose (file);

	*memory_total *= 1024;
	*memory_free *= 1024;
	*memory_cache *= 1024;
	*memory_buffers *= 1024;
	*swap_total *= 1024;
	*swap_free *= 1024;

	return TRUE;
}

gboolean
get_cpu_usage (gushort *cpu_count, gfloat *cpu_user, gfloat *cpu_system)
{
	FILE *file;
	gchar *filename = "/proc/stat";
	gchar buffer[1024];
	static gulong cur_jiffies_user = 0, old_jiffies_user = 0;
	static gulong cur_jiffies_system = 0, old_jiffies_system = 0;
	static gulong cur_jiffies = 0, old_jiffies = 0;
	gulong user, user_nice, system, idle;

	if ((file = fopen (filename, "r")) == NULL)
		return FALSE;

	fgets (buffer, 1024, file);
	sscanf (buffer, "cpu\t%u %u %u %u", &user, &user_nice, &system, &idle);

	_cpu_count = 0;
	while (fgets (buffer, 1024, file) != NULL)
	{
		if (buffer[0] != 'c' && buffer[1] != 'p' && buffer[2] != 'u')
			break;
		_cpu_count += 1;
	}
	fclose (file);

	old_jiffies_user = cur_jiffies_user;
	old_jiffies_system = cur_jiffies_system;
	old_jiffies = cur_jiffies;

	cur_jiffies_user = user + user_nice;
	cur_jiffies_system = system;
	cur_jiffies = cur_jiffies_user + cur_jiffies_system + idle;

	*cpu_user = (old_jiffies > 0) ? (cur_jiffies_user - old_jiffies_user) * 100 / (gdouble)(cur_jiffies - old_jiffies) : 0;
	*cpu_system = (old_jiffies > 0) ? (cur_jiffies_system - old_jiffies_system) * 100 / (gdouble)(cur_jiffies - old_jiffies) : 0;
	*cpu_count = (_cpu_count > 0) ? _cpu_count : 1;
	_cpu_count = *cpu_count;

	return TRUE;
}

static inline int get_pagesize ()
{
	static int pagesize = 0;
	if (pagesize == 0)
	{
		pagesize = sysconf (_SC_PAGESIZE);
		if (pagesize == 0)
			pagesize = 4096;
	}
	return pagesize;
}

static void
get_task_cmdline (Task *task)
{
	FILE *file;
	gchar filename[96];
	gint i;
	gchar c;

	snprintf (filename, 96, "/proc/%i/cmdline", task->pid);
	if ((file = fopen (filename, "r")) == NULL)
		return;

	/* Drop parentheses around task->name */
	// FIXME comm concats the name to 15 chars
	{
		gchar *p;
		g_strlcpy (task->name, &task->name[1], sizeof (task->name));
		p = g_strrstr (task->name, ")");
		*p = '\0';
	}

	/* Read byte per byte until EOF */
	for (i = 0; (c = fgetc (file)) != EOF && i < sizeof (task->cmdline) - 1; i++)
		task->cmdline[i] = (c == '\0') ? ' ' : c;
	if (task->cmdline[i-1] == ' ')
		task->cmdline[i-1] = '\0';
	fclose (file);

	/* Kernel processes don't have a cmdline nor an exec path */
	if (i == 0)
	{
		size_t len = strlen (task->name);
		g_strlcpy (&task->cmdline[1], task->name, len + 1);
		task->cmdline[0] = '[';
		task->cmdline[len+1] = ']';
		task->cmdline[len+2] = '\0';
	}
}

static void
get_cpu_percent (guint pid, gulong jiffies_user, gfloat *cpu_user, gulong jiffies_system, gfloat *cpu_system)
{
	static GHashTable *hash_cpu_user = NULL;
	static GHashTable *hash_cpu_system = NULL;
	gulong jiffies_user_old, jiffies_system_old;

	if (hash_cpu_user == NULL)
	{
		hash_cpu_user = g_hash_table_new (NULL, NULL);
		hash_cpu_system = g_hash_table_new (NULL, NULL);
	}

	jiffies_user_old = GPOINTER_TO_UINT (g_hash_table_lookup (hash_cpu_user, GUINT_TO_POINTER (pid)));
	jiffies_system_old = GPOINTER_TO_UINT (g_hash_table_lookup (hash_cpu_system, GUINT_TO_POINTER (pid)));
	g_hash_table_insert (hash_cpu_user, GUINT_TO_POINTER (pid), GUINT_TO_POINTER (jiffies_user));
	g_hash_table_insert (hash_cpu_system, GUINT_TO_POINTER (pid), GUINT_TO_POINTER (jiffies_system));

	if (_cpu_count > 0)
	{
		*cpu_user = (jiffies_user_old > 0) ? (jiffies_user - jiffies_user_old) / (gfloat)_cpu_count : 0;
		*cpu_system = (jiffies_system_old > 0) ? (jiffies_system - jiffies_system_old) / (gfloat)_cpu_count : 0;
	}
	else
	{
		*cpu_user = *cpu_system = 0;
	}
}

static gboolean
get_task_details (guint pid, Task *task)
{
	FILE *file;
	gchar filename[96];
	gchar buffer[1024];

	snprintf (filename, 96, "/proc/%d/stat", pid);
	if ((file = fopen (filename, "r")) == NULL)
		return FALSE;

	fgets (buffer, 1024, file);
	fclose (file);

	{
		gchar dummy[255];
		gint idummy;
		gulong jiffies_user, jiffies_system;
		struct passwd *pw;
		struct stat sstat;

		sscanf(buffer, "%i %255s %1s %i %i %i %i %i %255s %255s %255s %255s %255s %i %i %i %i %i %i %i %i %i %i %i %255s %255s %255s %i %255s %255s %255s %255s %255s %255s %255s %255s %255s %255s %i %255s %255s",
			&task->pid,	// processid
			task->name,	// processname
			task->state,	// processstate
			&task->ppid,	// parentid
			 &idummy,	// processs groupid

			 &idummy,	// session id
			 &idummy,	// tty id
			 &idummy,	// tpgid the process group ID of the process running on tty of the process
			 dummy,		// flags
			 dummy,		// minflt minor faults the process has maid

			 dummy,		// cminflt
			 dummy,		// majflt
			 dummy,		// cmajflt
			&jiffies_user,	// utime the number of jiffies that this process has scheduled in user mode
			&jiffies_system,// stime " system mode

			 &idummy,	// cutime " waited for children in user mode
			 &idummy,	// cstime " system mode
			 &idummy,	// priority (nice value + fifteen)
			&task->prio,	// nice range from 19 to -19
			 &idummy,	// hardcoded 0

			 &idummy,	// itrealvalue time in jiffies to next SIGALRM send to this process
			 &idummy,	// starttime jiffies the process startet after system boot
			&task->vsz,	// vsize in bytes
			&task->rss,	// rss (number of pages in real memory)
			 dummy,		// rlim limit in bytes for rss

			 dummy,		// startcode
			 dummy,		// endcode
			 &idummy,	// startstack
			 dummy,		// kstkesp value of esp (stack pointer)
			 dummy,		// kstkeip value of EIP (instruction pointer)

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

		task->rss *= get_pagesize ();
		get_cpu_percent (task->pid, jiffies_user, &task->cpu_user, jiffies_system, &task->cpu_system);

		stat (filename, &sstat);
		pw = getpwuid (sstat.st_uid);
		task->uid = sstat.st_uid;
		g_strlcpy (task->uid_name, (pw != NULL) ? pw->pw_name : "nobody", sizeof (task->uid_name));
	}

	get_task_cmdline (task);

	return TRUE;
}

gboolean
get_task_list (GArray *task_list)
{
	GDir *dir;
	const gchar *name;
	guint pid;
	Task task = { 0 };

	if ((dir = g_dir_open ("/proc", 0, NULL)) == NULL)
		return FALSE;

	while ((name = g_dir_read_name(dir)) != NULL)
	{
		if ((pid = (guint)g_ascii_strtoull (name, NULL, 0)) > 0)
		{
			if (get_task_details (pid, &task))
			{
				g_array_append_val (task_list, task);
			}
		}
	}

	g_dir_close (dir);

	return TRUE;
}

