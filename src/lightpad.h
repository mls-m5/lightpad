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
	const char *font;
	const char *scheme;
	gboolean highlight_syntax;
	gboolean highlight_curr_line;
	gboolean highlight_brackets;
	gboolean auto_indent;
	gboolean indent_on_tab;
	int indent_width;
	gboolean spaces_io_tabs;
	int smart_home_end;
	int wrap_mode;
	gboolean line_numbers;
	gboolean show_right_margin;
	int right_margin_pos;
	int tab_width;
	gboolean draw_spaces;
} Settings;

typedef struct {
	GtkWidget *window;
	GtkWidget *tabs;
	GtkWidget *status;
	GtkWidget *combo;
	guint id;
} Window;

void error_dialog(const char *message);
void append_new_tab(Document *doc);
void close_tab(void);
void update_tab_label(Document *doc);
Document *get_active_document(void);
gboolean check_for_save(Document *doc);
void reset_default_status(Document *doc);

Window *lightpad;
Settings *settings;
