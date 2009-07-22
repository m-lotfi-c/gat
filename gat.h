/* gat - The GNOME Task Scheduler
 * Copyright (C) 2000-2009 by Patrick Reynolds <patrick@piki.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef GAT_H
#define GAT_H

int run_job(char *cmd);
int make_ok_cancel_dialog(char *title, char *label);
void clist_select_row_callback(GtkWidget *clist, int row, int column,
     GdkEventButton *ev, gpointer d);

#endif
