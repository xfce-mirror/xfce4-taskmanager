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

#include "types.h"

struct task all_tasks[512];
gint task_count;

struct task task_list;

void refresh_task_list(void);
void send_signal_to_task(gchar *task_id, gchar *signal);
