#if defined(LINKED_GTK)
	#include <gtk/gtk.h>
	#include <gdk-pixbuf/gdk-pixdata.h>
#else
	#include "dynamicgtk.h"
#endif

#include "duke3d.h"
#include "grpscan.h"
#include "build.h"

#define TAB_CONFIG 0
#define TAB_GAME 1
#define TAB_MESSAGES 2
#define Polymost 3
#define Polymer 4

static struct
{
	GtkWidget *startwin;
	GtkWidget *hlayout;
	GtkWidget *banner;
	GtkWidget *vlayout;
    GtkWidget *tabs;
    GtkWidget *configvlayout;
    GtkWidget *configtlayout;
	GtkWidget *displayvlayout;
	GtkWidget *vmode3dlabel;
	GtkWidget *vmode3dcombo;
    GtkWidget *fullscreencheck;
	GtkWidget *polymercheck;
	GtkWidget *emptyhlayout;
    GtkWidget *inputdevlabel;
	GtkWidget *inputhlayout;
    GtkWidget *inputmousecheck;
    GtkWidget *inputjoycheck;
	GtkWidget *autoloadcheck;
    GtkWidget *alwaysshowcheck;
    GtkWidget *configtab;
    GtkWidget *gamevlayout;
    GtkWidget *gamelabel;
    GtkWidget *gamescroll;
    GtkWidget *gamelist;
    GtkWidget *gametab;
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
} stwidgets;
	
static struct
{
    int32_t fullscreen;
    int32_t polymer;
    int32_t xdim3d, ydim3d, bpp3d;
    int32_t forcesetup;
	int32_t autoload;
    int32_t usemouse, usejoy;
    int32_t game;
    int32_t crcval;
    char selectedgrp[BMAX_PATH];
} settings;

static int32_t retval = -1, mode = TAB_MESSAGES;
extern int32_t gtkenabled;
static void PopulateForm(int32_t pgs);


// -- EVENT CALLBACKS AND CREATION STUFF --------------------------------------

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
}

static void on_fullscreencheck_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    UNREFERENCED_PARAMETER(user_data);
    settings.fullscreen = gtk_toggle_button_get_active(togglebutton);
    PopulateForm(1<<TAB_CONFIG);
}

static void on_polymercheck_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    UNREFERENCED_PARAMETER(user_data);
    if (gtk_toggle_button_get_active(togglebutton))
    {
    	glrendmode = Polymer;
    	settings.polymer = TRUE;
    	if (settings.bpp3d == 8)
    	{
			settings.bpp3d = 32;
    		PopulateForm(1<<TAB_CONFIG);
    	}
    }
    else
    {
    	glrendmode = Polymost;
    	settings.polymer = FALSE;
    }
}

static void on_autoloadcheck_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    UNREFERENCED_PARAMETER(user_data);
    settings.autoload = gtk_toggle_button_get_active(togglebutton);
}

static void on_inputmousecheck_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    UNREFERENCED_PARAMETER(user_data);
    settings.usemouse = gtk_toggle_button_get_active(togglebutton);
}

static void on_inputjoycheck_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    UNREFERENCED_PARAMETER(user_data);
    settings.usejoy = gtk_toggle_button_get_active(togglebutton);
}

static void on_alwaysshowcheck_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
    UNREFERENCED_PARAMETER(user_data);
    settings.forcesetup = gtk_toggle_button_get_active(togglebutton);
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

static void on_gamelist_selection_changed(GtkTreeSelection *selection, gpointer user_data)
{
    GtkTreeIter iter;
    GtkTreeModel *model;
    struct grpfile *fg;
    UNREFERENCED_PARAMETER(user_data);
    if (gtk_tree_selection_get_selected(selection, &model, &iter))
    {
        gtk_tree_model_get(model, &iter, 2, (gpointer)&fg, -1);
        strcpy(settings.selectedgrp, fg->name);
        settings.game = fg->game;
        settings.crcval = fg->crcval;
    }
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
    extern const GdkPixdata startbanner_pixdata;
    return gdk_pixbuf_from_pixdata(&startbanner_pixdata, FALSE, NULL);
}

static void SetPage(int32_t n)
{
    if (!gtkenabled || !stwidgets.startwin) return;
    mode = n;
    gtk_notebook_set_current_page(GTK_NOTEBOOK(stwidgets.tabs), n);

    // each control in the config page vertical layout plus the start button should be made (in)sensitive
    if (n == TAB_CONFIG) n = TRUE; else n = FALSE;
    gtk_widget_set_sensitive(stwidgets.startbutton, n);
    gtk_container_foreach(GTK_CONTAINER(stwidgets.configvlayout),
                         (GtkCallback)gtk_widget_set_sensitive,
                         (gpointer)&n);
}

static void PopulateForm(int32_t pgs)
{
    if (pgs & (1<<TAB_CONFIG))
    {
        int32_t mode3d, i;
        GtkListStore *modes3d;
        GtkTreeIter iter;
        GtkComboBox *box3d;
        char buf[64];

        mode3d = checkvideomode(&settings.xdim3d, &settings.ydim3d, settings.bpp3d, settings.fullscreen, 1);
        if (mode3d < 0)
        {
            int32_t i, cd[] = { 32, 24, 16, 15, 8, 0 };
            for (i=0; cd[i];) { if (cd[i] >= settings.bpp3d) i++; else break; }
            for (; cd[i]; i++)
            {
                mode3d = checkvideomode(&settings.xdim3d, &settings.ydim3d, cd[i], settings.fullscreen, 1);
                if (mode3d < 0) continue;
                settings.bpp3d = cd[i];
                break;
            }
        }

        box3d = GTK_COMBO_BOX(stwidgets.vmode3dcombo);
        modes3d = GTK_LIST_STORE(gtk_combo_box_get_model(box3d));
        gtk_list_store_clear(modes3d);

        for (i=0; i<validmodecnt; i++)
        {
            if (validmode[i].fs != settings.fullscreen) continue;

            // all modes get added to the 3D mode list
            Bsprintf(buf, "%d x %d %dbpp", validmode[i].xdim, validmode[i].ydim, validmode[i].bpp);
            gtk_list_store_append(modes3d, &iter);
            gtk_list_store_set(modes3d, &iter, 0,buf, 1,i, -1);
            if (i == mode3d)
            {
                g_signal_handlers_block_by_func(box3d, on_vmode3dcombo_changed, NULL);
                gtk_combo_box_set_active_iter(box3d, &iter);
                g_signal_handlers_unblock_by_func(box3d, on_vmode3dcombo_changed, NULL);
            }
        }
        
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(stwidgets.fullscreencheck), settings.fullscreen);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(stwidgets.polymercheck), settings.polymer);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(stwidgets.autoloadcheck), settings.autoload);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(stwidgets.alwaysshowcheck), settings.forcesetup);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(stwidgets.inputmousecheck), settings.usemouse);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(stwidgets.inputjoycheck), settings.usejoy);
    }

    if (pgs & (1<<TAB_GAME))
    {
        struct grpfile *fg;
        int32_t i;
        GtkListStore *list;
        GtkTreeIter iter;
        GtkTreeView *gamelist;

        gamelist = GTK_TREE_VIEW(stwidgets.gamelist);
        list = GTK_LIST_STORE(gtk_tree_view_get_model(gamelist));
        gtk_list_store_clear(list);

        for (fg = foundgrps; fg; fg=fg->next)
        {
            for (i = 0; i<numgrpfiles; i++)
                if (fg->crcval == grpfiles[i].crcval) break;
            if (i == numgrpfiles) continue;	// unrecognised grp file

            gtk_list_store_append(list, &iter);
            gtk_list_store_set(list, &iter, 0, grpfiles[i].name, 1, fg->name, 2, (gpointer)fg, -1);
            if (!Bstrcasecmp(fg->name, settings.selectedgrp))
            {
                GtkTreeSelection *sel = gtk_tree_view_get_selection(gamelist);
                g_signal_handlers_block_by_func(sel, on_gamelist_selection_changed, NULL);
                gtk_tree_selection_select_iter(sel, &iter);
                g_signal_handlers_unblock_by_func(sel, on_gamelist_selection_changed, NULL);
            }
        }
    }
}

static gint name_sorter(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data)
{
    gchar *as, *bs;
    gint r;
    UNREFERENCED_PARAMETER(user_data);
    gtk_tree_model_get(model, a, 0, &as, -1);
    gtk_tree_model_get(model, b, 0, &bs, -1);

    r = g_utf8_collate(as,bs);

    g_free(as);
    g_free(bs);

    return r;
}

static GtkWidget *create_window(void)
{
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
	
	// Vertical layout of config page
	stwidgets.configvlayout = gtk_vbox_new(FALSE, 12);
	gtk_container_add (GTK_CONTAINER (stwidgets.tabs), stwidgets.configvlayout);

    // layout table of config page
	stwidgets.configtlayout = gtk_table_new(3, 3, FALSE);
	gtk_box_pack_start(GTK_BOX(stwidgets.configvlayout), stwidgets.configtlayout, TRUE, TRUE, 0);
	
	// 3D video mode LabelText
	stwidgets.vmode3dlabel = gtk_label_new_with_mnemonic("_Video mode:");
	gtk_misc_set_alignment (GTK_MISC(stwidgets.vmode3dlabel), 0.3, 0);
	gtk_table_attach (GTK_TABLE(stwidgets.configtlayout), stwidgets.vmode3dlabel, 0,1, 0,1, GTK_FILL, 0, 4, 6);
	
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
    gtk_table_attach (GTK_TABLE(stwidgets.configtlayout), stwidgets.vmode3dcombo, 1,2, 0,1, GTK_EXPAND | GTK_FILL, 0, 4, 6);

    // Fullscreen checkbox
	stwidgets.displayvlayout = gtk_vbox_new(TRUE, 0);
	gtk_table_attach (GTK_TABLE(stwidgets.configtlayout), stwidgets.displayvlayout, 2,3, 0,1, GTK_FILL, 0, 4, 6);
	stwidgets.fullscreencheck = gtk_check_button_new_with_mnemonic("_Fullscreen");
	gtk_box_pack_start(GTK_BOX(stwidgets.displayvlayout), stwidgets.fullscreencheck, FALSE, FALSE, 0);
	
	// Polymer checkbox
    stwidgets.polymercheck = gtk_check_button_new_with_mnemonic("_Polymer");
	gtk_box_pack_start(GTK_BOX(stwidgets.displayvlayout), stwidgets.polymercheck, FALSE, FALSE, 0);
	
	// Empty horizontal layout
	stwidgets.emptyhlayout = gtk_hbox_new(TRUE, 0);
	gtk_table_attach (GTK_TABLE(stwidgets.configtlayout), stwidgets.emptyhlayout, 0,1, 2,3, 0, GTK_EXPAND | GTK_FILL, 0, 0);

	// Input devices LabelText
    stwidgets.inputdevlabel = gtk_label_new("Input devices:");
	gtk_misc_set_alignment(GTK_MISC(stwidgets.inputdevlabel), 0.3, 0);
	gtk_table_attach (GTK_TABLE(stwidgets.configtlayout), stwidgets.inputdevlabel, 0,1, 3,4, GTK_FILL, 0, 4, 0);
	
	// Input devices checkbox
	stwidgets.inputhlayout = gtk_hbox_new(FALSE, 4);
	gtk_table_attach (GTK_TABLE(stwidgets.configtlayout), stwidgets.inputhlayout, 1,2, 3,4, GTK_EXPAND | GTK_FILL, 0, 4, 0);
	
    stwidgets.inputmousecheck = gtk_check_button_new_with_mnemonic("Mo_use");
    stwidgets.inputjoycheck = gtk_check_button_new_with_mnemonic("_Joystick");
	
	gtk_box_pack_start (GTK_BOX(stwidgets.inputhlayout), stwidgets.inputmousecheck, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(stwidgets.inputhlayout), stwidgets.inputjoycheck, FALSE, FALSE, 0);

    // Autoload checkbox
	stwidgets.autoloadcheck = gtk_check_button_new_with_mnemonic("_Enable \"autoload\" folder");
	gtk_box_pack_start(GTK_BOX(stwidgets.configvlayout), stwidgets.autoloadcheck, FALSE, FALSE, 0);
	
	// Always show config checkbox
    stwidgets.alwaysshowcheck = gtk_check_button_new_with_mnemonic("_Always show configuration on start");
	gtk_box_pack_start(GTK_BOX(stwidgets.configvlayout), stwidgets.alwaysshowcheck, FALSE, FALSE, 0);

    // Configuration tab
    stwidgets.configtab = gtk_label_new("Configuration");
    gtk_notebook_set_tab_label(GTK_NOTEBOOK(stwidgets.tabs), gtk_notebook_get_nth_page(GTK_NOTEBOOK(stwidgets.tabs), 0), stwidgets.configtab);

    // Game data layout
    stwidgets.gamevlayout = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(stwidgets.tabs), stwidgets.gamevlayout);
    gtk_container_set_border_width(GTK_CONTAINER(stwidgets.gamevlayout), 4);

    // Game data field LabelText
    stwidgets.gamelabel = gtk_label_new_with_mnemonic("_Game or addon:");
    gtk_box_pack_start(GTK_BOX(stwidgets.gamevlayout), stwidgets.gamelabel, FALSE, FALSE, 0);
    gtk_misc_set_alignment(GTK_MISC(stwidgets.gamelabel), 0, 0.5);

    // Game data scrollable area
    stwidgets.gamescroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(stwidgets.gamevlayout), stwidgets.gamescroll, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(stwidgets.gamescroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(stwidgets.gamescroll), GTK_SHADOW_IN);

    // Game data list
    {
        GtkListStore *list = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
        GtkCellRenderer *cell;
        GtkTreeViewColumn *col;

        gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list), 0, name_sorter, NULL, NULL);
        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(list), 0, GTK_SORT_ASCENDING);

        stwidgets.gamelist = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list));
        g_object_unref(G_OBJECT(list));

        cell = gtk_cell_renderer_text_new();
        col = gtk_tree_view_column_new_with_attributes("Game", cell, "text", 0, NULL);
        gtk_tree_view_column_set_expand(col, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(stwidgets.gamelist), col);
        col = gtk_tree_view_column_new_with_attributes("GRP file", cell, "text", 1, NULL);
        gtk_tree_view_column_set_min_width(col, 64);
        gtk_tree_view_append_column(GTK_TREE_VIEW(stwidgets.gamelist), col);
    }
    gtk_container_add(GTK_CONTAINER(stwidgets.gamescroll), stwidgets.gamelist);
	
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(stwidgets.gamelist), FALSE);
    gtk_tree_view_set_enable_search(GTK_TREE_VIEW(stwidgets.gamelist), FALSE);

    // Game tab
    stwidgets.gametab = gtk_label_new("Game");
    gtk_notebook_set_tab_label(GTK_NOTEBOOK(stwidgets.tabs), gtk_notebook_get_nth_page(GTK_NOTEBOOK(stwidgets.tabs), 1), stwidgets.gametab);

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
    gtk_notebook_set_tab_label(GTK_NOTEBOOK(stwidgets.tabs), gtk_notebook_get_nth_page(GTK_NOTEBOOK(stwidgets.tabs), 2), stwidgets.messagestab);

    // Dialogue box buttons layout
    stwidgets.buttons = gtk_hbutton_box_new();
    gtk_box_pack_start(GTK_BOX(stwidgets.vlayout), stwidgets.buttons, FALSE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(stwidgets.buttons), 3);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(stwidgets.buttons), GTK_BUTTONBOX_END);

    // Cancel button
    stwidgets.cancelbutton = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(stwidgets.buttons), stwidgets.cancelbutton);
    GTK_WIDGET_SET_FLAGS(stwidgets.cancelbutton, GTK_CAN_DEFAULT);

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
    g_signal_connect((gpointer) stwidgets.fullscreencheck, "toggled",
                     G_CALLBACK(on_fullscreencheck_toggled),
                     NULL);
    g_signal_connect((gpointer) stwidgets.polymercheck, "toggled",
                     G_CALLBACK(on_polymercheck_toggled),
                     NULL);
    g_signal_connect((gpointer) stwidgets.autoloadcheck, "toggled",
                     G_CALLBACK(on_autoloadcheck_toggled),
                     NULL);
    g_signal_connect((gpointer) stwidgets.inputmousecheck, "toggled",
                     G_CALLBACK(on_inputmousecheck_toggled),
                     NULL);
    g_signal_connect((gpointer) stwidgets.inputjoycheck, "toggled",
                     G_CALLBACK(on_inputjoycheck_toggled),
                     NULL);
    g_signal_connect((gpointer) stwidgets.vmode3dcombo, "changed",
                     G_CALLBACK(on_vmode3dcombo_changed),
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
    {
        GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(stwidgets.gamelist));
        gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);
        g_signal_connect((gpointer) sel, "changed",
                         G_CALLBACK(on_gamelist_selection_changed),
                         NULL);
    }

    // Associate labels with their controls
    gtk_label_set_mnemonic_widget(GTK_LABEL(stwidgets.vmode3dlabel), stwidgets.vmode3dcombo);
    gtk_label_set_mnemonic_widget(GTK_LABEL(stwidgets.gamelabel), stwidgets.gamelist);

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

extern char *duke3dgrp, *duke3dgrpstring;

int32_t startwin_run(void)
{
    if (!gtkenabled) return 1;
    if (!stwidgets.startwin) return 1;

    SetPage(TAB_CONFIG);

    settings.fullscreen = ud.config.ScreenMode;
    settings.xdim3d = ud.config.ScreenWidth;
    settings.ydim3d = ud.config.ScreenHeight;
    settings.bpp3d = ud.config.ScreenBPP;
    settings.forcesetup = ud.config.ForceSetup;
    settings.usemouse = ud.config.UseMouse;
    settings.usejoy = ud.config.UseJoystick;
    settings.game = g_gameType;
    Bstrncpy(settings.selectedgrp, duke3dgrp, BMAX_PATH);
    if (ud.config.NoAutoLoad) settings.autoload = FALSE;
    else settings.autoload = TRUE;
    if (glrendmode == Polymer)
    {
    	if (settings.bpp3d == 8) settings.bpp3d = 32;
    	settings.polymer = TRUE;
    }
    PopulateForm(-1);

    gtk_main();

    SetPage(TAB_MESSAGES);
    if (retval) // launch the game with these parameters
    {
        int32_t i;

        ud.config.ScreenMode = settings.fullscreen;
        ud.config.ScreenWidth = settings.xdim3d;
        ud.config.ScreenHeight = settings.ydim3d;
        ud.config.ScreenBPP = settings.bpp3d;
        ud.config.ForceSetup = settings.forcesetup;
        ud.config.UseMouse = settings.usemouse;
        ud.config.UseJoystick = settings.usejoy;
        duke3dgrp = settings.selectedgrp;
        g_gameType = settings.game;
        if (settings.autoload) ud.config.NoAutoLoad = FALSE;
        else ud.config.NoAutoLoad = TRUE;

        for (i = 0; i<numgrpfiles; i++) if (settings.crcval == grpfiles[i].crcval) break;
        if (i != numgrpfiles)
            duke3dgrpstring = (char *)grpfiles[i].name;
    }

    return retval;
}
