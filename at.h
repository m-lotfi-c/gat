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

#ifndef AT_H
#define AT_H

typedef struct {
  char *cmd;
  char *id, *when, *queue;
} at_job_t;

extern GArray *at_jobs;
GArray *get_at_jobs();
GtkWidget *make_at_page(GArray *j);
void print_at_jobs(GArray *aj);
at_job_t *make_new_at_job();
void at_free_job(at_job_t *job);
void at_add_job(at_job_t *job);

#endif
