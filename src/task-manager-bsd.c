/*
 * Copyright (c) 2008-2010 Landry Breuil <landry@xfce.org>
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

#include <stdlib.h>
#include <err.h>
/* for getpwuid() */
#include <sys/types.h>
#include <pwd.h>
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
#include "task-manager.h"

char	*state_abbrev[] = {
	"", "start", "run", "sleep", "stop", "zomb", "dead", "onproc"
};

gboolean get_task_list (GArray *task_list)
{
	int mib[6];
	size_t size;
	struct kinfo_proc2 *kp;
	Task t;
	struct passwd *passwdp;
	char **args, **ptr;
	char buf[127];
	int nproc, i;

	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC2;
	mib[2] = KERN_PROC_ALL;
	mib[3] = 0;
	mib[4] = sizeof(struct kinfo_proc2);
	mib[5] = 0;
	if (sysctl(mib, 6, NULL, &size, NULL, 0) < 0)
		errx(1, "could not get kern.proc2 size");
	size = 5 * size / 4;		/* extra slop */
	if ((kp = malloc(size)) == NULL)
		errx(1,"failed to allocate memory for proc structures");
	mib[5] = (int)(size / sizeof(struct kinfo_proc2));
	if (sysctl(mib, 6, kp, &size, NULL, 0) < 0)
		errx(1, "could not read kern.proc2");
	nproc = (int)(size / sizeof(struct kinfo_proc2));
	for (i=0 ; i < nproc ; i++)
	{
		struct kinfo_proc2 p = kp[i];
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
			size = 128;
			if ((args = malloc(size)) == NULL)
				errx(1,"failed to allocate memory for argv structures");
			for (;; size *= 2) {
				if ((args = realloc(args, size)) == NULL)
					errx(1,"failed to allocate memory for argv structures of pid %d",t.pid);
				mib[0] = CTL_KERN;
				mib[1] = KERN_PROC_ARGS;
				mib[2] = t.pid;
				mib[3] = KERN_PROC_ARGV;
				if (sysctl(mib, 4, args, &size, NULL, 0) == 0)
					break;
			}
			buf[0] = '\0';
			for (ptr = args; *ptr != NULL; ptr++) {
				if (ptr != args)
					strlcat(buf, " ", sizeof(buf));
				strlcat(buf, *ptr, sizeof(buf));
			}
			free(args);
			g_snprintf(t.cmdline, sizeof t.cmdline, "%s", buf);
		}

		t.cpu_user = (100.0 * ((double) p.p_pctcpu / FSCALE));
		t.cpu_system = 0; /* TODO ? */
		/* get username from uid */
		passwdp = getpwuid(t.uid);
		if(passwdp != NULL && passwdp->pw_name != NULL)
			g_strlcpy(t.uid_name, passwdp->pw_name, sizeof t.uid_name);
		g_array_append_val(task_list, t);
	}
	free(kp);

	return TRUE;
}

gboolean
pid_is_sleeping (guint pid)
{
	int mib[6];
	struct kinfo_proc2 kp;
	size_t size = sizeof(struct kinfo_proc2);

	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC2;
	mib[2] = KERN_PROC_PID;
	mib[3] = pid;
	mib[4] = sizeof(struct kinfo_proc2);
	mib[5] = 1;
	if (sysctl(mib, 6, &kp, &size, NULL, 0) < 0)
		errx(1, "could not read kern.proc2 for pid %d", pid);
	return (kp.p_stat == SSLEEP ? TRUE : FALSE);
}

gboolean get_cpu_usage (gushort *cpu_count, gfloat *cpu_user, gfloat *cpu_system)
{
	static gulong cur_user = 0, cur_system = 0, cur_total = 0;
	static gulong old_user = 0, old_system = 0, old_total = 0;

	int mib[] = {CTL_KERN, KERN_CPTIME};
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

	*cpu_user = (old_total > 0) ? (cur_user - old_user) * 100 / (gdouble)(cur_total - old_total) : 0;
	*cpu_system = (old_total > 0) ? (cur_system - old_system) * 100 / (gdouble)(cur_total - old_total) : 0;

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

gboolean get_memory_usage (guint64 *memory_total, guint64 *memory_free, guint64 *memory_cache, guint64 *memory_buffers, guint64 *swap_total, guint64 *swap_free)
{
	int mib[] = {CTL_VM, VM_METER};
	struct vmtotal vmtotal;
	struct swapent *swdev;
	int nswap, i;
	size_t size;
	size = sizeof(vmtotal);
	if (sysctl(mib, 2, &vmtotal, &size, NULL, 0) < 0)
		errx(1,"failed to get vm.meter");
	/* cheat : rm = tot used, add free to get total */
	*memory_total = pagetok(vmtotal.t_rm + vmtotal.t_free);
	*memory_free = pagetok(vmtotal.t_free);
	*memory_cache = 0;
	*memory_buffers = pagetok(vmtotal.t_rm - vmtotal.t_arm);

	/* get swap stats */
	if ((nswap = swapctl(SWAP_NSWAP, 0, 0)) == 0)
		errx(1,"failed to get swap device count");

	if ((swdev = calloc(nswap, sizeof(*swdev))) == NULL)
		errx(1,"failed to allocate memory for swdev structures");

	if (swapctl(SWAP_STATS, swdev, nswap) == -1) {
		free(swdev);
		errx(1,"failed to get swap stats");
	}

	/* Total things up */
	*swap_total = *swap_free = 0;
	for (i = 0; i < nswap; i++) {
		if (swdev[i].se_flags & SWF_ENABLE) {
			*swap_free += ((swdev[i].se_nblks - swdev[i].se_inuse) / DEV_BSIZE);
			*swap_total += (swdev[i].se_nblks / DEV_BSIZE);
		}
	}
	free(swdev);
	return TRUE;
}

