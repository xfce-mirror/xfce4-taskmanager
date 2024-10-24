/*
 * Copyright (c) 2024 Torrekie Gen <me@torrekie.dev>
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
 */

#include <stdlib.h>
#include <err.h>
#include <sys/types.h>
/* for sysctl() */
#include <sys/param.h>
#include <sys/sysctl.h>
/* for strlcpy() */
#include <string.h>
/* for getpagesize() */
#include <unistd.h>
/* for SZOMB */
#include <sys/proc.h>
/* for bool */
#include <stdbool.h>
/* Mach specific */
#include <mach/mach.h>
#include <mach/mach_time.h>
/* Darwin target conditional */
#include <TargetConditionals.h>
#include <Availability.h>

#include "task-manager.h"

#include <errno.h>

/* mark processes slept over 20 seconds as 'I' instead of 'S', thats how
 * `man 1 ps` telled us */
#ifndef MAXSLP
#define MAXSLP 20
#endif

/* Notes when using on iOS:
 * 1. com.apple.security.exception.sysctl.read-write is needed when sandboxed
 * 2. task_for_pid-allow is always required for task_for_pid()
 */

static int
mach_state_order(int s, long sleep_time)
{
	switch (s) {
	case TH_STATE_RUNNING:	return(1);
	case TH_STATE_UNINTERRUPTIBLE:
				return(2);
	case TH_STATE_WAITING:	return((sleep_time > MAXSLP) ? 4 : 3);
	case TH_STATE_STOPPED:	return(5);
	case TH_STATE_HALTED:	return(6);
	default:		return(7);
    }
}

static const gchar mach_state_table[] = {
	' ',
	'R', /* TH_STATE_RUNNING */
	'U', /* TH_STATE_UNINTERRUPTIBLE */
	'S', /* TH_STATE_WAITING */
	'I', /* TH_STATE_WAITING & p_slptime > MAXSLP */
	'T', /* TH_STATE_STOPPED */
	'H', /* TH_STATE_HALTED */
	'?'
};

__unused
static const gchar p_stat2state[] = {
	' ',
	'I', /* SIDL */
	'R', /* SRUN */
	'S', /* SSLEEP */
	'T', /* SSTOP */
	'Z' /* SZOMB */
};

#define TIME_VALUE_TO_NS(a)                                                                        \
	(((uint64_t)((a)->seconds) * NSEC_PER_SEC) + ((uint64_t)((a)->microseconds) * NSEC_PER_USEC))

static inline gulong
mach_time_nsec(void)
{
#if (defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && __MAC_OS_X_VERSION_MIN_REQUIRED >= __MAC_10_12) || \
    (defined(__IPHONE_OS_VERSION_MIN_REQUIRED) && __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_10_0)
	/* Darwin 16+ introduced clock_gettime_nsec_np() */
	return clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
#else
	mach_timebase_info_data_t clock_timebase;
	mach_timebase_info(&clock_timebase);
	return (mach_absolute_time() * clock_timebase.numer) / clock_timebase.denom;
#endif
}

/* task-manager-linux.c */
static void
get_cpu_percent (GPid pid, gulong task_time, gfloat *cpu_user)
{
	static GHashTable *hash_time = NULL;
	static gulong cpu_time_old = 0;

	gulong task_time_old;

	/* init time */
	if (hash_time == NULL)
		hash_time = g_hash_table_new (NULL, NULL);
	if (cpu_time_old == 0) {
		cpu_time_old = mach_time_nsec();
		g_hash_table_insert (hash_time, GINT_TO_POINTER (pid), (gpointer)(gulong)(task_time));
		*cpu_user = 0.0f;
		return;
	}

	task_time_old = (gulong) (g_hash_table_lookup (hash_time, GINT_TO_POINTER (pid)));
	gulong task_time_delta = (task_time - task_time_old) / NSEC_PER_USEC;
        /* store for next iter */
	g_hash_table_insert (hash_time, GINT_TO_POINTER (pid), (gpointer)(gulong)(task_time));

	gulong cpu_time = mach_time_nsec();
	gulong cpu_time_delta = (cpu_time - cpu_time_old) / NSEC_PER_USEC;
	/* store for next iter */
	cpu_time_old = cpu_time;

	if (cpu_time_delta == 0) {
		*cpu_user = 0.0f;
		return;
	}

	*cpu_user = 0.01f * task_time_delta / cpu_time_delta;
}

gboolean get_task_list (GArray *task_list)
{
	static bool have_tfp0 = true;

	int mib[4];
	size_t size;
	struct kinfo_proc *kp;
	Task t;
	char *args; /* OpenBSD KERN_PROC_ARGS: {"1", "2", "3"...}, Darwin KERN_PROCARGS2: "1" '\0' "2" '\0' "3"...ENV */
	gchar* buf;
	int nproc, i, nargs, argmax, c = 0;
	char *np, *sp, *cp;

	gulong total_time;

	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_ALL;
	mib[3] = 0;

	if (sysctl(mib, 4, NULL, &size, NULL, 0) < 0)
		errx(1, "could not get kern.proc size");

	if ((kp = (struct kinfo_proc *)malloc(size)) == NULL)
		errx(1,"failed to allocate memory for proc structures");

	if (sysctl(mib, 4, kp, &size, NULL, 0) < 0)
		errx(1, "could not read kern.proc");

	nproc = (int)(size / sizeof(struct kinfo_proc));

	for (i=0 ; i < nproc ; i++)
	{
		struct kinfo_proc p = kp[i];
		task_t result;
		int task_state;
		mach_port_t task;
		thread_port_array_t thread_list;
		uint32_t thread_count;
		struct task_basic_info_64 task_info_data = {0};
		struct task_thread_times_info thread_time_data = {0};
		struct thread_basic_info thread_info_data = {0};
		mach_msg_type_number_t task_info_count = TASK_BASIC_INFO_64_COUNT;
		mach_msg_type_number_t thread_time_count = TASK_THREAD_TIMES_INFO_COUNT;
		mach_msg_type_number_t thread_info_count = THREAD_BASIC_INFO_COUNT;

		/* task_for_pid fails on zombies */
		if (p.kp_proc.p_stat != SZOMB) {
			/* get task port */
			if ((result = task_for_pid(mach_task_self(), p.kp_proc.p_pid, &task)) != KERN_SUCCESS) {
				/* we are not allowed inspect kernel_task when no tfp0 */
				if (p.kp_proc.p_pid == 0) {
					if (have_tfp0) {
						g_warning("Unable to inspect kernel_task (%s)", mach_error_string(result));
						have_tfp0 = false;
					}
					continue;
				}
				G_DEBUG_FMT ("Unable to get task of PID %d (%s)", p.kp_proc.p_pid, mach_error_string(result));
			}
			/* get basic info (for t.vsz t.rss t.state) */
			if ((result = task_info(task, TASK_BASIC_INFO_64, (task_info_t)&task_info_data, &task_info_count)) != KERN_SUCCESS) {
				G_DEBUG_FMT ("Unable to inspect task info of PID %d (%s)", p.kp_proc.p_pid, mach_error_string(result));
				goto task_info_done;
			}
			/* get thread time info (for t.cpu_user) */
			if ((result = task_info(task, TASK_THREAD_TIMES_INFO, (task_info_t)&thread_time_data, &thread_time_count)) != KERN_SUCCESS) {
				G_DEBUG_FMT ("Unable to inspect task thread time of PID %d (%s)", p.kp_proc.p_pid, mach_error_string(result));
				goto task_info_done;
			}
			/* get threads */
			if ((result = task_threads(task, &thread_list, &thread_count)) != KERN_SUCCESS) {
				G_DEBUG_FMT ("Unable to get task threads of PID %d (%s)", p.kp_proc.p_pid, mach_error_string(result));
				goto task_info_done;
			}
			task_state = 7; /* Max state */
			for (c = 0; c < (int)thread_count; c++) {
				thread_info_count = THREAD_BASIC_INFO_COUNT;
				result = thread_info(thread_list[c], THREAD_BASIC_INFO, (thread_info_t)&thread_info_data, &thread_info_count);
				if (result != KERN_SUCCESS)
					G_DEBUG_FMT ("Unable to get thread %d info of PID %d (%s)", c, p.kp_proc.p_pid, mach_error_string(result));
				result = mach_state_order(thread_info_data.run_state, thread_info_data.sleep_time);
				if ((int)result < task_state)
					task_state = result;

				/* thread info provides 'flags', which can be used to check if thread is swapped out (typically 'W' state).
				 * but not listed in STAT by Apple ps(1), even they have actually written this logic (Why?) */
				/* swapped = ((thread_info_data.flags & TH_FLAGS_SWAPPED) != 0) */

				/* thread info also provides 'cpu_usage', which tells %CPU per thread, normally its ok to just use this to
				 * provide %CPU data, but not sure why all Apple top(1), GNU htop(1) and also Google Chromium preferred to
				 * retrieve Darwin process cpu usages by time calculation. */
				/* cpu_usage += thread_info_data.cpu_usage; */

				mach_port_deallocate(mach_task_self(), thread_list[c]);
			}

			result = vm_deallocate(mach_task_self(), (vm_address_t)(thread_list), sizeof(*thread_list) * thread_count);
			if (result) g_warning("Trouble freeing thread_list of PID %d (%s)", p.kp_proc.p_pid, mach_error_string(result));
task_info_done:
			mach_port_deallocate(mach_task_self(), task);
		}

		memset(&t, 0, sizeof(t));
		t.pid = p.kp_proc.p_pid;
		t.ppid = p.kp_eproc.e_ppid;
		t.uid = p.kp_eproc.e_pcred.p_ruid;
		t.vsz = (double)task_info_data.virtual_size / 1024.0; /* KiB */
		/* t.vsz *= getpagesize(); */
		t.rss = task_info_data.resident_size / 1024.0; /* KiB */
		/* PZERO no longer used by Darwin, no need to offset it like BSD */
		/* Which priority should we use, kinfo_proc or jetsam? */
		t.prio = p.kp_proc.p_priority;

		c = 0;
		switch (p.kp_proc.p_stat)
		{
			case SSTOP:
				t.state[c] = 'T';
				break;
			case SZOMB:
				t.state[c] = 'Z';
				break;
			default:
				/* Mach thread state provides more info */
				t.state[c] = mach_state_table[task_state];
		}
		c++;
		/* P_INMEM unused by Darwin */
		if (p.kp_proc.p_nice < 0)
			t.state[c++] = '<';
		else if (p.kp_proc.p_nice > 0)
			t.state[c++] = 'N';
		if (p.kp_proc.p_flag & P_TRACED)
			t.state[c++] = 'X';
		if (p.kp_proc.p_flag & P_WEXIT && p.kp_proc.p_stat != SZOMB)
			t.state[c++] = 'E';
		if (p.kp_proc.p_flag & P_PPWAIT)
			t.state[c++] = 'V';
		if (p.kp_proc.p_flag & (P_SYSTEM | P_NOSWAP | P_PHYSIO))
			t.state[c++] = 'L';
		if (p.kp_eproc.e_flag & EPROC_SLEADER)
			t.state[c++] = 's';
		if ((p.kp_proc.p_flag & P_CONTROLT) && p.kp_eproc.e_pgid == p.kp_eproc.e_tpgid)
			t.state[c++] = '+';
#if TARGET_OS_EMBEDDED
		/* Suspended by parent, launchd or debugger.
		 * Even this also exists on macOS, it is more likely describing the
		 * UIKit App state, which formally has Active, Inactive, Background.
		 * This is for checking App's 'Suspend' state */
		if (p.kp_proc.p_stat != SZOMB && task_info_data.suspend_count > 0)
			t.state[c++] = 'B';
#endif
		g_strlcpy(t.name, p.kp_proc.p_comm, strlen(p.kp_proc.p_comm) + 1);
		/* https://opensource.apple.com/source/adv_cmds/adv_cmds-158/ps/print.c */
		if (p.kp_proc.p_stat != SZOMB) {
			mib[1] = KERN_ARGMAX;
			size = sizeof(argmax);
			if (sysctl(mib, 2, &argmax, &size, NULL, 0) != 0)
				argmax = 1024; /* fallback */
			if ((args = malloc(argmax)) == NULL)
				errx(1,"failed to allocate memory for argv structures at %zu", size);
			memset(args, 0, argmax);

			mib[1] = KERN_PROCARGS2;
			mib[2] = t.pid;
			size = (size_t)argmax;
			if (sysctl(mib, 3, args, &size, NULL, 0) == 0) {
				c = 0;
				np = NULL;
				sp = NULL;
				cp = NULL;
				/* Save current pos */
				memcpy(&nargs, args, sizeof(nargs));
				cp = args + sizeof(nargs);

				/* Skip the saved t.name. */
				for (; cp < &args[size]; cp++) {
					if (*cp == '\0') {
						/* End of t.name reached. */
						break;
					}
				}
				/* Reached argmax */
				if (cp == &args[size]) {
					goto done;
				}

				/* Skip trailing '\0' characters. */
				for (; cp < &args[size]; cp++) {
					if (*cp != '\0') {
						/* Beginning of first argument reached. */
						break;
					}
				}
				/* Reached argmax */
				if (cp == &args[size]) {
					goto done;
				}
				/* Save where the argv[0] string starts. */
				sp = cp;
				/*
				 * Iterate through the '\0'-terminated strings and convert '\0' to ' '
				 * until a string is found that has a '=' character in it (or there are
				 * no more strings in args).  There is no way to deterministically
				 * know where the command arguments end and the environment strings
				 * start, which is why the '=' character is searched for as a heuristic.
				 */
				for (np = NULL; c < nargs && cp < &args[size]; cp++) {
					if (*cp == '\0') {
						c++;
						if (np != NULL) {
							/* Convert previous '\0'. */
							*np = ' ';
						}
						/* Note location of current '\0'. */
						np = cp;

					}
				}
				/*
				 * sp points to the beginning of the arguments/environment string, and
				 * np should point to the '\0' terminator for the string.
				 */
				if (np == NULL || np == sp) {
					/* Empty or unterminated string. */
					goto done;
				}
done:
				/* Make a copy of the string. */
				buf = g_strdup_printf("%s", sp);
			} else {
				/* When we reach here, the process has been gone after we got task info */
				buf = g_strdup("");
			}
			g_assert(g_utf8_validate(buf, -1, NULL));
			g_strlcpy(t.cmdline, buf, sizeof t.cmdline);
			g_free(buf);
			free(args);
		} else {
			/* Zombie processes */
			g_snprintf(t.name, strlen(p.kp_proc.p_comm) + 3, "(%s)", p.kp_proc.p_comm);
			g_strlcpy(t.cmdline, t.name, sizeof t.name);
		}

		/* p_pctcpu always 0 on Darwin Releases */
		t.cpu_user = (100.0f * ((gfloat)p.kp_proc.p_pctcpu / FSCALE));

		/* Set total_time. */
		/* thread info contains live time, task info contains terminated time */
		total_time = TIME_VALUE_TO_NS(&thread_time_data.user_time)
			   + TIME_VALUE_TO_NS(&thread_time_data.system_time)
			   + TIME_VALUE_TO_NS(&task_info_data.user_time)
			   + TIME_VALUE_TO_NS(&task_info_data.system_time);

		get_cpu_percent(t.pid, total_time, &t.cpu_user);

		t.cpu_system = 0.0f; /* Unused */
		g_array_append_val(task_list, t);
	}
	free(kp);

	g_array_sort (task_list, task_pid_compare_fn);

	return TRUE;
}

gboolean
pid_is_sleeping (GPid pid)
{
	int mib[4];
	struct kinfo_proc kp;
	size_t size = sizeof(struct kinfo_proc);

	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PID;
	mib[3] = pid;

	if (sysctl(mib, 4, &kp, &size, NULL, 0) < 0)
		errx(1, "could not read kern.proc for pid %d", pid);

	return (kp.kp_proc.p_stat == SSTOP ? TRUE : FALSE);
}

gboolean get_cpu_usage (gushort *cpu_count, gfloat *cpu_user, gfloat *cpu_system)
{
	static gulong cur_user = 0, cur_system = 0, cur_total = 0;
	static gulong old_user = 0, old_system = 0, old_total = 0;

	size_t size;
	int mib[2];
	mach_msg_type_number_t vm_count = HOST_CPU_LOAD_INFO_COUNT;
	host_cpu_load_info_data_t cpuload;

	if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpuload, &vm_count) != KERN_SUCCESS)
		errx(1, "failed to get host_statistics");

	old_user = cur_user;
	old_system = cur_system;
	old_total = cur_total;

	cur_user = cpuload.cpu_ticks[CPU_STATE_USER] + cpuload.cpu_ticks[CPU_STATE_NICE];
	cur_system = cpuload.cpu_ticks[CPU_STATE_SYSTEM];
	cur_total = cur_user + cur_system + cpuload.cpu_ticks[CPU_STATE_IDLE];

	*cpu_user = (old_total > 0) ? (((cur_user - old_user) * 100.0f) / (float)(cur_total - old_total)) : 0.0f;
	*cpu_system = (old_total > 0) ? (((cur_system - old_system) * 100.0f) / (float)(cur_total - old_total)) : 0.0f;

	/* get #cpu */
	size = sizeof(&cpu_count);
	mib[0] = CTL_HW;
	mib[1] = HW_NCPU; /* Some impls uses hw.physicalcpu or hw.activecpu */
	if (sysctl(mib, 2, cpu_count, &size, NULL, 0) == -1)
		errx(1,"failed to get cpu count");
	return TRUE;
}

/* vmtotal values in #pg */
#define pagetok(nb) ((nb) * (vm_pagesize))

gboolean
get_memory_usage (guint64 *memory_total, guint64 *memory_available, guint64 *memory_free, guint64 *memory_cache, guint64 *memory_buffers, guint64 *swap_total, guint64 *swap_free)
{
	int mib[2];
	struct xsw_usage swapused;
	vm_statistics64_data_t vm_stat;
	mach_msg_type_number_t vm_count = HOST_VM_INFO64_COUNT;
	vm_size_t vm_pagesize;
	uint64_t memsize;
	size_t size;

#if 0
	mib[0] = CTL_VM;
	mib[1] = VM_METER;
	struct vmtotal vmtotal;
	size = sizeof(vmtotal);
	if (sysctl(mib, 2, &vmtotal, &size, NULL, 0) < 0)
		errx(1,"failed to get vm.meter");
	/* cheat : rm = tot used, add free to get total */
	*memory_total = pagetok(vmtotal.t_rm + vmtotal.t_free);
	*memory_free = pagetok(vmtotal.t_free);
	*memory_cache = 0;
	*memory_buffers = pagetok(vmtotal.t_rm - vmtotal.t_arm);
	*memory_available = *memory_free + *memory_cache + *memory_buffers;
#else
	/* To match the behavior of Activity Monitor.app, we get hw.memsize first */
	mib[0] = CTL_HW;
	mib[1] = HW_MEMSIZE;
	size = sizeof(memsize);
	if (sysctl(mib, 2, &memsize, &size, NULL, 0) < 0) {
		/* Fallback to calculating, which matches the behavior of Apple top(1) */
		memsize = 0;
	}

	/* Sad truth, VM_METER is not used by Darwin but defined in header, Apple lied to us */
	host_page_size(mach_host_self(), &vm_pagesize); /* getpagesize() is much easier ig */
	if (host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info_t)&vm_stat, &vm_count) == KERN_SUCCESS) {
		/* Activity Monitor.app: UsedMem = pagetok(vm_stat.internal_page_count - vm_stat.purgeable_count) +
		 *				   pagetok(vm_stat.wire_count) + pagetok(vm_stat.compressor_page_count)
		 *			 Which is 'App Memory' + 'Wired Memory' + 'Compressed Memory'.
		 * Apple /usr/bin/top: UsedMem = pagetok(vm_stat.wire_count + vm_stat.inactive_count +
		 *				 vm_stat.active_count + vm_stat.compressor_page_count);
		 *		       Which not counting vm_stat.free_count and 'App Memory', resulting significant difference
		 * 		       between both results.
		 * Which one shall I believe?
		 */
		if (memsize) {
			*memory_total = memsize;
			*memory_free = memsize - pagetok(vm_stat.internal_page_count - vm_stat.purgeable_count
				+ vm_stat.wire_count + vm_stat.compressor_page_count);
		} else {
			*memory_free = pagetok(vm_stat.free_count);
			*memory_total = pagetok(vm_stat.free_count + vm_stat.wire_count + vm_stat.inactive_count
				+ vm_stat.active_count + vm_stat.compressor_page_count);
		}
		*memory_buffers = 0; /* total - active, but not actually used by taskmanager, skip */
		*memory_cache = 0; /* inactive,  but not actually used by taskmanager, skip */
		*memory_available = *memory_free + *memory_cache + *memory_buffers;
	} else {
		errx(1,"failed to get vm_statistics");
	}
#endif
	mib[0] = CTL_VM;
	mib[1] = VM_SWAPUSAGE;
	/* get swap usage */
	size = sizeof(swapused);
	if (sysctl(mib, 2, &swapused, &size, NULL, 0) < 0)
		errx(1,"failed to get vm.swapusage");

	/* Total things up */
	*swap_total = *swap_free = 0;
	*swap_total += swapused.xsu_total / 1024;
	*swap_free += swapused.xsu_avail / 1024;
	*swap_total *= DEV_BSIZE;
	*swap_free *= DEV_BSIZE;
	return TRUE;
}
