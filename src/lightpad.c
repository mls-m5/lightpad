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
#include "document.h"
#include "callbacks.h"
#include "io.h"

/*
 * TODO:
 * have sourceview grab focus
 * reorderable tabs with keyboard
     gtk_notebook_reorder_child
 * open file dialog should start in current folder
     gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), path);
 * look into line marks
 * backup copy?
 * auto-save every n minutes?
 * commandline
 * search (and replace)
     GtkSourceSearchContext
 */

void /*TODO: transform this into a fancy GtkInfoBar */
error_dialog(const char *message) {
	GtkWidget *dialog;

	g_fprintf(stderr, message);

	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, message);
	gtk_window_set_title(GTK_WINDOW(dialog), "Error!");
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	g_free((gpointer)message);
}

void
append_new_tab(Document *doc) {
	GtkWidget *scroll, *label;

	label = gtk_label_new(doc->basename);

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC,
			GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_NONE);
	gtk_widget_set_vexpand(scroll, TRUE);
	gtk_container_add(GTK_CONTAINER(scroll), doc->view);

	/* Store the structure inside the GtkScrolledWindow.
	 * This way, we can use GtkNotebook to supply the
	 * correct Document struct instead of keeping an
	 * additional list ourselves.
	 */
	g_object_set_data_full(G_OBJECT(scroll), "doc", doc, (GDestroyNotify)free_document);
	gtk_widget_show_all(scroll);
	gtk_notebook_append_page(GTK_NOTEBOOK(lightpad->tabs), scroll, label);
	gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(lightpad->tabs), scroll, TRUE);
}

void
close_tab(void) {
	Document *doc;
	GtkWidget *scroll;
	int index;

	/* Since a keypress only works on the currently active
	 * tab anyway, we might as well ask the index of the current
	 * page here and use that, instead of passing the index as
	 * a parameter.
	 * Also, since we need both the GtkScrolledWindow and the Document
	 * it is of no use to call get_active_document().
	 */
	index = gtk_notebook_get_current_page(GTK_NOTEBOOK(lightpad->tabs));
	scroll = gtk_notebook_get_nth_page(GTK_NOTEBOOK(lightpad->tabs), index);
	doc = g_object_get_data(G_OBJECT(scroll), "doc");
	if(doc->modified) {
		int save = check_for_save(doc);
		switch(save) {
			case GTK_RESPONSE_YES:
				if(save_to_file(doc, TRUE) < 0)
					return;
			case GTK_RESPONSE_NO: break;
			default: return;
		}
	}
	gtk_widget_destroy(scroll); /* this destroys both scroll's child and its container */
}

Document *
get_active_document(void) {
	GtkWidget *scroll;
	Document *doc = NULL;
	int index;

	if(gtk_notebook_get_n_pages(GTK_NOTEBOOK(lightpad->tabs)) > 0) {
		index = gtk_notebook_get_current_page(GTK_NOTEBOOK(lightpad->tabs));
		scroll = gtk_notebook_get_nth_page(GTK_NOTEBOOK(lightpad->tabs), index);
		doc = g_object_get_data(G_OBJECT(scroll), "doc");
	}

	return doc;
}

int
check_for_save(Document *doc) {
	int res = 0;

	if(doc->modified) {
		GtkWidget *dlg;
		char *msg;

		msg = g_strdup_printf(
				_("Do you want to save the changes in file '%s' before closing?\n"),
				doc->basename);
		dlg = gtk_message_dialog_new(GTK_WINDOW(lightpad->window),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE, msg);
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dlg),
				_("If you do not, the changes will be lost.\n"));
		gtk_window_set_title(GTK_WINDOW(dlg), _("Save changes?"));
		gtk_dialog_add_buttons(GTK_DIALOG(dlg), _("Close without saving"), GTK_RESPONSE_NO,
				_("Cancel"), GTK_RESPONSE_CANCEL, _("Save"), GTK_RESPONSE_YES, NULL);
		gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_YES);
		res = gtk_dialog_run(GTK_DIALOG(dlg));
		gtk_widget_destroy(dlg);
		g_free(msg);
	}

	return res;
}

static void
set_config(GKeyFile *cfg) {
	settings = g_slice_new0(Settings);
	//TODO: this might need error-checking
	settings->font = g_key_file_get_string(cfg, "Lightpad", "font", NULL);
	settings->scheme = g_key_file_get_string(cfg, "Lightpad", "scheme", NULL);
	settings->highlight_syntax = g_key_file_get_boolean(cfg, "Lightpad", "highlight_syntax", NULL);
	settings->highlight_curr_line = g_key_file_get_boolean(cfg, "Lightpad", "highlight_curr_line", NULL);
	settings->highlight_brackets = g_key_file_get_boolean(cfg, "Lightpad", "highlight_brackets", NULL);
	settings->auto_indent = g_key_file_get_boolean(cfg, "Lightpad", "auto_indent", NULL);
	settings->indent_width = g_key_file_get_integer(cfg, "Lightpad", "indent_width", NULL);
	settings->spaces_io_tabs = g_key_file_get_boolean(cfg, "Lightpad", "spaces_io_tabs", NULL);
	settings->smart_home_end = g_key_file_get_integer(cfg, "Lightpad", "smart_home_end", NULL);
	settings->wrap_mode = g_key_file_get_integer(cfg, "Lightpad", "wrap_mode", NULL);
	settings->line_numbers = g_key_file_get_boolean(cfg, "Lightpad", "line_numbers", NULL);
	settings->show_right_margin = g_key_file_get_boolean(cfg, "Lightpad", "show_right_margin", NULL);
	settings->right_margin_pos = g_key_file_get_integer(cfg, "Lightpad", "right_margin_pos", NULL);
	settings->tab_width = g_key_file_get_integer(cfg, "Lightpad", "tab_width", NULL);
	settings->draw_spaces = g_key_file_get_integer(cfg, "Lightpad", "draw_spaces", NULL);
}

static int
init_config(void) {
	GKeyFile *cfg;
	const char *path;
	char *file;
	gboolean loaded = FALSE;

	cfg = g_key_file_new();
	path = g_get_user_config_dir();
	file = g_build_filename(path, "lightpad/lightpad.cfg", NULL);
	g_free((gpointer)path);

	loaded = g_key_file_load_from_file(cfg, file, G_KEY_FILE_NONE, NULL);

	for(const char *const *dir = g_get_system_config_dirs(); !loaded && *dir; dir++) {
		file = g_build_filename(*dir, "/lightpad/lightpad.cfg", NULL);
		loaded = g_key_file_load_from_file(cfg, file, G_KEY_FILE_NONE, NULL);
	}

	if(loaded)
		set_config(cfg);
	else {
		g_fprintf(stderr, "Error: cannot find a configuration file!\nPlease make sure Lightpad\
				is installed correctly!\n");
		return -1;
	}

	g_free(file);
	g_key_file_free(cfg);

	return 0;
}

int
main(int argc, char **argv) {
	GtkWidget *vbox;

	gtk_init(&argc, &argv);

	lightpad = g_slice_new0(Window);
	lightpad->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(lightpad->window), "Lightpad");
	gtk_window_set_default_icon_name("accessories-text-editor");
	gtk_container_set_border_width(GTK_CONTAINER(lightpad->window), 0);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(lightpad->window), vbox);

	lightpad->tabs = gtk_notebook_new();
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(lightpad->tabs), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), lightpad->tabs, TRUE, TRUE, 0);

	g_signal_connect(lightpad->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(lightpad->window, "delete-event", G_CALLBACK(on_delete_window), NULL);
	g_signal_connect(lightpad->window, "key-press-event", G_CALLBACK(on_keypress_window), NULL);
	g_signal_connect(lightpad->tabs, "page-added", G_CALLBACK(on_page_added), NULL);

	if(init_config() < 0)
		return -1;

	new_view(NULL);

	gtk_widget_show_all(lightpad->window);
	gtk_main();

	/* This will destroy all the children, who in turn
	 * will destroy their own children et cetera.
	 * Thus, there is no need to explicitly destroy
	 * all the GtkScrolledWindow objects
	 */
	gtk_widget_destroy(lightpad->window);
	g_slice_free(Window, lightpad);
	g_free((gpointer)settings->font);
	g_free((gpointer)settings->scheme);
	g_slice_free(Settings, settings);
	return 0;
}
