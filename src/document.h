/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * document.h
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

#include <gtksourceview/gtksource.h>

Document *create_new_doc(const char *filename);
void free_document(Document *doc);
GtkSourceLanguage *guess_language(Document *doc);
void set_language(Document *doc, GtkSourceLanguage *lang);
void insert_into_buffer(GtkWidget *view, char *content);
