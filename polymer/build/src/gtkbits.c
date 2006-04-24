#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <gdk-pixbuf/gdk-pixdata.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "baselayer.h"
#include "build.h"

static int gtkenabled = 0;

extern const GdkPixdata startbanner_pixdata;
static GtkWidget *startwin = NULL;
static GdkPixbuf *appicon = NULL;

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)

#define lookup_widget(x,w) \
	(GtkWidget*) g_object_get_data(G_OBJECT(x), w)

static gboolean on_startwin_delete_event (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    quitevent++;
    return TRUE;	// FALSE would let the event go through. we want the game to decide when to close
}

void gtkbuild_create_startwin(void)
{
    GtkWidget *banner, *label, *content, *scroll;
    GtkWidget *hbox1, *fixed1;
    GdkPixbuf *startbanner_pixbuf;

    if (!gtkenabled) return;

    startwin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (startwin), apptitle);
    gtk_window_set_position (GTK_WINDOW (startwin), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable (GTK_WINDOW (startwin), FALSE);
    gtk_window_set_type_hint (GTK_WINDOW (startwin), GDK_WINDOW_TYPE_HINT_DIALOG);

    hbox1 = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox1);
    gtk_container_add (GTK_CONTAINER (startwin), hbox1);

    startbanner_pixbuf = gdk_pixbuf_from_pixdata(&startbanner_pixdata, FALSE, NULL);
    banner = gtk_image_new_from_pixbuf(startbanner_pixbuf);
    g_object_unref((gpointer)startbanner_pixbuf);
    gtk_widget_show (banner);
    gtk_box_pack_start (GTK_BOX (hbox1), banner, FALSE, FALSE, 0);

    fixed1 = gtk_fixed_new ();
    gtk_widget_show (fixed1);
    gtk_box_pack_start (GTK_BOX (hbox1), fixed1, TRUE, TRUE, 0);
    gtk_widget_set_size_request (fixed1, 390, -1);

    label = gtk_label_new (startwin_labeltext);
    gtk_widget_show (label);
    gtk_fixed_put (GTK_FIXED (fixed1), label, 6, 6);
    gtk_widget_set_size_request (label, 378, 16);
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);

    scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scroll);
    gtk_fixed_put (GTK_FIXED (fixed1), scroll, 6, 28);
    gtk_widget_set_size_request (scroll, 378, 248);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

    content = gtk_text_view_new ();
    gtk_widget_show (content);
    gtk_container_add (GTK_CONTAINER(scroll), content);
    //gtk_fixed_put (GTK_FIXED (fixed1), content, 6, 28);
    gtk_widget_set_size_request (content, 378, 248);
    gtk_text_view_set_editable (GTK_TEXT_VIEW (content), FALSE);
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (content), GTK_WRAP_WORD);
    gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (content), FALSE);

    g_signal_connect ((gpointer) startwin, "delete_event",
                      G_CALLBACK (on_startwin_delete_event),
                      NULL);

    /* Store pointers to all widgets, for use by lookup_widget(). */
    GLADE_HOOKUP_OBJECT_NO_REF (startwin, startwin, "startwin");
    GLADE_HOOKUP_OBJECT (startwin, banner, "banner");
    GLADE_HOOKUP_OBJECT (startwin, label, "label");
    GLADE_HOOKUP_OBJECT (startwin, scroll, "scroll");
    GLADE_HOOKUP_OBJECT (startwin, content, "content");

    g_signal_connect((gpointer)startwin, "destroy", G_CALLBACK(gtk_widget_destroyed), (gpointer)&startwin);
    gtk_widget_show (startwin);
    gtk_main_iteration_do (FALSE);
}

void gtkbuild_settitle_startwin(const char *title)
{
    if (!gtkenabled || !startwin) return;
    gtk_window_set_title (GTK_WINDOW (startwin), title);
}

void gtkbuild_puts_startwin(const char *str)
{
    GtkWidget *textview;
    GtkTextBuffer *textbuffer;
    GtkTextIter enditer;
    GtkTextMark *mark;
    const char *aptr, *bptr;

    if (!gtkenabled || !startwin || !str) return;
    if (!(textview = lookup_widget(startwin, "content"))) return;
    textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));

    gtk_text_buffer_get_end_iter(textbuffer, &enditer);
    for (aptr = bptr = str; *aptr != 0; ) {
        switch (*bptr) {
        case '\b':
            if (bptr > aptr)
                gtk_text_buffer_insert(textbuffer, &enditer, (const gchar *)aptr, (gint)(bptr-aptr)-1);
#if GTK_CHECK_VERSION(2,6,0)
            gtk_text_buffer_backspace(textbuffer, &enditer, FALSE, TRUE);
#else
            {
                GtkTextIter iter2 = enditer;
                gtk_text_iter_backward_cursor_position(&iter2);
                //FIXME: this seems be deleting one too many chars somewhere!
                if (!gtk_text_iter_equal(&iter2, &enditer))
                    gtk_text_buffer_delete_interactive(textbuffer, &iter2, &enditer, TRUE);
            }
#endif
            aptr = ++bptr;
            break;
        case 0:
            if (bptr > aptr)
                gtk_text_buffer_insert(textbuffer, &enditer, (const gchar *)aptr, (gint)(bptr-aptr));
            aptr = bptr;
            break;
        case '\r':	// FIXME
        default:
            bptr++;
            break;
        }
    }

    mark = gtk_text_buffer_create_mark(textbuffer, NULL, &enditer, 1);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(textview), mark, 0.0, FALSE, 0.0, 1.0);
    gtk_text_buffer_delete_mark(textbuffer, mark);
}

void gtkbuild_close_startwin(void)
{
    if (!gtkenabled) return;
    if (startwin) {
        gtk_widget_destroy (startwin);
        startwin = NULL;
    }
}

void gtkbuild_update_startwin(void)
{
    if (!gtkenabled) return;
    gtk_main_iteration_do (FALSE);
}


int gtkbuild_msgbox(char *name, char *msg)
{
    GtkWidget *dialog;

    if (!gtkenabled) return -1;

    dialog = gtk_message_dialog_new(NULL,
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_INFO,
                                    GTK_BUTTONS_OK,
                                    msg);
    gtk_window_set_title(GTK_WINDOW(dialog), name);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    return 1;
}

int gtkbuild_ynbox(char *name, char *msg)
{
    int r;
    GtkWidget *dialog;

    if (!gtkenabled) return -1;

    dialog = gtk_message_dialog_new(NULL,
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_INFO,
                                    GTK_BUTTONS_YES_NO,
                                    msg);
    gtk_window_set_title(GTK_WINDOW(dialog), name);
    r = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (r == GTK_RESPONSE_YES) return 1;
    return 0;
}

#ifdef RENDERTYPESDL
#include "sdlayer.h"
extern struct sdlappicon sdlappicon;
#endif
void gtkbuild_init(int argc, char **argv)
{
    gtkenabled = gtk_init_check(argc, argv);
    if (!gtkenabled) return;
#ifdef RENDERTYPESDL
    appicon = gdk_pixbuf_new_from_data((const guchar *)sdlappicon.pixels,
                                       GDK_COLORSPACE_RGB, TRUE, 8, sdlappicon.width, sdlappicon.height,
                                       sdlappicon.width*4, NULL, NULL);
#endif
    if (appicon) gtk_window_set_default_icon(appicon);
}

void gtkbuild_exit(int r)
{
    if (!gtkenabled) return;
    if (appicon) g_object_unref((gpointer)appicon);
    //gtk_exit(r);
}

void *gtkbuild_get_app_icon(void)
{
    return appicon;
}
