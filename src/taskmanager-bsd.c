/* $Id$
 *
 * Copyright (c) 2008 Landry Breuil <landry@xfce.org>
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

#include "taskmanager.h"
/* for getpwuid() */
#include <sys/types.h>
#include <pwd.h>
/* for sysctl() */
#include <sys/param.h>
#include <sys/sysctl.h>
/* for kill() */
#include <signal.h>
#include <err.h>

char	*state_abbrev[] = {
	"", "start", "run", "sleep", "stop", "zomb", "dead", "onproc"
};

GArray *get_task_list(void)
{
	GArray *task_list;
	int mib[6];
	size_t size;
	struct kinfo_proc2 *kp;
	struct task t;
	struct passwd *passwdp;
	double d;
	char **args, **ptr;
	char buf[127];
	int nproc, i;
	fixpt_t ccpu; /* The scheduler exponential decay value. */
	int fscale; /* The kernel fixed-point scale factor. */

	task_list = g_array_new (FALSE, FALSE, sizeof (struct task));

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
		t.checked = FALSE;
		t.pid = p.p_pid;
		t.ppid = p.p_ppid;
		t.uid = p.p_uid;
		t.prio = p.p_priority - 22;
		t.vsize = p.p_vm_dsize + p.p_vm_ssize + p.p_vm_tsize;
		t.vsize *= getpagesize();
		t.rss = p.p_vm_rssize;
		g_snprintf(t.state, sizeof t.state, "%s", state_abbrev[p.p_stat]);
		/* shamelessly stolen from top/machine.c */
		/* short version: g_strlcpy(t.name, p.p_comm, strlen(p.p_comm) + 1); */
		size = 128;
		if ((args = malloc(size)) == NULL)
			errx(1,"failed to allocate memory for argv structures");
		for (;; size *= 2) {
			if ((args = realloc(args, size)) == NULL)
				errx(1,"failed to allocate memory for argv structures");
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
		g_snprintf(t.name, sizeof t.name, "%s", buf);

		t.time_percentage = (100.0 * ((double) p.p_pctcpu / FSCALE));
		/* get username from uid */
		passwdp = getpwuid(t.uid);
		if(passwdp != NULL && passwdp->pw_name != NULL)
			g_strlcpy(t.uname, passwdp->pw_name, sizeof t.uname);
		g_array_append_val(task_list, t);
	}

	return task_list;
}

gboolean get_cpu_usage_from_proc(system_status *sys_stat)
{
	/* tosee: remove this, get cpu perc from CPTIME */
	return FALSE;
}

/* vmtotal values in #pg, mem wanted in kB */
#define pagetok(nb) ((nb) * (getpagesize() / 1024))

gboolean get_system_status (system_status *sys_stat)
{
	int mib[] = {CTL_VM, VM_METER};
	struct vmtotal vmtotal;
	size_t size;
	size = sizeof(vmtotal);
	if (sysctl(mib, 2, &vmtotal, &size, NULL, 0) < 0)
		errx(1,"failed to get vm.meter");
	/* cheat : rm = tot used, add free to get total */
	sys_stat->mem_total = pagetok(vmtotal.t_rm + vmtotal.t_free);
	sys_stat->mem_free = pagetok(vmtotal.t_free);
	sys_stat->mem_cached = 0;
	sys_stat->mem_buffers = pagetok(vmtotal.t_rm - vmtotal.t_arm);
	size = sizeof(sys_stat->cpu_count);
	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;
	if (sysctl(mib, 2, &sys_stat->cpu_count, &size, NULL, 0) == -1)
		errx(1,"failed to get cpu count");
	/* cpu_user/idle/system unused atm so we don't care */
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

