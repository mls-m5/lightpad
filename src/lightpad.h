/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * lightpad.h
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

typedef struct {
	GtkWidget *view;
	char *filename;
	gboolean new;
} Editor;

void error_dialog(const gchar *message);
void append_new_tab(Editor *editor);
void insert_into_view(GtkWidget *view, char *content);
gboolean on_keypress_view(GtkWidget *widget, GdkEventKey *event);
gboolean on_keypress_window(GtkWidget *widget, GdkEventKey *event);

GtkWidget *tabs;
