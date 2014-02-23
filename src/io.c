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
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>

#include "lightpad.h"
#include "editor.h"
#include "io.h"

#define HIGHLIGHT          TRUE
#define HIGHLIGHT_BRACKETS TRUE
#define STYLE_SCHEME       "oblivion"

void
save_to_file(Document *doc, gboolean saveas) {
	GtkWidget *dialog;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	gchar *path = NULL, *contents, *status;
	gboolean result = FALSE;
	GError *error = NULL;

	if(saveas || doc->new) {
		dialog = gtk_file_chooser_dialog_new(_("Save File"), GTK_WINDOW(lightpad->window),
				GTK_FILE_CHOOSER_ACTION_SAVE, _("_Cancel"), GTK_RESPONSE_CANCEL,
				_("_Save"), GTK_RESPONSE_ACCEPT, NULL);

		gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

		if(saveas)
			gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), doc->filename);
		else
			gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), _("Untitled document"));

		if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
			path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
		gtk_widget_destroy(dialog);
		if(path == NULL)
			return;

		if(doc->filename != NULL)
			g_free(doc->filename);
		doc->filename = g_strdup(path);
	} else
		path = doc->filename;

	status = g_strdup_printf("Saving %s...", path);
	gtk_statusbar_push(GTK_STATUSBAR(lightpad->status), lightpad->id, status);
	g_free(status);
	while(gtk_events_pending()) gtk_main_iteration();

	gtk_widget_set_sensitive(doc->view, FALSE);
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(doc->view));
	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(buffer), &start);
	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(buffer), &end);
	contents = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(buffer), &start, &end, FALSE);       
	gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(buffer), FALSE);
	gtk_widget_set_sensitive(doc->view, TRUE);

	result = g_file_set_contents(path, contents, -1, &error);
	g_free(contents);
	g_free(path);
	gtk_statusbar_pop(GTK_STATUSBAR(lightpad->status), lightpad->id);
	reset_default_status(doc);
	if(!result) {
		if(error) {
			error_bar(error->message);
			g_error_free(error);
		} else
			error_bar("Error: cannot save to file. Something went wrong\n");
		return;
	}
}

gchar*
open_get_filename(void) {
	GtkWidget *dialog;
	gchar *filename = NULL;

	dialog = gtk_file_chooser_dialog_new(_("Open File"), GTK_WINDOW(lightpad->window),
			GTK_FILE_CHOOSER_ACTION_OPEN, _("_Cancel"), GTK_RESPONSE_CANCEL,
			_("_Open"), GTK_RESPONSE_ACCEPT, NULL);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	gtk_widget_destroy(dialog);
	return filename;
}

void
open_file(gboolean existing) {
	Document *new;
	GtkSourceBuffer *buffer;
	gchar *filename = NULL, *status;
	gsize length;
	gboolean result;
	GError *error = NULL;

	if(existing) {
		filename = open_get_filename();

		if(filename == NULL) {
			error_bar("Error: filename is null\n");
			return;
		}
		status = g_strdup_printf("Loading %s...", filename);
	} else
		status = g_strdup("Loading...");

	gtk_statusbar_push(GTK_STATUSBAR(lightpad->status),
			lightpad->id, status);
	g_free(status);
	while(gtk_events_pending()) gtk_main_iteration();

	new = g_slice_new0(Document);
	buffer = gtk_source_buffer_new(NULL);
	new->view = gtk_source_view_new_with_buffer(buffer);
	g_signal_connect(new->view, "key-press-event", G_CALLBACK(on_keypress_view), NULL);

	if(existing) {
		gchar *contents = NULL;

		result = g_file_get_contents(filename, &contents, &length, &error);
		if(!result) {
			if(error) {
				error_bar(error->message);
				g_error_free(error);
			} else
				error_bar("Error: cannot read file\n");
			g_free(filename);
			return;
		}

		if(!(g_utf8_validate(contents, length, NULL))) {
			g_fprintf(stderr, "Error: file contents were not utf-8\n");
			g_free(contents);
			g_free(filename);
			return;
		}

		insert_into_view(new->view, contents);
		new->filename = g_strdup(filename);
		new->new = FALSE;
		g_free(filename);
		g_free(contents);
	} else {
		new->filename = g_strdup(_("New file"));
		new->new = TRUE;
	}

	/* syntax, theme, etc */
	GtkSourceStyleScheme *scheme;

	gtk_source_buffer_set_highlight_syntax(buffer, HIGHLIGHT);
	gtk_source_buffer_set_highlight_matching_brackets(buffer, HIGHLIGHT_BRACKETS);
	scheme = get_style_scheme(STYLE_SCHEME);
	if(scheme != NULL)
		gtk_source_buffer_set_style_scheme(buffer, scheme);

	append_new_tab(new);
	gtk_statusbar_pop(GTK_STATUSBAR(lightpad->status), lightpad->id);
	reset_default_status(new);
}

void
insert_file(Document *doc, GtkWidget *scroll) {
	gchar *filename = NULL, *contents = NULL, *basename;
	gsize length;
	gboolean status;
	GError *error = NULL;

	filename = open_get_filename();

	if(filename == NULL) {
		error_bar("Error: filename is null\n");
		return;
	}

	status = g_file_get_contents(filename, &contents, &length, &error);
	if(!status) {
		if(error) {
			error_bar(error->message);
			g_error_free(error);
		} else
			error_bar("Error: cannot read file\n");
		g_free(filename);
		return;
	}

	if(!(g_utf8_validate(contents, length, NULL))) {
		g_fprintf(stderr, "Error: file contents were not utf-8\n");
		g_free(contents);
		g_free(filename);
		return;
	}

	insert_into_view(doc->view, contents);

	doc->new = FALSE;
	if(doc->filename != NULL)
		g_free(doc->filename);
	doc->filename = g_strdup(filename);
	basename = g_path_get_basename(filename);
	gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(lightpad->tabs), scroll, basename);
	reset_default_status(doc);
	g_free(basename);
	g_free(filename);
	g_free(contents);
}
