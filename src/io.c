/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * io.c
 * Copyright (C) 2014 Jente Hidskes <hjdskes@gmail.com>
 *
 * sedit is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * sedit is distributed in the hope that it will be useful, but
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

#include "sedit.h"
#include "io.h"

void
save_to_file(GtkWindow *parent, Editor *editor, gboolean save_as) {
	GtkWidget *dialog;
	GtkTextBuffer *buffer;
	GtkTextIter start, end;
	char *path = NULL, *contents;
	gboolean status = FALSE;
	GError *error = NULL;

	if(save_as || editor->new) {
		dialog = gtk_file_chooser_dialog_new(_("Save File"), parent,
				GTK_FILE_CHOOSER_ACTION_SAVE, _("_Cancel"), GTK_RESPONSE_CANCEL,
				_("_Save"), GTK_RESPONSE_ACCEPT, NULL);

		gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

		if(save_as)
			gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), editor->filename);
		else
			gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "Untitled document");

		if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
			path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		gtk_widget_destroy(dialog);
	} else
		path = editor->filename;	

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(editor->view));
	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(buffer), &start);
	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(buffer), &end);
	contents = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(buffer), &start, &end, FALSE);       
	gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(buffer), FALSE);

	status = g_file_set_contents(path, contents, -1, &error);
	g_free(contents);
	g_free(path);
	if(!status) {
		if(error) {
			error_msg(error->message);
			g_clear_error(&error);
		} else
			error_msg("Error: cannot save to file. Something went wrong\n"); //FIXME: segfault
	}
}

char*
open_get_filename(GtkWindow *parent) {
	GtkWidget *dialog;
	char *filename = NULL;

	dialog = gtk_file_chooser_dialog_new(_("Open File"), parent,
			GTK_FILE_CHOOSER_ACTION_OPEN, _("_Cancel"), GTK_RESPONSE_CANCEL,
			_("_Open"), GTK_RESPONSE_ACCEPT, NULL);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	gtk_widget_destroy(dialog);
	return filename;
}

int
load_file(char *filename, char **contents) {
	gsize length;
	gboolean status;
	GError *error = NULL;

	status = g_file_get_contents(filename, contents, &length, &error);
	if(!status) {
		if(error) {
			error_msg(error->message);
			g_clear_error(&error);
		} else
			error_msg("Error: can't read file\n");
		return -1;
	}

	/*if(!(g_utf8_validate(contents, length, NULL))) {
		g_fprintf(stderr, "Error: file contents were not utf-8\n");
		g_free(contents);
		return -1;
	}*/

	return 0;
}

void
open_file(GtkWindow *parent, gboolean existing) {
	Editor *new;
	GtkSourceBuffer *buffer;
	char *contents = NULL, *filename = NULL;

	if(existing) {
		filename = open_get_filename(parent);

		if(filename == NULL) {
			error_msg("Error: filename is null\n");
			return;
		}
	}

	new = g_slice_new(Editor);
	buffer = gtk_source_buffer_new(NULL);
	new->view = gtk_source_view_new_with_buffer(buffer);
	g_signal_connect(new->view, "key-press-event", G_CALLBACK(on_keypress_view), NULL);

	if(existing) {
		if(load_file(filename, &contents) < 0) {
			error_msg("Error: cannot read file\n");
			g_free(filename);
			//TODO: free new
			return;
		}

		insert_into_view(new->view, contents);
		new->filename = g_strdup(filename);
		new->new = FALSE;
		g_free(filename);
	} else {
		new->filename = _("New file");
		new->new = TRUE;
	}

	/* store the structure inside the view itself
	 * this way, we can use GtkNotebook to supply the
	 * correct Editor struct instead of keeping an
	 * additional list ourselves
	 */
	g_object_set_data(G_OBJECT(new->view), "struct", new);
	append_new_tab(new);
}

void
insert_file(GtkWindow *parent, Editor *editor, GtkWidget *scroll) {
	char *filename = NULL, *contents = NULL, *basename;

	filename = open_get_filename(parent);

	if(filename == NULL) {
		error_msg("Error: filename is null\n");
		return;
	}

	if(load_file(filename, &contents) < 0) {
		error_msg("Error: cannot read file\n");
		g_free(filename);
		return;
	}

	insert_into_view(editor->view, contents);

	editor->new = FALSE;
	editor->filename = filename;
	basename = g_path_get_basename(filename);
	gtk_notebook_set_tab_label_text(GTK_NOTEBOOK(tabs), scroll, basename);
	g_free(basename);
	g_free(filename);
}
