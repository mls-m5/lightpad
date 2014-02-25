/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * lightpad.h
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

typedef struct {
	GtkWidget *view;
	char *filename;
	gboolean new;
} Document;

typedef struct {
	GtkWidget *window;
	GtkWidget *tabs;
	GtkWidget *status;
	guint id;
} Window;

void error_dialog(const char *message);
void append_new_tab(Document *doc);
void close_tab(Document *doc, int index);
void update_tab_label(Document *doc);
gboolean check_for_save(Document *doc);
void reset_default_status(Document *doc);

Window *lightpad;
