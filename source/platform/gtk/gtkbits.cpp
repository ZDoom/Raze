#include "compat.h"

#include "dynamicgtk.h"

#include "baselayer.h"
#include "build.h"

#include "gtkbits.h"

int32_t gtkenabled = 0;

static GdkPixbuf *appicon = NULL;

int gtkbuild_msgbox(const char *name, const char *msg)
{
    GtkWidget *dialog;

    if (!gtkenabled) return -1;

    dialog = gtk_message_dialog_new(NULL,
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_INFO,
                                    GTK_BUTTONS_OK,
                                    "%s", msg);
    gtk_window_set_title(GTK_WINDOW(dialog), name);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    return 1;
}

int gtkbuild_ynbox(const char *name, const char *msg)
{
    int32_t r;
    GtkWidget *dialog;

    if (!gtkenabled) return -1;

    dialog = gtk_message_dialog_new(NULL,
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_INFO,
                                    GTK_BUTTONS_YES_NO,
                                    "%s", msg);
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
void gtkbuild_init(int32_t *argc, char ***argv)
{
    gtkenabled = dynamicgtk_init();
    if (gtkenabled < 0)
    {
        gtkenabled = 0;
        return;
    }

    gtkenabled = gtk_init_check(argc, argv);
    if (!gtkenabled) return;
#ifdef RENDERTYPESDL
    appicon = gdk_pixbuf_new_from_data((const guchar *)sdlappicon.pixels,
                                       GDK_COLORSPACE_RGB, TRUE, 8, sdlappicon.width, sdlappicon.height,
                                       sdlappicon.width*4, NULL, NULL);
#endif
    if (appicon) gtk_window_set_default_icon(appicon);
}

void gtkbuild_exit(int32_t r)
{
    UNREFERENCED_PARAMETER(r);
    if (gtkenabled)
    {
        if (appicon) g_object_unref((gpointer)appicon);
    }

    dynamicgtk_uninit();
}
