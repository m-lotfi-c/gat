/* gat - The GNOME Task Scheduler
 * Copyright (C) 2000-2005 by Patrick Reynolds <reynolds@cs.duke.edu>
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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <gnome.h>

#include "at.h"
#include "cron.h"
#include "gat.h"

GdkColor dark_grey, black;

#define CHK(op, msg) \
  if ((op) == -1) { perror(msg); exit(-1); }

/** Run a job in the background.
 *    @param cmd The command to run with sh -c
 *    @return the pid of the newly spawned job, or -1 on error forking
 */
int run_job(char *cmd) {
  int fd;
  pid_t pid;
  
  switch (pid = fork()) {
    case -1:
      perror("fork");
      return -1;
    case 0:
      CHK(fd=open("/dev/null", O_RDWR), "/dev/null");
      CHK(dup2(fd, STDIN_FILENO), "dup2");
      CHK(dup2(fd, STDOUT_FILENO), "dup2");
      CHK(close(fd), "close");
      CHK(setpgrp(), "setpgrp");
      CHK(chdir("/"), "/");
      CHK(execl("/bin/sh", "sh", "-c", cmd, NULL), "/bin/sh");
      exit(-1);
      break;
    default:
      printf("job running as pid=%d\n", (int)pid);
      return pid;
  }
}

/** Make a modal ok-cancel dialog and return 1 for ok or 0 for cancel.
 *    @param title The window title (goes in the title bar)
 *    @param label The label (goes above ok and cancel buttons)
 */
int make_ok_cancel_dialog(char *title, char *label) {
  GtkWidget *dialog = gnome_message_box_new(label, GNOME_MESSAGE_BOX_QUESTION,
    GNOME_STOCK_BUTTON_OK, GNOME_STOCK_BUTTON_CANCEL, NULL);
  gtk_widget_show(dialog);
  return (gnome_dialog_run(GNOME_DIALOG(dialog)) == 0);
}

typedef void (*double_click_callback_t)(GtkWidget *clist);

/** Called whenever the user clicks on any CList object.  It's a
 *  dispatcher -- it calls the function passed to it as 'd' if the click
 *  was a double-click.
 *    @param clist The widget clicked upon.
 *    @param row The row clicked (ignored).
 *    @param column The column clicked (ignored).
 *    @param ev Event information (ignored).
 *    @param d A pointer to the function to be called, if this was a
 *    double-click event.
 */
void clist_select_row_callback(GtkWidget *clist, int row, int column,
  GdkEventButton *ev, gpointer d) {
  g_return_if_fail(GTK_IS_CLIST(clist));
  if (ev)
    switch (ev->type) {
      case GDK_2BUTTON_PRESS:
        ((double_click_callback_t)d)(clist);
        break;
      default: ;
    }
}

/** Print all of the jobs in aj and cj, for debugging purposes.
 *    @param aj A GArray of at_job_t struct pointers (at jobs).
 *    @param cj A GArray of cron_job_t struct pointers (cron jobs).
 */
void print_jobs(GArray *aj, GArray *cj) {
  print_at_jobs(aj);
  print_cron_jobs(cj);
}

/* SIG_IGN should work instead of this, but it doesn't.  Weird. */
void ignore(int sig) { }

/** Start program, do stuff, return 0 */
int main(int argc, char **argv) {
  GtkWidget *w, *notebook, *vbox, *button;

  signal(SIGCHLD, ignore);
  
  at_jobs = get_at_jobs();
  cron_jobs = get_cron_jobs();
  if (!at_jobs || !cron_jobs) return -1;
/*  print_jobs(at_jobs, cron_jobs);*/

  gnome_init("gat", VERSION, argc, argv);
  w = gnome_app_new("gat", "GTK Task Scheduler");
  gtk_container_border_width(GTK_CONTAINER(w), 8);
  gtk_signal_connect(GTK_OBJECT(w), "delete_event",
    GTK_SIGNAL_FUNC(gtk_main_quit), 0);
  notebook = gtk_notebook_new();
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), make_at_page(at_jobs),
    gtk_label_new("One-time jobs"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), make_cron_page(cron_jobs),
    gtk_label_new("Recurring jobs"));
  vbox = gtk_vbox_new(FALSE, 5);
  button = gtk_button_new_with_label("Close");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(gtk_main_quit), 0);
  gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 5);
  gnome_app_set_contents(GNOME_APP(w), vbox);
  gtk_widget_show_all(w);

  dark_grey.red = dark_grey.green = dark_grey.blue = 0x8000;
  gdk_color_alloc(gdk_window_get_colormap(w->window), &dark_grey);
  gdk_color_black(gdk_window_get_colormap(w->window), &black);

  gtk_main();
  
  return 0;
}
