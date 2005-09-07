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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <signal.h>

#include "types.h"

#include "interface.h"
#include "functions.h"

/* handler for SIGALRM to refresh the list */
void refresh_handler(void)
{
	refresh_task_list();
	alarm(REFRESH_INTERVAL);
}

int main (int argc, char *argv[])
{
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	gtk_set_locale ();
	gtk_init (&argc, &argv);

	window1 = create_window1 ();
	gtk_widget_show (window1);
	
	own_uid = getuid();
	show_user_tasks = TRUE;
	show_root_tasks = FALSE;
	show_other_tasks = FALSE;
  
	task_array = g_array_new (FALSE, FALSE, sizeof (struct task));
	tasks = 0;
	
	if(!refresh_task_list())
		return 0;

	signal(SIGALRM, (void *)refresh_handler);
	alarm(REFRESH_INTERVAL);
	
	gtk_main ();
	
	return 0;
}

