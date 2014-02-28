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

/* This function is taken from GEdit.
 * GEdit is licensed under the GPLv2.
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002-2005 Paolo Maggi
 */
static GtkSourceStyleScheme *
get_style_scheme(void) {
	GtkSourceStyleSchemeManager *manager;
	GtkSourceStyleScheme *def_style;

	manager = gtk_source_style_scheme_manager_get_default();
	def_style = gtk_source_style_scheme_manager_get_scheme(manager, settings->scheme);
	if(def_style == NULL) {
		error_dialog("Style scheme cannot be found, falling back to 'classic' style scheme");

		def_style = gtk_source_style_scheme_manager_get_scheme(manager, "classic");
		if(def_style == NULL)
			error_dialog("Style scheme 'classic' cannot be found, check your GtkSourceView installation.");
	}

	return def_style;
}

Document *
create_new_doc(char *filename) {
	Document *new;
	GtkSourceBuffer *buffer;
	GtkSourceStyleScheme *scheme;
	PangoFontDescription *font_desc;

	new = g_slice_new0(Document);
	if(filename != NULL) {
		new->basename = g_path_get_basename(filename);
		new->filename = g_strdup(filename);
		new->new = FALSE;
	} else {
		new->basename = g_strdup(_("New file"));
		new->filename = g_strdup(_("New file"));
		new->new = TRUE;
	}
	new->modified = FALSE;
	buffer = gtk_source_buffer_new(NULL);
	new->view = gtk_source_view_new_with_buffer(buffer);
	g_signal_connect(buffer, "modified-changed", G_CALLBACK(on_modified_buffer), new);
	g_signal_connect(new->view, "key-press-event", G_CALLBACK(on_keypress_view), NULL);

	/* font, style scheme, etc */
	font_desc = pango_font_description_from_string(settings->font);
	if(font_desc != NULL) {
		gtk_widget_override_font(GTK_WIDGET(new->view), font_desc);
		pango_font_description_free(font_desc);
	}
	scheme = get_style_scheme();
	if(scheme != NULL) {
		gtk_source_buffer_set_style_scheme(buffer, scheme);
		gtk_source_buffer_set_highlight_syntax(buffer, settings->highlight_syntax);
		gtk_source_buffer_set_highlight_matching_brackets(buffer, settings->highlight_brackets);
	} else {
		gtk_source_buffer_set_highlight_syntax(buffer, FALSE);
		gtk_source_buffer_set_highlight_matching_brackets(buffer, FALSE);
	}

	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(new->view), settings->wrap_mode);
	gtk_source_view_set_auto_indent(GTK_SOURCE_VIEW(new->view), settings->auto_indent);
	gtk_source_view_set_indent_on_tab(GTK_SOURCE_VIEW(new->view), settings->indent_on_tab);
	gtk_source_view_set_indent_width(GTK_SOURCE_VIEW(new->view), settings->indent_width);
	gtk_source_view_set_insert_spaces_instead_of_tabs(GTK_SOURCE_VIEW(new->view), settings->spaces_io_tabs);
	gtk_source_view_set_smart_home_end(GTK_SOURCE_VIEW(new->view), settings->smart_home_end);
	gtk_source_view_set_highlight_current_line(GTK_SOURCE_VIEW(new->view), settings->highlight_curr_line);
	gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(new->view), settings->line_numbers);
	if(settings->show_right_margin) {
		gtk_source_view_set_show_right_margin(GTK_SOURCE_VIEW(new->view), TRUE);
		gtk_source_view_set_right_margin_position(GTK_SOURCE_VIEW(new->view), settings->right_margin_pos);
	}
	gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(new->view), settings->tab_width);
	gtk_source_view_set_draw_spaces(GTK_SOURCE_VIEW(new->view), settings->draw_spaces);

	return new;
}

void
free_document(Document *doc) {
	/*the view is automatically freed by destroying its container */
	g_free(doc->basename);
	g_free(doc->filename);
	g_slice_free(Document, doc);
}

GtkSourceLanguage *
guess_language(Document *doc) {
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	GtkSourceLanguage *lang;
	GtkSourceLanguageManager *lm;
	gboolean result_uncertain;
	char *content_type;
	char *data;
	gsize length;

	if(doc->filename == NULL || strcmp(doc->filename, _("New file")) == 0)
		return NULL;

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
	g_free(content_type);
	return lang;
}

void
set_language(Document *doc, GtkSourceLanguage *lang) {
	GtkTextBuffer *buffer;
	GtkSourceLanguage *oldlang = NULL;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(doc->view));
	oldlang = gtk_source_buffer_get_language(GTK_SOURCE_BUFFER(buffer));
	if(oldlang != lang)
		gtk_source_buffer_set_language(GTK_SOURCE_BUFFER(buffer), lang);
}

void
insert_into_buffer(GtkWidget *view, char *contents) {
	GtkTextBuffer *buffer;
	GtkTextIter iter;

	if(contents == NULL) {
		error_dialog("Error: can not insert file into the buffer! The file contents are null\n");		
		return;
	}

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
	gtk_source_buffer_begin_not_undoable_action(GTK_SOURCE_BUFFER(buffer));
	gtk_text_buffer_set_text(buffer, contents, -1);
	gtk_source_buffer_end_not_undoable_action(GTK_SOURCE_BUFFER(buffer));

	/* move cursor to the beginning */
	gtk_text_buffer_get_start_iter(buffer, &iter);
	gtk_text_buffer_place_cursor(buffer, &iter);

	/* to detect whether file has changed */
	gtk_text_buffer_set_modified(buffer, FALSE);
}
