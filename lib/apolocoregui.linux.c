/* Copyright (C) 2017 Luiz Romário Santana Rios

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

#include "apolocoregui.h"

#include <gtk/gtk.h>

int argc = 0;
char **argv = NULL;
int initialized = 0;

#include <stdio.h>

void dialog_on_response(GtkDialog *diag, gint response_id, gpointer user_data)
{
    printf("%d\n", response_id);
    gtk_widget_destroy(GTK_WIDGET (diag));
}

int native_dialog_show(enum dialog_type t, const char *title, const char *body)
{
    GtkWidget *diag;
    GtkMessageType type;
    GtkButtonsType buttons;
    gint res;

    if (!initialized) {
        gtk_init(&argc, &argv);
        initialized = 1;
    }

    switch (t) {
    case Info:
        type = GTK_MESSAGE_INFO;
        buttons = GTK_BUTTONS_OK;
        break;
    case Warning:
        type = GTK_MESSAGE_WARNING;
        buttons = GTK_BUTTONS_OK;
        break;
    case Question:
        type = GTK_MESSAGE_QUESTION;
        buttons = GTK_BUTTONS_YES_NO;
        break;
    case Fatal:
        type = GTK_MESSAGE_ERROR;
        buttons = GTK_BUTTONS_CLOSE;
        break;
    default:
        return 0;
    }

    diag = gtk_message_dialog_new(NULL, 0, type, buttons, body);
    gtk_window_set_title(GTK_WINDOW (diag), title);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW (diag), FALSE);

    g_signal_connect(
        G_OBJECT (diag), "response", G_CALLBACK (dialog_on_response), NULL);

    switch (gtk_dialog_run(GTK_DIALOG (diag))) {
    case GTK_RESPONSE_ACCEPT:
        res = 1;
        break;
    default:
        res = 0;
        break;
    }

    return res;
}
