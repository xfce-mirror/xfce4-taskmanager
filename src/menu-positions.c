/*
 *  xfce4-taskmanager - very simple taskmanger
 *
 *  Copyright (c) 2008  Mike Massonnet <mmassonnet@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "menu-positions.h"

void position_mainmenu(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
{
	GtkWidget *widget;
	GtkRequisition requisition;

	widget = user_data;
	gtk_widget_size_request(GTK_WIDGET(menu), &requisition);
	gdk_window_get_origin(widget->window, x, y);

	*x += widget->allocation.x;
	*y += widget->allocation.y;

	if(*y + requisition.height > gdk_screen_height ())
		*y = gdk_screen_height () - requisition.height;
	else if(*y < 0)
		*y = 0;
}
