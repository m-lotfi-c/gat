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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <gtk/gtk.h>
#include "cron.h"
#include "gat.h"
#include "generic_new.h"

#define CRONTAB_PATH "/usr/bin/crontab"
#define MAX_CRON_LENGTH 1024
#define BLOCK_SIZE 4096

GArray *cron_jobs;
static GtkWidget *cron_clist;

GArray *get_cron_jobs() {
  GArray *j;
  int p[2];
  pid_t pid;
  FILE *fp;
  char buf[MAX_CRON_LENGTH];
  
  if (pipe(p) < 0) { perror("pipe"); return NULL; }
  switch ((pid = fork())) {
    case -1:  perror("fork"); return NULL;
    case 0:
      close(p[0]);
      dup2(p[1], STDOUT_FILENO);
      close(p[1]);
      execl(CRONTAB_PATH, CRONTAB_PATH, "-l", NULL);
      perror(CRONTAB_PATH);
      return NULL;
    default:
      close(p[1]);
      fp = fdopen(p[0], "r");
      j = g_array_new(0, 0, sizeof(cron_job_t*));
      while (fgets(buf, MAX_CRON_LENGTH, fp)) {
        cron_job_t *job;
        char *p;
        if ((p = strchr(buf, '#')) != NULL) *p = '\0';
        if ((p = strchr(buf, '\n')) != NULL) *p = '\0';
        if (!buf[0]) continue;
        job = g_new(cron_job_t, 1);
        job->minute = g_strdup(strtok(buf, " \t"));
        job->hour = g_strdup(strtok(NULL, " \t"));
        job->monthday = g_strdup(strtok(NULL, " \t"));
        job->month = g_strdup(strtok(NULL, " \t"));
        job->weekday = g_strdup(strtok(NULL, " \t"));
        p = strtok(NULL, "");
        while (*p == ' ' || *p == '\t') p++;
        job->cmd = g_strdup(p);
        g_array_append_val(j, job);
      }
      close(p[0]);
      waitpid(pid, 0, 0);
      break;
  }
  return j;
}

#define HOUR(n) ((((n)+11) % 12) + 1)
#define AMPM(n) (((n)>=12)?"pm":"am")

char *cron_make_description(cron_job_t *j) {
  int hour = atoi(j->hour), minute = atoi(j->minute);

  if (strchr(j->minute, ',') || strchr(j->hour, ',') ||
      strchr(j->monthday, ',') || strchr(j->month, ',') ||
      strchr(j->minute, '-') || strchr(j->hour, '-') ||
      strchr(j->monthday, '-') || strchr(j->month, '-')) goto punt;
  
  /* note: in the comments below, | should be /, but that screws up the
   * parser... */

  /* every N minutes: "*|N * * * *"  */
  if (j->minute[0] == '*' && !strcmp(j->hour, "*") &&
      !strcmp(j->monthday, "*") && !strcmp(j->month, "*") &&
      !strcmp(j->weekday, "*")) {
    int n = (j->minute[1] == '/') ? atoi(j->minute+2) : 1;
    if (n == 1)
      return g_strdup("Every minute");
    else
      return g_strdup_printf("Every %d minutes", n);
  }

  /* every hour: "N *|N * * *"  */
  if (j->minute[0] != '*' && j->hour[0] == '*' &&
      !strcmp(j->monthday, "*") && !strcmp(j->month, "*") &&
      !strcmp(j->weekday, "*")) {
    int n = (j->hour[1] == '/') ? atoi(j->hour+2) : 1;
    if (n == 1)
      return g_strdup_printf("Every hour at %d past the hour", minute);
    else
      return g_strdup_printf("Every %d hours at %d past the hour", n, minute);
  }

  /* daily: "N N *|N * *"  */
  if (j->minute[0] != '*' && j->hour[0] != '*' &&
      j->monthday[0] == '*' && !strcmp(j->month, "*") &&
      !strcmp(j->weekday, "*")) {
    int n = (j->monthday[1] == '/') ? atoi(j->monthday+2) : 1;
    if (n == 1)
      return g_strdup_printf("Every day at %d:%02d%s",
        HOUR(hour), minute, AMPM(hour));
    else
      return g_strdup_printf("Every %d days at %d:%02d%s",
        n, HOUR(hour), minute, AMPM(hour));
  }

  /* weekly: "N N * * N,N-N,N"  */
  if (j->minute[0] != '*' && j->hour[0] != '*' &&
      !strcmp(j->monthday, "*") && !strcmp(j->month, "*") &&
      j->weekday[0] != '*') {
    char *p, *ret;
    int commas = 0;
    GString *str = g_string_new("Every ");
    for (p=j->weekday; *p; p++)
      switch (*p) {
        case ',':
          if (strchr(p+1, ','))
            g_string_append(str, ", ");
          else if (commas == 0)
            g_string_append(str, " and ");
          else
            g_string_append(str, ", and ");
          commas++;
          break;
        case '-':
          g_string_append(str, " through ");
          break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
          g_string_append(str, week_name[(*p-'0') % 7]);
          break;
        default:
          fprintf(stderr, "WARNING: invalid character '%c'"
            "in cron wday string\n", *p);
          break;
      }
    g_string_append_printf(str, " at %d:%02d%s",
      HOUR(hour), minute, AMPM(hour));
    ret = str->str;
    g_string_free(str, FALSE);
    return ret;
  }

  /* every N months: "N N N *|N *"  */
  if (j->minute[0] != '*' && j->hour[0] != '*' &&
      j->monthday[0] != '*' && j->month[0] == '*' &&
      !strcmp(j->weekday, "*")) {
    int n = (j->month[1] == '/') ? atoi(j->month+2) : 1;
    if (n == 1)
      return g_strdup_printf("On the %s%s of every month at %d:%02d%s",
        j->monthday, ordinal_ending(atoi(j->monthday)),
        HOUR(hour), minute, AMPM(hour));
    else
      return g_strdup_printf("On the %s%s of every %d%s month at %d:%02d%s",
        j->monthday, ordinal_ending(atoi(j->monthday)), n,
        ordinal_ending(n), HOUR(hour), minute, AMPM(hour));
  }

  /* every year: "N N N N *"  */
  if (j->minute[0] != '*' && j->hour[0] != '*' &&
      j->monthday[0] != '*' && j->month[0] != '*' &&
      !strcmp(j->weekday, "*"))
    return g_strdup_printf("Every year on the %s%s of %s at %d:%02d%s",
      j->monthday, ordinal_ending(atoi(j->monthday)),
      month_name[atoi(j->month)-1], HOUR(hour), minute, AMPM(hour));

punt:
  /* punt! */
  return g_strdup_printf("%s %s %s %s %s",
    j->minute, j->hour, j->monthday, j->month, j->weekday);
}

void cron_clist_refresh(GtkCList *clist, GArray *j) {
  char *text[2];
  int i;
  for (i=0; i<j->len/*sizeof(cron_job_t*)*/; i++) {
    cron_job_t *job = g_array_index(j, cron_job_t*, i);
    char *t = cron_make_description(job);
    text[0] = t;
    text[1] = job->cmd;
    gtk_clist_append(GTK_CLIST(clist), text);
    g_free(t);
  }
}

void cron_save(GArray *arr) {
  int p[2], i;
  pid_t pid;
  char *buf;
  
  if (pipe(p) < 0) { perror("pipe"); return; }
  switch ((pid = fork())) {
    case -1:  perror("fork"); return;
    case 0:
      close(p[1]);
      dup2(p[0], STDIN_FILENO);
      close(p[0]);
      execl(CRONTAB_PATH, CRONTAB_PATH, "-", NULL);
      perror(CRONTAB_PATH);
      return;
    default:
      close(p[0]);
      for (i=0; i<arr->len/*sizeof(cron_job_t*)*/; i++) {
        cron_job_t *job = g_array_index(arr, cron_job_t*, i);
        if (!job) continue;
        buf = g_strdup_printf("%s %s %s %s %s %s\n", job->minute, job->hour,
          job->monthday, job->month, job->weekday, job->cmd);
        write(p[1], buf, strlen(buf));
        g_free(buf);
      }
      close(p[1]);
      waitpid(pid, 0, 0);
      break;
  }
}

void cron_free_job(cron_job_t *p) {
  g_free(p->minute);
  g_free(p->hour);
  g_free(p->monthday);
  g_free(p->month);
  g_free(p->weekday);
  g_free(p->cmd);
  g_free(p);
}

void cron_sync() {
  int i;
  cron_save(cron_jobs);
  for (i=0; i<cron_jobs->len/*sizeof(cron_job_t*)*/; i++) {
    cron_job_t *job = g_array_index(cron_jobs, cron_job_t*, i);
    if (job) cron_free_job(job);
  }
  g_array_free(cron_jobs, 1);
  cron_jobs = get_cron_jobs();
  gtk_clist_freeze(GTK_CLIST(cron_clist));
  for (i=GTK_CLIST(cron_clist)->rows-1; i>=0; i--)
    gtk_clist_remove(GTK_CLIST(cron_clist), i);
  gtk_clist_thaw(GTK_CLIST(cron_clist));
  cron_clist_refresh(GTK_CLIST(cron_clist), cron_jobs);
}

void cron_delete_callback(GtkWidget *clist) {
  GList *sel = GTK_CLIST(clist)->selection;
  while (sel) {
    cron_job_t *job = g_array_index(cron_jobs, cron_job_t*, GPOINTER_TO_INT(sel->data));
    cron_free_job(job);
    g_array_index(cron_jobs, cron_job_t*, GPOINTER_TO_INT(sel->data)) = NULL;
    sel = g_list_next(sel);
  }
  cron_sync();
}

void cron_test_callback(GtkWidget *clist) {
  GList *sel = GTK_CLIST(clist)->selection;
  while (sel) {
    int i, skip = 0;
    cron_job_t *job = g_array_index(cron_jobs, cron_job_t*, GPOINTER_TO_INT(sel->data));
    char *buf = g_strdup_printf("Run job: \"%s\" ?", job->cmd);
    for (i=0; ; i++) {
      while (buf[i+skip] == '\n') skip++;
      buf[i] = buf[i+skip];
      if (!buf[i]) break;
    }
    if (make_ok_cancel_dialog("Run job?", buf))
      run_job(job->cmd);
    g_free(buf);
    
    sel = g_list_next(sel);
  }
}

void cron_new_callback(GtkWidget *clist) {
  printf("New cron job!\n");
  make_new_cron_job();
}

void cron_add_job(cron_job_t *job) {
  g_array_append_val(cron_jobs, job);
  cron_sync();
}

GtkWidget *make_cron_page(GArray *j) {
  char *text[2] = { "When", "Job" };

  GtkWidget *clist, *vbox, *hbox, *button;
  cron_clist = clist = gtk_clist_new_with_titles(2, text);
  gtk_clist_set_selection_mode(GTK_CLIST(clist), GTK_SELECTION_MULTIPLE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 0, TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 1, TRUE);
  cron_clist_refresh(GTK_CLIST(clist), j);

  gtk_widget_set_usize(clist, 440, 300);  
  
  vbox = gtk_vbox_new(FALSE, 5);
  hbox = gtk_hbox_new(FALSE, 5);
  button = gtk_button_new_with_label("New job...");
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(cron_new_callback), (gpointer)clist);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 5);
  button = gtk_button_new_with_label("Delete job");
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(cron_delete_callback), (gpointer)clist);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 5);
  button = gtk_button_new_with_label("Test job");
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(cron_test_callback), (gpointer)clist);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), clist, TRUE, TRUE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

  return vbox;
}

void print_cron_jobs(GArray *cj) {
  int i;
  printf("\n\nCron\n----\n\n");
  for (i=0; i<cj->len/*sizeof(cron_job_t*)*/; i++) {
    cron_job_t *job = g_array_index(cj, cron_job_t*, i);
    printf("min=\"%s\" hour=\"%s\" mday=\"%s\" "
      "month=\"%s\" wday=\"%s\" job=\"%s \"\n",
      job->minute, job->hour, job->monthday,
      job->month, job->weekday, job->cmd);
  }
}
