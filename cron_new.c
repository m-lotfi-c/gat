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
#include "cron.h"
#include "generic_new.h"

enum { MINUTE, HOUR, DAY, WEEK, MONTH, YEAR, CUSTOM };

typedef struct {
  GtkWidget *rb_minute, *rb_hour, *rb_day, *rb_week, *rb_month, *rb_year;
  GtkWidget *rb_custom;
  GtkWidget *dp_minute, *dp_hour, *dp_day, *dp_week, *dp_month, *dp_year;
  GtkWidget *dp_custom, *dp_pick, *dp_start, *dp_job, *dp_finish;

  GtkWidget *command;

  GtkWidget *minute_minute;

  GtkWidget *hour_hour, *hour_minute;
  
  GtkWidget *day_hour, *day_minute;
  
  GtkWidget *week_wday[7], *week_hour, *week_minute;

  GtkWidget *month_day, *month_hour, *month_minute;

  GtkWidget *year_month, *year_day, *year_hour, *year_minute;

  GtkWidget *custom_minute, *custom_hour, *custom_mday, *custom_month;
  GtkWidget *custom_wday;

  GtkWidget *win;

  int which;
} cron_druid_info;

static gboolean druid_simple_next_callback(GnomeDruidPage *page,
    GnomeDruid *druid, GnomeDruidPage *next) {
  gtk_object_set_data(GTK_OBJECT(next), "back", page);
  gnome_druid_set_page(druid, GNOME_DRUID_PAGE(next));
  return TRUE;
}

static gboolean druid_back_callback(GnomeDruidPage *page, GnomeDruid *druid) {
  GtkWidget *back = gtk_object_get_data(GTK_OBJECT(page), "back");
  if (back != NULL) {
    gtk_object_set_data(GTK_OBJECT(page), "back", NULL);
    gnome_druid_set_page(druid, GNOME_DRUID_PAGE(back));
    return TRUE;
  }
  else
    return FALSE;
}

#define EGET(name) gtk_entry_get_text(GTK_ENTRY(info->name))
#define SGET(name) gtk_spin_button_get_value_as_int(\
  GTK_SPIN_BUTTON(info->name))
#define TGET(name) gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(info->name))

static gboolean cron_commit_job(cron_druid_info *info) {
  const char *t;
  char buf[32];
  int last, i;
  cron_job_t *job = NULL;
  t = EGET(command);
  if (!t || !*t) goto error_out;
  job = g_new(cron_job_t, 1);
  job->cmd = g_strdup(t);
  job->minute = job->hour = job->monthday = job->month = job->weekday = NULL;
  switch (info->which) {
    case MINUTE:
      t = EGET(minute_minute);
      if (atoi(t) == 0)
        job->minute = g_strdup("*");
      else {
        sprintf(buf, "*/%d", atoi(t));
        job->minute = g_strdup(buf);
      }
      job->hour = g_strdup("*");
      job->monthday = g_strdup("*");
      job->month = g_strdup("*");
      job->weekday = g_strdup("*");
      break;
    case HOUR:
      sprintf(buf, "%d", SGET(hour_minute));
      job->minute = g_strdup(buf);
      t = EGET(hour_hour);
      if (atoi(t) == 0)
        job->hour = g_strdup("*");
      else {
        sprintf(buf, "*/%d", atoi(t));
        job->hour = g_strdup(buf);
      }
      job->monthday = g_strdup("*");
      job->month = g_strdup("*");
      job->weekday = g_strdup("*");
      break;
    case DAY:
      sprintf(buf, "%d", SGET(day_minute));
      job->minute = g_strdup(buf);
      sprintf(buf, "%d", SGET(day_hour));
      job->hour = g_strdup(buf);
      job->monthday = g_strdup("*");
      job->month = g_strdup("*");
      job->weekday = g_strdup("*");
      break;
    case WEEK:
      sprintf(buf, "%d", SGET(week_minute));
      job->minute = g_strdup(buf);
      sprintf(buf, "%d", SGET(week_hour));
      job->hour = g_strdup(buf);
      job->monthday = g_strdup("*");
      job->month = g_strdup("*");
      buf[0] = '\0';
      last = -1;
      for (i=0; i<7; i++)
        if (TGET(week_wday[i])) {
          if (last == -1) {
            sprintf(buf+strlen(buf), "%d", i);
            last = i;
          }
        }
        else {
          if (last != -1) {
            if (last == i-1)
              sprintf(buf+strlen(buf), ",");
            else if (last == i-2)
              sprintf(buf+strlen(buf), ",%d,", i-1);
            else
              sprintf(buf+strlen(buf), "-%d,", i-1);
            last = -1;
          }
        }
      if (last != -1) {
        if (last == 6)
          sprintf(buf+strlen(buf), ",");
        else if (last == 5)
          sprintf(buf+strlen(buf), ",6,");
        else
          sprintf(buf+strlen(buf), "-6,");
        last = -1;
      }
      if (!buf[0]) goto error_out; /* no days selected */
      buf[strlen(buf)-1] = '\0';   /* remove trailing comma */
      job->weekday = g_strdup(buf);
      break;
    case MONTH:
      sprintf(buf, "%d", SGET(month_minute));
      job->minute = g_strdup(buf);
      sprintf(buf, "%d", SGET(month_hour));
      job->hour = g_strdup(buf);
      sprintf(buf, "%d", atoi(EGET(month_day)));
      job->monthday = g_strdup(buf);
      job->month = g_strdup("*");
      job->weekday = g_strdup("*");
      break;
    case YEAR:
      sprintf(buf, "%d", SGET(year_minute));
      job->minute = g_strdup(buf);
      sprintf(buf, "%d", SGET(year_hour));
      job->hour = g_strdup(buf);
      sprintf(buf, "%d", atoi(EGET(year_day)));
      job->monthday = g_strdup(buf);
      job->month = NULL;
      t = EGET(year_month);
      for (i=0; i<12; i++)
        if (!strcmp(t, month_name[i])) {
          sprintf(buf, "%d", i+1);
          job->month = g_strdup(buf);
          break;
        }
      if (job->month == NULL) goto error_out;  /* month did not match */
      job->weekday = g_strdup("*");
      break;
    case CUSTOM:
      job->minute = g_strdup(EGET(custom_minute));
      job->hour = g_strdup(EGET(custom_hour));
      job->monthday = g_strdup(EGET(custom_mday));
      job->month = g_strdup(EGET(custom_month));
      job->weekday = g_strdup(EGET(custom_wday));
      break;
    default:
      fprintf(stderr, "Warning: info->which is invalid: %d\n", info->which);
      goto error_out;
  }
  gtk_widget_destroy(info->win);
  cron_add_job(job);
  return TRUE;

error_out:
  if (job) {
    if (job->cmd) g_free(job->cmd);
    if (job->minute) g_free(job->minute);
    if (job->hour) g_free(job->hour);
    if (job->monthday) g_free(job->monthday);
    if (job->month) g_free(job->month);
    if (job->weekday) g_free(job->weekday);
    g_free(job);
  }
  return FALSE;
}

static gboolean druid_next_callback(GnomeDruidPage *page, GnomeDruid *druid,
    cron_druid_info *info) {
  GtkWidget *next = NULL;
  g_return_val_if_fail(page == GNOME_DRUID_PAGE(info->dp_pick), FALSE);
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(info->rb_minute))) {
    info->which = MINUTE;
    next = info->dp_minute;
  }
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(info->rb_hour))) {
    info->which = HOUR;
    next = info->dp_hour;
  }
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(info->rb_day))) {
    info->which = DAY;
    next = info->dp_day;
  }
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(info->rb_week))) {
    info->which = WEEK;
    next = info->dp_week;
  }
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(info->rb_month))) {
    info->which = MONTH;
    next = info->dp_month;
  }
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(info->rb_year))) {
    info->which = YEAR;
    next = info->dp_year;
  }
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(info->rb_custom))) {
    info->which = CUSTOM;
    next = info->dp_custom;
  }
  g_return_val_if_fail(next != NULL, FALSE);
  gtk_object_set_data(GTK_OBJECT(next), "back", page);
  gnome_druid_set_page(druid, GNOME_DRUID_PAGE(next));
  return TRUE;
}

cron_job_t *make_new_cron_job() {
  GtkWidget *win, *druid;
  cron_druid_info *info;
  char *fname;
  GdkPixbuf *logo = NULL;
  GSList *radio_group;
  GtkWidget *vbox, *hbox, *hbox2, *align, *cb, *table;
  GList *cbitems;
  struct tm *tp;
  time_t t;
  int i;
  char buf[5];

  time(&t);
  tp = localtime(&t);

  fname = gnome_pixmap_file("gnome-clock.png");
  if (fname)
    logo = gdk_pixbuf_new_from_file(fname, NULL);
  g_free(fname);

  win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(win), "New Recurring Job");
  druid = gnome_druid_new();

  info = g_new(cron_druid_info, 1);
  info->win = win;
  info->dp_start = gnome_druid_page_edge_new_with_vals(
    GNOME_EDGE_START, FALSE,
    "Create new recurring job",
    "This series of dialogs will step\n"
    "you through the process of creating\n"
    "a new recurring job.", logo, NULL, NULL);
  info->dp_pick = gnome_druid_page_standard_new_with_vals(
    "Select job frequency", logo, NULL);
  info->dp_minute = gnome_druid_page_standard_new_with_vals(
    "Select job frequency", logo, NULL);
  info->dp_hour = gnome_druid_page_standard_new_with_vals(
    "Select job frequency", logo, NULL);
  info->dp_day = gnome_druid_page_standard_new_with_vals(
    "Select time", logo, NULL);
  info->dp_week = gnome_druid_page_standard_new_with_vals(
    "Select day(s) and time", logo, NULL);
  info->dp_month = gnome_druid_page_standard_new_with_vals(
    "Select day and time", logo, NULL);
  info->dp_year = gnome_druid_page_standard_new_with_vals(
    "Select date and time", logo, NULL);
  info->dp_custom = gnome_druid_page_standard_new_with_vals(
    "Enter crontab strings", logo, NULL);
  info->dp_job = gnome_druid_page_standard_new_with_vals(
    "Select command", logo, NULL);
  info->dp_finish = gnome_druid_page_edge_new_with_vals(
    GNOME_EDGE_FINISH, FALSE,
    "Confirm new job",
    "Please confirm that the new job\n"
    "described here is correct, and\n"
    "click the \"Finish\" button.", logo, NULL, NULL);

  gtk_signal_connect_object(GTK_OBJECT(info->dp_start), "cancel",
    GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)win);
  gnome_druid_append_page(GNOME_DRUID(druid),
    GNOME_DRUID_PAGE(info->dp_start));


  /******************************************************/
  /* dp_pick: ask the user how often their job will run */
  /******************************************************/

  vbox = gtk_vbox_new(FALSE, 0);
  align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_container_add(GTK_CONTAINER(align), vbox);
  gtk_box_pack_start(GTK_BOX(GNOME_DRUID_PAGE_STANDARD(info->dp_pick)->vbox),
    align, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox),
    gtk_label_new("How often will your job run?"), FALSE, FALSE, 0);

  radio_group = NULL;
  info->rb_minute = make_new_radiobutton("Every minute or every few minutes",
    0, &radio_group, vbox);
  info->rb_hour = make_new_radiobutton("Every hour or every few hours",
    0, &radio_group, vbox);
  info->rb_day = make_new_radiobutton("Every day", 1, &radio_group, vbox);
  info->rb_week = make_new_radiobutton("One or more days each week", 0,
    &radio_group, vbox);
  info->rb_month = make_new_radiobutton("Every month", 0, &radio_group, vbox);
  info->rb_year = make_new_radiobutton("Every year", 0, &radio_group, vbox);
  info->rb_custom = make_new_radiobutton("Custom", 0, &radio_group, vbox);

  gtk_signal_connect_object(GTK_OBJECT(info->dp_pick), "cancel",
    GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)win);
  gtk_signal_connect(GTK_OBJECT(info->dp_pick), "next",
    GTK_SIGNAL_FUNC(druid_next_callback), (gpointer)info);
  gnome_druid_append_page(GNOME_DRUID(druid),
    GNOME_DRUID_PAGE(info->dp_pick));


  /**************************************/
  /* dp_minute: it runs every N minutes */
  /**************************************/
  
  hbox = gtk_hbox_new(FALSE, 5);
  align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_container_add(GTK_CONTAINER(align), hbox);
  gtk_box_pack_start(GTK_BOX(GNOME_DRUID_PAGE_STANDARD(info->dp_minute)->vbox),
    align, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox),
    gtk_label_new("Run job every"), FALSE, FALSE, 0);

  cbitems = NULL;
  cbitems = g_list_append(cbitems, "minute");
  cbitems = g_list_append(cbitems, "2 minutes");
  cbitems = g_list_append(cbitems, "3 minutes");
  cbitems = g_list_append(cbitems, "4 minutes");
  cbitems = g_list_append(cbitems, "5 minutes");
  cbitems = g_list_append(cbitems, "6 minutes");
  cbitems = g_list_append(cbitems, "10 minutes");
  cbitems = g_list_append(cbitems, "12 minutes");
  cbitems = g_list_append(cbitems, "15 minutes");
  cbitems = g_list_append(cbitems, "20 minutes");
  cbitems = g_list_append(cbitems, "30 minutes");
  cb = gtk_combo_new();
  gtk_combo_set_popdown_strings(GTK_COMBO(cb), cbitems);
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(cb)->entry), "minute");
  gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(cb)->entry), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), cb, FALSE, FALSE, 0);
  info->minute_minute = GTK_COMBO(cb)->entry;
  gtk_box_pack_start(GTK_BOX(hbox),
    gtk_label_new("."), FALSE, FALSE, 0);

  gtk_signal_connect_object(GTK_OBJECT(info->dp_minute), "cancel",
    GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)win);
  gtk_signal_connect(GTK_OBJECT(info->dp_minute), "next",
    GTK_SIGNAL_FUNC(druid_simple_next_callback), (gpointer)info->dp_job);
  gtk_signal_connect(GTK_OBJECT(info->dp_minute), "back",
    GTK_SIGNAL_FUNC(druid_back_callback), NULL);
  gnome_druid_append_page(GNOME_DRUID(druid),
    GNOME_DRUID_PAGE(info->dp_minute));

  
  /**********************************/
  /* dp_hour: it runs every N hours */
  /**********************************/
  
  hbox = gtk_hbox_new(FALSE, 5);
  align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_container_add(GTK_CONTAINER(align), hbox);
  gtk_box_pack_start(GTK_BOX(GNOME_DRUID_PAGE_STANDARD(info->dp_hour)->vbox),
    align, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox),
    gtk_label_new("Run job every"), FALSE, FALSE, 0);

  cbitems = NULL;
  cbitems = g_list_append(cbitems, "hour");
  cbitems = g_list_append(cbitems, "2 hours");
  cbitems = g_list_append(cbitems, "3 hours");
  cbitems = g_list_append(cbitems, "4 hours");
  cbitems = g_list_append(cbitems, "6 hours");
  cbitems = g_list_append(cbitems, "8 hours");
  cbitems = g_list_append(cbitems, "12 hours");
  cb = gtk_combo_new();
  gtk_combo_set_popdown_strings(GTK_COMBO(cb), cbitems);
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(cb)->entry), "hour");
  gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(cb)->entry), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), cb, FALSE, FALSE, 0);
  info->hour_hour = GTK_COMBO(cb)->entry;
  gtk_box_pack_start(GTK_BOX(hbox),
    gtk_label_new("at"), FALSE, FALSE, 0);
  info->hour_minute = make_new_spinner(NULL, 0, 0, 59, 5, hbox);
  gtk_box_pack_start(GTK_BOX(hbox),
    gtk_label_new("minute(s) past the hour."), FALSE, FALSE, 0);

  gtk_signal_connect_object(GTK_OBJECT(info->dp_hour), "cancel",
    GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)win);
  gtk_signal_connect(GTK_OBJECT(info->dp_hour), "next",
    GTK_SIGNAL_FUNC(druid_simple_next_callback), (gpointer)info->dp_job);
  gtk_signal_connect(GTK_OBJECT(info->dp_hour), "back",
    GTK_SIGNAL_FUNC(druid_back_callback), NULL);
  gnome_druid_append_page(GNOME_DRUID(druid),
    GNOME_DRUID_PAGE(info->dp_hour));


  /*****************************/
  /* dp_day: it runs every day */
  /*****************************/
  
  hbox = gtk_hbox_new(FALSE, 5);
  align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_container_add(GTK_CONTAINER(align), hbox);
  gtk_box_pack_start(GTK_BOX(GNOME_DRUID_PAGE_STANDARD(info->dp_day)->vbox),
    align, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox),
    gtk_label_new("Run job every day at"), FALSE, FALSE, 0);
  info->day_hour = make_new_spinner(NULL, tp->tm_hour, 0, 23, 5, hbox);
  info->day_minute = make_new_spinner(NULL, tp->tm_min, 0, 59, 5, hbox);
  gtk_box_pack_start(GTK_BOX(hbox),
    gtk_label_new("."), FALSE, FALSE, 0);

  gtk_signal_connect_object(GTK_OBJECT(info->dp_day), "cancel",
    GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)win);
  gtk_signal_connect(GTK_OBJECT(info->dp_day), "next",
    GTK_SIGNAL_FUNC(druid_simple_next_callback), (gpointer)info->dp_job);
  gtk_signal_connect(GTK_OBJECT(info->dp_day), "back",
    GTK_SIGNAL_FUNC(druid_back_callback), NULL);
  gnome_druid_append_page(GNOME_DRUID(druid),
    GNOME_DRUID_PAGE(info->dp_day));
  
  
  /****************************************************/
  /* dp_week: it runs on one or more days of the week */
  /****************************************************/
  
  hbox = gtk_hbox_new(FALSE, 5);
  vbox = gtk_vbox_new(FALSE, 0);
  align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_container_add(GTK_CONTAINER(align), vbox);
  gtk_box_pack_start(GTK_BOX(GNOME_DRUID_PAGE_STANDARD(info->dp_week)->vbox),
    align, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox),
    gtk_label_new("Run job on"), FALSE, FALSE, 0);
  for (i=0; i<7; i++)
    info->week_wday[i] = make_new_checkbutton(week_name[i], 0, vbox);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox),
    gtk_label_new("at"), FALSE, FALSE, 0);

  info->week_hour = make_new_spinner(NULL, tp->tm_hour, 0, 23, 5, hbox);
  info->week_minute = make_new_spinner(NULL, tp->tm_min, 0, 59, 5, hbox);
  gtk_box_pack_start(GTK_BOX(hbox),
    gtk_label_new("."), FALSE, FALSE, 0);

  gtk_signal_connect_object(GTK_OBJECT(info->dp_week), "cancel",
    GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)win);
  gtk_signal_connect(GTK_OBJECT(info->dp_week), "next",
    GTK_SIGNAL_FUNC(druid_simple_next_callback), (gpointer)info->dp_job);
  gtk_signal_connect(GTK_OBJECT(info->dp_week), "back",
    GTK_SIGNAL_FUNC(druid_back_callback), NULL);
  gnome_druid_append_page(GNOME_DRUID(druid),
    GNOME_DRUID_PAGE(info->dp_week));
  
  
  /************************************/
  /* dp_month: it runs once per month */
  /************************************/
  
  hbox = gtk_hbox_new(FALSE, 5);
  align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_container_add(GTK_CONTAINER(align), hbox);
  gtk_box_pack_start(GTK_BOX(GNOME_DRUID_PAGE_STANDARD(info->dp_month)->vbox),
    align, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox),
    gtk_label_new("Run job on the"), FALSE, FALSE, 0);
  cbitems = NULL;
  for (i=1; i<=31; i++) {
    sprintf(buf, "%d%s", i, ordinal_ending(i));
    cbitems = g_list_append(cbitems, g_strdup(buf));
  }

  cb = gtk_combo_new();
  gtk_combo_set_popdown_strings(GTK_COMBO(cb), cbitems);
  sprintf(buf, "%d%s", tp->tm_mday, ordinal_ending(tp->tm_mday));
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(cb)->entry), buf);
  gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(cb)->entry), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), cb, FALSE, FALSE, 0);
  info->month_day = GTK_COMBO(cb)->entry;
  gtk_box_pack_start(GTK_BOX(hbox),
    gtk_label_new("of each month at"), FALSE, FALSE, 0);
  info->month_hour = make_new_spinner(NULL, tp->tm_hour, 0, 23, 5, hbox);
  info->month_minute = make_new_spinner(NULL, tp->tm_min, 0, 59, 5, hbox);
  gtk_box_pack_start(GTK_BOX(hbox),
    gtk_label_new("."), FALSE, FALSE, 0);

  gtk_signal_connect_object(GTK_OBJECT(info->dp_month), "cancel",
    GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)win);
  gtk_signal_connect(GTK_OBJECT(info->dp_month), "next",
    GTK_SIGNAL_FUNC(druid_simple_next_callback), (gpointer)info->dp_job);
  gtk_signal_connect(GTK_OBJECT(info->dp_month), "back",
    GTK_SIGNAL_FUNC(druid_back_callback), NULL);
  gnome_druid_append_page(GNOME_DRUID(druid),
    GNOME_DRUID_PAGE(info->dp_month));
  
  
  /**********************************/
  /* dp_year: it runs once per year */
  /**********************************/
  
  hbox = gtk_hbox_new(FALSE, 5);
  hbox2 = gtk_hbox_new(FALSE, 5);
  vbox = gtk_vbox_new(FALSE, 5);
  align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_container_add(GTK_CONTAINER(align), vbox);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(GNOME_DRUID_PAGE_STANDARD(info->dp_year)->vbox),
    align, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox),
    gtk_label_new("Run job on "), FALSE, FALSE, 0);
  cbitems = NULL;
  for (i=0; i<12; i++)
    cbitems = g_list_append(cbitems, month_name[i]);
  cb = gtk_combo_new();
  gtk_combo_set_popdown_strings(GTK_COMBO(cb), cbitems);
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(cb)->entry), month_name[tp->tm_mon]);
  gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(cb)->entry), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), cb, FALSE, FALSE, 0);
  info->year_month = GTK_COMBO(cb)->entry;
  cbitems = NULL;
  for (i=1; i<=31; i++) {
    sprintf(buf, "%d%s", i, ordinal_ending(i));
    cbitems = g_list_append(cbitems, g_strdup(buf));
  }

  cb = gtk_combo_new();
  gtk_combo_set_popdown_strings(GTK_COMBO(cb), cbitems);
  sprintf(buf, "%d%s", tp->tm_mday, ordinal_ending(tp->tm_mday));
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(cb)->entry), buf);
  gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(cb)->entry), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), cb, FALSE, FALSE, 0);
  info->year_day = GTK_COMBO(cb)->entry;
  gtk_box_pack_start(GTK_BOX(hbox2),
    gtk_label_new("of each year at"), FALSE, FALSE, 0);
  info->year_hour = make_new_spinner(NULL, tp->tm_hour, 0, 23, 5, hbox2);
  info->year_minute = make_new_spinner(NULL, tp->tm_min, 0, 59, 5, hbox2);
  gtk_box_pack_start(GTK_BOX(hbox2),
    gtk_label_new("."), FALSE, FALSE, 0);

  gtk_signal_connect_object(GTK_OBJECT(info->dp_year), "cancel",
    GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)win);
  gtk_signal_connect(GTK_OBJECT(info->dp_year), "next",
    GTK_SIGNAL_FUNC(druid_simple_next_callback), (gpointer)info->dp_job);
  gtk_signal_connect(GTK_OBJECT(info->dp_year), "back",
    GTK_SIGNAL_FUNC(druid_back_callback), NULL);
  gnome_druid_append_page(GNOME_DRUID(druid),
    GNOME_DRUID_PAGE(info->dp_year));
  
  
  /****************************************************/
  /* dp_custom: let the user specify raw cron strings */
  /****************************************************/
  
  vbox = gtk_vbox_new(FALSE, 5);
  align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_container_add(GTK_CONTAINER(align), vbox);
  gtk_box_pack_start(GTK_BOX(GNOME_DRUID_PAGE_STANDARD(info->dp_custom)->vbox),
    align, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(
    "Enter frequency strings as described\nin the crontab(5) man page."),
    FALSE, FALSE, 0);
  table = gtk_table_new(5, 2, FALSE);
  gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);
  gtk_table_attach(GTK_TABLE(table), gtk_label_new("Minute"),
    0, 1, 0, 1, 0, 0, 5, 0);
  info->custom_minute = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(info->custom_minute), "*");
  gtk_table_attach(GTK_TABLE(table), info->custom_minute,
    1, 2, 0, 1, 0, 0, 0, 0);
  gtk_table_attach(GTK_TABLE(table), gtk_label_new("Hour"),
    0, 1, 1, 2, 0, 0, 5, 0);
  info->custom_hour = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(info->custom_hour), "*");
  gtk_table_attach(GTK_TABLE(table), info->custom_hour,
    1, 2, 1, 2, 0, 0, 0, 0);
  gtk_table_attach(GTK_TABLE(table), gtk_label_new("Day of month"),
    0, 1, 2, 3, 0, 0, 5, 0);
  info->custom_mday = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(info->custom_mday), "*");
  gtk_table_attach(GTK_TABLE(table), info->custom_mday,
    1, 2, 2, 3, 0, 0, 0, 0);
  gtk_table_attach(GTK_TABLE(table), gtk_label_new("Month"),
    0, 1, 3, 4, 0, 0, 5, 0);
  info->custom_month = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(info->custom_month), "*");
  gtk_table_attach(GTK_TABLE(table), info->custom_month,
    1, 2, 3, 4, 0, 0, 0, 0);
  gtk_table_attach(GTK_TABLE(table), gtk_label_new("Day of week"),
    0, 1, 4, 5, 0, 0, 5, 0);
  info->custom_wday = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(info->custom_wday), "*");
  gtk_table_attach(GTK_TABLE(table), info->custom_wday,
    1, 2, 4, 5, 0, 0, 0, 0);

  gtk_signal_connect_object(GTK_OBJECT(info->dp_custom), "cancel",
    GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)win);
  gtk_signal_connect(GTK_OBJECT(info->dp_custom), "next",
    GTK_SIGNAL_FUNC(druid_simple_next_callback), (gpointer)info->dp_job);
  gtk_signal_connect(GTK_OBJECT(info->dp_custom), "back",
    GTK_SIGNAL_FUNC(druid_back_callback), NULL);
  gnome_druid_append_page(GNOME_DRUID(druid),
    GNOME_DRUID_PAGE(info->dp_custom));
  
  
  /*************************************/
  /* dp_job: select the command to run */
  /*************************************/
  
  info->command = make_job_page(
    GNOME_DRUID_PAGE_STANDARD(info->dp_job)->vbox);

  gtk_signal_connect_object(GTK_OBJECT(info->dp_job), "cancel",
    GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)win);
  gtk_signal_connect(GTK_OBJECT(info->dp_job), "back",
    GTK_SIGNAL_FUNC(druid_back_callback), NULL);
  gnome_druid_append_page(GNOME_DRUID(druid),
    GNOME_DRUID_PAGE(info->dp_job));

  gtk_signal_connect_object(GTK_OBJECT(info->dp_finish), "finish",
    GTK_SIGNAL_FUNC(cron_commit_job), (gpointer)info);
  gtk_signal_connect_object(GTK_OBJECT(info->dp_finish), "cancel",
    GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer)win);
  gtk_signal_connect_object(GTK_OBJECT(win), "destroy",
    GTK_SIGNAL_FUNC(g_free), (gpointer)info);
  gnome_druid_append_page(GNOME_DRUID(druid),
    GNOME_DRUID_PAGE(info->dp_finish));

  gtk_container_add(GTK_CONTAINER(win), druid);
  gnome_druid_set_page(GNOME_DRUID(druid),
    GNOME_DRUID_PAGE(info->dp_start));
  gtk_window_set_modal(GTK_WINDOW(win), TRUE);
  gtk_widget_show_all(win);

  gdk_pixbuf_unref(logo);

  return NULL;
}
