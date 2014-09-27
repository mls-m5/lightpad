/*
 * popup.c
 *
 *  Created on: 27 sep 2014
 *      Author: Mattias Larsson Sköld
 */



#include <gtk/gtk.h>
#include <string.h>

#include "complete.h"
#include "lightpad.h"

void clearPopup(){
	//Remove all children
	GList *children, *iter;
	children = gtk_container_get_children(GTK_CONTAINER(lightpad->popupBox));
	for(iter = children; iter != NULL; iter = g_list_next(iter)){
		gtk_widget_destroy(GTK_WIDGET(iter->data));
	}
	g_list_free(children);
}

static gboolean
on_click_event (GtkWidget *widget,
                 GdkEvent  *event,
                 gpointer   data)
{
	gtk_widget_hide(lightpad->popup);
	selectAlternative(gtk_button_get_label(GTK_BUTTON(widget)));
	return 0;
}

void selectPopup(int num){
	gtk_widget_hide(lightpad->popup);
	GList *children, *iter;
	int i = 0;
	children = gtk_container_get_children(GTK_CONTAINER(lightpad->popupBox));
	for(iter = children; iter != NULL; iter = g_list_next(iter)){
		if (i == num){
			on_click_event(GTK_WIDGET(iter->data), 0,0);
		}
		++i;
	}
}

void setPosition(int x, int y){
	int windowX = 0, windowY = 0;
	int rootX = 0, rootY = 0;
	gdk_window_get_root_coords(GDK_WINDOW(lightpad->window), &windowX, &windowY, &rootX, &rootY);
	gtk_window_move(GTK_WINDOW(lightpad->popup), windowX + x, windowY + y);
}

void hidePopup(){
	gtk_widget_hide(lightpad->popup);
}

void populate(const char *text){
	int start = 0;
	int stop = 0;


	char subbuff[100];
	for (int i = 0; text[i] != 0; ++i){
		char c = text[i];
		if (c == '\n'){
			stop = i;
			memcpy( subbuff, &text[start], stop - start );
			subbuff[stop-start] = 0;
			GtkWidget *button = gtk_button_new_with_label(subbuff);
			gtk_widget_get_can_focus(GTK_WIDGET(button));
			gtk_box_pack_end(GTK_BOX(lightpad->popupBox), button, 1, 1, 0);
			g_signal_connect(GTK_WIDGET(button), "clicked", G_CALLBACK(on_click_event), 0);
			gtk_container_set_focus_child(lightpad->popupBox, button);

			start = i;
		}
	}

	gtk_widget_activate(lightpad->popup);

	gtk_widget_show_all(lightpad->popup);
}
