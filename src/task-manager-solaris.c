/*
 * Copyright (c) 2024 Jehan-Antoine Vayssade, <javayss@sleek-think.ovh>
 * Copyright (c) 2010  Mike Massonnet <mmassonnet@xfce.org>
 * Copyright (c) 2009  Peter Tribble <peter.tribble@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "inode-to-sock.h"
#include "network-analyzer.h"
#include "task-manager.h"

#include <dirent.h>
#include <fcntl.h>
#include <inet/common.h> /* typedef int (*pfi_t)() for inet/optcom.h */
#include <inet/optcom.h>
#include <kstat.h>
#include <procfs.h>
#include <stdlib.h>
#include <string.h>
#include <sys/procfs.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/swap.h>
#include <sys/types.h>
#include <unistd.h>

// clang-format off
#include <sys/socket.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/tcp.h>
#include <ifaddrs.h>

#include <arpa/inet.h>
//#include <stropts.h>
#include <sys/stropts.h>
#include <inet/mib2.h>
#include <sys/tihdr.h>
// clang-format on

static XtmInodeToSock *inode_to_sock = NULL;
static XtmNetworkAnalyzer *analyzer = NULL;

static kstat_ctl_t *kc;
static gushort _cpu_count = 0;
static gulong ticks_total_delta = 0;

void addtoconninode (XtmInodeToSock *its, gint64 pid, char *ip, guint64 port);

static void
init_stats (void)
{
	kc = kstat_open ();
}

int
get_mac_address (const char *device, uint8_t mac[6])
{
#ifdef HAVE_LIBSOCKET
	struct ifaddrs *ifaddr, *ifa;

	if (getifaddrs (&ifaddr) == -1)
		return -1;

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == NULL)
			continue;

		// Check if the interface is a network interface (AF_PACKET for Linux)
		if (ifa->ifa_addr->sa_family == AF_LINK && strcmp (device, ifa->ifa_name) == 0)
		{
			// Check if the interface has a hardware address (MAC address)
			if (ifa->ifa_data != NULL)
			{
				// warning: cast from 'struct sockaddr *' to 'struct sockaddr_dl *' increases required alignment from 1 to 2
				// struct sockaddr_dl *sdl = (struct sockaddr_dl *)ifa->ifa_addr;
				struct sockaddr_dl sdl;
				memcpy (&sdl, ifa->ifa_addr, sizeof (struct sockaddr_dl));
				memcpy (mac, sdl.sdl_data + sdl.sdl_nlen, sizeof (uint8_t) * 6);
				freeifaddrs (ifaddr);
				return 0;
			}
		}
	}

	freeifaddrs (ifaddr);
	return -1;
#else
	memset (mac, 0, sizeof (uint8_t) * 6);
	return FALSE;
#endif
}

gboolean
get_network_usage (guint64 *tcp_rx, guint64 *tcp_tx, guint64 *tcp_error)
{
	kstat_named_t *knp;
	kstat_t *ksp;

	if (!kc)
		init_stats ();

	if (!(ksp = kstat_lookup (kc, "link", -1, NULL)))
	{
		printf ("kstat_lookup failed\n");
		return FALSE;
	}

	kstat_read (kc, ksp, NULL);
	*tcp_error = 0;

	if ((knp = kstat_data_lookup (ksp, "rbytes64")) != NULL)
		*tcp_rx = knp->value.ui64;

	if ((knp = kstat_data_lookup (ksp, "obytes64")) != NULL)
		*tcp_tx = knp->value.ui64;

	if ((knp = kstat_data_lookup (ksp, "ierrors")) != NULL)
		*tcp_error += knp->value.ui32;

	if ((knp = kstat_data_lookup (ksp, "oerrors")) != NULL)
		*tcp_error += knp->value.ui32;

	return TRUE;
}

#ifdef HAVE_LIBPCAP
void
packet_callback (u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
	// Extract source and destination IP addresses and ports from the packet
	struct ether_header eth_header;
	struct ip ip_header;
	struct tcphdr tcp_header;
	XtmNetworkAnalyzer *iface;
	long int src_port, dst_port;
	char local_mac[18];
	char src_mac[18];
	char dst_mac[18];

	memcpy (&eth_header, packet, sizeof (struct ether_header));
	memcpy (&ip_header, packet + sizeof (struct ether_header), sizeof (struct ip));
	memcpy (&tcp_header, packet + sizeof (struct ether_header) + sizeof (struct ip), sizeof (struct ip));

	// cast -> increases required alignment from 1 to 2 [-Wcast-align]
	// const struct ether_header *eth_header = (const struct ether_header *)packet;
	// struct ip *ip_header = (struct ip *)(packet + sizeof (struct ether_header));
	// struct tcphdr *tcp_header = (struct tcphdr *)(packet + sizeof (struct ether_header) + sizeof (struct ip));
	// printf("%d, %d \n", eth_heade.ether_type , ip_header.ip_p);

	// Dropped non-ip packet
	if (eth_header.ether_type != 8 || ip_header.ip_p != 6)
		return;

	iface = (XtmNetworkAnalyzer *)args;

	src_port = ntohs (tcp_header.th_sport);
	dst_port = ntohs (tcp_header.th_dport);

	// directly use strcmp on analyzer->mac, eth_header->ether_shost doesnt work

	snprintf (local_mac, sizeof (local_mac),
		"%02X:%02X:%02X:%02X:%02X:%02X",
		iface->mac[0], iface->mac[1],
		iface->mac[2], iface->mac[3],
		iface->mac[4], iface->mac[5]);

	snprintf (src_mac, sizeof (local_mac),
		"%02X:%02X:%02X:%02X:%02X:%02X",
		eth_header.ether_shost.ether_addr_octet[0], eth_header.ether_shost.ether_addr_octet[1],
		eth_header.ether_shost.ether_addr_octet[2], eth_header.ether_shost.ether_addr_octet[3],
		eth_header.ether_shost.ether_addr_octet[4], eth_header.ether_shost.ether_addr_octet[5]);

	snprintf (dst_mac, sizeof (local_mac),
		"%02X:%02X:%02X:%02X:%02X:%02X",
		eth_header.ether_dhost.ether_addr_octet[0], eth_header.ether_dhost.ether_addr_octet[1],
		eth_header.ether_dhost.ether_addr_octet[2], eth_header.ether_dhost.ether_addr_octet[3],
		eth_header.ether_dhost.ether_addr_octet[4], eth_header.ether_dhost.ether_addr_octet[5]);

	// Debug
	// pthread_mutex_lock(&iface->lock);

	if (strcmp (local_mac, src_mac) == 0)
		increament_packet_count (local_mac, "in ", iface->packetin, src_port);

	if (strcmp (local_mac, dst_mac) == 0)
		increament_packet_count (local_mac, "out", iface->packetout, dst_port);

	// pthread_mutex_unlock(&iface->lock);
}
#endif

void
set_task_network_data (Task *task, guint64 port)
{
	XtmNetworkAnalyzer *current;
	task->active_socket += 1;

	current = analyzer;
	while (current)
	{
		task->packet_in += (guint64)g_hash_table_lookup (current->packetin, &port);
		task->packet_out += (guint64)g_hash_table_lookup (current->packetout, &port);
		current = current->next;
	}
}

void
list_process_fds (Task *task)
{
	XtmNetworkAnalyzer *current;
	GHashTableIter iter;
	gpointer key, value;
	gint key_int, value_int;

	g_hash_table_iter_init (&iter, inode_to_sock->pid);
	while (g_hash_table_iter_next (&iter, &key, &value))
	{
		key_int = *(int *)(key);
		value_int = GPOINTER_TO_INT (value);
		if (task->pid == value_int)
		{
			set_task_network_data (task, key_int);
		}
	}
}

void
addtoconninode (XtmInodeToSock *its, gint64 pid, char *ip, guint64 port)
{
	int *inode;

	if (its == NULL)
		return;

	// seem like idle socket are given back to pid 0 (kernel ?)
	if (pid <= 0)
		return;

	if (strcmp (ip, "0.0.0.0") == 0)
		return;

	if (strcmp (ip, "::") == 0)
		return;

	if (strcmp (ip, "127.0.0.1") == 0)
		return;

	inode = g_new0 (gint, 1);
	*inode = port;

	g_hash_table_replace (inode_to_sock->pid, inode, (gpointer)(intptr_t)pid);

	printf ("PID %ld: Local Address: [%s]:%ld\n", pid, ip, port);
}

void
xtm_refresh_inode_to_sock (XtmInodeToSock *its)
{
	// inspired by :
	// nxsensor/src/sysdeps/solaris.c
	// net-snmp/agent/mibgroup/mibII/tcpTable.c
	// net-snmp/agent/mibgroup/mibgroup/kernel_sunos5.c
	// psutil/psutil/_psutil_sunos.c
	// illumos-joyent/master/usr/src/uts/common/io/tl.c

	int sd, ret, flags, getcode, num_ent, i;
	char buf[4096];
	char lip[INET6_ADDRSTRLEN];

	mib2_tcpConnEntry_t tp;
	mib2_udpEntry_t ude;

#if defined(AF_INET6)
	mib2_tcp6ConnEntry_t tp6;
	mib2_udp6Entry_t ude6;
#endif

	struct strbuf ctlbuf, databuf;
	struct T_optmgmt_req tor = { 0 };
	struct T_optmgmt_ack toa = { 0 };
	struct T_error_ack tea = { 0 };
	struct opthdr mibhdr = { 0 };

	sd = open ("/dev/arp", O_RDWR);
	if (sd == -1)
	{
		perror ("open");
		return;
	}

	ret = ioctl (sd, I_PUSH, "tcp");
	if (ret == -1)
	{
		perror ("ioctl");
		close (sd);
		return;
	}

	ret = ioctl (sd, I_PUSH, "udp");
	if (ret == -1)
	{
		perror ("ioctl");
		close (sd);
		return;
	}

	// g_hash_table_remove_all (its->pid);
	// g_hash_table_remove_all (its->hash);

	// Set up the request
	tor.PRIM_type = T_SVR4_OPTMGMT_REQ;
	tor.OPT_offset = sizeof (struct T_optmgmt_req);
	tor.OPT_length = sizeof (struct opthdr);
	tor.MGMT_flags = T_CURRENT;
	mibhdr.level = MIB2_IP;
	mibhdr.name = 0;
#ifdef NEW_MIB_COMPLIANT
	mibhdr.len = 1;
#else
	mibhdr.len = 0;
#endif
	memcpy (buf, &tor, sizeof (tor));
	memcpy (buf + tor.OPT_offset, &mibhdr, sizeof (mibhdr));
	ctlbuf.buf = buf;
	ctlbuf.len = tor.OPT_offset + tor.OPT_length;
	flags = 0;

	// Send the request
	if (putmsg (sd, &ctlbuf, NULL, flags) == -1)
	{
		perror ("putmsg");
		close (sd);
		return;
	}

	ctlbuf.maxlen = sizeof (buf);

	for (;;)
	{
		getcode = getmsg (sd, &ctlbuf, NULL, &flags);
		memcpy (&toa, buf, sizeof (toa));
		memcpy (&tea, buf, sizeof (tea));

		if (getcode != MOREDATA || ctlbuf.len < (int)sizeof (struct T_optmgmt_ack) ||
				toa.PRIM_type != T_OPTMGMT_ACK || toa.MGMT_flags != T_SUCCESS)
		{
			break;
		}

		if (ctlbuf.len >= (int)sizeof (struct T_error_ack) && tea.PRIM_type == T_ERROR_ACK)
		{
			fprintf (stderr, "ERROR_ACK\n");
			close (sd);
			return;
		}

		if (getcode == 0 && ctlbuf.len >= (int)sizeof (struct T_optmgmt_ack) &&
				toa.PRIM_type == T_OPTMGMT_ACK && toa.MGMT_flags == T_SUCCESS)
		{
			fprintf (stderr, "ERROR_T_OPTMGMT_ACK\n");
			close (sd);
			return;
		}

		memset (&mibhdr, 0x0, sizeof (mibhdr));
		memcpy (&mibhdr, buf + toa.OPT_offset, toa.OPT_length);
		databuf.maxlen = mibhdr.len;
		databuf.len = 0;
		databuf.buf = (char *)malloc ((int)mibhdr.len);
		if (!databuf.buf)
		{
			fprintf (stderr, "Out of memory\n");
			close (sd);
			return;
		}

		flags = 0;
		getcode = getmsg (sd, NULL, &databuf, &flags);

		if (getcode < 0)
		{
			perror ("getmsg");
			free (databuf.buf);
			close (sd);
			return;
		}

		// TCPv4
		if (mibhdr.level == MIB2_TCP && mibhdr.name == MIB2_TCP_13)
		{
			num_ent = mibhdr.len / sizeof (mib2_tcpConnEntry_t);
			for (i = 0; i < num_ent; i++)
			{
				memcpy (&tp, databuf.buf + i * sizeof (tp), sizeof (tp));
				inet_ntop (AF_INET, &tp.tcpConnLocalAddress, lip, sizeof (lip));
				addtoconninode (inode_to_sock, tp.tcpConnCreationProcess, lip, tp.tcpConnLocalPort);
				// interesting properties, allowing to remove pcpa
				// tp.tcpConnEntryInfo.ce_in_data_inorder_bytes
				// tp.tcpConnEntryInfo.ce_in_data_unorder_bytes
				// tp.tcpConnEntryInfo.ce_out_data_bytes
				// tp.tcpConnEntryInfo.ce_out_retrans_bytes
				// tp.tcpConnEntryInfo.ce_out_retrans_bytes
			}
		}
#if defined(AF_INET6)
		// TCPv6
		else if (mibhdr.level == MIB2_TCP6 && mibhdr.name == MIB2_TCP6_CONN)
		{
			num_ent = mibhdr.len / sizeof (mib2_tcp6ConnEntry_t);
			for (i = 0; i < num_ent; i++)
			{
				memcpy (&tp6, databuf.buf + i * sizeof (tp6), sizeof (tp6));
				inet_ntop (AF_INET6, &tp6.tcp6ConnLocalAddress, lip, sizeof (lip));
				addtoconninode (inode_to_sock, tp6.tcp6ConnCreationProcess, lip, tp6.tcp6ConnLocalPort);
			}
		}
#endif
		// UDPv4
		else if (mibhdr.level == MIB2_UDP || mibhdr.level == MIB2_UDP_ENTRY)
		{
			num_ent = mibhdr.len / sizeof (mib2_udpEntry_t);
			for (i = 0; i < num_ent; i++)
			{
				memcpy (&ude, databuf.buf + i * sizeof (ude), sizeof (ude));
				inet_ntop (AF_INET, &ude.udpLocalAddress, lip, sizeof (lip));
				addtoconninode (inode_to_sock, ude.udpCreationProcess, lip, ude.udpLocalPort);
			}
		}
#if defined(AF_INET6)
		// UDPv6
		else if (mibhdr.level == MIB2_UDP6 || mibhdr.level == MIB2_UDP6_ENTRY)
		{
			num_ent = mibhdr.len / sizeof (mib2_udp6Entry_t);
			for (i = 0; i < num_ent; i++)
			{
				memcpy (&ude6, databuf.buf + i * sizeof (ude6), sizeof (ude6));
				inet_ntop (AF_INET6, &ude6.udp6LocalAddress, lip, sizeof (lip));
				addtoconninode (inode_to_sock, ude6.udp6CreationProcess, lip, ude6.udp6LocalPort);
			}
		}
#endif

		free (databuf.buf);
	}

	close (sd);
}

gboolean
get_memory_usage (guint64 *memory_total, guint64 *memory_available, guint64 *memory_free, guint64 *memory_cache, guint64 *memory_buffers, guint64 *swap_total, guint64 *swap_free)
{
	kstat_t *ksp;
	kstat_named_t *knp;
	gint n;

	if (!kc)
		init_stats ();

	if (!(ksp = kstat_lookup (kc, "unix", 0, "system_pages")))
		return FALSE;
	kstat_read (kc, ksp, NULL);
	knp = kstat_data_lookup (ksp, "physmem");
	*memory_total = getpagesize () * knp->value.ui64;
	knp = kstat_data_lookup (ksp, "freemem");
	*memory_free = getpagesize () * knp->value.ui64;
	*memory_cache = 0;
	*memory_buffers = 0;
	*memory_available = *memory_free + *memory_cache + *memory_buffers;

	*swap_total = *swap_free = 0;
	if ((n = swapctl (SC_GETNSWP, NULL)) > 0)
	{
		struct swaptable *st;
		struct swapent *swapent;
		gchar path[MAXPATHLEN];
		gint i;

		if ((st = malloc (sizeof (int) + n * sizeof (swapent_t))) == NULL)
			return FALSE;
		st->swt_n = n;

		swapent = st->swt_ent;
		for (i = 0; i < n; i++, swapent++)
			swapent->ste_path = path;

		if ((swapctl (SC_LIST, st)) == -1)
		{
			free (st);
			return FALSE;
		}

		swapent = st->swt_ent;
		for (i = 0; i < n; i++, swapent++)
		{
			*swap_total += swapent->ste_pages * getpagesize ();
			*swap_free += swapent->ste_free * getpagesize ();
		}

		free (st);
	}

	return TRUE;
}

static void
get_cpu_percent (GPid pid, gulong ticks_user, gfloat *cpu_user, gulong ticks_system, gfloat *cpu_system)
{
	static GHashTable *hash_cpu_user = NULL;
	static GHashTable *hash_cpu_system = NULL;
	gulong ticks_user_old, ticks_system_old;

	if (hash_cpu_user == NULL)
	{
		hash_cpu_user = g_hash_table_new (NULL, NULL);
		hash_cpu_system = g_hash_table_new (NULL, NULL);
	}

	ticks_user_old = GPOINTER_TO_UINT (g_hash_table_lookup (hash_cpu_user, GINT_TO_POINTER (pid)));
	ticks_system_old = GPOINTER_TO_UINT (g_hash_table_lookup (hash_cpu_system, GINT_TO_POINTER (pid)));
	g_hash_table_insert (hash_cpu_user, GINT_TO_POINTER (pid), GUINT_TO_POINTER (ticks_user));
	g_hash_table_insert (hash_cpu_system, GINT_TO_POINTER (pid), GUINT_TO_POINTER (ticks_system));

	if (ticks_user < ticks_user_old || ticks_system < ticks_system_old)
		return;

	if (_cpu_count > 0 && ticks_total_delta > 0)
	{
		*cpu_user = (((ticks_user - ticks_user_old) * 100.0f) / (float)ticks_total_delta);
		*cpu_system = (((ticks_system - ticks_system_old) * 100.0f) / (float)ticks_total_delta);
	}
	else
	{
		*cpu_user = *cpu_system = 0.0f;
	}
}

gboolean
get_cpu_usage (gushort *cpu_count, gfloat *cpu_user, gfloat *cpu_system)
{
	kstat_t *ksp;
	kstat_named_t *knp;
	static gulong ticks_total = 0, ticks_total_old = 0;
	gulong ticks_user = 0, ticks_system = 0;

	if (!kc)
		init_stats ();

	_cpu_count = 0;
	kstat_chain_update (kc);

	ticks_total_old = ticks_total;
	ticks_total = 0;

	for (ksp = kc->kc_chain; ksp != NULL; ksp = ksp->ks_next)
	{
		if (!g_strcmp0 (ksp->ks_module, "cpu_info"))
		{
			_cpu_count += 1;
		}
		else if (!g_strcmp0 (ksp->ks_module, "cpu") && !g_strcmp0 (ksp->ks_name, "sys"))
		{
			kstat_read (kc, ksp, NULL);
			knp = kstat_data_lookup (ksp, "cpu_nsec_user");
			ticks_user += knp->value.ul / 100000;
			knp = kstat_data_lookup (ksp, "cpu_nsec_kernel");
			ticks_system += knp->value.ul / 100000;
			knp = kstat_data_lookup (ksp, "cpu_nsec_intr");
			ticks_system += knp->value.ul / 100000;
			knp = kstat_data_lookup (ksp, "cpu_nsec_idle");
			ticks_total += knp->value.ul / 100000;
			ticks_total += ticks_user + ticks_system;
		}
	}

	if (ticks_total > ticks_total_old)
		ticks_total_delta = ticks_total - ticks_total_old;

	get_cpu_percent (0, ticks_user, cpu_user, ticks_system, cpu_system);
	*cpu_count = _cpu_count;

	return TRUE;
}

static gboolean
get_task_details (GPid pid, Task *task)
{
	FILE *file;
	gchar filename[96];
	psinfo_t process;

	snprintf (filename, sizeof (filename), "/proc/%d/psinfo", pid);
	if ((file = fopen (filename, "r")) == NULL)
		return FALSE;

	if (fread (&process, sizeof (psinfo_t), 1, file) != 1)
	{
		fclose (file);
		return FALSE;
	}

	memset (task, 0, sizeof (Task));
	task->pid = process.pr_pid;
	task->ppid = process.pr_ppid;
	g_strlcpy (task->name, process.pr_fname, sizeof (task->name));
	snprintf (task->cmdline, sizeof (task->cmdline), "%s", process.pr_psargs);
	snprintf (task->state, sizeof (task->state), "%c", process.pr_lwp.pr_sname);
	task->vsz = (guint64)process.pr_size * 1024;
	task->rss = (guint64)process.pr_rssize * 1024;
	task->prio = process.pr_lwp.pr_pri;
	task->uid = (guint)process.pr_uid;
	get_cpu_percent (task->pid, (process.pr_time.tv_sec * 1000 + process.pr_time.tv_nsec / 100000), &task->cpu_user, 0, &task->cpu_system);

	fclose (file);

	list_process_fds (task);

	return TRUE;
}

gboolean
get_task_list (GArray *task_list)
{
	GDir *dir;
	const gchar *name;
	GPid pid;
	Task task;

	printf ("------------\n");
	analyzer = xtm_network_analyzer_get_default ();
	inode_to_sock = xtm_inode_to_sock_get_default ();
	xtm_refresh_inode_to_sock (inode_to_sock);

	if ((dir = g_dir_open ("/proc", 0, NULL)) == NULL)
		return FALSE;

	while ((name = g_dir_read_name (dir)) != NULL)
	{
		if ((pid = (GPid)g_ascii_strtoull (name, NULL, 0)) > 0)
		{
			if (get_task_details (pid, &task))
				g_array_append_val (task_list, task);
		}
	}

	g_dir_close (dir);

	g_array_sort (task_list, task_pid_compare_fn);

	return FALSE;
}

gboolean
pid_is_sleeping (GPid pid)
{
	FILE *file;
	gchar filename[96];
	gchar state[2];
	psinfo_t process;

	snprintf (filename, sizeof (filename), "/proc/%d/psinfo", pid);
	if ((file = fopen (filename, "r")) == NULL)
		return FALSE;

	if (fread (&process, sizeof (psinfo_t), 1, file) != 1)
	{
		fclose (file);
		return FALSE;
	}

	snprintf (state, sizeof (state), "%c", process.pr_lwp.pr_sname);
	fclose (file);

	return (state[0] == 'T') ? TRUE : FALSE;
}
