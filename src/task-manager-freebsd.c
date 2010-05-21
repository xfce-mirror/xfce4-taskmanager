/*
 * Copyright (c) 2010  Mike Massonnet <mmassonnet@xfce.org>
 * Copyright (c) 2006  Oliver Lehmann <oliver@FreeBSD.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <kvm.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <pwd.h>
#include <fcntl.h>
#include <paths.h>
#include <unistd.h>

#include <glib.h>

#include "task-manager.h"

gboolean
get_memory_usage (guint64 *memory_total, guint64 *memory_free, guint64 *memory_cache, guint64 *memory_buffers, guint64 *swap_total, guint64 *swap_free)
{
	/* Get memory usage */
	{
		gulong total = 0, free = 0, inactive = 0;
		size_t size;

		size = sizeof (gulong);
		sysctlbyname ("vm.stats.vm.v_page_count", &total, &size, NULL, 0);
		size = sizeof (gulong);
		sysctlbyname ("vm.stats.vm.v_free_count", &free, &size, NULL, 0);
		size = sizeof (gulong);
		sysctlbyname ("vm.stats.vm.v_inactive_count", &inactive, &size, NULL, 0);

		*memory_total = total * getpagesize ();
		*memory_free = free * getpagesize ();
		*memory_cache = inactive * getpagesize ();
		*memory_buffers = 0;
	}

	/* Get swap usage */
	{
		kvm_t *kd;
		struct kvm_swap kswap;

		if ((kd = kvm_openfiles (_PATH_DEVNULL, _PATH_DEVNULL, NULL, O_RDONLY, NULL)) == NULL)
			return FALSE;

		kvm_getswapinfo (kd, &kswap, 1, 0);
		*swap_total = kswap.ksw_total * getpagesize ();
		*swap_free = (kswap.ksw_total - kswap.ksw_used) * getpagesize ();

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

		*cpu_user = *cpu_system = 0.0;
		if (cp_total > cp_total_old)
		{
			*cpu_user = (cp_user - cp_user_old) * 100 / (gdouble)(cp_total - cp_total_old);
			*cpu_system = (cp_system - cp_system_old) * 100 / (gdouble)(cp_total - cp_total_old);
		}
	}

	return TRUE;
}

static gboolean
get_task_details (kvm_t *kd, struct kinfo_proc *kp, Task *task)
{
	struct passwd *pw;
	char **argv;
	int i;

	task->pid = kp->ki_pid;
	task->ppid = kp->ki_ppid;
	task->cpu_user = 100 * ((double)kp->ki_pctcpu / FSCALE);
	task->cpu_system = 0;
	task->vsz = kp->ki_size;
	task->rss = kp->ki_rssize * getpagesize ();
	pw = getpwuid (kp->ki_uid);
	task->uid = kp->ki_uid;
	g_strlcpy (task->uid_name, (pw != NULL) ? pw->pw_name : "nobody", sizeof (task->uid_name));
	task->prio = (gushort)kp->ki_nice;
	g_strlcpy (task->name, kp->ki_comm, 256);

	task->cmdline[0] = '\0';
	if ((argv = kvm_getargv (kd, kp, 1024)) != NULL)
	{
		for (i = 0; argv[i] != NULL; i++)
		{
			g_strlcat (task->cmdline, argv[i], 1024);
			g_strlcat (task->cmdline, " ", 1024);
		}
	}
	else
	{
		g_strlcpy (task->cmdline, kp->ki_comm, 1024);
	}

	i = 0;
	switch (kp->ki_stat)
	{
		case SSTOP:
		task->state[i] = 'T';
		break;

		case SSLEEP:
		if (kp->ki_tdflags & TDF_SINTR)
			task->state[i] = kp->ki_slptime >= MAXSLP ? 'I' : 'S';
		else
			task->state[i] = 'D';
		break;

		case SRUN:
		case SIDL:
		task->state[i] = 'R';
		break;

		case SWAIT:
		task->state[i] = 'W';
		break;

		case SLOCK:
		task->state[i] = 'L';
		break;

		case SZOMB:
		task->state[i] = 'Z';
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
	task->state[i] = '\0';

	return TRUE;
}

gboolean
get_task_list (GArray *task_list)
{
	kvm_t *kd;
	struct kinfo_proc *kp;
	int cnt, i;
	Task task;

	if ((kd = kvm_openfiles (_PATH_DEVNULL, _PATH_DEVNULL, NULL, O_RDONLY, NULL)) == NULL)
		return FALSE;

	if ((kp = kvm_getprocs (kd, KERN_PROC_PROC, 0, &cnt)) == NULL)
	{
		kvm_close (kd);
		return FALSE;
	}

	for (i = 0; i < cnt; kp++, i++)
	{
		get_task_details (kd, kp, &task);
		g_array_append_val (task_list, task);
	}

	kvm_close (kd);

	return TRUE;
}

gboolean
pid_is_sleeping (guint pid)
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
