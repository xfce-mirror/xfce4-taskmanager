/*
 * Copyright (c) 2008-2014 Landry Breuil <landry@xfce.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <err.h>
#include <sys/types.h>
/* for sysctl() */
#include <sys/param.h>
#include <sys/sched.h>
#include <sys/sysctl.h>
/* for swapctl() */
#include <sys/swap.h>
/* for strlcpy() */
#include <string.h>
/* for getpagesize() */
#include <unistd.h>
/* for P_ZOMBIE & SSLEEP */
#include <sys/proc.h>
/* for struct vmtotal */
#include <sys/vmmeter.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/if_ether.h>
#include <netinet/in_pcb.h>

#include <arpa/inet.h>
#include <ifaddrs.h>

// struct file in NetBSD
#define	_KERNEL
// ugly ! where is defined this ?
// plateform dependent
typedef int register_t;
typedef unsigned long paddr_t;
#include <machine/types.h>
#include <sys/types.h>
#include <sys/file.h>
#undef _KERNEL

#include "inode-to-sock.h"
#include "task-manager.h"
#include "network-analyzer.h"

#include <errno.h>

#ifdef __OpenBSD__
extern int errno;
#else
#include <machine/types.h>
#include <sys/protosw.h>
#include <sys/unpcb.h>
#include <sys/socketvar.h>
#include <sys/filedesc.h>
#include <sys/domain.h>
#include <kvm.h>
#endif

char	*state_abbrev[] = {
	"", "start", "run", "sleep", "stop", "zomb", "dead", "onproc"
};

static XtmInodeToSock *inode_to_sock = NULL;
static XtmNetworkAnalyzer *analyzer = NULL;

void increament_packet_count(char*, char*, GHashTable* hash_table, long int port);
gboolean get_if_count(int *data);
gboolean get_network_usage_if(int interface, guint64 *tcp_rx, guint64 *tcp_tx, guint64 *tcp_error);
struct kinfo_file* get_process_fds(int *nfiles, int kern, int arg);


#ifdef __OpenBSD__
void list_process_fds(Task *task, struct kinfo_proc *kp);
#else
void list_process_fds(Task *task, struct kinfo_proc2 *kp);
#endif

void
packet_callback(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
	// Extract source and destination IP addresses and ports from the packet
	struct ether_header *eth_header = (struct ether_header*)packet;
	struct ip *ip_header = (struct ip*)(packet + sizeof(struct ether_header));
	struct tcphdr *tcp_header = (struct tcphdr*)(packet + sizeof(struct ether_header) + sizeof(struct ip));

	// -Wdeclaration-after-statement

	long int src_port, dst_port;
	char local_mac[18];
	char src_mac[18];
	char dst_mac[18];

	// printf("%d, %d \n", eth_header->ether_type , ip_header->ip_p);

	// Dropped non-ip packet
	if (eth_header->ether_type != 8 || ip_header->ip_p != 6)
		return;

	src_port = ntohs(tcp_header->th_sport);
	dst_port = ntohs(tcp_header->th_dport);

	// directly use strcmp on analyzer->mac, eth_header->ether_shost doesnt work

	sprintf(local_mac,
		"%02X:%02X:%02X:%02X:%02X:%02X",
		analyzer->mac[0], analyzer->mac[1],
		analyzer->mac[2], analyzer->mac[3],
		analyzer->mac[4], analyzer->mac[5]
	);

	sprintf(src_mac,
		"%02X:%02X:%02X:%02X:%02X:%02X",
		eth_header->ether_shost[0], eth_header->ether_shost[1],
		eth_header->ether_shost[2], eth_header->ether_shost[3],
		eth_header->ether_shost[4], eth_header->ether_shost[5]
	);

	sprintf(dst_mac,
		"%02X:%02X:%02X:%02X:%02X:%02X",
		eth_header->ether_dhost[0], eth_header->ether_dhost[1],
		eth_header->ether_dhost[2], eth_header->ether_dhost[3],
		eth_header->ether_dhost[4], eth_header->ether_dhost[5]
	);

	// Debug
	//pthread_mutex_lock(&analyzer->lock);

	if(strcmp(local_mac, src_mac) == 0)
		increament_packet_count(local_mac, "in ", analyzer->packetin, src_port);

	if(strcmp(local_mac, dst_mac) == 0)
		increament_packet_count(local_mac, "out", analyzer->packetout, dst_port);

	//pthread_mutex_unlock(&analyzer->lock);
}

gboolean
get_network_usage(guint64 *tcp_rx, guint64 *tcp_tx, guint64 *tcp_error)
{
	// can also be get trough tcpstat struct
	// int mib[] = { CTL_NET, PF_INET, IPPROTO_TCP, TCPCTL_STATS }
	// sysctl(mib, sizeof(mib) / sizeof(mib[0]), %tcpstat, &len, NULL, 0)
	// repeat for IPPROTO_UDP or use IPPROTO_ETHERIP
	struct ifaddrs *ifaddr, *ifa;

       *tcp_error = 0;
       *tcp_rx = 0;
       *tcp_tx = 0;


	if (getifaddrs(&ifaddr) == -1)
		return -1;

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == NULL)
			continue;

		// Check if the interface is a network interface (AF_PACKET for Linux)
		if (ifa->ifa_addr->sa_family == AF_LINK)
		{
			struct if_data *ifdata = (struct if_data*)ifa->ifa_data;
			// Check if the interface has a hardware address (MAC address)
			if (ifdata != NULL)
			{
				*tcp_error += ifdata->ifi_oerrors + ifdata->ifi_ierrors;;
				*tcp_rx += ifdata->ifi_ibytes;
				*tcp_tx += ifdata->ifi_obytes;
			}
			//printf("%d, %d \n", *tcp_rx, *tcp_tx);
		}
	}

	freeifaddrs(ifaddr);

        return 0;
}

int
get_mac_address(const char *device, uint8_t mac[6])
{
	struct ifaddrs *ifaddr, *ifa;

	if (getifaddrs(&ifaddr) == -1)
		return -1;

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
	{
		if (ifa->ifa_addr == NULL)
			continue;

		// Check if the interface is a network interface (AF_PACKET for Linux)
		if (ifa->ifa_addr->sa_family == AF_LINK && strcmp(device, ifa->ifa_name) == 0)
		{
			// Check if the interface has a hardware address (MAC address)
			if (ifa->ifa_data != NULL)
			{
				struct sockaddr_dl *sdl = (struct sockaddr_dl *)ifa->ifa_addr;
				memcpy(mac, sdl->sdl_data + sdl->sdl_nlen, sizeof(uint8_t) * 6);
				freeifaddrs(ifaddr);
				return 0;
			}
		}
	}

	freeifaddrs(ifaddr);
	return -1;
}

struct kinfo_file*
get_process_fds(int *nfiles, int kern, int arg)
{
	// inspired by NetBSD pstat.c
	int mib[6];
	size_t len;
	struct kinfo_file *kf;

	// Set the MIB (Management Information Base) for the sysctl call
	mib[0] = CTL_KERN;

#ifdef __OpenBSD__
	mib[1] = KERN_FILE;
#else
	mib[1] = KERN_FILE2;
#endif

	mib[2] = kern;
	mib[3] = arg;
	mib[4] = sizeof(struct kinfo_file);
	mib[5] = 0;

	// Get the number of open files
	if (sysctl(mib, 6, NULL, &len, NULL, 0) < 0)
	{
		printf("sysctl Get the number of opened fd");
		return NULL;
	}

	*nfiles = len / sizeof(struct kinfo_file);

	// Allocate memory for the kinfo_file structures
	kf = malloc(len);
	if (kf == NULL)
	{
		printf("malloc");
		return NULL;
	}

	mib[5] = len / sizeof(struct kinfo_file);

	// Get the list of open files
	if (sysctl(mib, 6, kf, &len, NULL, 0) < 0)
	{
		printf("sysctl, enable to get kinfo_file *");
		free(kf);
		return NULL;
	}

	return kf;
}

void
xtm_refresh_inode_to_sock(XtmInodeToSock *its)
{
#ifdef __OpenBSD__
	// unneeded
#else
#endif
}

void
#ifdef __OpenBSD__
list_process_fds(Task *task, struct kinfo_proc *kp)
#else
list_process_fds(Task *task, struct kinfo_proc2 *kp)
#endif
{
#ifdef __OpenBSD__
	// inspired by openbsd netstat/inet.c
	// inspired by openbsd fstat/fstat.c

	int nfiles;
	struct kinfo_file *kf  = get_process_fds(&nfiles, KERN_FILE_BYPID, task->pid);

	if(kf == NULL)
		return;

	// Parse the kinfo_file structures
	for (int i = 0; i < nfiles; i++)
	{
		if (kf[i].f_type == DTYPE_SOCKET)
		{
			//interesting properties
			//task->packet_in +=  kf[i].f_rxfer;
			//task->packet_in +=  kf[i].f_rbytes;
			//task->packet_out +=  kf[i].f_rwfer;
			//task->packet_out +=  kf[i].f_wbytes;netstat

			int port = ntohs(kf[i].inp_lport);
			task->packet_in += (guint64)g_hash_table_lookup(analyzer->packetin, &port);
			task->packet_out += (guint64)g_hash_table_lookup(analyzer->packetout, &port);
			task->active_socket += 1;
		}
	}

	free(kf);
#else
	// inspired by netbsd fstat/fstat.c
	char *memf, *nlistf;
	char buf[_POSIX2_LINE_MAX];
	kvm_t *kd;
	struct filedesc filed;
	struct fdtab dt;
	size_t len;

	fdfile_t **ofiles;
	fdfile_t *fp;
	struct socket *sock;
	struct socket	so;
	struct file file;
	fdfile_t fdfile;
	struct protosw proto;
	struct domain dom;
	struct in4pcb	in4pcb;
	struct in6pcb	in6pcb;
	int port;

	nlistf = memf = NULL;
	kd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, buf);

	kvm_read(kd, kp->p_fd, &filed, sizeof (filed));
	kvm_read(kd, (u_long)filed.fd_dt, &dt, sizeof(dt));

	len = (filed.fd_lastfile+1) * sizeof(fdfile_t *);
	ofiles = malloc(len);
	kvm_read(kd, (u_long)&filed.fd_dt->dt_ff, ofiles, (filed.fd_lastfile+1) * (sizeof (fdfile_t *)));
	
	for (int i = 0; i <= filed.fd_lastfile; i++)
	{
		if (ofiles[i] == NULL)
			continue;

		fp = ofiles[i];

		kvm_read(kd, (u_long)fp, &fdfile, sizeof(fdfile));

		if (fdfile.ff_file == NULL)
			continue;

		kvm_read(kd, (u_long)fdfile.ff_file, &file, sizeof(file));

		if (file.f_type != DTYPE_SOCKET)
			continue;

		sock = (struct socket *)file.f_data;
		kvm_read(kd, (u_long)sock, &so, sizeof(struct socket));
		kvm_read(kd, (u_long)so.so_proto, &proto, sizeof(struct protosw));
		kvm_read(kd,  (u_long)proto.pr_domain, &dom, sizeof(struct domain));

		port = 0;

		if (dom.dom_family == AF_INET)
		{
			if (proto.pr_protocol == IPPROTO_TCP)
			{
				if (so.so_pcb == NULL)
					continue;

				kvm_read(kd, (u_long)so.so_pcb, (char *)&in4pcb, sizeof(in4pcb));
				struct inpcb *inp = (struct inpcb *)&in4pcb;
				port = ntohs(inp->inp_lport);
			}
		}

		if (dom.dom_family == AF_INET6)
		{
			if (proto.pr_protocol == IPPROTO_TCP)
			{
				if (so.so_pcb == NULL)
					continue;

				kvm_read(kd, (u_long)so.so_pcb, (char *)&in6pcb, sizeof(in6pcb));
				struct inpcb *inp = (struct inpcb *)&in6pcb;
				port = ntohs(inp->inp_lport);
			}
		}

		if (port == 0)
			continue;

		task->active_socket += 1;
		task->packet_in += (guint64)g_hash_table_lookup(analyzer->packetin, &port);
		task->packet_out += (guint64)g_hash_table_lookup(analyzer->packetout, &port);
	}

	kvm_close(kd);
	free(ofiles);

#endif
}

gboolean get_task_list (GArray *task_list)
{
	int mib[6];
	size_t size;
#ifdef __OpenBSD__
	struct kinfo_proc *kp;
#else
	struct kinfo_proc2 *kp;
#endif
	Task t;
	char **args;
	gchar* buf;
	int nproc, i;

	analyzer = xtm_network_analyzer_get_default();
	inode_to_sock = xtm_inode_to_sock_get_default();
	xtm_refresh_inode_to_sock(inode_to_sock);

	mib[0] = CTL_KERN;
#ifdef __OpenBSD__
	mib[1] = KERN_PROC;
#else
	mib[1] = KERN_PROC2;
#endif
	mib[2] = KERN_PROC_ALL;
	mib[3] = 0;
#ifdef __OpenBSD__
	mib[4] = sizeof(struct kinfo_proc);
#else
	mib[4] = sizeof(struct kinfo_proc2);
#endif
	mib[5] = 0;
	if (sysctl(mib, 6, NULL, &size, NULL, 0) < 0)
#ifdef __OpenBSD__
		errx(1, "could not get kern.proc size");
#else
		errx(1, "could not get kern.proc2 size");
#endif
	size = 5 * size / 4;		/* extra slop */
	if ((kp = malloc(size)) == NULL)
		errx(1,"failed to allocate memory for proc structures");
#ifdef __OpenBSD__
	mib[5] = (int)(size / sizeof(struct kinfo_proc));
#else
	mib[5] = (int)(size / sizeof(struct kinfo_proc2));
#endif
	if (sysctl(mib, 6, kp, &size, NULL, 0) < 0)
#ifdef __OpenBSD__
		errx(1, "could not read kern.proc");
	nproc = (int)(size / sizeof(struct kinfo_proc));
#else
		errx(1, "could not read kern.proc2");
	nproc = (int)(size / sizeof(struct kinfo_proc2));
#endif
	for (i=0 ; i < nproc ; i++)
	{
#ifdef __OpenBSD__
		struct kinfo_proc p = kp[i];
#else
		struct kinfo_proc2 p = kp[i];
#endif
		memset(&t, 0, sizeof(t));
		t.pid = p.p_pid;
		t.ppid = p.p_ppid;
		t.uid = p.p_uid;
		t.prio = p.p_priority - PZERO;
		t.vsz = p.p_vm_dsize + p.p_vm_ssize + p.p_vm_tsize;
		t.vsz *= getpagesize();
		t.rss = p.p_vm_rssize * getpagesize();
		g_snprintf(t.state, sizeof t.state, "%s", state_abbrev[p.p_stat]);
		g_strlcpy(t.name, p.p_comm, strlen(p.p_comm) + 1);
		/* shamelessly stolen from top/machine.c */
		if (!P_ZOMBIE(&p)) {
			size = 1024;
			if ((args = malloc(size)) == NULL)
				errx(1,"failed to allocate memory for argv structures at %zu", size);
			memset(args, 0, size);
			for (;; size *= 2) {
				if ((args = realloc(args, size)) == NULL)
					errx(1,"failed to allocate memory (size=%zu) for argv structures of pid %d", size, t.pid);
				memset(args, 0, size);
				mib[0] = CTL_KERN;
				mib[1] = KERN_PROC_ARGS;
				mib[2] = t.pid;
				mib[3] = KERN_PROC_ARGV;

				if (sysctl(mib, 4, args, &size, NULL, 0) == 0)
					break;
				if (errno != ENOMEM) { /* ESRCH: process disappeared */
					 /* printf ("process with pid %d disappeared, errno=%d\n", t.pid, errno); */
					args[0] ='\0';
					args[1] = NULL;
					break;
				}
			}

#ifdef __OpenBSD__
			buf = g_strjoinv(" ", args);
			g_assert(g_utf8_validate(buf, -1, NULL));
			g_strlcpy(t.cmdline, buf, sizeof t.cmdline);
			g_free(buf);
#else
			// g_strjoinv crash under NetBSD 10.0
#endif
			free(args);
		}

		t.cpu_user = (100.0f * ((gfloat)p.p_pctcpu / FSCALE));
		t.cpu_system = 0.0f; /* TODO ? */
		list_process_fds(&t, &p);

		g_array_append_val(task_list, t);
	}
	free(kp);

	g_array_sort (task_list, task_pid_compare_fn);

	return TRUE;
}

gboolean
pid_is_sleeping (GPid pid)
{
	int mib[6];
#ifdef __OpenBSD__
	struct kinfo_proc kp;
	size_t size = sizeof(struct kinfo_proc);
#else
	struct kinfo_proc2 kp;
	size_t size = sizeof(struct kinfo_proc2);
#endif

	mib[0] = CTL_KERN;
#ifdef __OpenBSD__
	mib[1] = KERN_PROC;
#else
	mib[1] = KERN_PROC2;
#endif
	mib[2] = KERN_PROC_PID;
	mib[3] = pid;
#ifdef __OpenBSD__
	mib[4] = sizeof(struct kinfo_proc);
#else
	mib[4] = sizeof(struct kinfo_proc2);
#endif
	mib[5] = 1;
	if (sysctl(mib, 6, &kp, &size, NULL, 0) < 0)
#ifdef __OpenBSD__
		errx(1, "could not read kern.proc for pid %d", pid);
#else
		errx(1, "could not read kern.proc2 for pid %d", pid);
#endif
	return (kp.p_stat == SSTOP ? TRUE : FALSE);
}

gboolean get_cpu_usage (gushort *cpu_count, gfloat *cpu_user, gfloat *cpu_system)
{
	static gulong cur_user = 0, cur_system = 0, cur_total = 0;
	static gulong old_user = 0, old_system = 0, old_total = 0;

#ifdef KERN_CPTIME
	int mib[] = {CTL_KERN, KERN_CPTIME};
#elseif KERN_CPTIME2
	int mib[] = {CTL_KERN, KERN_CPTIME2};
#else
	// NetBSD 10.0
	int mib[] = {CTL_KERN, KERN_CP_TIME};
#endif

 	glong cp_time[CPUSTATES];
 	gsize size = sizeof( cp_time );
	if (sysctl(mib, 2, &cp_time, &size, NULL, 0) < 0)
		errx(1,"failed to get kern.cptime");

	old_user = cur_user;
	old_system = cur_system;
	old_total = cur_total;

	cur_user = cp_time[CP_USER] + cp_time[CP_NICE];
	cur_system = cp_time[CP_SYS] + cp_time[CP_INTR];
	cur_total = cur_user + cur_system + cp_time[CP_IDLE];

	*cpu_user = (old_total > 0) ? (((cur_user - old_user) * 100.0f) / (float)(cur_total - old_total)) : 0.0f;
	*cpu_system = (old_total > 0) ? (((cur_system - old_system) * 100.0f) / (float)(cur_total - old_total)) : 0.0f;

	/* get #cpu */
	size = sizeof(&cpu_count);
	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;
	if (sysctl(mib, 2, cpu_count, &size, NULL, 0) == -1)
		errx(1,"failed to get cpu count");
	return TRUE;
}

/* vmtotal values in #pg */
#define pagetok(nb) ((nb) * (getpagesize()))

gboolean
get_memory_usage (guint64 *memory_total, guint64 *memory_available, guint64 *memory_free, guint64 *memory_cache, guint64 *memory_buffers, guint64 *swap_total, guint64 *swap_free)
{
#ifdef __OpenBSD__
	int mib[] = {CTL_VM, VM_UVMEXP};
	struct uvmexp  uvmexp;
#else
	int mib[] = {CTL_VM, VM_METER};
	struct vmtotal vmtotal;
#endif
	struct swapent *swdev;
	int nswap, i;
	size_t size;
#ifdef __OpenBSD__
	size = sizeof(uvmexp);
	if (sysctl(mib, 2, &uvmexp, &size, NULL, 0) < 0)
		errx(1,"failed to get vm.uvmexp");
	/* cheat : rm = tot used, add free to get total */
	*memory_free = pagetok((guint64)uvmexp.free);
	*memory_total = pagetok((guint64)uvmexp.npages);
	*memory_cache = 0;
	*memory_buffers = 0; /*pagetok(uvmexp.npages - uvmexp.free - uvmexp.active);*/
#else
	size = sizeof(vmtotal);
	if (sysctl(mib, 2, &vmtotal, &size, NULL, 0) < 0)
		errx(1,"failed to get vm.meter");
	/* cheat : rm = tot used, add free to get total */
	*memory_total = pagetok(vmtotal.t_rm + vmtotal.t_free);
	*memory_free = pagetok(vmtotal.t_free);
	*memory_cache = 0;
	*memory_buffers = pagetok(vmtotal.t_rm - vmtotal.t_arm);
#endif
	*memory_available = *memory_free + *memory_cache + *memory_buffers;

	/* get swap stats */
	*swap_total = *swap_free = 0;
	if ((nswap = swapctl(SWAP_NSWAP, 0, 0)) == 0)
		return TRUE;

	if ((swdev = calloc(nswap, sizeof(*swdev))) == NULL)
		errx(1,"failed to allocate memory for swdev structures");

	if (swapctl(SWAP_STATS, swdev, nswap) == -1) {
		free(swdev);
		errx(1,"failed to get swap stats");
	}

	/* Total things up */
	for (i = 0; i < nswap; i++) {
		if (swdev[i].se_flags & SWF_ENABLE) {
			*swap_free += (swdev[i].se_nblks - swdev[i].se_inuse);
			*swap_total += swdev[i].se_nblks;
		}
	}
	*swap_total *= DEV_BSIZE;
	*swap_free *= DEV_BSIZE;
	free(swdev);
	return TRUE;
}

