/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * document.c
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

#include <glib/gi18n.h>
#include <gtksourceview/gtksource.h>

#include "lightpad.h"
#include "callbacks.h"
#include "document.h"

#define HIGHLIGHT          TRUE
#define HIGHLIGHT_BRACKETS TRUE
#define STYLE_SCHEME       "oblivion"

/* This function is taken from GEdit.
 * GEdit is licensed under the GPLv2.
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002-2005 Paolo Maggi
 */
static GtkSourceStyleScheme *
get_style_scheme(char *scheme_id) {
	GtkSourceStyleSchemeManager *manager;
	GtkSourceStyleScheme *def_style;

	manager = gtk_source_style_scheme_manager_get_default();
	def_style = gtk_source_style_scheme_manager_get_scheme(manager, scheme_id);
	if(def_style == NULL) {
		error_dialog("Style scheme cannot be found, falling back to 'classic' style scheme");

		def_style = gtk_source_style_scheme_manager_get_scheme(manager, "classic");
		if(def_style == NULL) {
			error_dialog("Style scheme 'classic' cannot be found, check your GtkSourceView installation.");
		}
	}

	//g_free(scheme_id);
	return def_style;
}

Document *
create_new_doc(char *filename) {
	Document *new;
	GtkSourceStyleScheme *scheme;
	GtkSourceBuffer *buffer;

	new = g_slice_new0(Document);
	if(filename != NULL) {
		new->filename = g_strdup(filename);
		new->new = FALSE;
	} else {
		new->filename = g_strdup(_("New file"));
		new->new = TRUE;
	}
	buffer = gtk_source_buffer_new(NULL);
	new->view = gtk_source_view_new_with_buffer(buffer);
	g_signal_connect(new->view, "key-press-event", G_CALLBACK(on_keypress_view), NULL);

	/* syntax, theme, etc */
	gtk_source_buffer_set_highlight_syntax(buffer, HIGHLIGHT);
	gtk_source_buffer_set_highlight_matching_brackets(buffer, HIGHLIGHT_BRACKETS);
	scheme = get_style_scheme(STYLE_SCHEME);
	if(scheme != NULL)
		gtk_source_buffer_set_style_scheme(buffer, scheme);
	set_language(new);

	return new;
}

void
free_document(Document *doc) {
	/*the view is automatically freed by destroying its container */
	g_free(doc->filename);
	g_slice_free(Document, doc);
}

void
set_language(Document *doc) {
	GtkTextBuffer *buffer;
	GtkSourceLanguageManager *lm;
	GtkSourceLanguage *lang = NULL;
	GtkTextIter start, end;
	gboolean result_uncertain;
	char *content_type;
	char *data;
	gsize length;

	if(doc->filename == NULL || strcmp(doc->filename, _("New file")) == 0)
		return;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(doc->view));
	gtk_text_buffer_get_start_iter(buffer, &start);
	end = start;
	gtk_text_iter_forward_chars(&end, 255);

	data = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);
	length = strlen(data);
	content_type = g_content_type_guess(doc->filename, (const guchar *)data, length,
			&result_uncertain);
	if(result_uncertain) {
		g_free(content_type);
		content_type = NULL;
	}
	g_free(data);

	lm = gtk_source_language_manager_get_default();
	lang = gtk_source_language_manager_guess_language(lm, doc->filename, content_type);
	gtk_source_buffer_set_language(GTK_SOURCE_BUFFER(buffer), lang);

	g_free(content_type);
}

void
insert_into_buffer(GtkWidget *view, char *contents) {
	GtkTextBuffer *buffer;
	GtkTextIter iter;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	gtk_source_buffer_begin_not_undoable_action(GTK_SOURCE_BUFFER(buffer));
	if(contents != NULL)
		gtk_text_buffer_set_text(buffer, contents, -1);
	else
		error_dialog("Error: can not insert file into the buffer! The file contents are null\n");
	gtk_source_buffer_end_not_undoable_action(GTK_SOURCE_BUFFER(buffer));

	/* move cursor to the beginning */
	gtk_text_buffer_get_start_iter(buffer, &iter);
	gtk_text_buffer_place_cursor(buffer, &iter);

	/* to detect whether file has changed */
	gtk_text_buffer_set_modified(buffer, FALSE);
}
