/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * callbacks.c
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
#include <glib/gprintf.h>

#include "complete.h"
#include "lightpad.h"
#include "document.h"
#include "io.h"

gboolean
on_keypress_view(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
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
			case GDK_KEY_space: g_fprintf(stdout, "Complete\n");
				runComplete();
			return TRUE; break;
			default: return FALSE; break;
		}
	}
	return FALSE;
}

/* This function is taken from GEdit.
 * GEdit is licensed under the GPLv2.
 *
 * Copyright (C) 2005 - Paolo Maggi
 *
 * GtkWindow catches keybindings _before_ passing them to
 * the focused widget. 
 * Here we override GtkWindow's handler to do the same things that it
 * does, but in the opposite order and then we chain up to the grand
 * parent handler, skipping gtk_window_key_press_event.
 */
gboolean
on_keypress_window(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
	GtkWindow *window = GTK_WINDOW(widget);
	gboolean handled = FALSE;

	/* handle focus widget key events */
	if(!handled)
		handled = gtk_window_propagate_key_event(window, event);

	/* handle mnemonics and accelerators */
	if(!handled)
		handled = gtk_window_activate_key(window, event);

	/* we went up all the way, these bindings are set on the window */
	Document *doc = get_active_document();
	char *filename;

	if(!handled && (event->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK) {
		switch(event->keyval) {
			case GDK_KEY_o:
					if(open_get_filename(&filename) == GTK_RESPONSE_ACCEPT) {
						if(doc != NULL && doc->new_document && !doc->modified)
							insert_into_view(doc, filename);
						else
							new_view(filename);
						g_free(filename);
					}
					return TRUE;
			case GDK_KEY_t:
			case GDK_KEY_n:
					new_view(NULL); return TRUE;
			case GDK_KEY_w:
					close_tab(); return TRUE;
			case GDK_KEY_s:
					save_to_file(doc, FALSE); return TRUE;
			case GDK_KEY_S:
					save_to_file(doc, TRUE); return TRUE;
			case GDK_KEY_r:
					/*reload_file();*/; return TRUE;
			case GDK_KEY_q:
					gtk_main_quit(); return TRUE;
			case GDK_KEY_Tab: //FIXME: not working
					gtk_notebook_next_page(GTK_NOTEBOOK(lightpad->tabs)); return TRUE;
			case GDK_KEY_ISO_Left_Tab: //FIXME: not working
					gtk_notebook_prev_page(GTK_NOTEBOOK(lightpad->tabs)); return TRUE;
			default: break;
		}
		if (event->keyval >= GDK_KEY_1 && event->keyval <= GDK_KEY_9){
			selectPopup(event->keyval - GDK_KEY_1);
			g_fprintf(stdout, "valde alternativ %d\n", event->keyval - GDK_KEY_0);
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
on_delete_window(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
	Document *doc;
	GtkWidget *scroll;
	int pages, save;

	pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(lightpad->tabs));
	for(int i = 0; i < pages; i++) {
		scroll = gtk_notebook_get_nth_page(GTK_NOTEBOOK(lightpad->tabs), i);
		doc = g_object_get_data(G_OBJECT(scroll), "doc");
		save = check_for_save(doc);
		switch(save) {
			case GTK_RESPONSE_YES:
				if(save_to_file(doc, TRUE) < 0) break;
			case GTK_RESPONSE_CANCEL: return TRUE; /* abort */
			default: break;
		}
	}
	return FALSE; /* propogate event */
}

void
on_page_added(GtkNotebook *notebook, GtkWidget *child, guint page_num, gpointer user_data) {
	gtk_notebook_set_current_page(notebook, page_num);
}

void
on_modified_buffer(GtkTextBuffer *buffer, Document *doc) {
	GtkWidget *scroll;
	char *title;
	int index;

	doc->modified = !doc->modified;

	index = gtk_notebook_get_current_page(GTK_NOTEBOOK(lightpad->tabs));
	scroll = gtk_notebook_get_nth_page(GTK_NOTEBOOK(lightpad->tabs), index);

	if(doc->modified)
		title = g_strdup_printf("%s%s", "*", doc->basename);
	else
		title = g_strdup(doc->basename);
	gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(lightpad->tabs), scroll, title);
	g_free(title);
}
