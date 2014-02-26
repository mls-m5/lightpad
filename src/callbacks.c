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
#include <gtksourceview/gtksource.h>

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

	if(!handled && (event->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK) {
		switch(event->keyval) {
			case GDK_KEY_o:
					if(doc->new) {
						if(check_for_save(doc) == TRUE)
							save_to_file(doc, TRUE);
						insert_into_view(doc);
					} else
						new_view(TRUE);
					return TRUE; break;
			case GDK_KEY_t:
			case GDK_KEY_n:
					new_view(FALSE); return TRUE; break;
			case GDK_KEY_w:
					close_tab(); return TRUE; break;
			case GDK_KEY_s:
					save_to_file(doc, FALSE); return TRUE; break;
			case GDK_KEY_S:
					save_to_file(doc, TRUE); return TRUE; break;
			case GDK_KEY_r:
					/*reload_file();*/; return TRUE; break;
			case GDK_KEY_q:
					gtk_main_quit(); return TRUE; break;
			case GDK_KEY_Tab: //FIXME: not working
					gtk_notebook_next_page(GTK_NOTEBOOK(lightpad->tabs)); return TRUE; break;
			case GDK_KEY_ISO_Left_Tab: //FIXME: not working
					gtk_notebook_prev_page(GTK_NOTEBOOK(lightpad->tabs)); return TRUE; break;
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
on_delete_window(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
	Document *doc;
	GtkWidget *scroll;
	int pages;

	pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(lightpad->tabs));
	for(int i = 0; i < pages; i++) {
		scroll = gtk_notebook_get_nth_page(GTK_NOTEBOOK(lightpad->tabs), i);
		doc = g_object_get_data(G_OBJECT(scroll), "doc");
		if(check_for_save(doc) == TRUE)
			save_to_file(doc, TRUE);
	}
	return FALSE; /* propogate event */
}

void
on_page_switch(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data) {
	Document *doc;
	GtkWidget *scroll;
	GtkTextBuffer *buffer;
	GtkSourceLanguage *lang;
	const char *id;

	scroll = gtk_notebook_get_nth_page(notebook, page_num);
	doc = g_object_get_data(G_OBJECT(scroll), "doc");
	reset_default_status(doc);

	//FIXME: this calls on_lang_changed again which changes the language
	/*buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(doc->view));
	lang = gtk_source_buffer_get_language(GTK_SOURCE_BUFFER(buffer));
	id = gtk_source_language_get_id(lang);
	gtk_combo_box_set_active_id(GTK_COMBO_BOX(lightpad->combo), id);*/
}

void
on_page_added(GtkNotebook *notebook, GtkWidget *child, guint page_num, gpointer user_data) {
	gtk_notebook_set_current_page(notebook, page_num);
}

void
on_lang_changed(GtkComboBox *widget, gpointer user_data) {
	Document *doc;
	GtkSourceLanguage *lang;
	GtkSourceLanguageManager *lm;
	char *id;

	id = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget));
	lm = gtk_source_language_manager_get_default();
	lang = gtk_source_language_manager_get_language(lm, id);
	//g_object_unref(lm);
	g_free(id);

	if(lang == NULL)
		return;

	doc = get_active_document();
	set_language(doc, lang);
}
