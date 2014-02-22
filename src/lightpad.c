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
#include "io.h"

void
error_dialog(const gchar *message) {
	GtkWidget *dialog;

	g_fprintf(stderr, message);

	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, message);

	gtk_window_set_title(GTK_WINDOW(dialog), "Error!");
	gtk_dialog_run(GTK_DIALOG (dialog));
	gtk_widget_destroy(dialog);
	g_free((gpointer)message);
}

void
append_new_tab(Editor *editor) {
	GtkWidget *scroll, *label;
	char *basename;

	basename = g_path_get_basename(editor->filename);
	label = gtk_label_new(basename);
	g_free(basename);

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_NONE);
	gtk_widget_set_vexpand(scroll, TRUE);

	gtk_container_add(GTK_CONTAINER(scroll), editor->view);

	if(gtk_notebook_append_page(GTK_NOTEBOOK(tabs), scroll, label) < 0)
		error_dialog("Error: failed to add new tab\n");
	gtk_widget_show_all(tabs); //FIXME: why is this necessary?
}

void
insert_into_view(GtkWidget *view, char *contents) {
	GtkTextBuffer *buffer;
	GtkTextIter iter;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	gtk_widget_set_sensitive(view, FALSE);
	gtk_source_buffer_begin_not_undoable_action(GTK_SOURCE_BUFFER(buffer));
	if(contents != NULL) {
		gtk_text_buffer_set_text(buffer, contents, -1);
		g_free(contents);
	} else
		error_dialog("Error: contents are null\n");
	gtk_source_buffer_end_not_undoable_action(GTK_SOURCE_BUFFER(buffer));
	gtk_widget_set_sensitive(view, TRUE);

	/* move cursor to the beginning */
	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(buffer), &iter);
	gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(buffer), &iter);

	/* to detect whether file has changed */
	gtk_text_buffer_set_modified(buffer, FALSE);
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
	GtkWidget *scroll, *view;
	Editor *curr;
	int index;

	index = gtk_notebook_get_current_page(GTK_NOTEBOOK(tabs));
	scroll = gtk_notebook_get_nth_page(GTK_NOTEBOOK(tabs), index);
	view = gtk_bin_get_child(GTK_BIN(scroll));
	curr = g_object_get_data(G_OBJECT(view), "struct");

	if(!handled && (event->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK) {
		switch(event->keyval) {
			case GDK_KEY_o:
				if(curr->new)
					insert_file(window, curr, scroll);
				else
					open_file(window, TRUE); //FIXME: segfault when filename is null
				return TRUE; break;
			case GDK_KEY_t:
			case GDK_KEY_n: open_file(window, FALSE); return TRUE; break;
			case GDK_KEY_w: /*close_tab(curr, index)*/ g_fprintf(stdout, "Close tab\n"); return TRUE; break;
			case GDK_KEY_s: save_to_file(window, curr, FALSE); return TRUE; break;
			case GDK_KEY_S: save_to_file(window, curr, TRUE); return TRUE; break;
			case GDK_KEY_r: /*reload_file();*/ g_fprintf(stdout, "Reload file\n"); return TRUE; break;
			case GDK_KEY_q: gtk_main_quit(); return TRUE; break;
			case GDK_KEY_Tab: gtk_notebook_next_page(GTK_NOTEBOOK(tabs)); return TRUE; break;
			case GDK_KEY_ISO_Left_Tab: gtk_notebook_prev_page(GTK_NOTEBOOK(tabs)); return TRUE; break;
			default: break;
		}
	}

	return handled;
}

int
main(int argc, char **argv) {
	GtkWidget *window;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Lightpad");
	gtk_window_set_default_icon_name("accessories-text-editor");
	gtk_container_set_border_width(GTK_CONTAINER(window), 2);

	tabs = gtk_notebook_new();
	gtk_container_add(GTK_CONTAINER(window), tabs);

	open_file(GTK_WINDOW(window), FALSE);

	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(window, "key-press-event", G_CALLBACK(on_keypress_window), NULL);

	gtk_widget_show_all(window);
	gtk_main();

	//TODO: free all remaining objects
	return 0;
}
