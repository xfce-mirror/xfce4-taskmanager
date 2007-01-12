#ifndef LINUX_H
#define LINUX_H

#include <glib.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"

struct task get_task_details(gint pid);
GArray *get_task_list(void);
gboolean get_system_status(system_status *sys_stat);
gboolean get_cpu_usage_from_proc(system_status *sys_stat);

#endif
