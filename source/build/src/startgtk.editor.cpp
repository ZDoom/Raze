
#include "gtkpixdata.h"

#include "dynamicgtk.h"

#include "build.h"
#include "editor.h"
#include "baselayer.h"

enum
{
    TAB_CONFIG,
    TAB_MESSAGES,
};

static struct
{
    GtkWidget *startwin;
    GtkWidget *hlayout;
    GtkWidget *banner;
    GtkWidget *vlayout;
    GtkWidget *tabs;
    GtkWidget *configtlayout;
    GtkWidget *vmode2dlabel;
    GtkWidget *vmode3dlabel;
    GtkWidget *vmode2dcombo;
    GtkWidget *vmode3dcombo;
    GtkWidget *fullscreencheck;
    GtkWidget *emptyhlayout;
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
} stwidgets;

static struct
{
    int32_t fullscreen;
    int32_t xdim2d, ydim2d;
    int32_t xdim3d, ydim3d, bpp3d;
    int32_t forcesetup;
} settings;

static int32_t retval = -1, mode = TAB_MESSAGES;
extern int32_t gtkenabled;
static void PopulateForm(void);

// -- EVENT CALLBACKS AND CREATION STUFF --------------------------------------

static void on_vmode2dcombo_changed(GtkComboBox *combobox, gpointer user_data)
{
    GtkTreeModel *data;
    GtkTreeIter iter;
    int32_t val;
    UNREFERENCED_PARAMETER(user_data);
    if (!gtk_combo_box_get_active_iter(combobox, &iter)) return;
    if (!(data = gtk_combo_box_get_model(combobox))) return;
    gtk_tree_model_get(data, &iter, 1, &val, -1);
    settings.xdim2d = validmode[val].xdim;
    settings.ydim2d = validmode[val].ydim;
}

static void on_vmode3dcombo_changed(GtkComboBox *combobox, gpointer user_data)
{
    GtkTreeModel *data;
    GtkTreeIter iter;
    int32_t val;
    UNREFERENCED_PARAMETER(user_data);
    if (!gtk_combo_box_get_active_iter(combobox, &iter)) return;
    if (!(data = gtk_combo_box_get_model(combobox))) return;
    gtk_tree_model_get(data, &iter, 1, &val, -1);
    settings.xdim3d = validmode[val].xdim;
    settings.ydim3d = validmode[val].ydim;
    settings.bpp3d = validmode[val].bpp;
}

static void on_fullscreencheck_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    UNREFERENCED_PARAMETER(user_data);
    settings.fullscreen = (gtk_toggle_button_get_active(togglebutton) == TRUE);
    PopulateForm();
}

static void on_alwaysshowcheck_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    UNREFERENCED_PARAMETER(user_data);
    settings.forcesetup = (gtk_toggle_button_get_active(togglebutton) == TRUE);
}

static void on_cancelbutton_clicked(GtkButton *button, gpointer user_data)
{
    UNREFERENCED_PARAMETER(button);
    UNREFERENCED_PARAMETER(user_data);
    if (mode == TAB_CONFIG) { retval = 0; gtk_main_quit(); }
    else quitevent++;
}

static void on_startbutton_clicked(GtkButton *button, gpointer user_data)
{
    UNREFERENCED_PARAMETER(button);
    UNREFERENCED_PARAMETER(user_data);
    retval = 1;
    gtk_main_quit();
}

static gboolean on_startwin_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    UNREFERENCED_PARAMETER(widget);
    UNREFERENCED_PARAMETER(event);
    UNREFERENCED_PARAMETER(user_data);
    if (mode == TAB_CONFIG) { retval = 0; gtk_main_quit(); }
    else quitevent++;
    return TRUE;	// FALSE would let the event go through. we want the game to decide when to close
}

// -- SUPPORT FUNCTIONS -------------------------------------------------------

static GdkPixbuf *load_banner(void)
{
    return gdk_pixbuf_from_pixdata((GdkPixdata const *)&startbanner_pixdata, FALSE, NULL);
}

static void SetPage(int32_t n)
{
    if (!gtkenabled || !stwidgets.startwin) return;
    mode = n;
    gtk_notebook_set_current_page(GTK_NOTEBOOK(stwidgets.tabs), n);

    // each control in the config page vertical layout plus the start button should be made (in)sensitive
    if (n == TAB_CONFIG) n = TRUE; else n = FALSE;
    gtk_widget_set_sensitive(stwidgets.startbutton, n);
    gtk_container_foreach(GTK_CONTAINER(stwidgets.configtlayout),
                          (GtkCallback)gtk_widget_set_sensitive,
                          (gpointer)&n);
}

static void PopulateForm(void)
{
    int32_t mode2d, mode3d, i;
    GtkListStore *modes2d, *modes3d;
    GtkTreeIter iter;
    GtkComboBox *box2d, *box3d;
    char buf[64];

    mode2d = videoCheckMode(&settings.xdim2d, &settings.ydim2d, 8, settings.fullscreen, 1);
    mode3d = videoCheckMode(&settings.xdim3d, &settings.ydim3d, settings.bpp3d, settings.fullscreen, 1);
    if (mode2d < 0) mode2d = 0;
    if (mode3d < 0)
    {
        int32_t i, cd[] = { 32, 24, 16, 15, 8, 0 };
        for (i=0; cd[i];) { if (cd[i] >= settings.bpp3d) i++; else break; }
        for (; cd[i]; i++)
        {
            mode3d = videoCheckMode(&settings.xdim3d, &settings.ydim3d, cd[i], settings.fullscreen, 1);
            if (mode3d < 0) continue;
            settings.bpp3d = cd[i];
            break;
        }
    }

    box2d = GTK_COMBO_BOX(stwidgets.vmode2dcombo);
    box3d = GTK_COMBO_BOX(stwidgets.vmode3dcombo);
    modes2d = GTK_LIST_STORE(gtk_combo_box_get_model(box2d));
    modes3d = GTK_LIST_STORE(gtk_combo_box_get_model(box3d));
    gtk_list_store_clear(modes2d);
    gtk_list_store_clear(modes3d);

    for (i=0; i<validmodecnt; i++)
    {
        if (validmode[i].fs != settings.fullscreen) continue;

        // all modes get added to the 3D mode list
        Bsprintf(buf, "%dx%d %s", validmode[i].xdim, validmode[i].ydim, validmode[i].bpp == 8 ? "software" : "OpenGL");
        gtk_list_store_append(modes3d, &iter);
        gtk_list_store_set(modes3d, &iter, 0,buf, 1,i, -1);
        if (i == mode3d)
        {
            g_signal_handlers_block_by_func(box3d, (gpointer)on_vmode3dcombo_changed, NULL);
            gtk_combo_box_set_active_iter(box3d, &iter);
            g_signal_handlers_unblock_by_func(box3d, (gpointer)on_vmode3dcombo_changed, NULL);
        }

        // only 8-bit modes get used for 2D
        if (validmode[i].bpp != 8 || validmode[i].xdim < 640 || validmode[i].ydim < 480) continue;
        Bsprintf(buf, "%d x %d", validmode[i].xdim, validmode[i].ydim);
        gtk_list_store_append(modes2d, &iter);
        gtk_list_store_set(modes2d, &iter, 0,buf, 1,i, -1);
        if (i == mode2d)
        {
            g_signal_handlers_block_by_func(box2d, (gpointer)on_vmode2dcombo_changed, NULL);
            gtk_combo_box_set_active_iter(box2d, &iter);
            g_signal_handlers_unblock_by_func(box2d, (gpointer)on_vmode2dcombo_changed, NULL);
        }
    }

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(stwidgets.fullscreencheck), settings.fullscreen);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(stwidgets.alwaysshowcheck), settings.forcesetup);
}

static GtkWidget *create_window(void)
{
    stwidgets.accel_group = gtk_accel_group_new();

    // Basic window
    stwidgets.startwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(stwidgets.startwin), apptitle);	// NOTE: use global app title
    gtk_window_set_position(GTK_WINDOW(stwidgets.startwin), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(stwidgets.startwin), FALSE);
    gtk_window_set_type_hint(GTK_WINDOW(stwidgets.startwin), GDK_WINDOW_TYPE_HINT_DIALOG);

    // Horizontal layout of banner and controls
    stwidgets.hlayout = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(stwidgets.startwin), stwidgets.hlayout);

    // banner
    {
        GdkPixbuf *pixbuf = load_banner();
        stwidgets.banner = gtk_image_new_from_pixbuf(pixbuf);
        g_object_unref((gpointer)pixbuf);
    }
    gtk_box_pack_start(GTK_BOX(stwidgets.hlayout), stwidgets.banner, FALSE, FALSE, 0);
    gtk_misc_set_alignment(GTK_MISC(stwidgets.banner), 0.5, 0);

    // Vertical layout of tab control and start+cancel buttons
    stwidgets.vlayout = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(stwidgets.hlayout), stwidgets.vlayout, TRUE, TRUE, 0);

    // Tab control
    stwidgets.tabs = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(stwidgets.vlayout), stwidgets.tabs, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(stwidgets.tabs), 4);

    // layout table of config page
    stwidgets.configtlayout = gtk_table_new(4, 3, FALSE);
    gtk_container_add(GTK_CONTAINER(stwidgets.tabs), stwidgets.configtlayout);

    // 2D video mode label
    stwidgets.vmode2dlabel = gtk_label_new_with_mnemonic("_2D Video mode:");
    gtk_misc_set_alignment(GTK_MISC(stwidgets.vmode2dlabel), 0.3, 0);
    gtk_table_attach(GTK_TABLE(stwidgets.configtlayout), stwidgets.vmode2dlabel, 0,1, 0,1, GTK_FILL, (GtkAttachOptions)0, 4, 6);

    // 2D video mode combo
    {
        GtkListStore *list = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
        GtkCellRenderer *cell;

        stwidgets.vmode2dcombo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(list));
        g_object_unref(G_OBJECT(list));

        cell = gtk_cell_renderer_text_new();
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(stwidgets.vmode2dcombo), cell, FALSE);
        gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(stwidgets.vmode2dcombo), cell, "text", 0, NULL);
    }
    gtk_table_attach(GTK_TABLE(stwidgets.configtlayout), stwidgets.vmode2dcombo, 1,2, 0,1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), (GtkAttachOptions)0, 4, 6);
    gtk_widget_add_accelerator(stwidgets.vmode2dcombo, "grab_focus", stwidgets.accel_group,
                               GDK_2, GDK_MOD1_MASK,
                               GTK_ACCEL_VISIBLE);

    // Fullscreen checkbox
    stwidgets.fullscreencheck = gtk_check_button_new_with_mnemonic("_Fullscreen");
    gtk_table_attach(GTK_TABLE(stwidgets.configtlayout), stwidgets.fullscreencheck, 2,3, 0,1, GTK_FILL, (GtkAttachOptions)0, 4, 6);
    gtk_widget_add_accelerator(stwidgets.fullscreencheck, "grab_focus", stwidgets.accel_group,
                               GDK_F, GDK_MOD1_MASK,
                               GTK_ACCEL_VISIBLE);

    // 3D video mode label
    stwidgets.vmode3dlabel = gtk_label_new_with_mnemonic("_3D Video mode:");
    gtk_misc_set_alignment(GTK_MISC(stwidgets.vmode3dlabel), 0.3, 0);
    gtk_table_attach(GTK_TABLE(stwidgets.configtlayout), stwidgets.vmode3dlabel, 0,1, 1,2, GTK_FILL, (GtkAttachOptions)0, 4, 6);

    // 3D video mode combo
    {
        GtkListStore *list = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);
        GtkCellRenderer *cell;

        stwidgets.vmode3dcombo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(list));
        g_object_unref(G_OBJECT(list));

        cell = gtk_cell_renderer_text_new();
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(stwidgets.vmode3dcombo), cell, FALSE);
        gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(stwidgets.vmode3dcombo), cell, "text", 0, NULL);
    }
    gtk_table_attach(GTK_TABLE(stwidgets.configtlayout), stwidgets.vmode3dcombo, 1,2, 1,2,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), (GtkAttachOptions)0, 4, 0);
    gtk_widget_add_accelerator(stwidgets.vmode3dcombo, "grab_focus", stwidgets.accel_group,
                               GDK_3, GDK_MOD1_MASK,
                               GTK_ACCEL_VISIBLE);
    // Empty horizontal layout
    stwidgets.emptyhlayout = gtk_hbox_new(TRUE, 0);
    gtk_table_attach(GTK_TABLE(stwidgets.configtlayout), stwidgets.emptyhlayout, 0,1, 2,3, (GtkAttachOptions)0,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 0, 0);

    // Always show config checkbox
    stwidgets.alwaysshowcheck = gtk_check_button_new_with_mnemonic("_Always show configuration on start");
    gtk_table_attach(GTK_TABLE(stwidgets.configtlayout), stwidgets.alwaysshowcheck, 0,2, 3,4, GTK_FILL, (GtkAttachOptions)0, 4, 6);
    gtk_widget_add_accelerator(stwidgets.alwaysshowcheck, "grab_focus", stwidgets.accel_group,
                               GDK_A, GDK_MOD1_MASK,
                               GTK_ACCEL_VISIBLE);

    // Configuration tab
    stwidgets.configtab = gtk_label_new("Configuration");
    gtk_notebook_set_tab_label(GTK_NOTEBOOK(stwidgets.tabs), gtk_notebook_get_nth_page(GTK_NOTEBOOK(stwidgets.tabs), 0), stwidgets.configtab);

    // Messages scrollable area
    stwidgets.messagesscroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(stwidgets.tabs), stwidgets.messagesscroll);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(stwidgets.messagesscroll), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);

    // Messages text area
    stwidgets.messagestext = gtk_text_view_new();
    gtk_container_add(GTK_CONTAINER(stwidgets.messagesscroll), stwidgets.messagestext);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(stwidgets.messagestext), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(stwidgets.messagestext), GTK_WRAP_WORD);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(stwidgets.messagestext), FALSE);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(stwidgets.messagestext), 2);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(stwidgets.messagestext), 2);

    // Messages tab
    stwidgets.messagestab = gtk_label_new("Messages");
    gtk_notebook_set_tab_label(GTK_NOTEBOOK(stwidgets.tabs), gtk_notebook_get_nth_page(GTK_NOTEBOOK(stwidgets.tabs), 1), stwidgets.messagestab);

    // Dialogue box buttons layout
    stwidgets.buttons = gtk_hbutton_box_new();
    gtk_box_pack_start(GTK_BOX(stwidgets.vlayout), stwidgets.buttons, FALSE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(stwidgets.buttons), 3);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(stwidgets.buttons), GTK_BUTTONBOX_END);

    // Cancel button
    stwidgets.cancelbutton = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(stwidgets.buttons), stwidgets.cancelbutton);
    GTK_WIDGET_SET_FLAGS(stwidgets.cancelbutton, GTK_CAN_DEFAULT);
    gtk_widget_add_accelerator(stwidgets.cancelbutton, "grab_focus", stwidgets.accel_group,
                               GDK_C, GDK_MOD1_MASK,
                               GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(stwidgets.cancelbutton, "clicked", stwidgets.accel_group,
                               GDK_Escape, (GdkModifierType)0,
                               GTK_ACCEL_VISIBLE);

    stwidgets.cancelbuttonalign = gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_container_add(GTK_CONTAINER(stwidgets.cancelbutton), stwidgets.cancelbuttonalign);

    stwidgets.cancelbuttonlayout = gtk_hbox_new(FALSE, 2);
    gtk_container_add(GTK_CONTAINER(stwidgets.cancelbuttonalign), stwidgets.cancelbuttonlayout);

    stwidgets.cancelbuttonicon = gtk_image_new_from_stock("gtk-cancel", GTK_ICON_SIZE_BUTTON);
    gtk_box_pack_start(GTK_BOX(stwidgets.cancelbuttonlayout), stwidgets.cancelbuttonicon, FALSE, FALSE, 0);

    stwidgets.cancelbuttonlabel = gtk_label_new_with_mnemonic("_Cancel");
    gtk_box_pack_start(GTK_BOX(stwidgets.cancelbuttonlayout), stwidgets.cancelbuttonlabel, FALSE, FALSE, 0);

    // Start button
    stwidgets.startbutton = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(stwidgets.buttons), stwidgets.startbutton);
    GTK_WIDGET_SET_FLAGS(stwidgets.startbutton, GTK_CAN_DEFAULT);
    gtk_widget_add_accelerator(stwidgets.startbutton, "grab_focus", stwidgets.accel_group,
                               GDK_S, GDK_MOD1_MASK,
                               GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(stwidgets.startbutton, "clicked", stwidgets.accel_group,
                               GDK_Return, (GdkModifierType)0,
                               GTK_ACCEL_VISIBLE);

    stwidgets.startbuttonalign = gtk_alignment_new(0.5, 0.5, 0, 0);
    gtk_container_add(GTK_CONTAINER(stwidgets.startbutton), stwidgets.startbuttonalign);

    stwidgets.startbuttonlayout = gtk_hbox_new(FALSE, 2);
    gtk_container_add(GTK_CONTAINER(stwidgets.startbuttonalign), stwidgets.startbuttonlayout);

    stwidgets.startbuttonicon = gtk_image_new_from_stock("gtk-execute", GTK_ICON_SIZE_BUTTON);
    gtk_box_pack_start(GTK_BOX(stwidgets.startbuttonlayout), stwidgets.startbuttonicon, FALSE, FALSE, 0);

    stwidgets.startbuttonlabel = gtk_label_new_with_mnemonic("_Start");
    gtk_box_pack_start(GTK_BOX(stwidgets.startbuttonlayout), stwidgets.startbuttonlabel, FALSE, FALSE, 0);

    // Wire up the signals
    g_signal_connect((gpointer) stwidgets.startwin, "delete_event",
                     G_CALLBACK(on_startwin_delete_event),
                     NULL);
    g_signal_connect((gpointer) stwidgets.vmode2dcombo, "changed",
                     G_CALLBACK(on_vmode2dcombo_changed),
                     NULL);
    g_signal_connect((gpointer) stwidgets.vmode3dcombo, "changed",
                     G_CALLBACK(on_vmode3dcombo_changed),
                     NULL);
    g_signal_connect((gpointer) stwidgets.fullscreencheck, "toggled",
                     G_CALLBACK(on_fullscreencheck_toggled),
                     NULL);
    g_signal_connect((gpointer) stwidgets.alwaysshowcheck, "toggled",
                     G_CALLBACK(on_alwaysshowcheck_toggled),
                     NULL);
    g_signal_connect((gpointer) stwidgets.cancelbutton, "clicked",
                     G_CALLBACK(on_cancelbutton_clicked),
                     NULL);
    g_signal_connect((gpointer) stwidgets.startbutton, "clicked",
                     G_CALLBACK(on_startbutton_clicked),
                     NULL);

    // Associate labels with their controls
    gtk_label_set_mnemonic_widget(GTK_LABEL(stwidgets.vmode2dlabel), stwidgets.vmode2dcombo);
    gtk_label_set_mnemonic_widget(GTK_LABEL(stwidgets.vmode3dlabel), stwidgets.vmode3dcombo);

    gtk_window_add_accel_group(GTK_WINDOW(stwidgets.startwin), stwidgets.accel_group);

    return stwidgets.startwin;
}

// -- BUILD ENTRY POINTS ------------------------------------------------------

int32_t startwin_open(void)
{
    if (!gtkenabled) return 0;
    if (stwidgets.startwin) return 1;

    stwidgets.startwin = create_window();
    if (stwidgets.startwin)
    {
        SetPage(TAB_MESSAGES);
        gtk_widget_show_all(stwidgets.startwin);
        gtk_main_iteration_do(FALSE);
        return 0;
    }
    return -1;
}

int32_t startwin_close(void)
{
    if (!gtkenabled) return 0;
    if (!stwidgets.startwin) return 1;
    gtk_widget_destroy(stwidgets.startwin);
    stwidgets.startwin = NULL;
    return 0;
}

int32_t startwin_puts(const char *str)
{
    GtkWidget *textview;
    GtkTextBuffer *textbuffer;
    GtkTextIter enditer;
    GtkTextMark *mark;
    const char *aptr, *bptr;

    if (!gtkenabled || !str) return 0;
    if (!stwidgets.startwin) return 1;
    if (!(textview = stwidgets.messagestext)) return -1;
    textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));

    gtk_text_buffer_get_end_iter(textbuffer, &enditer);
    for (aptr = bptr = str; *aptr != 0;)
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
        case '\r':	// FIXME
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

int32_t startwin_settitle(const char *title)
{
    if (!gtkenabled) return 0;
    if (!stwidgets.startwin) return 1;
    gtk_window_set_title(GTK_WINDOW(stwidgets.startwin), title);
    return 0;
}

int32_t startwin_idle(void *s)
{
    UNREFERENCED_PARAMETER(s);
    if (!gtkenabled) return 0;
    //if (!stwidgets.startwin) return 1;
    gtk_main_iteration_do(FALSE);
    return 0;
}

int32_t startwin_run(void)
{
    if (!gtkenabled) return 1;
    if (!stwidgets.startwin) return 1;

    SetPage(TAB_CONFIG);

    settings.fullscreen = fullscreen;
    settings.xdim2d = xdim2d;
    settings.ydim2d = ydim2d;
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
        xdim2d = settings.xdim2d;
        ydim2d = settings.ydim2d;
        xdimgame = settings.xdim3d;
        ydimgame = settings.ydim3d;
        bppgame = settings.bpp3d;
        forcesetup = settings.forcesetup;
    }

    return retval;
}
