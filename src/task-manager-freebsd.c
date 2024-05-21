/*
 * Copyright (c) 2010  Mike Massonnet <mmassonnet@xfce.org>
 * Copyright (c) 2006  Oliver Lehmann <oliver@FreeBSD.org>
 * Copyright (c) 2018 Rozhuk Ivan <rozhuk.im@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "task-manager.h"

// clang-format off
#include <fcntl.h>
#include <kvm.h>
#include <paths.h>
#include <string.h>

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/user.h>

#if defined(__FreeBSD_version) && __FreeBSD_version >= 900044
#include <sys/vmmeter.h>
#endif

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_mib.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
// clang-format on

#include <glib.h>

#include "inode-to-sock.h"
#include "network-analyzer.h"
#include "task-manager.h"

static const gchar ki_stat2state[] = {
	' ', /* - */
	'R', /* SIDL */
	'R', /* SRUN */
	'S', /* SSLEEP */
	'T', /* SSTOP */
	'Z', /* SZOMB */
	'W', /* SWAIT */
	'L' /* SLOCK */
};

static XtmInodeToSock *inode_to_sock = NULL;
static XtmNetworkAnalyzer *analyzer = NULL;

void list_process_fds (Task *task);
gboolean get_if_count (int *data);
gboolean get_network_usage_if (int interface, guint64 *tcp_rx, guint64 *tcp_tx, guint64 *tcp_error);

#ifdef HAVE_LIBPCAP
void
packet_callback (u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
	// Extract source and destination IP addresses and ports from the packet
	struct ether_header *eth_header = (struct ether_header *)packet;
	struct ip *ip_header = (struct ip *)(packet + sizeof (struct ether_header));
	struct tcphdr *tcp_header = (struct tcphdr *)(packet + sizeof (struct ether_header) + sizeof (struct ip));
	XtmNetworkAnalyzer *iface = (XtmNetworkAnalyzer*)args;

	// Dropped non-ip packet
	if (eth_header->ether_type != 8 || ip_header->ip_p != 6)
		return;

	long int src_port = ntohs (tcp_header->th_sport);
	long int dst_port = ntohs (tcp_header->th_dport);

	// directly use strcmp on analyzer->mac, eth_header->ether_shost doesnt work

	char local_mac[18];
	char src_mac[18];
	char dst_mac[18];

	sprintf (local_mac,
		"%02X:%02X:%02X:%02X:%02X:%02X",
		iface->mac[0], iface->mac[1],
		iface->mac[2], iface->mac[3],
		iface->mac[4], iface->mac[5]);

	sprintf (src_mac,
		"%02X:%02X:%02X:%02X:%02X:%02X",
		eth_header->ether_shost[0], eth_header->ether_shost[1],
		eth_header->ether_shost[2], eth_header->ether_shost[3],
		eth_header->ether_shost[4], eth_header->ether_shost[5]);

	sprintf (dst_mac,
		"%02X:%02X:%02X:%02X:%02X:%02X",
		eth_header->ether_dhost[0], eth_header->ether_dhost[1],
		eth_header->ether_dhost[2], eth_header->ether_dhost[3],
		eth_header->ether_dhost[4], eth_header->ether_dhost[5]);

	// Debug
	// pthread_mutex_lock(&iface->lock);

	if (strcmp (local_mac, src_mac) == 0)
		increament_packet_count (local_mac, "in ", iface->packetin, src_port);

	if (strcmp (local_mac, dst_mac) == 0)
		increament_packet_count (local_mac, "out", iface->packetout, dst_port);

	// pthread_mutex_unlock(&iface->lock);
}
#endif


gboolean
get_if_count (int *data)
{
	size_t len = sizeof (*data);

	static int32_t name[] = { CTL_NET, PF_LINK, NETLINK_GENERIC, IFMIB_SYSTEM, IFMIB_IFCOUNT };
	name[0] = CTL_NET;
	name[1] = PF_LINK;
	name[2] = NETLINK_GENERIC;
	name[3] = IFMIB_SYSTEM;
	name[4] = IFMIB_IFCOUNT;

	return sysctl (name, 5, data, &len, 0, 0) < 0;
}

gboolean
get_network_usage_if (int interface, guint64 *tcp_rx, guint64 *tcp_tx, guint64 *tcp_error)
{
	struct ifmibdata data;
	size_t len = sizeof (data);

	static int32_t name[] = { CTL_NET, PF_LINK, NETLINK_GENERIC, IFMIB_IFDATA, 0, IFDATA_GENERAL };
	name[4] = interface;

	if (sysctl (name, 6, &data, &len, 0, 0) < 0)
		return 1;

	//*tcp_error = data.ifmd_data.ifi_oerrors + data.ifmd_data.ifi_ierrors;
	*tcp_rx += data.ifmd_data.ifi_ibytes;
	*tcp_tx += data.ifmd_data.ifi_obytes;

	return 0;
}

gboolean
get_network_usage (guint64 *tcp_rx, guint64 *tcp_tx, guint64 *tcp_error)
{
	int ifcount = 0;

	if (get_if_count (&ifcount))
		return 1;

	*tcp_error = 0;
	*tcp_rx = 0;
	*tcp_tx = 0;

	for (int i = 0; i < ifcount; ++i)
		get_network_usage_if (i, tcp_rx, tcp_tx, tcp_error);

	return 0;
}

int
get_mac_address (const char *device, uint8_t mac[6])
{
	struct ifaddrs *ifaddr, *ifa;

	if (getifaddrs (&ifaddr) == -1)
		return -1;

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == NULL)
			continue;

		int family = ifa->ifa_addr->sa_family;

		// Check if the interface is a network interface (AF_PACKET for Linux)
		if (family == AF_LINK && strcmp (device, ifa->ifa_name) == 0)
		{
			// Check if the interface has a hardware address (MAC address)
			if (ifa->ifa_data != NULL)
			{
				struct sockaddr_dl *sdl = (struct sockaddr_dl *)ifa->ifa_addr;
				memcpy (mac, sdl->sdl_data + sdl->sdl_nlen, sizeof (uint8_t) * 6);
				freeifaddrs (ifaddr);
				return 0;
			}
		}
	}

	freeifaddrs (ifaddr);
	return -1;
}

static guint64
get_mem_by_bytes (const gchar *name)
{
	guint64 buf = 0;
	gsize len = sizeof (buf);

	if (sysctlbyname (name, &buf, &len, NULL, 0) < 0)
		return 0;

	return buf;
}

static guint64
get_mem_by_pages (const gchar *name)
{
	guint64 res;

	res = get_mem_by_bytes (name);
	if (res > 0)
		res *= (guint64)getpagesize ();

	return res;
}

gboolean
get_memory_usage (guint64 *memory_total, guint64 *memory_available, guint64 *memory_free, guint64 *memory_cache, guint64 *memory_buffers, guint64 *swap_total, guint64 *swap_free)
{
	/* Get memory usage */
	{
		*memory_total = get_mem_by_bytes ("hw.physmem");
		*memory_free = get_mem_by_pages ("vm.stats.vm.v_free_count");
		*memory_cache = get_mem_by_pages ("vm.stats.vm.v_inactive_count");
		*memory_buffers = get_mem_by_bytes ("vfs.bufspace");
		*memory_available = *memory_free + *memory_cache + *memory_buffers;
	}

	/* Get swap usage */
	{
		kvm_t *kd;
		struct kvm_swap kswap;

		if ((kd = kvm_openfiles (_PATH_DEVNULL, _PATH_DEVNULL, NULL, O_RDONLY, NULL)) == NULL)
			return FALSE;

		kvm_getswapinfo (kd, &kswap, 1, 0);
		*swap_total = (guint64)kswap.ksw_total * (guint64)getpagesize ();
		*swap_free = ((guint64)(kswap.ksw_total - kswap.ksw_used)) * (guint64)getpagesize ();

		kvm_close (kd);
	}

	return TRUE;
}

gboolean
get_cpu_usage (gushort *cpu_count, gfloat *cpu_user, gfloat *cpu_system)
{
	/* Get CPU count */
	{
		size_t len = sizeof (*cpu_count);
		sysctlbyname ("hw.ncpu", cpu_count, &len, NULL, 0);
	}

	/* Get CPU usage */
	{
		static gulong cp_user = 0, cp_system = 0, cp_total = 0;
		static gulong cp_user_old = 0, cp_system_old = 0, cp_total_old = 0;

		gulong cpu_state[CPUSTATES] = { 0 };
		size_t len = sizeof (cpu_state);
		sysctlbyname ("kern.cp_time", &cpu_state, &len, NULL, 0);

		cp_user_old = cp_user;
		cp_system_old = cp_system;
		cp_total_old = cp_total;
		cp_user = cpu_state[CP_USER] + cpu_state[CP_NICE];
		cp_system = cpu_state[CP_SYS] + cpu_state[CP_INTR];
		cp_total = cpu_state[CP_IDLE] + cp_user + cp_system;

		*cpu_user = *cpu_system = 0.0f;
		if (cp_total > cp_total_old)
		{
			*cpu_user = (((cp_user - cp_user_old) * 100.0f) / (float)(cp_total - cp_total_old));
			*cpu_system = (((cp_system - cp_system_old) * 100.0f) / (float)(cp_total - cp_total_old));
		}
	}

	return TRUE;
}

// TODO check the OpenBSD version here
// (struct sockaddr_in*)kinfo_file.kf_sa_local

void
xtm_refresh_inode_to_sock (XtmInodeToSock *its)
{
	FILE *fp;
	char line[2048];
	char *token;
	char *delim = " ";

	if (analyzer == NULL)
		return;

	// Execute the sockstat command and get the output
	fp = popen ("sockstat -L -P tcp,udp", "r");
	if (fp == NULL)
		return;

	g_hash_table_remove_all (its->hash);
	g_hash_table_remove_all (its->pid);

	// Skip the header line
	fgets (line, 2048, fp);

	// Parse the output line by line
	while (fgets (line, 2048, fp) != NULL)
	{
		// Remove the newline character
		line[strcspn (line, "\n")] = '\0';

		// Split the line into tokens
		token = strtok (line, delim);

		// Parse the columns
		char user[50];
		char command[50];
		char pid[10];
		char fd[10];
		char proto[10];
		char local_address[50];
		char foreign_address[50];

		strcpy (user, token);
		token = strtok (NULL, delim);
		strcpy (command, token);
		token = strtok (NULL, delim);
		strcpy (pid, token);
		token = strtok (NULL, delim);
		strcpy (fd, token);
		token = strtok (NULL, delim);
		strcpy (proto, token);
		token = strtok (NULL, delim);
		strcpy (local_address, token);
		token = strtok (NULL, delim);
		strcpy (foreign_address, token);

		int local_port, assos;
		int *inode1 = g_new0 (gint, 1);
		int *inode2 = g_new0 (gint, 1);

		*inode1 = atoi (fd);
		*inode2 = atoi (fd);
		assos = atoi (pid);

		sscanf (local_address, "%*[^:]:%d", &local_port);
		// (intptr_t) -> cast to pointer from integer of different size [-Wint-to-pointer-cast]
		g_hash_table_replace (its->hash, inode1, (gpointer)(intptr_t)local_port);
		g_hash_table_replace (its->pid, inode2, (gpointer)(intptr_t)assos);

		// Print the parsed values
		// printf("%d -> %d\n", *inode, local_port);
	}

	// Close the pipe
	pclose (fp);
}

void
list_process_fds (Task *task)
{
	XtmNetworkAnalyzer *current;
	GHashTableIter iter;
	gpointer key, value;
	gint key_int, value_int;
	long int port;

	g_hash_table_iter_init (&iter, inode_to_sock->pid);
	while (g_hash_table_iter_next (&iter, &key, &value))
	{
		key_int = *(int *)(key);
		value_int = GPOINTER_TO_INT (value);
		if (task->pid == value_int)
		{
			port = (long int)g_hash_table_lookup (inode_to_sock->hash, &key_int);
			task->active_socket += 1;

			current = analyzer;
			while(current)
			{
				task->packet_in += (guint64)g_hash_table_lookup (current->packetin, &port);
				task->packet_out += (guint64)g_hash_table_lookup (current->packetout, &port);
				current = current->next;
			}
		}
	}
}

static gboolean
get_task_details (struct kinfo_proc *kp, Task *task)
{
	char buf[1024], *p;
	size_t bufsz;
	int i, oid[4];

	memset (task, 0, sizeof (Task));
	task->pid = kp->ki_pid;
	task->ppid = kp->ki_ppid;
	task->cpu_user = 100.0f * ((float)kp->ki_pctcpu / FSCALE);
	task->cpu_system = 0.0f;
	task->vsz = kp->ki_size;
	task->rss = ((guint64)kp->ki_rssize * (guint64)getpagesize ());
	task->uid = kp->ki_uid;
	task->prio = kp->ki_nice;
	g_strlcpy (task->name, kp->ki_comm, sizeof (task->name));

	oid[0] = CTL_KERN;
	oid[1] = KERN_PROC;
	oid[2] = KERN_PROC_ARGS;
	oid[3] = kp->ki_pid;
	bufsz = sizeof (buf);
	memset (buf, 0, sizeof (buf));
	if (sysctl (oid, 4, buf, &bufsz, 0, 0) == -1)
	{
		/*
		 * If the supplied buf is too short to hold the requested
		 * value the sysctl returns with ENOMEM. The buf is filled
		 * with the truncated value and the returned bufsz is equal
		 * to the requested len.
		 */
		if (errno != ENOMEM || bufsz != sizeof (buf))
		{
			bufsz = 0;
		}
		else
		{
			buf[(bufsz - 1)] = 0;
		}
	}

	if (0 != bufsz)
	{
		p = buf;
		do
		{
			g_strlcat (task->cmdline, p, sizeof (task->cmdline));
			g_strlcat (task->cmdline, " ", sizeof (task->cmdline));
			p += (strlen (p) + 1);
		} while (p < buf + bufsz);
	}
	else
	{
		g_strlcpy (task->cmdline, kp->ki_comm, sizeof (task->cmdline));
	}

	i = 0;
	switch (kp->ki_stat)
	{
		case SIDL:
		case SRUN:
		case SSTOP:
		case SZOMB:
		case SWAIT:
		case SLOCK:
			task->state[i] = ki_stat2state[(size_t)kp->ki_stat];
			break;

		case SSLEEP:
			if (kp->ki_tdflags & TDF_SINTR)
				task->state[i] = kp->ki_slptime >= MAXSLP ? 'I' : 'S';
			else
				task->state[i] = 'D';
			break;

		default:
			task->state[i] = '?';
	}
	i++;
	if (!(kp->ki_sflag & PS_INMEM))
		task->state[i++] = 'W';
	if (kp->ki_nice < NZERO)
		task->state[i++] = '<';
	else if (kp->ki_nice > NZERO)
		task->state[i++] = 'N';
	if (kp->ki_flag & P_TRACED)
		task->state[i++] = 'X';
	if (kp->ki_flag & P_WEXIT && kp->ki_stat != SZOMB)
		task->state[i++] = 'E';
	if (kp->ki_flag & P_PPWAIT)
		task->state[i++] = 'V';
	if ((kp->ki_flag & P_SYSTEM) || kp->ki_lock > 0)
		task->state[i++] = 'L';
	if (kp->ki_kiflag & KI_SLEADER)
		task->state[i++] = 's';
	if ((kp->ki_flag & P_CONTROLT) && kp->ki_pgid == kp->ki_tpgid)
		task->state[i++] = '+';
	if (kp->ki_flag & P_JAILED)
		task->state[i++] = 'J';

	if (analyzer != NULL)
		list_process_fds (task);

	return TRUE;
}

gboolean
get_task_list (GArray *task_list)
{
	analyzer = xtm_network_analyzer_get_default ();

	kvm_t *kd;
	struct kinfo_proc *kp;
	int cnt = 0, i;
	Task task;

	inode_to_sock = xtm_inode_to_sock_get_default ();
	xtm_refresh_inode_to_sock (inode_to_sock);

	if ((kd = kvm_openfiles (_PATH_DEVNULL, _PATH_DEVNULL, NULL, O_RDONLY, NULL)) == NULL)
		return FALSE;

	if ((kp = kvm_getprocs (kd, KERN_PROC_PROC, 0, &cnt)) == NULL)
	{
		kvm_close (kd);
		return FALSE;
	}

	for (i = 0; i < cnt; kp++, i++)
	{
		get_task_details (kp, &task);
		g_array_append_val (task_list, task);
	}

	kvm_close (kd);

	g_array_sort (task_list, task_pid_compare_fn);

	return TRUE;
}

gboolean
pid_is_sleeping (GPid pid)
{
	kvm_t *kd;
	struct kinfo_proc *kp;
	gint cnt;
	gboolean ret;

	if ((kd = kvm_openfiles (_PATH_DEVNULL, _PATH_DEVNULL, NULL, O_RDONLY, NULL)) == NULL)
		return FALSE;

	if ((kp = kvm_getprocs (kd, KERN_PROC_PID, pid, &cnt)) == NULL)
	{
		kvm_close (kd);
		return FALSE;
	}

	ret = (kp->ki_stat == SSTOP);
	kvm_close (kd);

	return ret;
}
