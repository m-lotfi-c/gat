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

#include <gnome.h>
#include "at.h"
#include "generic_new.h"

typedef struct {
  GtkWidget *month, *day, *year, *hour, *minute, *command, *win;
} at_druid_info;

#define EGET(name) gtk_entry_get_text(GTK_ENTRY(info->name))
#define SGET(name) gtk_spin_button_get_value_as_int(\
  GTK_SPIN_BUTTON(info->name))

static gboolean at_commit_job(at_druid_info *info) {
  const char *t;
  char buf[32];
  at_job_t *job = NULL;
  t = EGET(command);
  if (!t || !*t) goto error_out;
  job = g_new(at_job_t, 1);
  job->cmd = strdup(t);
  job->id = job->queue = job->when = NULL;

  sprintf(buf, "%02d%02d %02d/%02d/%04d",
    SGET(hour), SGET(minute), SGET(month), SGET(day), SGET(year));
  job->when = strdup(buf);

  gtk_widget_destroy(info->win);
  at_add_job(job);
  return TRUE;

error_out:
  at_free_job(job);
  return FALSE;
}

at_job_t *make_new_at_job() {
  GtkWidget *win, *druid, *page_start, *page_at1, *page_at2, *page_finish;
  GtkWidget *hbox, *frame, *align, *bigvbox;
  at_druid_info *info;
  struct tm *tp;
  time_t t;
  char *fname;
  GdkPixbuf *logo = NULL;

  fname = gnome_pixmap_file("gnome-clock.png");
  if (fname)
    logo = gdk_pixbuf_new_from_file(fname, NULL);
  g_free(fname);

  time(&t);
  tp = localtime(&t);

  info = g_new(at_druid_info, 1);
  info->win = win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(win), "New One-Time Job");
  druid = gnome_druid_new();

  page_start = gnome_druid_page_edge_new_with_vals(
    GNOME_EDGE_START, FALSE,
    "Create new one-time job",
    "This series of dialogs will step\n"
    "you through the process of creating\n"
    "a new one-time job.", logo, NULL, NULL);
  page_at1 = gnome_druid_page_standard_new_with_vals(
    "Choose a time and date", logo, NULL);
  page_at2 = gnome_druid_page_standard_new_with_vals(
    "Select command", logo, NULL);
  page_finish = gnome_druid_page_edge_new_with_vals(
    GNOME_EDGE_FINISH, FALSE,
    "Confirm new job",
    "Please confirm that the new job\n"
    "described here is correct, and\n"
    "click the \"Finish\" button.", logo, NULL, NULL);

  bigvbox = gtk_vbox_new(FALSE, 0);
  align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_container_add(GTK_CONTAINER(align), bigvbox);
  gtk_box_pack_start(GTK_BOX(GNOME_DRUID_PAGE_STANDARD(page_at1)->vbox),
    align, TRUE, TRUE, 0);

  hbox = gtk_hbox_new(FALSE, 0);
  frame = gtk_frame_new("Date");
  gtk_container_set_border_width(GTK_CONTAINER(frame), 8);
  gtk_box_pack_start(GTK_BOX(bigvbox), frame, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), hbox);

  info->month = make_new_spinner("Month:", tp->tm_mon+1, 1, 12, 5, hbox);
  info->day = make_new_spinner("Day:", tp->tm_mday, 1, 31, 5, hbox);
  info->year =
    make_new_spinner("Year:", tp->tm_year+1900, 1900, 2100, 10, hbox);

  hbox = gtk_hbox_new(FALSE, 0);
  frame = gtk_frame_new("Time");
  gtk_container_set_border_width(GTK_CONTAINER(frame), 8);
  gtk_box_pack_start(GTK_BOX(bigvbox), frame, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), hbox);

  info->hour = make_new_spinner("Hour:", tp->tm_hour, 0, 23, 5, hbox);
  info->minute = make_new_spinner("Minute:", tp->tm_min, 0, 59, 5, hbox);

  info->command = make_job_page(GNOME_DRUID_PAGE_STANDARD(page_at2)->vbox);

  gtk_signal_connect_object(GTK_OBJECT(page_start), "cancel",
    GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)win);
  gtk_signal_connect_object(GTK_OBJECT(page_at1), "cancel",
    GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)win);
  gtk_signal_connect_object(GTK_OBJECT(page_at2), "cancel",
    GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)win);
  gtk_signal_connect_object(GTK_OBJECT(page_finish), "finish",
    GTK_SIGNAL_FUNC(at_commit_job), (gpointer)info);
  gtk_signal_connect_object(GTK_OBJECT(page_finish), "cancel",
    GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)win);
  gtk_signal_connect_object(GTK_OBJECT(win), "destroy",
    GTK_SIGNAL_FUNC(g_free), (gpointer)info);

  gtk_container_add(GTK_CONTAINER(win), druid);
  gnome_druid_append_page(GNOME_DRUID(druid), GNOME_DRUID_PAGE(page_start));
  gnome_druid_append_page(GNOME_DRUID(druid), GNOME_DRUID_PAGE(page_at1));
  gnome_druid_append_page(GNOME_DRUID(druid), GNOME_DRUID_PAGE(page_at2));
  gnome_druid_append_page(GNOME_DRUID(druid), GNOME_DRUID_PAGE(page_finish));
  gnome_druid_set_page(GNOME_DRUID(druid), GNOME_DRUID_PAGE(page_start));
  gtk_window_set_modal(GTK_WINDOW(win), TRUE);
  gtk_widget_show_all(win);

  gdk_pixbuf_unref(logo);

  return NULL;
}
