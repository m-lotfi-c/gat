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

#ifndef GENERIC_NEW_H
#define GENERIC_NEW_H

#include <gnome.h>

GtkWidget *make_new_radiobutton(char *label, gboolean val, GSList **group,
    GtkWidget *box);
GtkWidget *make_new_checkbutton(char *label, gboolean val, GtkWidget *box);
GtkWidget *make_new_spinner(char *label, int val, int min, int max,
    int step, GtkWidget *box);
char *ordinal_ending(int n);
extern char *month_name[];
extern char *week_name[];
GtkWidget *make_job_page(GtkWidget *box);
gboolean cmd_changed(GtkEntry *entry, GnomeDruid *druid);

#endif
