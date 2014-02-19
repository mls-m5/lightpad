#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>

typedef struct {
	GtkWidget *view;
	char *filename;
} Editor;

static void append_new_tab(Editor *editor);
static char *open_file_get_filename(GtkWindow *parent);
static void open_existing_file(GtkWindow *parent);
static void open_new_file(void);
static gboolean on_keypress_view(GtkWidget *widget, GdkEventKey *event);
static gboolean on_keypress_window(GtkWidget *widget, GdkEventKey *event);

GtkWidget *tabs;

static void
append_new_tab(Editor *editor) {
	GtkWidget *scroll, *label;
	char *basename;

	basename = g_path_get_basename(editor->filename);
	label = gtk_label_new(basename);
	g_free(basename);

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll), GTK_SHADOW_NONE);
	gtk_widget_set_vexpand(scroll, TRUE);

	gtk_container_add(GTK_CONTAINER(scroll), editor->view);

	if(gtk_notebook_append_page(GTK_NOTEBOOK(tabs), scroll, label) < 0)
		g_fprintf(stderr, "Error: failed to add new tab\n");
	gtk_widget_show_all(tabs); //FIXME: why is this necessary?
}

static char*
open_file_get_filename(GtkWindow *parent) {
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

static void
open_existing_file(GtkWindow *parent) {
	Editor *new;
	GtkSourceBuffer *buffer;
	GtkTextIter iter;
	char *contents, *filename;
	gsize length;
	gboolean status;
	GError *error = NULL;

	filename = open_file_get_filename(parent);

	if(filename == NULL) {
		g_fprintf(stderr, "Error: filename is null\n");
		return;
	}

	status = g_file_get_contents(filename, &contents, &length, &error);
	if(!status) {
		if(error) {
			g_fprintf(stderr, "Error: %s\n", error->message);
			g_clear_error(&error);
		} else
			g_fprintf(stderr, "Error: can't read file\n");
		return;
	}

	/* load the file */
	if(!(g_utf8_validate(contents, length, NULL))) {
		g_fprintf(stderr, "Error: file contents were not utf-8\n");
		g_free(contents);
		return;
	}

	new = g_slice_new(Editor);
	buffer = gtk_source_buffer_new(NULL);
	new->filename = g_strdup(filename);
	new->view = gtk_source_view_new_with_buffer(GTK_SOURCE_BUFFER(buffer));
	g_signal_connect(new->view, "key-press-event", G_CALLBACK(on_keypress_view), NULL);

	gtk_widget_set_sensitive(new->view, FALSE);
	gtk_source_buffer_begin_not_undoable_action(buffer);
	gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), contents, length);
	gtk_source_buffer_end_not_undoable_action(buffer);
	gtk_widget_set_sensitive(new->view, TRUE);

	/* to detect whether file has changed */
	gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(buffer), FALSE);

	/* move cursor to the beginning */
	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(buffer), &iter);
	gtk_text_buffer_place_cursor(GTK_TEXT_BUFFER(buffer), &iter);

	append_new_tab(new);
	g_free(filename);
	g_free(contents);
}

static void
open_new_file(void) {
	Editor *new;
	GtkSourceBuffer *buffer;

	new = g_slice_new(Editor);
	new->filename = _("New file");
	buffer = gtk_source_buffer_new(NULL);
	new->view = gtk_source_view_new_with_buffer(buffer);
	g_signal_connect(new->view, "key-press-event", G_CALLBACK(on_keypress_view), NULL);

	/* to detect whether file has changed */
	gtk_text_buffer_set_modified(GTK_TEXT_BUFFER(buffer), FALSE);

	append_new_tab(new);
}

static gboolean
on_keypress_view(GtkWidget *widget, GdkEventKey *event) {
	if((event->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK) {
		switch(event->keyval) {
			default: return FALSE; break;
		}
	}
	return FALSE;
}

/*
 * GtkWindow catches keybindings _before_ passing them to
 * the focused widget. 
 * Here we override GtkWindow's handler to do the same things that it
 * does, but in the opposite order and then we chain up to the grand
 * parent handler, skipping gtk_window_key_press_event.
 */
static gboolean
on_keypress_window(GtkWidget *widget, GdkEventKey *event) {
	GtkWindow *window = GTK_WINDOW(widget);
	gboolean handled = FALSE;

	/* handle focus widget key events */
	if(!handled)
		handled = gtk_window_propagate_key_event(window, event);

	/* handle mnemonics and accelerators */
	if(!handled)
		handled = gtk_window_activate_key(window, event);

	/* we went up all the way, these bindings are set on the window */
	if(!handled && (event->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK) {
		switch(event->keyval) {
			case GDK_KEY_o:
				/* TODO: detect whether we are in a new or existing document
				 * If new, we should open inside this tab
				 * else, open in new tab
				 */
				open_existing_file(window); return TRUE; break;
			case GDK_KEY_t:
			case GDK_KEY_n: open_new_file(); return TRUE; break;
			case GDK_KEY_w: /*close_tab(index);*/ return TRUE; break;
			case GDK_KEY_s: /*save_file(window, FALSE);*/ return TRUE; break;
			case GDK_KEY_S: /*save_file(window, TRUE);*/ return TRUE; break;
			case GDK_KEY_r: /*reload_file();*/ return TRUE; break;
			default: break;
		}
	}
	return handled;
}

int
main(int argc, char **argv) {
	GtkWidget *window;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Sedit");
	gtk_window_set_default_icon_name("accessories-text-editor");
	gtk_container_set_border_width(GTK_CONTAINER(window), 2);

	tabs = gtk_notebook_new();
	gtk_container_add(GTK_CONTAINER(window), tabs);

	open_new_file();

	g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
	g_signal_connect(window, "key-press-event", G_CALLBACK(on_keypress_window), NULL);

	gtk_widget_show_all(window);
	gtk_main();

	return 0;
}
