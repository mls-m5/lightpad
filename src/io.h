/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * io.h
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

void save_to_file(GtkWindow *parent, Editor *editor, gboolean save_as);
char *open_get_filename(GtkWindow *parent);
int load_file(char *filename, char **contents);
void open_file(GtkWindow *parent, gboolean existing);
void insert_file(GtkWindow *parent, Editor *editor, GtkWidget *scroll);
