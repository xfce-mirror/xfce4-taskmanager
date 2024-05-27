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
#include <string.h>
#include <unistd.h>
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "task-manager.h"

/* Cache some values */
/* Example:
static gushort _cpu_count = 0;
*/

int
get_mac_address (const char *device, uint8_t mac[6])
{
	memset (mac, 0, sizeof (uint8_t) * 6);
	return FALSE;
}

gboolean
get_network_usage (guint64 *tcp_rx, guint64 *tcp_tx, guint64 *tcp_error)
{
	*tcp_rx = 0;
	*tcp_tx = 0;
	*tcp_error = 0;
	return TRUE;
}

#ifdef HAVE_LIBPCAP
void
packet_callback (u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
}
#endif

gboolean
get_memory_usage (guint64 *memory_total, guint64 *memory_available, guint64 *memory_free, guint64 *memory_cache, guint64 *memory_buffers, guint64 *swap_total, guint64 *swap_free)
{
	*memory_total = 0;
	*memory_free = 0;
	*memory_cache = 0;
	*memory_buffers = 0;
	*memory_available = 0;
	*swap_total = 0;
	*swap_free = 0;

	return TRUE;
}

gboolean
get_cpu_usage (gushort *cpu_count, gfloat *cpu_user, gfloat *cpu_system)
{
	*cpu_user = *cpu_system = 0.0f;
	*cpu_count = 0; /*_cpu_count;*/

	return TRUE;
}

static gboolean
get_task_details (GPid pid, Task *task)
{
	memset (task, 0, sizeof (Task));
	g_snprintf (task->name, sizeof (task->name), "foo");
	g_snprintf (task->cmdline, sizeof (task->cmdline), "foo -bar");
	task->uid = 1000;

	return TRUE;
}

gboolean
get_task_list (GArray *task_list)
{
	GPid pid = 0;
	Task task;

	if (get_task_details (pid, &task))
	{
		g_array_append_val (task_list, task);
	}

	g_array_sort (task_list, task_pid_compare_fn);

	return TRUE;
}

gboolean
pid_is_sleeping (GPid pid)
{
	/* Read state of PID @pid... */

	return FALSE; /* (state == sleeping) ? TRUE : FALSE;*/
}
