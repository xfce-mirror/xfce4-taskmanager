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
#include <glib.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include "gui.h"

/* config vars */
#define REFRESH_INTERVAL 1

/* handler for SIGALRM to refresh the list */
void refresh_handler(void)
{
	refresh_task_list(FALSE);
	alarm(REFRESH_INTERVAL);
}

/* main */
int main(int argc, char *argv[])
{
	gtk_init(&argc, &argv);
	
	load_config();
	create_gui();
	
	signal(SIGALRM, refresh_handler);
	alarm(REFRESH_INTERVAL);
	
	gtk_main();
	
	return 0;
}
