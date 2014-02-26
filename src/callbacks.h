/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * callbacks.h
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

gboolean on_keypress_view(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
gboolean on_keypress_window(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
gboolean on_delete_window(GtkWidget *widget, GdkEvent *event, gpointer user_data);
void on_page_switch(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data);
void on_page_added(GtkNotebook *notebook, GtkWidget *child, guint page_num, gpointer user_data);
void on_lang_changed(GtkComboBox *widget, gpointer user_data);
