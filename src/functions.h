/*
 *  xfce4-taskmanager - very simple taskmanger
 *
 *  Copyright (c) 2004 Johannes Zellner, <webmaster@nebulon.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <gtk/gtk.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/types.h>
#include <stdio.h>
#include <libxfce4util/libxfce4util.h>
#include <libgtop-2.0/glibtop.h>
#include <libgtop-2.0/glibtop/proctime.h>

#include "types.h"

/* config vars */
gboolean config_show_user_tasks;
gboolean config_show_root_tasks;
gboolean config_show_other_tasks;

struct task all_tasks[512];

struct task task_list;

void refresh_task_list(gboolean first_time);
void send_signal_to_task(gchar *task_id, gchar *signal);
void remove_task_from_array(gint count);
struct task *get_parent_task(struct task task);
void show_user_tasks(void);
void hide_user_tasks(void);
void show_root_tasks(void);
void hide_root_tasks(void);
void show_other_tasks(void);
void hide_other_tasks(void);
void load_config(void);
