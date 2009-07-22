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

#include <gnome.h>
#include "at.h"
#include "generic_new.h"

typedef struct {
  GtkWidget *cal, *hour, *minute, *command, *win;
} at_druid_info;

#define EGET(name) gtk_entry_get_text(GTK_ENTRY(info->name))
#define SGET(name) gtk_spin_button_get_value_as_int(\
  GTK_SPIN_BUTTON(info->name))

static gboolean at_commit_job(at_druid_info *info) {
  const char *t;
  guint month, day, year;
  at_job_t *job = NULL;
  t = EGET(command);
  if (!t || !*t) return FALSE;
  job = g_new(at_job_t, 1);
  job->cmd = strdup(t);
  job->id = job->queue = job->when = NULL;

  gtk_calendar_get_date(GTK_CALENDAR(info->cal), &year, &month, &day);
  job->when = g_strdup_printf("%02d%02d %02d/%02d/%04d",
    SGET(hour), SGET(minute), month+1, day, year);

  gtk_widget_destroy(info->win);
  at_add_job(job);
  return TRUE;
}

static gboolean prepare_last_page(GnomeDruidPageEdge *pg, GtkWidget *ign,
    at_druid_info *info) {
  char *desc;
  const char *cmd = EGET(command);
  guint month, day, year;
  if (!cmd || !*cmd) {
    gnome_druid_page_edge_set_text(pg,
      "No command set.  Click \"Back\" to set one.");
    return TRUE;
  }
  gtk_calendar_get_date(GTK_CALENDAR(info->cal), &year, &month, &day);
  desc = g_strdup_printf("When: %02d/%02d/%04d at %02d:%02d\nWhat: %s\n\n"
    "If this is correct, click \"Apply\"", month+1, day, year,
    SGET(hour), SGET(minute), EGET(command));
  gnome_druid_page_edge_set_text(pg, desc);
  g_free(desc);
  return TRUE;
}

at_job_t *make_new_at_job() {
  GtkWidget *druid, *page_start, *page_at1, *page_at2, *page_finish;
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
  druid = gnome_druid_new_with_window("New One-Time Job",
    NULL, TRUE, &info->win);

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
    "Confirm new job", NULL, logo, NULL, NULL);

  bigvbox = gtk_vbox_new(FALSE, 0);
  align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_container_add(GTK_CONTAINER(align), bigvbox);
  gtk_box_pack_start(GTK_BOX(GNOME_DRUID_PAGE_STANDARD(page_at1)->vbox),
    align, TRUE, TRUE, 0);

  info->cal = gtk_calendar_new();
  gtk_box_pack_start(GTK_BOX(bigvbox), info->cal, FALSE, FALSE, 0);

  hbox = gtk_hbox_new(FALSE, 0);
  frame = gtk_frame_new("Time");
  gtk_container_set_border_width(GTK_CONTAINER(frame), 8);
  gtk_box_pack_start(GTK_BOX(bigvbox), frame, FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), hbox);

  info->hour = make_new_spinner("Hour:", tp->tm_hour, 0, 23, 5, hbox);
  info->minute = make_new_spinner("Minute:", tp->tm_min, 0, 59, 5, hbox);

  info->command = make_job_page(GNOME_DRUID_PAGE_STANDARD(page_at2)->vbox);

  gtk_signal_connect_object(GTK_OBJECT(page_finish), "finish",
    GTK_SIGNAL_FUNC(at_commit_job), (gpointer)info);
  gtk_signal_connect_object(GTK_OBJECT(info->win), "destroy",
    GTK_SIGNAL_FUNC(g_free), (gpointer)info);
  gtk_signal_connect(GTK_OBJECT(page_finish), "prepare",
    GTK_SIGNAL_FUNC(prepare_last_page), (gpointer)info);
  gtk_signal_connect(GTK_OBJECT(info->command), "changed",
    GTK_SIGNAL_FUNC(cmd_changed), (gpointer)druid);

  gnome_druid_append_page(GNOME_DRUID(druid), GNOME_DRUID_PAGE(page_start));
  gnome_druid_append_page(GNOME_DRUID(druid), GNOME_DRUID_PAGE(page_at1));
  gnome_druid_append_page(GNOME_DRUID(druid), GNOME_DRUID_PAGE(page_at2));
  gnome_druid_append_page(GNOME_DRUID(druid), GNOME_DRUID_PAGE(page_finish));
  gnome_druid_set_page(GNOME_DRUID(druid), GNOME_DRUID_PAGE(page_start));
  gtk_window_set_modal(GTK_WINDOW(info->win), TRUE);
  gtk_widget_show_all(info->win);

  gdk_pixbuf_unref(logo);

  return NULL;
}
