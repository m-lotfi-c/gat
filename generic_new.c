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

GtkWidget *make_new_radiobutton(char *label, gboolean val, GSList **group,
    GtkWidget *box) {
  GtkWidget *ret = gtk_radio_button_new_with_label(*group, label);
  *group = gtk_radio_button_group(GTK_RADIO_BUTTON(ret));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ret), val);
  gtk_box_pack_start(GTK_BOX(box), ret, FALSE, FALSE, 0);
  return ret;
}

GtkWidget *make_new_checkbutton(char *label, gboolean val, GtkWidget *box) {
  GtkWidget *ret = gtk_check_button_new_with_label(label);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ret), val);
  gtk_box_pack_start(GTK_BOX(box), ret, FALSE, FALSE, 0);
  return ret;
}

GtkWidget *make_new_spinner(char *label, int val, int min, int max,
    int step, GtkWidget *box) {
  GtkWidget *vbox = NULL, *ret;
  GtkAdjustment *adj;
  if (label != NULL) {
    vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(label), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), vbox, FALSE, FALSE, 0);
  }
  adj = GTK_ADJUSTMENT(gtk_adjustment_new(val, min, max, 1, step, 0));
  ret = gtk_spin_button_new(adj, 0, 0);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(ret), TRUE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(ret), TRUE);
  gtk_box_pack_start(GTK_BOX(label ? vbox : box), ret, FALSE, FALSE, 0);
  return ret;
}

char *ordinal_ending(int n) {
  switch (n % 10) {
    case 1:  return "st";
    case 2:  return "nd";
    case 3:  return "rd";
    default: return "th";
  }
}

char *month_name[] = {
  "January", "February", "March", "April", "May", "June",
  "July", "August", "September", "October", "November", "December"
};

char *week_name[] = {
  "Sunday", "Monday", "Tuesday", "Wednesday",
  "Thursday", "Friday", "Saturday", "Sunday"
};

typedef struct {
  GtkWidget *filesel, *entry;
} filesel_data;

static gboolean not_set_filename(gpointer data) {
  filesel_data *fsd = (filesel_data*)data;
  gtk_widget_destroy(fsd->filesel);
  g_free(fsd);
  return TRUE;
}

static gboolean set_filename(gpointer data) {
  filesel_data *fsd = (filesel_data*)data;
  gtk_entry_set_text(GTK_ENTRY(fsd->entry),
    gtk_file_selection_get_filename(GTK_FILE_SELECTION(fsd->filesel)));
  gtk_widget_destroy(fsd->filesel);
  g_free(fsd);
  return TRUE;
}

static gboolean browse_button(gpointer data) {
  filesel_data *fsd;
  
  fsd = g_new(filesel_data, 1);
  fsd->entry = GTK_WIDGET(data);
  fsd->filesel = gtk_file_selection_new("Select job");
  gtk_window_set_modal(GTK_WINDOW(fsd->filesel), TRUE);
  gtk_signal_connect_object(
    GTK_OBJECT(GTK_FILE_SELECTION(fsd->filesel)->ok_button), "clicked",
    GTK_SIGNAL_FUNC(set_filename), (gpointer)fsd);
  gtk_signal_connect_object(
    GTK_OBJECT(GTK_FILE_SELECTION(fsd->filesel)->cancel_button), "clicked",
    GTK_SIGNAL_FUNC(not_set_filename), (gpointer)fsd);
  gtk_widget_show(fsd->filesel);

  return TRUE;
}

GtkWidget *make_job_page(GtkWidget *box) {
  GtkWidget *ret, *hbox, *align, *button; 

  hbox = gtk_hbox_new(FALSE, 5);
  align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_container_add(GTK_CONTAINER(align), hbox);
  gtk_box_pack_start(GTK_BOX(box), align, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox),
    gtk_label_new("Enter the command to run:"), FALSE, FALSE, 0);

  ret = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(hbox), ret, FALSE, FALSE, 0);
  button = gtk_button_new_with_label("Browse...");
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
    GTK_SIGNAL_FUNC(browse_button), (gpointer)ret);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

  return ret;
}
