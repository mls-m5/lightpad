/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * io.c
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
#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>

#include "lightpad.h"
#include "document.h"

void
save_to_file(Document *doc, gboolean saveas) {
	GtkWidget *dialog;
	GtkSourceLanguage *lang;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	char *filename = NULL;
	char *contents;
	gboolean result = FALSE;
	GError *error = NULL;

	if(saveas || doc->new) {
		dialog = gtk_file_chooser_dialog_new(_("Save File"), GTK_WINDOW(lightpad->window),
				GTK_FILE_CHOOSER_ACTION_SAVE, _("_Cancel"), GTK_RESPONSE_CANCEL,
				_("_Save"), GTK_RESPONSE_ACCEPT, NULL);

		gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

		if(saveas)
			gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), doc->basename);
		else
			gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), _("Untitled document"));

		if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
			filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		gtk_widget_destroy(dialog);
		if(filename == NULL)
			return;

		doc->new = FALSE;
		g_free(doc->basename);
		doc->basename = g_path_get_basename(filename);
		g_free(doc->filename);
		doc->filename = g_strdup(filename);
		lang = guess_language(doc);
		set_language(doc, lang);
		update_tab_label(doc);
	} else
		filename = doc->filename;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(doc->view));
	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(buffer), &start);
	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(buffer), &end);
	contents = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(buffer), &start, &end, FALSE);       
	gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(buffer), FALSE);

	result = g_file_set_contents(filename, contents, -1, &error);
	if(saveas || doc->new)
		g_free(filename);
	g_free(contents);
	if(!result) {
		if(error) {
			error_dialog(error->message);
			g_error_free(error);
		} else
			error_dialog("Error: cannot save to file. Something went wrong\n");
		return;
	}
	gtk_text_buffer_set_modified(buffer, FALSE);
}

char *
open_get_filename(void) {
	GtkWidget *dialog;
	char *filename = NULL;

	dialog = gtk_file_chooser_dialog_new(_("Open File"), GTK_WINDOW(lightpad->window),
			GTK_FILE_CHOOSER_ACTION_OPEN, _("_Cancel"), GTK_RESPONSE_CANCEL,
			_("_Open"), GTK_RESPONSE_ACCEPT, NULL);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	gtk_widget_destroy(dialog);
	return filename;
}

void
new_view(gboolean open_file) {
	Document *new;
	GtkSourceLanguage *lang;
	char *filename = NULL;
	gsize length;
	gboolean result;
	GError *error = NULL;

	if(open_file) {
		filename = open_get_filename();

		if(filename == NULL) {
			error_dialog("Error: filename is null\n");
			return;
		}
	}

	new = create_new_doc(filename);

	if(open_file && filename != NULL) {
		char *contents = NULL;

		result = g_file_get_contents(filename, &contents, &length, &error);
		g_free(filename);
		if(!result) {
			if(error) {
				error_dialog(error->message);
				g_error_free(error);
			} else
				error_dialog("Error: cannot read file\n");
			free_document(new);
			return;
		}

		if(!(g_utf8_validate(contents, length, NULL))) {
			error_dialog("Error: file contents were not utf-8\n");
			g_free(contents);
			return; //FIXME: segfault
		}

		insert_into_buffer(new->view, contents);
		g_free(contents);
	}

	lang = guess_language(new);
	set_language(new, lang);

	append_new_tab(new);
}

void
insert_into_view(Document *doc) {
	GtkSourceLanguage *lang;
	char *filename;
	char *contents;
	gsize length;
	gboolean result;
	GError *error = NULL;

	filename = open_get_filename();

	if(filename == NULL) {
		error_dialog("Error: filename is null\n"); //FIXME: segfault
		return;
	}

	result = g_file_get_contents(filename, &contents, &length, &error);
	if(!result) {
		if(error) {
			error_dialog(error->message);
			g_error_free(error);
		} else
			error_dialog("Error: cannot read file\n");
		g_free(filename);
		return;
	}

	if(!(g_utf8_validate(contents, length, NULL))) {
		error_dialog("Error: file contents were not utf-8\n");
		g_free(contents);
		g_free(filename);
		return;
	}

	insert_into_buffer(doc->view, contents);

	doc->new = FALSE;
	g_free(doc->basename);
	doc->basename = g_path_get_basename(filename);
	g_free(doc->filename);
	doc->filename = g_strdup(filename);
	lang = guess_language(doc);
	set_language(doc, lang);
	update_tab_label(doc);
	g_free(filename);
	g_free(contents);
}
