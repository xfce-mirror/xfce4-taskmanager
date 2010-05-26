/*
 * Copyright (c) <YEAR>  <AUTHOR> <EMAIL>
 *
 * <LICENCE, BELOW IS GPL2+ AS EXAMPLE>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/* Add includes for system functions needed */
/* Example:
#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <string.h>
*/

#include <glib.h>

#include "task-manager.h"

/* Cache some values */
/* Example:
static gushort _cpu_count = 0;
*/

gboolean
get_memory_usage (guint64 *memory_total, guint64 *memory_free, guint64 *memory_cache, guint64 *memory_buffers, guint64 *swap_total, guint64 *swap_free)
{
	*memory_total = 0;
	*memory_free = 0;
	*memory_cache = 0;
	*memory_buffers = 0;
	*swap_total = 0;
	*swap_free = 0;

	return TRUE;
}

gboolean
get_cpu_usage (gushort *cpu_count, gfloat *cpu_user, gfloat *cpu_system)
{
	*cpu_user = *cpu_system = 0.0;
	*cpu_count = 0; /*_cpu_count;*/

	return TRUE;
}

static gboolean
get_task_details (guint pid, Task *task)
{
	g_snprintf (task->name, 256, "foo");
	g_snprintf (task->cmdline, 1024, "foo -bar");
	g_snprintf (task->uid_name, 256, "baz");

	return TRUE;
}

gboolean
get_task_list (GArray *task_list)
{
	guint pid;
	Task task = { 0 };

	//while (/* read all PIDs */)
	{
		// if (/* pid is valid */)
		{
			if (get_task_details (pid, &task))
			{
				g_array_append_val (task_list, task);
			}
		}
	}

	return TRUE;
}

gboolean
pid_is_sleeping (guint pid)
{
	/* Read state of PID @pid... */

	return FALSE; /* (state == sleeping) ? TRUE : FALSE;*/
}

