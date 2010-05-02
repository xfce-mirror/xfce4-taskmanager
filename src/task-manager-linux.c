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

#include <glib.h>

#include "task-manager.h"

gboolean
get_memory_usage (guint64 *memory_total, guint64 *memory_free, guint64 *memory_cache, guint64 *memory_buffers, guint64 *swap_total, guint64 *swap_free)
{
	FILE *file;
	gchar buffer[1024];
	gchar *filename = "/proc/meminfo";
	gushort found = 0;

	if ((file = fopen (filename, "r")) == NULL)
	{
		return FALSE;
	}

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
	gushort count = 0;

	if ((file = fopen (filename, "r")) == NULL)
	{
		return FALSE;
	}

	fgets (buffer, 1024, file);
	sscanf (buffer, "cpu\t%u %u %u %u", &user, &user_nice, &system, &idle);

	while (fgets (buffer, 1024, file) != NULL)
	{
		if (buffer[0] != 'c' && buffer[1] != 'p' && buffer[2] != 'u')
			break;
		count += 1;
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
	*cpu_count = (count != 0) ? count : 1;

	return TRUE;
}

