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
#include "editor.h"

#define HIGHLIGHT          TRUE
#define HIGHLIGHT_BRACKETS TRUE
#define STYLE_SCHEME       "oblivion"

/* taken from GEdit */
static GtkSourceStyleScheme *
get_style_scheme(gchar *scheme_id) {
	GtkSourceStyleSchemeManager *manager;
	GtkSourceStyleScheme *def_style;

	manager = gtk_source_style_scheme_manager_get_default();
	def_style = gtk_source_style_scheme_manager_get_scheme(manager, scheme_id);
	if(def_style == NULL) {
		error_bar("Style scheme cannot be found, falling back to 'classic' style scheme ");

		def_style = gtk_source_style_scheme_manager_get_scheme(manager, "classic");
		if(def_style == NULL) {
			error_bar("Style scheme 'classic' cannot be found, check your GtkSourceView installation.");
		}
	}

	//g_free(scheme_id);
	return def_style;
}

Document *
create_new_doc(gchar *filename) {
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
set_language(Document *doc) {
	GtkTextBuffer *buffer;
	GtkSourceLanguageManager *lm;
	GtkSourceLanguage *lang = NULL;
	gboolean result_uncertain;
	gchar *content_type;

	if(doc->filename == NULL || strcmp(doc->filename, _("New file")) == 0)
		return;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(doc->view));
	content_type = g_content_type_guess(doc->filename, NULL, 0, &result_uncertain);
	if(result_uncertain) {
		g_free(content_type);
		content_type = NULL;
	}

	lm = gtk_source_language_manager_get_default();
	lang = gtk_source_language_manager_guess_language(lm, doc->filename, content_type);
	gtk_source_buffer_set_language(GTK_SOURCE_BUFFER(buffer), lang);

	g_free (content_type);
}
