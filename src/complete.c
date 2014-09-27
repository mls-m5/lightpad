/*
 * complete.c
 *
 *  Created on: 27 sep 2014
 *      Author: Mattias Larsson Sköld
 */
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <cairo/cairo.h>
#include <stdio.h>
#include <string.h>

#include "lightpad.h"
#include "popup.h"

int currentWordLength;

int compareStrings(const char *str1,const char *str2){
	int i = 0;
	while(str1[i] != 0 && str2[i] != 0){
		if (str1[i] != str2[i]){
			return 0;
		}
		++i;
	}

	return 1;
}

void exec(char* cmd, char *input) {
    FILE* pipe;
    pipe = popen(cmd, "w");
    if (!pipe) return;
    char buffer[10000];
    fprintf(pipe, input);
    fprintf(pipe, "\0");
    pclose(pipe);

    pipe = fopen(".tmp", "r");

    clearPopup();

    while(!feof(pipe)) {
    	if(fgets(buffer, 10000, pipe) != NULL){
    		printf(buffer);
    		populate(buffer);
    	}
    }
    fclose(pipe);
}

void selectAlternative(char *text){
	hidePopup();
	printf("vald: %s\n", text);

	const char *prefix = "COMPLETION: ";
	int len = strlen(prefix);

	int textLen = strlen(text);
	if (compareStrings(text, prefix)){
		for (int i = len; text[i] != 0; ++i){
			if (text[i] == ':'){
					i --;
					textLen = i - len;

					break;
			}
		}
	}
	else {
		len = 0;
	}



	Document *document = get_active_document();
	GtkTextBuffer *buffer;

	len += currentWordLength;
	textLen -= currentWordLength;

	char subString[textLen + 1];

	memcpy(subString, &text[len], textLen);
	subString[textLen] = 0;
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(document->view));
	printf("infogar %s, längd %d\n", subString, textLen);


	//Convert to utf-8
	char output[10];
	char *c = subString;
	GError *error = NULL;
	char *utf8_text = g_convert (subString, textLen, "UTF-8", "ISO-8859-1",
	                            NULL, NULL, &error);
	gtk_text_buffer_insert_at_cursor( buffer, utf8_text, strlen(utf8_text) );
}

int currentWord(GtkTextIter iter, char * ret){
	GtkTextIter start, stop = iter;
	int breakLoop = 0;
	for (start = iter; gtk_text_iter_get_offset(&start) > 0; gtk_text_iter_backward_char(&start)){
		char c = gtk_text_iter_get_char(&start);
		switch(c){
		case '.':
		case '\t':
		case ' ':
		case '\n':
		case ';':
			breakLoop = 1;
			gtk_text_iter_forward_char(&start);

			break;
		}
		if (breakLoop){
			break;
		}
	}
	int i;
	int length = gtk_text_iter_get_offset(&stop) - gtk_text_iter_get_offset(&start) + 1;
	if (ret){
		for (i = 0; i < length; ++i){
			ret[i] = gtk_text_iter_get_char(&start);
			gtk_text_iter_forward_char(&start);
		}
		ret[i] = 0;
	}
	return length;
}

void runComplete() {
	Document *document = get_active_document();

	GtkTextBuffer *buffer;
	GtkTextIter start, end, cursorPos;
	cairo_rectangle_int_t strong, weak;
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(document->view));

	gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(buffer), &start);
	gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(buffer), &end);
	GtkTextMark* mark = gtk_text_buffer_get_insert(GTK_TEXT_BUFFER(buffer));
	gtk_text_buffer_get_iter_at_mark(buffer, &cursorPos, mark);
	gtk_text_view_get_cursor_locations(GTK_TEXT_VIEW(document->view), &cursorPos, &strong, &weak);
	setPosition(strong.x, strong.y);
	gtk_text_iter_backward_char(&cursorPos);
	int line = gtk_text_iter_get_line(&cursorPos);
	int col = gtk_text_iter_get_line_offset(&cursorPos);
	char c = gtk_text_iter_get_char(&cursorPos);
	printf ("char %c\n", c);

	char* contents;
	contents = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(buffer), &start, &end, FALSE);

	char commandLine[400];
	char word[400];
	int wordLength = currentWord(cursorPos, word);
	currentWordLength = wordLength;
	printf ("word %s, length %d\n", word, wordLength);
	printf("\n=====\n");
	int corrCol = col - wordLength + 2;
//	sprintf(commandLine, "clang -stdlib=libc++ -std=c++11 -x c++ -fsyntax-only -code-completion-at -:%d:%d - | grep %s>.tmp", line, col - wordLength+ 3, word);

	line ++;
	if (wordLength > 0){
		sprintf(commandLine, "clang -cc1 -x c++ -fsyntax-only -code-completion-at -:%d:%d - | grep %s>.tmp", line, corrCol, word);
	}
	else{
		sprintf(commandLine, "clang -cc1 -x c++ -fsyntax-only -code-completion-at -:%d:%d ->.tmp", line, corrCol);
	}


//	sprintf(commandLine, "clang -cc1 -x c++ -fsyntax-only -code-completion-at -:%d:%d - | grep %s>.tmp", line, col - wordLength+ 3, word);

	exec(commandLine, contents);
	printf("rad %d, kolumn %d, korr-kol %d\n" , line, col, corrCol);
}
