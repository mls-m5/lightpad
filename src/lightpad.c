/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * lightpad.c
 * Copyright (C) 2014 Jente Hidskes <hjdskes@gmail.com>
 *
 * Lightpad is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Lightpad is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <gtksourceview/gtksource.h>

#include "lightpad.h"
#include "document.h"
#include "callbacks.h"
#include "io.h"

/*
 * TODO:
 *   file saved/not saved indicator in tab label
 *   dont ask to save file when undoes have happened
 *   look more into signals
 *   expand statusbar
       buttons to change settings
         settings
           syntax highlighting
           tab size
       row number
       colom number
     configuration à la gedit?
       font
       colorscheme
       show rownumbers
       show right border
       linebreak
       dont split words on two rows
       highlight current line
       highlight matching brackets
       tabsize
       spaces instead of tabs
       automatic indentation
       backup copy
       auto-save every n minutes
 */

void /*ToDo: transform this into a fancy GtkInfoBar */
error_dialog(const char *message) {
	GtkWidget *dialog;

	g_fprintf(stderr, message);

	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, message);
	gtk_window_set_title(GTK_WINDOW(dialog), "Error!");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	g_free((gpointer)message);
}

void
append_new_tab(Document *doc) {
	GtkWidget *scroll, *label;
	char *basename;

	basename = g_path_get_basename(doc->filename);
	label = gtk_label_new(basename);
	g_free(basename);

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC,
			GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_NONE);
	gtk_widget_set_vexpand(scroll, TRUE);
	gtk_container_add(GTK_CONTAINER(scroll), doc->view);

	/* Store the structure inside the GtkScrolledWindow.
	 * This way, we can use GtkNotebook to supply the
	 * correct Document struct instead of keeping an
	 * additional list ourselves.
	 */
	g_object_set_data_full(G_OBJECT(scroll), "doc", doc, (GDestroyNotify)free_document);
	gtk_widget_show_all(scroll);
	gtk_notebook_append_page(GTK_NOTEBOOK(lightpad->tabs), scroll, label);
}

void
close_tab(Document *doc, int index) {
	GtkWidget *scroll;

	scroll = gtk_notebook_get_nth_page(GTK_NOTEBOOK(lightpad->tabs), index);
	gtk_widget_destroy(scroll); /* this destroys both scroll's child as its container */
}

void
update_tab_label(Document *doc) {
	GtkWidget *scroll;
	char *basename;
	int index;

	index = gtk_notebook_get_current_page(GTK_NOTEBOOK(lightpad->tabs));
	scroll = gtk_notebook_get_nth_page(GTK_NOTEBOOK(lightpad->tabs), index);

	basename = g_path_get_basename(doc->filename);
	gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(lightpad->tabs), scroll, basename);
	g_free(basename);
}

gboolean
check_for_save(Document *doc) {
	GtkTextBuffer *buffer;
	gboolean save = FALSE;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(doc->view));

	if(gtk_text_buffer_get_modified(buffer) == TRUE) {
		GtkWidget *dialog;
		char *message;

		message = g_strdup_printf(
				_("Do you want to save the changes in file '%s' before closing?\n"
				"If you do not, the changes will be lost.\n"), doc->filename);
		dialog = gtk_message_dialog_new(GTK_WINDOW(lightpad->window),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO,
				message);
		gtk_window_set_title(GTK_WINDOW(dialog), _("Save changes?"));
		if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES)
			save = TRUE;
		gtk_widget_destroy(dialog);
		g_free(message);
	}     

	return save;
}

void
reset_default_status(Document *doc) {
	char *status, *basename;

	basename = g_path_get_basename(doc->filename);
	status = g_strdup_printf(_("File: %s"), basename);
	gtk_statusbar_pop(GTK_STATUSBAR(lightpad->status), lightpad->id);
	gtk_statusbar_push(GTK_STATUSBAR(lightpad->status), lightpad->id, status);
	g_free(basename);
	g_free(status);
}

int
main(int argc, char **argv) {
	GtkWidget *vbox;

	lightpad = g_slice_new0(Window);

	gtk_init(&argc, &argv);

	lightpad->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(lightpad->window), "Lightpad");
	gtk_window_set_default_icon_name("accessories-text-editor");
	gtk_container_set_border_width(GTK_CONTAINER(lightpad->window), 2);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(lightpad->window), vbox);

	lightpad->tabs = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), lightpad->tabs, TRUE, TRUE, 0);

	lightpad->status = gtk_statusbar_new();
	lightpad->id = gtk_statusbar_get_context_id(GTK_STATUSBAR(lightpad->status),
			"Lightpad text editor");
	gtk_box_pack_start(GTK_BOX(vbox), lightpad->status, FALSE, TRUE, 0);

	new_view(FALSE);

	g_signal_connect(lightpad->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(lightpad->window, "delete-event", G_CALLBACK(on_delete_window), NULL);
	g_signal_connect(lightpad->window, "key-press-event", G_CALLBACK(on_keypress_window), NULL);
	g_signal_connect(lightpad->tabs, "switch-page", G_CALLBACK(on_page_switch), NULL);
	g_signal_connect(lightpad->tabs, "page-added", G_CALLBACK(on_page_added), NULL);

	gtk_widget_show_all(lightpad->window);
	gtk_main();

	/* This will destroy all the childs, who in turn
	 * will destroy their own childs et cetera.
	 * Thus, there is no need to explicitly destroy
	 * all the GtkScrolledWindow objects */
	gtk_widget_destroy(lightpad->window);
	g_slice_free(Window, lightpad);
	return 0;
}
