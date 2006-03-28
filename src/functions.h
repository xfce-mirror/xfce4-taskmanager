/*
 *  xfce4-taskmanager - very simple taskmanger
 *
 *  Copyright (c) 2005 Johannes Zellner, <webmaster@nebulon.de>
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

#ifndef __FUNCTIONS_H_
#define __FUNCTIONS_H_

#include <gtk/gtk.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/types.h>
#include <stdio.h>

#include <libxfce4util/libxfce4util.h>

#include "types.h"

#define PROC_DIR_1 "/compat/linux/proc"
#define PROC_DIR_2 "/emul/linux/proc"
#define PROC_DIR_3 "/proc"

gboolean refresh_task_list(void);
void fill_list_item(gint i, GtkTreeIter *iter);
void add_new_list_item(gint i);
gint compare_list_item(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data);
void remove_list_item(gint i);
void refresh_list_item(gint i);
void send_signal_to_task(gchar *task_id, gchar *signal);
void change_task_view(void);

void load_config(void);
void save_config(void);

#endif


