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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gtksourceview/gtksource.h>

#include "lightpad.h"
#include "document.h"
#include "io.h"

int
save_to_file(Document *doc, gboolean saveas) {
	GtkWidget *dialog;
	GtkSourceLanguage *lang;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	char *filename = NULL;
	char *contents;
	gboolean result = FALSE;
	GError *error = NULL;

	if(doc == NULL)
		return -1;

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
			return -1;

		doc->new = FALSE;
		g_free(doc->basename);
		doc->basename = g_path_get_basename(filename);
		g_free(doc->filename);
		doc->filename = g_strdup(filename);

		lang = guess_language(doc);
		set_language(doc, lang);
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
		return -1;
	}
	gtk_text_buffer_set_modified(buffer, FALSE);
	return 0;
}

int
open_get_filename(char **filename) {
	GtkWidget *dialog;
	int res;

	dialog = gtk_file_chooser_dialog_new(_("Open File"), GTK_WINDOW(lightpad->window),
			GTK_FILE_CHOOSER_ACTION_OPEN, _("_Cancel"), GTK_RESPONSE_CANCEL,
			_("_Open"), GTK_RESPONSE_ACCEPT, NULL);
	res = gtk_dialog_run(GTK_DIALOG(dialog));
	if(res == GTK_RESPONSE_ACCEPT)
		*filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	gtk_widget_destroy(dialog);
	return res;
}

void
new_view(const char *filename) {
	Document *new;

	new = create_new_doc(filename);
	append_new_tab(new);
	if(filename != NULL)
		insert_into_view(new, filename);
}

void
insert_into_view(Document *doc, const char *filename) {
	GtkSourceLanguage *lang;
	char *contents;
	gsize length;
	gboolean result;
	GError *error = NULL;

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
		return;
	}

	//FIXME: keep?
	//if(!(g_utf8_validate(contents, length, NULL))) {
	//	error_dialog("Error: file contents were not utf-8\n"); //FIXME: segfault
	//	g_free(contents);
	//	return;
	//}

	doc->new = FALSE;
	g_free(doc->basename);
	doc->basename = g_path_get_basename(filename);
	g_free(doc->filename);
	doc->filename = g_strdup(filename);

	insert_into_buffer(doc->view, contents);
	g_free(contents);

	lang = guess_language(doc);
	set_language(doc, lang);
}
