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
#include <unistd.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <gtk/gtk.h>

static pid_t pid;

void parent(char *cmd);

void reap(int sig) {
  pid_t p;
  int flag = 0;
  while ((p = waitpid(pid, 0, WNOHANG)) > 0)
    if (p == pid)
      flag = 1;
  printf("flag=%d\n", flag);
  if (flag) {
    printf("exiting\n");
    exit(0);
  }
  else
    printf("not exiting\n");
}

int main(int argc, char **argv) {
  if (argc <= 1) {
    fprintf(stderr, "Usage:\n  %s command [arg [arg [...]]]\n\n", argv[0]);
    return -1;
  }
  signal(SIGCHLD, reap);
  gtk_init(&argc, &argv);
  switch ((pid = fork())) {
    case -1:
      perror("fork");
      return -1;
    case 0:
      execvp(argv[1], argv+1);
      perror(argv[1]);
      return -1;
    default:
      parent(argv[1]);
      return 0;
  }
}

void killit() {
  kill(pid, SIGTERM);
  exit(0);
}

void parent(char *cmd) {
  GtkWidget *w, *vbox, *button;
  char buf[20];
  sprintf(buf, "PID: %d", (int)pid);
  w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(w), cmd);
  gtk_window_set_policy(GTK_WINDOW(w), FALSE, FALSE, TRUE);
  gtk_container_set_border_width(GTK_CONTAINER(w), 8);
  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_add(GTK_CONTAINER(w), vbox);
  gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(cmd), FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(buf), FALSE, FALSE, 0);
  button = gtk_button_new_with_label("Kill");
  gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
  gtk_signal_connect_object(GTK_OBJECT(w), "destroy",
    GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(killit), NULL);
  gtk_widget_show_all(w);
  reap(0);
  gtk_main();
}
