/* gat - The GNOME Task Scheduler
 * Copyright (C) 2000 by Patrick Reynolds <reynolds@cs.duke.edu>
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

#ifndef CRON_H
#define CRON_H

typedef struct {
  char *cmd;
  char *minute, *hour, *monthday, *month, *weekday;
} cron_job_t;

extern GArray *cron_jobs;
GArray *get_cron_jobs();
GtkWidget *make_cron_page(GArray *j);
void print_cron_jobs(GArray *cj);
cron_job_t *make_new_cron_job();
void cron_add_job(cron_job_t *job);

#endif
