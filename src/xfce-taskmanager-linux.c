#include "xfce-taskmanager-linux.h"

struct task get_task_details(gint pid)
{
	FILE *task_file;
	FILE *cmdline_file;
	gchar dummy[255];
	gchar buffer_status[255];
	struct task task;
	struct passwd *passwdp;
	struct stat status;
	gchar filename[255];
	gchar cmdline_filename[255];

	sprintf(filename, "/proc/%i/stat", pid);
	sprintf(cmdline_filename, "/proc/%i/cmdline", pid);

	stat(filename, &status);
	
	task.pid = -1;
	task.checked = FALSE;

	if((task_file = fopen(filename,"r")) != NULL)
	{
		while(fgets(buffer_status, sizeof(buffer_status), task_file) != NULL)
		{
			gint utime = 0;
			gint stime = 0;
			
			sscanf(buffer_status, "%i (%255s %1s %i %i %i %i %i %255s %255s %255s %255s %255s %i %i %i %i %i %i %i %i %i %i %i %255s %255s %255s %i %255s %255s %255s %255s %255s %255s %255s %255s %255s %255s %i %255s %255s",
						&task.pid,	// processid
						&task.name,	// processname
						&task.state,	// processstate
						&task.ppid,	// parentid
						&dummy,		// processs groupid

						&dummy,		// session id
						&dummy,		// tty id
						&dummy,		// tpgid: The process group ID of the process running on tty of the process
						&dummy,		// flags
						&dummy,		// minflt minor faults the process has maid

						&dummy,		// cminflt
						&dummy,		// majflt
						&dummy,		// cmajflt
						&utime,		// utime the number of jiffies that this process has scheduled in user mode
						&stime,		// stime " kernel mode

						&dummy,		// cutime " waited for children in user
						&dummy,		// cstime " kernel mode
						&dummy,		// priority (nice value + fifteen)
						&dummy,		// nice range from 19 to -19
						&dummy,		// hardcoded 0

						&dummy,		// itrealvalue time in jiffies to next SIGALRM send to this process
						&dummy,		// starttime jiffies the process startet after system boot
						&task.size,	// vsize in bytes
						&task.rss,	// rss
						&dummy,		// rlim limit in bytes for rss

						&dummy,		// startcode
						&dummy,		// endcode
						&dummy,		// startstack
						&dummy,		// kstkesp value of esp (stack pointer)
						&dummy, 	// kstkeip value of EIP (instruction pointer)

						&dummy,		// signal. bitmap of pending signals
						&dummy,		// blocked: bitmap of blocked signals
						&dummy,		// sigignore: bitmap of ignored signals
						&dummy,		// sigcatch: bitmap of catched signals
						&dummy,		// wchan

						&dummy,		// nswap
						&dummy,		// cnswap
						&dummy,		// exit_signal
						&dummy,		// CPU number last executed on
						&dummy,

						&dummy
					);
			task.time = stime + utime;
			task.old_time = task.time;
			task.time_percentage = 0;
		}
		task.uid = status.st_uid;
		passwdp = getpwuid(task.uid);
		if(passwdp != NULL && passwdp->pw_name != NULL)
			g_strlcpy(task.uname, passwdp->pw_name, sizeof task.uname);
	}
	

	if(task_file != NULL)
		fclose(task_file);

	if((cmdline_file = fopen(cmdline_filename,"r")) != NULL)
	{
		gchar dummy[255];
		strcpy(&dummy, "");
		fscanf(cmdline_file, "%255s", &dummy);
		if(strcmp(dummy, "") != 0)
		{
			if(g_strrstr(dummy,"/") != NULL)
				g_strlcpy(task.name, g_strrstr(dummy,"/")+1, 255);
			else
				g_strlcpy(task.name, dummy, 255);
				
			// workaround for cmd-line entries with leading "-"
			if(g_str_has_prefix(task.name, "-"))
				sscanf(task.name, "-%255s", task.name);
		}
	}
	
	if(cmdline_file != NULL)
		fclose(cmdline_file);
	
	if(g_str_has_suffix(task.name, ")"))
		*g_strrstr(task.name, ")") = '\0';

	return task;
}

GArray *get_task_list()
{
	DIR *dir;
	struct dirent *dir_entry;
	GArray *task_list;

	task_list = g_array_new (FALSE, FALSE, sizeof (struct task));
	
	if((dir = opendir("/proc/")) == NULL)
	{
		fprintf(stderr, "Error: couldn't load the /proc directory\n");
		return NULL;
	}

	gint count = 0;
	
	while((dir_entry = readdir(dir)) != NULL)
	{
		if(atoi(dir_entry->d_name) != 0)
		{
			struct task task = get_task_details(atoi(dir_entry->d_name));
			if(task.pid != -1)
				g_array_append_val(task_list, task);
		}
		count++;
	}

	closedir(dir);

	return task_list;
}
