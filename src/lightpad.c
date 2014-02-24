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
#include "editor.h"
#include "io.h"

/*
 * TODO:
 *   detect file type
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

void
error_bar(const gchar *message) {
	/*GtkWidget *infobar, *label, *content;*/

	g_fprintf(stderr, message);

	/*infobar = gtk_info_bar_new();
	gtk_info_bar_add_button(GTK_INFO_BAR(infobar), _("Ok"), GTK_RESPONSE_OK);
	gtk_info_bar_set_message_type(GTK_INFO_BAR(infobar), GTK_MESSAGE_ERROR);
	g_signal_connect(infobar, "response", G_CALLBACK(gtk_widget_destroy), NULL);

	label = gtk_label_new(message);
	g_free((gpointer)message);

	content = gtk_info_bar_get_content_area(GTK_INFO_BAR(infobar));
	gtk_container_add(GTK_CONTAINER(content), label);

	//pack it somewhere
	gtk_widget_show_all(infobar); //FIXME: is this necessary?*/
}

void
append_new_tab(Document *doc) {
	GtkWidget *scroll, *label;
	gchar *basename;

	basename = g_path_get_basename(doc->filename);
	label = gtk_label_new(basename);
	g_free(basename);

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_NONE);
	gtk_widget_set_vexpand(scroll, TRUE);

	gtk_container_add(GTK_CONTAINER(scroll), doc->view);

	if(gtk_notebook_append_page(GTK_NOTEBOOK(lightpad->tabs), scroll, label) < 0) {
		error_bar("Error: failed to add new tab\n");
		g_object_unref(scroll);
		//FIXME: remove view?
		return;
	}

	/* Store the structure inside the GtkScrolledWindow.
	 * This way, we can use GtkNotebook to supply the
	 * correct Document struct instead of keeping an
	 * additional list ourselves.
	 */
	g_object_set_data(G_OBJECT(scroll), "struct", doc);
	gtk_widget_show_all(lightpad->tabs); //FIXME: why is this necessary?
}

void
insert_into_buffer(GtkWidget *view, gchar *contents) {
	GtkTextBuffer *buffer;
	GtkTextIter iter;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	gtk_widget_set_sensitive(view, FALSE);
	gtk_source_buffer_begin_not_undoable_action(GTK_SOURCE_BUFFER(buffer));
	if(contents != NULL)
		gtk_text_buffer_set_text(buffer, contents, -1);
	else
		error_bar("Error: can not insert file into the buffer! The file contents are null\n");
	gtk_source_buffer_end_not_undoable_action(GTK_SOURCE_BUFFER(buffer));
	gtk_widget_set_sensitive(view, TRUE);

	/* move cursor to the beginning */
	gtk_text_buffer_get_start_iter(buffer, &iter);
	gtk_text_buffer_place_cursor(buffer, &iter);

	/* to detect whether file has changed */
	gtk_text_buffer_set_modified(buffer, FALSE);
}

gboolean
check_for_save(Document *doc) {
	gboolean save = FALSE;
	GtkTextBuffer *buffer;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(doc->view));

	if(gtk_text_buffer_get_modified(buffer) == TRUE) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new(GTK_WINDOW(lightpad->window),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
				_("Do you want to save the changes you have made?"));
		gtk_window_set_title(GTK_WINDOW(dialog), _("Save changes?"));
		if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES)
			save = TRUE;
		gtk_widget_destroy(dialog);      
	}     

	return save;
}

void
reset_default_status(Document *doc) {
	gchar *status, *basename;

	basename = g_path_get_basename(doc->filename);
	status = g_strdup_printf(_("File: %s"), basename);
	gtk_statusbar_pop(GTK_STATUSBAR(lightpad->status), lightpad->id);
	gtk_statusbar_push(GTK_STATUSBAR(lightpad->status), lightpad->id, status);
	g_free(basename);
	g_free(status);
}

gboolean
on_keypress_view(GtkWidget *widget, GdkEventKey *event) {
	if((event->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK) {
		switch(event->keyval) {
			case GDK_KEY_m: g_fprintf(stdout, "Mark\n"); return TRUE; break;
			case GDK_KEY_f: g_fprintf(stdout, "Search\n"); return TRUE; break;
			case GDK_KEY_g: g_fprintf(stdout, "Search next\n"); return TRUE; break;
			case GDK_KEY_G: g_fprintf(stdout, "Search previous\n"); return TRUE; break;
			case GDK_KEY_h: g_fprintf(stdout, "Replace\n"); return TRUE; break;
			case GDK_KEY_i: g_fprintf(stdout, "Go to line\n"); return TRUE; break;
			case GDK_KEY_d: g_fprintf(stdout, "Delete line\n"); return TRUE; break;
			case GDK_KEY_k: g_fprintf(stdout, "Clear search highlight\n"); return TRUE; break;
			default: return FALSE; break;
		}
	}
	return FALSE;
}

/*
 * GtkWindow catches keybindings _before_ passing them to
 * the focused widget. 
 * Here we override GtkWindow's handler to do the same things that it
 * does, but in the opposite order and then we chain up to the grand
 * parent handler, skipping gtk_window_key_press_event.
 */
gboolean
on_keypress_window(GtkWidget *widget, GdkEventKey *event) {
	GtkWindow *window = GTK_WINDOW(widget);
	gboolean handled = FALSE;

	/* handle focus widget key events */
	if(!handled)
		handled = gtk_window_propagate_key_event(window, event);

	/* handle mnemonics and accelerators */
	if(!handled)
		handled = gtk_window_activate_key(window, event);

	/* we went up all the way, these bindings are set on the window */
	GtkWidget *scroll;
	Document *curr;
	int index;

	index = gtk_notebook_get_current_page(GTK_NOTEBOOK(lightpad->tabs));
	scroll = gtk_notebook_get_nth_page(GTK_NOTEBOOK(lightpad->tabs), index);
	curr = g_object_get_data(G_OBJECT(scroll), "struct");

	if(!handled && (event->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK) {
		switch(event->keyval) {
			case GDK_KEY_o:
				if(curr->new) {
					if(check_for_save(curr) == TRUE) save_to_file(curr, TRUE);
					insert_into_view(curr, scroll);
				} else new_view(TRUE);
				return TRUE; break;
			case GDK_KEY_t:
			case GDK_KEY_n: new_view(FALSE); return TRUE; break;
			case GDK_KEY_w: /*close_tab(curr, index)*/ g_fprintf(stdout, "Close tab\n"); return TRUE; break;
			case GDK_KEY_s: save_to_file(curr, FALSE); return TRUE; break;
			case GDK_KEY_S: save_to_file(curr, TRUE); return TRUE; break;
			case GDK_KEY_r: /*reload_file();*/ g_fprintf(stdout, "Reload file\n"); return TRUE; break;
			case GDK_KEY_q: gtk_main_quit(); return TRUE; break;
			case GDK_KEY_Tab: gtk_notebook_next_page(GTK_NOTEBOOK(lightpad->tabs)); return TRUE; break;
			case GDK_KEY_ISO_Left_Tab: gtk_notebook_prev_page(GTK_NOTEBOOK(lightpad->tabs)); return TRUE; break;
			default: break;
		}
	}

	return handled;
}

/*
 * When the window is requested to be closed, we need to check if they have 
 * unsaved work. We use this callback to prompt the user to save their work before
 * they exit the application. From the "delete-event" signal, we can choose to
 * effectively cancel the close based on the value we return.
*/
gboolean 
on_delete_window(GtkWidget *widget, GdkEvent *event) {
	int pages;
	GtkWidget *scroll;
	Document *curr;

	pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(lightpad->tabs));
	for(int i = 0; i < pages; i++) {
		scroll = gtk_notebook_get_nth_page(GTK_NOTEBOOK(lightpad->tabs), i);
		curr = g_object_get_data(G_OBJECT(scroll), "struct");
		if(check_for_save(curr) == TRUE)
			save_to_file(curr, TRUE);
	}
	return FALSE; /* propogate event */
}

/*void
cleanup() {
	int pages;
	//GtkWidget *scroll;
	//Document *curr;

	pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(lightpad->tabs));
	for(int i = 0; i < pages; i++) {
		//scroll = gtk_notebook_get_nth_page(GTK_NOTEBOOK(lightpad->tabs), i);
		//curr = g_object_get_data(G_OBJECT(scroll), "struct");
		//TODO: free
		g_fprintf(stdout, "Cleanup\n");
	}
	g_slice_free(Window, lightpad);
}*/

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
	//g_signal_connect(lightpad->tabs, "switch-page", G_CALLBACK(reset_default_status), NULL);

	gtk_widget_show_all(lightpad->window);
	gtk_main();

	//cleanup();
	return 0;
}
