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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <gtk/gtk.h>
#include "at.h"
#include "gat.h"

#define AT_PATH "/usr/bin/at"
#define ATQ_PATH "/usr/bin/atq"
#define ATRM_PATH "/usr/bin/atrm"
#define MAX_ATQ_LENGTH 256
#define BLOCK_SIZE 4096

GArray *at_jobs;
GdkColor dark_grey, black;
static GtkWidget *at_clist;

/** Use 'at -c <id>' to get the text of an at job.
 *    @param id The id tag of the job to fetch.
 *    @return The text of the job (what is passed to 'sh -c')
 *  The return is newly allocated; the caller is responsible for g_freeing
 *  it.
 */
char *get_at_job_job(char *id) {
  int p[2], n, size = BLOCK_SIZE;
  pid_t pid;
  char *buf = g_new(char, BLOCK_SIZE), *bufp = buf;
  
  if (pipe(p) < 0) { perror("pipe"); return NULL; }
  switch ((pid = fork())) {
    case -1:
      perror("fork");
      close(p[0]);
      close(p[1]);
      return NULL;
    case 0:
      close(p[0]);
      dup2(p[1], STDOUT_FILENO);
      close(p[1]);
      execl(AT_PATH, AT_PATH, "-c", id, NULL);
      perror(AT_PATH);
      exit(-1);
    default:
      close(p[1]);
      while ((n = read(p[0], bufp, BLOCK_SIZE)) >= 0) {
        if (n < BLOCK_SIZE) {
          bufp[n] = '\0';
          break;
        }
        buf = realloc(buf, size + BLOCK_SIZE);
        bufp = buf + size;
        size += BLOCK_SIZE;
      }
      if (n == -1) {
        perror("read");
        exit(1);
      }
      close(p[0]);
      waitpid(pid, 0, 0);
      break;
  }
  return buf;
}

/** Build an array of at_job_t structs by calling atq.
 *    @return A GArray of at_job_t struct pointers.
 *  Note: return is newly allocated; the caller must g_free it.
 */
GArray *get_at_jobs() {
  GArray *j;
  int p[2];
  pid_t pid;
  FILE *fp;
  char buf[MAX_ATQ_LENGTH];
  
  if (pipe(p) < 0) { perror("pipe"); return NULL; }
  switch ((pid = fork())) {
    case -1:
      perror("fork");
      close(p[0]);
      close(p[1]);
      return NULL;
    case 0:
      close(p[0]);
      dup2(p[1], STDOUT_FILENO);
      close(p[1]);
      execl(ATQ_PATH, ATQ_PATH, NULL);
      perror(ATQ_PATH);
      exit(-1);
    default:
      close(p[1]);
      if (!(fp = fdopen(p[0], "r"))) perror("fopen");
      j = g_array_new(0, 0, sizeof(at_job_t*));
      while (fgets(buf, MAX_ATQ_LENGTH, fp)) {
        char *q, *p = buf + strlen(buf) - 1;
        at_job_t *job = g_new(at_job_t, 1);
        while (*p == '\n' || *p == '\r') *p-- = '\0';
        while (*p != ' ') *p-- = '\0';
        while (*p == ' ') *p-- = '\0';
        job->queue = g_strdup(p);
        *--p = '\0';
        q = buf;
        while (*q == ' ' || *q == '\t') q++;
        p = q;
        while (*q != ' ' && *q != '\t' && *q) q++;
        if (!q) {
          at_free_job(job);
          return j;
        }
        *q = '\0';
        job->id = g_strdup(p);
        p = q + 1;
        while (*p == ' ' || *p == '\t') p++;
        job->when = g_strdup(p);
        job->cmd = get_at_job_job(job->id);
        g_array_append_val(j, job);
      }
      close(p[0]);
      waitpid(pid, 0, 0);
      break;
  }
  return j;
}

/** Delete the at job with the given id using atrm.
 *    @param id The id tag of the job to delete.
 */
void delete_at_job(char *id) {
  pid_t pid;
  switch ((pid = fork())) {
    case -1:  perror("fork"); return;
    case 0:
      execl(ATRM_PATH, ATRM_PATH, id, NULL);
      perror(ATRM_PATH);
      exit(-1);
    default:
      waitpid(pid, 0, 0);
      printf("Deleted at job \"%s\"\n", id);
      break;
  }
}

/** Attempt to find the start of what the user specified for the at job.
 *  At pads the job with a bunch of initialization code that the user
 *  probably isn't very interested in seeing.  With Koenig's at 3.1.8 for
 *  Linux (as comes with Debian 2.1/slink), everything after the last '}'
 *  is user-specified.  If the user types a '}' of his own, we lose.
 *
 *  As an alternative, we could insert '# *** separator ***' or something
 *  when creating new at jobs, but then we lose when trying to manage jobs
 *  on the command line.  This could use some fixing...
 *
 *    @param job the command string for an at job
 *    @return a pointer into the above string indicating where the
 *    user-specified portion of the job (probably) starts.
 */
char *at_get_job_start(char *job) {
  char *p = strrchr(job, '}');
  if (!p) return NULL;
  p++;
  while (*p == '\r' || *p == '\n') p++;
  return (*p) ? p : NULL;
}

/** Put data into the at panel CList from the given at_job_t array.
 *    @param clist The CList widget to populate.  Should have two columns,
 *    "id" and "when" and should initially empty.
 *    @param j A GArray of at_job_t structs pointers.
 */
void at_clist_refresh(GtkCList *clist, GArray *j) {
  char *text[2];
  int i;
  for (i=0; i<j->len/*sizeof(at_job_t*)*/; i++) {
    at_job_t *job = g_array_index(j, at_job_t*, i);
    text[0] = job->when;
    text[1] = at_get_job_start(job->cmd);
    gtk_clist_append(GTK_CLIST(clist), text);
  }
}

void at_free_job(at_job_t *job) {
  if (job) {
    if (job->cmd) g_free(job->cmd);
    if (job->id) g_free(job->id);
    if (job->queue) g_free(job->queue);
    if (job->when) g_free(job->when);
    g_free(job);
  }
}

void at_reread() {
  int i;
  for (i=0; i<at_jobs->len/*sizeof(at_job_t*)*/; i++) {
    at_job_t *job = g_array_index(at_jobs, at_job_t*, i);
    at_free_job(job);
  }
  g_array_free(at_jobs, 1);
  at_jobs = get_at_jobs();
  gtk_clist_freeze(GTK_CLIST(at_clist));
  for (i=GTK_CLIST(at_clist)->rows-1; i>=0; i--)
    gtk_clist_remove(GTK_CLIST(at_clist), i);
  at_clist_refresh(GTK_CLIST(at_clist), at_jobs);
  gtk_clist_thaw(GTK_CLIST(at_clist));
}

/** Called when the user hits the "Delete job" button in the at panel.
 *    @param clist the at CList widget
 */
void at_delete_callback(GtkWidget *clist) {
  GList *sel = GTK_CLIST(clist)->selection;
  while (sel) {
    at_job_t *job = g_array_index(at_jobs, at_job_t*, (int)(sel->data));
    delete_at_job(job->id);
    sel = g_list_next(sel);
  }
  at_reread();
}

/** Called when the user hits the "Job details..." button in the at panel.
 *    @param clist the at CList widget
 */
void at_details_callback(GtkWidget *clist) {
  GList *sel = GTK_CLIST(clist)->selection;
  while (sel) {
    at_job_t *job = g_array_index(at_jobs, at_job_t*, (int)(sel->data));
    GtkWidget *w, *vbox, *button, *table, *hbox, *label, *text_view;
    GtkTextBuffer *text_buffer;
    char *buf = g_new(char, strlen(job->id)+4+1);
    char *buf2 = g_new(char, strlen(job->id) + strlen(job->when) + strlen(job->queue) + 3);
    char *job_start;

    sprintf(buf, "Job %s", job->id);
    sprintf(buf2, "%s\n%s\n%s", job->when, job->queue, job->id);

    w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(w), buf);
    gtk_window_set_policy(GTK_WINDOW(w), TRUE, TRUE, FALSE);
    gtk_container_border_width(GTK_CONTAINER(w), 5);
    g_free(buf);

    vbox = gtk_vbox_new(FALSE, 5);
    hbox = gtk_hbox_new(FALSE, 10);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    label = gtk_label_new("Time:\nQueue:\nID:");
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
    label = gtk_label_new(buf2);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    table = gtk_table_new(2, 2, FALSE);
    gtk_table_set_row_spacing(GTK_TABLE(table), 0, 2);
    gtk_table_set_col_spacing(GTK_TABLE(table), 0, 2);
    gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);

    text_view = gtk_text_view_new();
    text_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_widget_set_usize(text_view, 400, 300);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_table_attach(GTK_TABLE(table), text_view, 0, 1, 0, 1,  
       GTK_EXPAND | GTK_SHRINK | GTK_FILL,
       GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
#if 0
    scrollbar = gtk_hscrollbar_new(GTK_TEXT(text)->hadj);  
    gtk_table_attach(GTK_TABLE(table), scrollbar, 0, 1, 1, 2,
       GTK_EXPAND | GTK_FILL | GTK_SHRINK, GTK_FILL, 0, 0);
    scrollbar = gtk_vscrollbar_new(GTK_TEXT(text)->vadj);  
    gtk_table_attach(GTK_TABLE(table), scrollbar, 1, 2, 0, 1,
       GTK_FILL, GTK_EXPAND | GTK_FILL | GTK_SHRINK, 0, 0);
#endif

    button = gtk_button_new_with_label("Close");
    gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(gtk_widget_destroy), GTK_OBJECT(w));
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 5);
    gtk_container_add(GTK_CONTAINER(w), vbox);

#if 0
    gtk_widget_realize(text);
    fixed = gdk_font_load("fixed");
#endif
    job_start = at_get_job_start(job->cmd);
    if (job_start && job_start > job->cmd)
      gtk_text_buffer_set_text(GTK_TEXT_BUFFER(text_buffer),
        job_start, -1);
		else
      gtk_text_buffer_set_text(GTK_TEXT_BUFFER(text_buffer),
        job->cmd, -1);

    gtk_widget_show_all(w);

    sel = g_list_next(sel);
  }
}

/** Called when the user hits the "Test job" button in the at panel.
 *    @param clist the at CList widget
 */
void at_test_callback(GtkWidget *clist) {
  GList *sel = GTK_CLIST(clist)->selection;
  while (sel) {
    int i, skip = 0;
    at_job_t *job = g_array_index(at_jobs, at_job_t*, (int)(sel->data));
    char *buf = g_new(char, strlen(job->id)+12+1);

    sprintf(buf, "Run job \"%s\" ?", job->id);
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

/** Called when the user hits the "New job..." button in the at panel.
 *    @param clist the at CList widget
 */
void at_new_callback(GtkWidget *clist) {
  printf("New at job!\n");
  make_new_at_job();
}

/** Create a new widget containing everything that should appear on the at
 *  panel.
 *    @param j a GArray containing at_job_t struct pointers to display
 *    @return a GtkWidget with a panel and some buttons in it
 *  The widget is (as usual) newly created and must be g_freed by the
 *  caller.
 */
GtkWidget *make_at_page(GArray *j) {
  char *text[] = { "When", "Job" };
  GtkWidget *clist, *vbox, *hbox, *button;
  at_clist = clist = gtk_clist_new_with_titles(2, text);
  gtk_clist_set_selection_mode(GTK_CLIST(clist), GTK_SELECTION_MULTIPLE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 0, TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 1, TRUE);
  at_clist_refresh(GTK_CLIST(clist), j);
  gtk_signal_connect(GTK_OBJECT(clist), "select_row",
    GTK_SIGNAL_FUNC(clist_select_row_callback), at_details_callback);
  
  vbox = gtk_vbox_new(FALSE, 5);
  hbox = gtk_hbox_new(FALSE, 5);
  button = gtk_button_new_with_label("New job...");
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 5);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(at_new_callback), (gpointer)clist);
  button = gtk_button_new_with_label("Delete job");
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(at_delete_callback), (gpointer)clist);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 5);
  button = gtk_button_new_with_label("Job details...");
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(at_details_callback), (gpointer)clist);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 5);
  button = gtk_button_new_with_label("Test job");
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(at_test_callback), (gpointer)clist);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), clist, TRUE, TRUE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);

  return vbox;
}

void at_add_job(at_job_t *job) {
  pid_t pid;
  int p[2];
  if (pipe(p) < 0) { perror("pipe"); at_free_job(job); return; }
  switch ((pid = fork())) {
    case -1:
      perror("fork");
      close(p[0]);
      close(p[1]);
      at_free_job(job);
      return;
    case 0:
      close(p[1]);
      dup2(p[0], STDIN_FILENO);
      close(p[0]);
      execl(AT_PATH, AT_PATH, job->when, NULL);
      perror(AT_PATH);
      exit(-1);
    default:
      close(p[0]);
      write(p[1], job->cmd, strlen(job->cmd));
      write(p[1], "\n", 1);
      close(p[1]);
      waitpid(pid, 0, 0);
      printf("Added at job: %s at %s\n", job->cmd, job->when);
      at_reread();
  }
}

/** Print a list of all at jobs in an at_job_t array
 *    @param aj a GArray containing at_job_t struct pointers
 */
void print_at_jobs(GArray *aj) {
  int i;
  printf("At\n--\n\n");
  for (i=0; i<aj->len/*sizeof(at_job_t*)*/; i++) {
    at_job_t *job = g_array_index(aj, at_job_t*, i);
    printf("id=\"%s\" when=\"%s\" queue=\"%s\"\n",
      job->id, job->when, job->queue);
  }
}
