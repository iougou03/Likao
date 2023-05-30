#include <gtk/gtk.h>

#include "../lib/likao_chat.h"

#if !defined(HEADER_APP_WINDOW)
#define HEADER_APP_WINDOW

GtkBuilder *builder;
GtkWidget *window;
GObject *stack;
GObject *gtk_stack;

#endif // HEADER_APP_WINDOW
