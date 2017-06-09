/* NOTE: Glade will generate code for a dialogue box which you should
 * then patch into this file whenever you make a change to the Glade
 * template.
 */

#include "compat.h"

#include "gtkpixdata.h"
#include <gdk-pixbuf/gdk-pixdata.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "dynamicgtk.h"

#include "baselayer.h"
#include "compat.h"
#include "build.h"

#define TAB_CONFIG 0
#define TAB_MESSAGES 1

static struct
{
    int fullscreen;
    int xdim3d, ydim3d, bpp3d;
    int forcesetup;
} settings;

extern int gtkenabled;

static GtkWidget *startwin = NULL;
static int retval = -1, mode = TAB_MESSAGES;

// -- SUPPORT FUNCTIONS -------------------------------------------------------

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
    g_object_set_data_full (G_OBJECT (component), name, \
                            gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
    g_object_set_data (G_OBJECT (component), name, widget)

#define lookup_widget(x,w) \
    (GtkWidget*) g_object_get_data(G_OBJECT(x), w)

static GdkPixbuf *load_banner(void)
{
    return gdk_pixbuf_from_pixdata((GdkPixdata const *)&startbanner_pixdata, FALSE, NULL);
}

static void SetPage(int n)
{
    if (!gtkenabled || !startwin) return;
    mode = n;
    gtk_notebook_set_current_page(GTK_NOTEBOOK(lookup_widget(startwin,"tabs")), n);

    // each control in the config page vertical layout plus the start button should be made (in)sensitive
    if (n == TAB_CONFIG) n = TRUE; else n = FALSE;
    gtk_widget_set_sensitive(lookup_widget(startwin,"startbutton"), n);
    gtk_container_foreach(GTK_CONTAINER(lookup_widget(startwin,"configvlayout")),
                          (GtkCallback)gtk_widget_set_sensitive, (gpointer)n);
}

static void on_vmode3dcombo_changed(GtkComboBox *, gpointer);
static void PopulateForm(void)
{
    int mode3d, i;
    GtkListStore *modes3d;
    GtkTreeIter iter;
    GtkComboBox *box3d;
    char buf[64];

    mode3d = checkvideomode(&settings.xdim3d, &settings.ydim3d, settings.bpp3d, settings.fullscreen, 1);
    if (mode3d < 0)
    {
        int i, cd[] = { 32, 24, 16, 15, 8, 0 };
        for (i=0; cd[i]; ) { if (cd[i] >= settings.bpp3d) i++; else break; }
        for (; cd[i]; i++)
        {
            mode3d = checkvideomode(&settings.xdim3d, &settings.ydim3d, cd[i], settings.fullscreen, 1);
            if (mode3d < 0) continue;
            settings.bpp3d = cd[i];
            break;
        }
    }

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(startwin,"fullscreencheck")), settings.fullscreen);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lookup_widget(startwin,"alwaysshowcheck")), settings.forcesetup);

    box3d = GTK_COMBO_BOX(lookup_widget(startwin,"vmode3dcombo"));
    modes3d = GTK_LIST_STORE(gtk_combo_box_get_model(box3d));
    gtk_list_store_clear(modes3d);

    for (i=0; i<validmodecnt; i++)
    {
        if (validmode[i].fs != settings.fullscreen) continue;

        // all modes get added to the 3D mode list
        Bsprintf(buf, "%ld x %ld %dbpp", validmode[i].xdim, validmode[i].ydim, validmode[i].bpp);
        gtk_list_store_append(modes3d, &iter);
        gtk_list_store_set(modes3d, &iter, 0,buf, 1,i, -1);
        if (i == mode3d)
        {
            g_signal_handlers_block_by_func(box3d, on_vmode3dcombo_changed, NULL);
            gtk_combo_box_set_active_iter(box3d, &iter);
            g_signal_handlers_unblock_by_func(box3d, on_vmode3dcombo_changed, NULL);
        }
    }
}

// -- EVENT CALLBACKS AND CREATION STUFF --------------------------------------

static void on_vmode3dcombo_changed(GtkComboBox *combobox, gpointer user_data)
{
    GtkTreeModel *data;
    GtkTreeIter iter;
    int val;
    if (!gtk_combo_box_get_active_iter(combobox, &iter)) return;
    if (!(data = gtk_combo_box_get_model(combobox))) return;
    gtk_tree_model_get(data, &iter, 1, &val, -1);
    settings.xdim3d = validmode[val].xdim;
    settings.ydim3d = validmode[val].ydim;
}

static void on_fullscreencheck_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    settings.fullscreen = (gtk_toggle_button_get_active(togglebutton) == TRUE);
    PopulateForm();
}

static void on_alwaysshowcheck_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    settings.forcesetup = (gtk_toggle_button_get_active(togglebutton) == TRUE);
}

static void on_cancelbutton_clicked(GtkButton *button, gpointer user_data)
{
    if (mode == TAB_CONFIG) { retval = 0; gtk_main_quit(); }
    else quitevent++;
}

static void on_startbutton_clicked(GtkButton *button, gpointer user_data)
{
    retval = 1;
    gtk_main_quit();
}

static gboolean on_startwin_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    if (mode == TAB_CONFIG) { retval = 0; gtk_main_quit(); }
    else quitevent++;
    return TRUE;    // FALSE would let the event go through. we want the game to decide when to close
}


static GtkWidget *create_window(void)
{
    GtkWidget *startwin;
    GtkWidget *hlayout;
    GtkWidget *banner;
    GtkWidget *vlayout;
    GtkWidget *tabs;
    GtkWidget *configvlayout;
    GtkWidget *configlayout;
    GtkWidget *vmode3dlabel;
    GtkWidget *vmode3dcombo;
    GtkWidget *fullscreencheck;
    GtkWidget *alwaysshowcheck;
    GtkWidget *configtab;
    GtkWidget *messagesscroll;
    GtkWidget *messagestext;
    GtkWidget *messagestab;
    GtkWidget *buttons;
    GtkWidget *cancelbutton;
    GtkWidget *cancelbuttonalign;
    GtkWidget *cancelbuttonlayout;
    GtkWidget *cancelbuttonicon;
    GtkWidget *cancelbuttonlabel;
    GtkWidget *startbutton;
    GtkWidget *startbuttonalign;
    GtkWidget *startbuttonlayout;
    GtkWidget *startbuttonicon;
    GtkWidget *startbuttonlabel;
    GtkAccelGroup *accel_group;

    accel_group = gtk_accel_group_new();

    // Basic window
    startwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(startwin), apptitle);     // NOTE: use global app title
    gtk_window_set_position(GTK_WINDOW(startwin), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(startwin), FALSE);
    gtk_window_set_type_hint(GTK_WINDOW(startwin), GDK_WINDOW_TYPE_HINT_DIALOG);

    // Horizontal layout of banner and controls
    hlayout = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hlayout);
    gtk_container_add(GTK_CONTAINER(startwin), hlayout);

    // Banner
    {
        GdkPixbuf *pixbuf = load_banner();
        banner = gtk_image_new_from_pixbuf(pixbuf);
        g_object_unref((gpointer)pixbuf);
    }
    gtk_widget_show(banner);
    gtk_box_pack_start(GTK_BOX(hlayout), banner, FALSE, FALSE, 0);
    gtk_misc_set_alignment(GTK_MISC(banner), 0.5, 0);

    // Vertical layout of tab control and start+cancel buttons
    vlayout = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(vlayout);
    gtk_box_pack_start(GTK_BOX(hlayout), vlayout, TRUE, TRUE, 0);

    // Tab control
    tabs = gtk_notebook_new();
    gtk_widget_show(tabs);
    gtk_box_pack_start(GTK_BOX(vlayout), tabs, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(tabs), 4);

    // Vertical layout of config page main body
    configvlayout = gtk_vbox_new(FALSE, 0);
    gtk_widget_show(configvlayout);
    gtk_container_add(GTK_CONTAINER(tabs), configvlayout);

    // Fixed-position layout of config page controls
    configlayout = gtk_fixed_new();
    gtk_widget_show(configlayout);
    gtk_box_pack_start(GTK_BOX(configvlayout), configlayout, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(configlayout), 6);

    // 3D video mode label
    vmode3dlabel = gtk_label_new_with_mnemonic("_Video mode:");
    gtk_widget_show(vmode3dlabel);
    gtk_fixed_put(GTK_FIXED(configlayout), vmode3dlabel, 0, 0);
    gtk_widget_set_size_request(vmode3dlabel, 88, 29);
    gtk_misc_set_alignment(GTK_MISC(vmode3dlabel), 0, 0.5);

    // 3D video mode combo
    {
        GtkListStore *list = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
        GtkCellRenderer *cell;

        vmode3dcombo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(list));
        g_object_unref(G_OBJECT(list));

        cell = gtk_cell_renderer_text_new();
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(vmode3dcombo), cell, FALSE);
        gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(vmode3dcombo), cell, "text", 0, NULL);
    }
    gtk_widget_show(vmode3dcombo);
    gtk_fixed_put(GTK_FIXED(configlayout), vmode3dcombo, 88, 0);
    gtk_widget_set_size_request(vmode3dcombo, 150, 29);
    gtk_widget_add_accelerator(vmode3dcombo, "grab_focus", accel_group,
                               GDK_V, GDK_MOD1_MASK,
                               GTK_ACCEL_VISIBLE);

    // Fullscreen checkbox
    fullscreencheck = gtk_check_button_new_with_mnemonic("_Fullscreen");
    gtk_widget_show(fullscreencheck);
    gtk_fixed_put(GTK_FIXED(configlayout), fullscreencheck, 248, 0);
    gtk_widget_set_size_request(fullscreencheck, 85, 29);
    gtk_widget_add_accelerator(fullscreencheck, "grab_focus", accel_group,
                               GDK_F, GDK_MOD1_MASK,
                               GTK_ACCEL_VISIBLE);

    // Always show config checkbox
    alwaysshowcheck = gtk_check_button_new_with_mnemonic("_Always show configuration on start");
    gtk_widget_show(alwaysshowcheck);
    gtk_box_pack_start(GTK_BOX(configvlayout), alwaysshowcheck, FALSE, FALSE, 0);
    gtk_widget_add_accelerator(alwaysshowcheck, "grab_focus", accel_group,
                               GDK_A, GDK_MOD1_MASK,
                               GTK_ACCEL_VISIBLE);

    // Configuration tab
    configtab = gtk_label_new("Configuration");
    gtk_widget_show(configtab);
    gtk_notebook_set_tab_label(GTK_NOTEBOOK(tabs), gtk_notebook_get_nth_page(GTK_NOTEBOOK(tabs), 0), configtab);

    // Messages scrollable area
    messagesscroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(messagesscroll);
    gtk_container_add(GTK_CONTAINER(tabs), messagesscroll);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(messagesscroll), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

    // Messages text area
    messagestext = gtk_text_view_new();
    gtk_widget_show(messagestext);
    gtk_container_add(GTK_CONTAINER(messagesscroll), messagestext);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(messagestext), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(messagestext), GTK_WRAP_WORD);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(messagestext), FALSE);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(messagestext), 2);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(messagestext), 2);

    // Messages tab
    messagestab = gtk_label_new("Messages");
    gtk_widget_show(messagestab);
    gtk_notebook_set_tab_label(GTK_NOTEBOOK(tabs), gtk_notebook_get_nth_page(GTK_NOTEBOOK(tabs), 1), messagestab);

    // Dialogue box buttons layout
    buttons = gtk_hbutton_box_new();
    gtk_widget_show(buttons);
    gtk_box_pack_start(GTK_BOX(vlayout), buttons, FALSE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(buttons), 3);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(buttons), GTK_BUTTONBOX_END);

    // Cancel button
    cancelbutton = gtk_button_new();
    gtk_widget_show(cancelbutton);
    gtk_container_add(GTK_CONTAINER(buttons), cancelbutton);
    GTK_WIDGET_SET_FLAGS(cancelbutton, GTK_CAN_DEFAULT);
    gtk_widget_add_accelerator(cancelbutton, "grab_focus", accel_group,
                               GDK_C, GDK_MOD1_MASK,
                               GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(cancelbutton, "clicked", accel_group,
                               GDK_Escape, 0,
                               GTK_ACCEL_VISIBLE);

    cancelbuttonalign = gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_widget_show(cancelbuttonalign);
    gtk_container_add(GTK_CONTAINER(cancelbutton), cancelbuttonalign);

    cancelbuttonlayout = gtk_hbox_new(FALSE, 2);
    gtk_widget_show(cancelbuttonlayout);
    gtk_container_add(GTK_CONTAINER(cancelbuttonalign), cancelbuttonlayout);

    cancelbuttonicon = gtk_image_new_from_stock("gtk-cancel", GTK_ICON_SIZE_BUTTON);
    gtk_widget_show(cancelbuttonicon);
    gtk_box_pack_start(GTK_BOX(cancelbuttonlayout), cancelbuttonicon, FALSE, FALSE, 0);

    cancelbuttonlabel = gtk_label_new_with_mnemonic("_Cancel");
    gtk_widget_show(cancelbuttonlabel);
    gtk_box_pack_start(GTK_BOX(cancelbuttonlayout), cancelbuttonlabel, FALSE, FALSE, 0);

    // Start button
    startbutton = gtk_button_new();
    gtk_widget_show(startbutton);
    gtk_container_add(GTK_CONTAINER(buttons), startbutton);
    GTK_WIDGET_SET_FLAGS(startbutton, GTK_CAN_DEFAULT);
    gtk_widget_add_accelerator(startbutton, "grab_focus", accel_group,
                               GDK_S, GDK_MOD1_MASK,
                               GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(startbutton, "clicked", accel_group,
                               GDK_Return, 0,
                               GTK_ACCEL_VISIBLE);

    startbuttonalign = gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_widget_show(startbuttonalign);
    gtk_container_add(GTK_CONTAINER(startbutton), startbuttonalign);

    startbuttonlayout = gtk_hbox_new(FALSE, 2);
    gtk_widget_show(startbuttonlayout);
    gtk_container_add(GTK_CONTAINER(startbuttonalign), startbuttonlayout);

    startbuttonicon = gtk_image_new_from_stock("gtk-execute", GTK_ICON_SIZE_BUTTON);
    gtk_widget_show(startbuttonicon);
    gtk_box_pack_start(GTK_BOX(startbuttonlayout), startbuttonicon, FALSE, FALSE, 0);

    startbuttonlabel = gtk_label_new_with_mnemonic("_Start");
    gtk_widget_show(startbuttonlabel);
    gtk_box_pack_start(GTK_BOX(startbuttonlayout), startbuttonlabel, FALSE, FALSE, 0);

    // Wire up the signals
    g_signal_connect((gpointer) startwin, "delete_event",
                     G_CALLBACK(on_startwin_delete_event),
                     NULL);
    g_signal_connect((gpointer) vmode3dcombo, "changed",
                     G_CALLBACK(on_vmode3dcombo_changed),
                     NULL);
    g_signal_connect((gpointer) fullscreencheck, "toggled",
                     G_CALLBACK(on_fullscreencheck_toggled),
                     NULL);
    g_signal_connect((gpointer) alwaysshowcheck, "toggled",
                     G_CALLBACK(on_alwaysshowcheck_toggled),
                     NULL);
    g_signal_connect((gpointer) cancelbutton, "clicked",
                     G_CALLBACK(on_cancelbutton_clicked),
                     NULL);
    g_signal_connect((gpointer) startbutton, "clicked",
                     G_CALLBACK(on_startbutton_clicked),
                     NULL);

    // Associate labels with their controls
    gtk_label_set_mnemonic_widget(GTK_LABEL(vmode3dlabel), vmode3dcombo);

    /* Store pointers to all widgets, for use by lookup_widget(). */
    GLADE_HOOKUP_OBJECT_NO_REF(startwin, startwin, "startwin");
    GLADE_HOOKUP_OBJECT(startwin, hlayout, "hlayout");
    GLADE_HOOKUP_OBJECT(startwin, banner, "banner");
    GLADE_HOOKUP_OBJECT(startwin, vlayout, "vlayout");
    GLADE_HOOKUP_OBJECT(startwin, tabs, "tabs");
    GLADE_HOOKUP_OBJECT(startwin, configvlayout, "configvlayout");
    GLADE_HOOKUP_OBJECT(startwin, configlayout, "configlayout");
    GLADE_HOOKUP_OBJECT(startwin, vmode3dlabel, "vmode3dlabel");
    GLADE_HOOKUP_OBJECT(startwin, vmode3dcombo, "vmode3dcombo");
    GLADE_HOOKUP_OBJECT(startwin, fullscreencheck, "fullscreencheck");
    GLADE_HOOKUP_OBJECT(startwin, alwaysshowcheck, "alwaysshowcheck");
    GLADE_HOOKUP_OBJECT(startwin, configtab, "configtab");
    GLADE_HOOKUP_OBJECT(startwin, messagesscroll, "messagesscroll");
    GLADE_HOOKUP_OBJECT(startwin, messagestext, "messagestext");
    GLADE_HOOKUP_OBJECT(startwin, messagestab, "messagestab");
    GLADE_HOOKUP_OBJECT(startwin, buttons, "buttons");
    GLADE_HOOKUP_OBJECT(startwin, cancelbutton, "cancelbutton");
    GLADE_HOOKUP_OBJECT(startwin, cancelbuttonalign, "cancelbuttonalign");
    GLADE_HOOKUP_OBJECT(startwin, cancelbuttonlayout, "cancelbuttonlayout");
    GLADE_HOOKUP_OBJECT(startwin, cancelbuttonicon, "cancelbuttonicon");
    GLADE_HOOKUP_OBJECT(startwin, cancelbuttonlabel, "cancelbuttonlabel");
    GLADE_HOOKUP_OBJECT(startwin, startbutton, "startbutton");
    GLADE_HOOKUP_OBJECT(startwin, startbuttonalign, "startbuttonalign");
    GLADE_HOOKUP_OBJECT(startwin, startbuttonlayout, "startbuttonlayout");
    GLADE_HOOKUP_OBJECT(startwin, startbuttonicon, "startbuttonicon");
    GLADE_HOOKUP_OBJECT(startwin, startbuttonlabel, "startbuttonlabel");

    gtk_window_add_accel_group(GTK_WINDOW(startwin), accel_group);

    return startwin;
}

// -- BUILD ENTRY POINTS ------------------------------------------------------

int startwin_open(void)
{
    if (!gtkenabled) return 0;
    if (startwin) return 1;

    startwin = create_window();
    if (startwin)
    {
        SetPage(TAB_MESSAGES);
        gtk_widget_show(startwin);
        gtk_main_iteration_do(FALSE);
        return 0;
    }
    return -1;
}

int startwin_close(void)
{
    if (!gtkenabled) return 0;
    if (!startwin) return 1;
    gtk_widget_destroy(startwin);
    startwin = NULL;
    return 0;
}

int startwin_puts(const char *str)
{
    GtkWidget *textview;
    GtkTextBuffer *textbuffer;
    GtkTextIter enditer;
    GtkTextMark *mark;
    const char *aptr, *bptr;

    if (!gtkenabled || !str) return 0;
    if (!startwin) return 1;
    if (!(textview = lookup_widget(startwin, "messagestext"))) return -1;
    textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));

    gtk_text_buffer_get_end_iter(textbuffer, &enditer);
    for (aptr = bptr = str; *aptr != 0; )
    {
        switch (*bptr)
        {
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
        case '\r':  // FIXME
        default:
            bptr++;
            break;
        }
    }

    mark = gtk_text_buffer_create_mark(textbuffer, NULL, &enditer, 1);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(textview), mark, 0.0, FALSE, 0.0, 1.0);
    gtk_text_buffer_delete_mark(textbuffer, mark);

    return 0;
}

int startwin_settitle(const char *title)
{
    if (!gtkenabled) return 0;
    if (!startwin) return 1;
    gtk_window_set_title(GTK_WINDOW(startwin), title);
    return 0;
}

int startwin_idle(void *s)
{
    if (!gtkenabled) return 0;
    //if (!startwin) return 1;
    gtk_main_iteration_do(FALSE);
    return 0;
}

extern int xdimgame, ydimgame, bppgame, forcesetup;

int startwin_run(void)
{
    if (!gtkenabled) return 0;
    if (!startwin) return 1;

    SetPage(TAB_CONFIG);

    settings.fullscreen = fullscreen;
    settings.xdim3d = xdimgame;
    settings.ydim3d = ydimgame;
    settings.bpp3d = bppgame;
    settings.forcesetup = forcesetup;
    PopulateForm();

    gtk_main();

    SetPage(TAB_MESSAGES);
    if (retval)
    {
        fullscreen = settings.fullscreen;
        xdimgame = settings.xdim3d;
        ydimgame = settings.ydim3d;
        bppgame = settings.bpp3d;
        forcesetup = settings.forcesetup;
    }

    return retval;
}
