#include "compat.h"
#include <dlfcn.h>

#define dynamicgtkfoo__
#include "dynamicgtk.h"

static void *handle = NULL;
struct _dynamicgtksyms dynamicgtksyms;

#define GETDLSYM(x) { \
	dynamicgtksyms.x = (x ## _ptr) dlsym(handle, (failsym = (const char *)#x)); \
	if (!dynamicgtksyms.x ) { err = 1; break; } \
}

int32_t dynamicgtk_init(void)
{
    int32_t err = 0;
    const char *failsym = NULL;
    const char *gtk_shared_object;

    if (handle) return 1;

#if defined __OpenBSD__
    gtk_shared_object = "libgtk-x11-2.0.so";
#else
    gtk_shared_object = "libgtk-x11-2.0.so.0";
#endif

    handle = dlopen(gtk_shared_object, RTLD_NOW|RTLD_GLOBAL);
    if (!handle) return -1;

    memset(&dynamicgtksyms, 0, sizeof(dynamicgtksyms));

    do
    {
        GETDLSYM(g_free)
        GETDLSYM(g_object_get_data)
        GETDLSYM(g_object_set_data)
        GETDLSYM(g_object_set_data_full)
        GETDLSYM(g_object_unref)
        GETDLSYM(g_signal_connect_data)
        GETDLSYM(g_signal_handlers_block_matched)
        GETDLSYM(g_signal_handlers_unblock_matched)
        GETDLSYM(g_type_check_instance_cast)
        GETDLSYM(gdk_pixbuf_from_pixdata)
        GETDLSYM(gdk_pixbuf_new_from_data)
        GETDLSYM(gtk_accel_group_new)
        GETDLSYM(gtk_alignment_new)
        GETDLSYM(gtk_box_get_type)
        GETDLSYM(gtk_box_pack_start)
        GETDLSYM(gtk_button_box_get_type)
        GETDLSYM(gtk_button_box_set_layout)
        GETDLSYM(gtk_button_new)
        GETDLSYM(gtk_cell_layout_get_type)
        GETDLSYM(gtk_cell_layout_pack_start)
        GETDLSYM(gtk_cell_layout_set_attributes)
        GETDLSYM(gtk_cell_renderer_text_new)
        GETDLSYM(gtk_check_button_new_with_mnemonic)
        GETDLSYM(gtk_combo_box_get_active)
        GETDLSYM(gtk_combo_box_get_active_iter)
        GETDLSYM(gtk_combo_box_get_active_text)
        GETDLSYM(gtk_combo_box_get_model)
        GETDLSYM(gtk_combo_box_get_type)
        GETDLSYM(gtk_combo_box_new_text)
        GETDLSYM(gtk_combo_box_new_with_model)
        GETDLSYM(gtk_combo_box_set_active)
        GETDLSYM(gtk_combo_box_set_active_iter)
        GETDLSYM(gtk_container_add)
        GETDLSYM(gtk_container_foreach)
        GETDLSYM(gtk_container_get_type)
        GETDLSYM(gtk_container_set_border_width)
        GETDLSYM(gtk_dialog_get_type)
        GETDLSYM(gtk_dialog_run)
        GETDLSYM(gtk_hbox_new)
        GETDLSYM(gtk_hbutton_box_new)
        GETDLSYM(gtk_image_new_from_pixbuf)
        GETDLSYM(gtk_image_new_from_stock)
        GETDLSYM(gtk_init_check)
        GETDLSYM(gtk_label_get_type)
        GETDLSYM(gtk_label_new)
        GETDLSYM(gtk_label_new_with_mnemonic)
        GETDLSYM(gtk_label_set_mnemonic_widget)
        GETDLSYM(gtk_list_store_append)
        GETDLSYM(gtk_list_store_clear)
        GETDLSYM(gtk_list_store_get_type)
        GETDLSYM(gtk_list_store_new)
        GETDLSYM(gtk_list_store_set)
        GETDLSYM(gtk_main)
        GETDLSYM(gtk_main_iteration_do)
        GETDLSYM(gtk_main_quit)
        GETDLSYM(gtk_message_dialog_new)
        GETDLSYM(gtk_misc_get_type)
        GETDLSYM(gtk_misc_set_alignment)
        GETDLSYM(gtk_notebook_get_nth_page)
        GETDLSYM(gtk_notebook_get_type)
        GETDLSYM(gtk_notebook_new)
        GETDLSYM(gtk_notebook_set_current_page)
        GETDLSYM(gtk_notebook_set_tab_label)
        GETDLSYM(gtk_object_get_type)
        GETDLSYM(gtk_scrolled_window_get_type)
        GETDLSYM(gtk_scrolled_window_new)
        GETDLSYM(gtk_scrolled_window_set_policy)
        GETDLSYM(gtk_scrolled_window_set_shadow_type)
        GETDLSYM(gtk_table_get_type)
        GETDLSYM(gtk_table_new)
        GETDLSYM(gtk_table_attach)
        GETDLSYM(gtk_text_buffer_backspace)
        GETDLSYM(gtk_text_buffer_create_mark)
        GETDLSYM(gtk_text_buffer_delete_mark)
        GETDLSYM(gtk_text_buffer_get_end_iter)
        GETDLSYM(gtk_text_buffer_insert)
        // FIXME: should I put a #if !GTK_CHECK_VERSION(2,6,0)
        // around these three, or should I not care?
        GETDLSYM(gtk_text_iter_backward_cursor_position)
        GETDLSYM(gtk_text_iter_equal)
        GETDLSYM(gtk_text_buffer_delete_interactive)
        //
        GETDLSYM(gtk_text_view_get_buffer)
        GETDLSYM(gtk_text_view_get_type)
        GETDLSYM(gtk_text_view_new)
        GETDLSYM(gtk_text_view_scroll_to_mark)
        GETDLSYM(gtk_text_view_set_cursor_visible)
        GETDLSYM(gtk_text_view_set_editable)
        GETDLSYM(gtk_text_view_set_left_margin)
        GETDLSYM(gtk_text_view_set_right_margin)
        GETDLSYM(gtk_text_view_set_wrap_mode)
        GETDLSYM(gtk_toggle_button_get_active)
        GETDLSYM(gtk_toggle_button_get_type)
        GETDLSYM(gtk_toggle_button_set_active)
        GETDLSYM(gtk_tree_model_get)
        GETDLSYM(gtk_tree_model_get_iter)
        GETDLSYM(gtk_tree_model_get_path)
        GETDLSYM(gtk_tree_model_get_type)
        GETDLSYM(gtk_tree_path_get_indices)
        GETDLSYM(gtk_tree_path_new_from_indices)
        GETDLSYM(gtk_tree_selection_get_selected)
        GETDLSYM(gtk_tree_selection_select_iter)
        GETDLSYM(gtk_tree_selection_set_mode)
        GETDLSYM(gtk_tree_sortable_get_type)
        GETDLSYM(gtk_tree_sortable_set_sort_column_id)
        GETDLSYM(gtk_tree_sortable_set_sort_func)
        GETDLSYM(gtk_tree_view_append_column)
        GETDLSYM(gtk_tree_view_column_new_with_attributes)
        GETDLSYM(gtk_tree_view_column_set_expand)
        GETDLSYM(gtk_tree_view_column_set_min_width)
        GETDLSYM(gtk_tree_view_get_model)
        GETDLSYM(gtk_tree_view_get_selection)
        GETDLSYM(gtk_tree_view_get_type)
        GETDLSYM(gtk_tree_view_new_with_model)
        GETDLSYM(gtk_tree_view_set_enable_search)
        GETDLSYM(gtk_tree_view_set_headers_visible)
        GETDLSYM(gtk_vbox_new)
        GETDLSYM(gtk_widget_add_accelerator)
        GETDLSYM(gtk_widget_destroy)
        GETDLSYM(gtk_widget_ref)
        GETDLSYM(gtk_widget_set_sensitive)
        GETDLSYM(gtk_widget_set_size_request)
        GETDLSYM(gtk_widget_show_all)
        GETDLSYM(gtk_widget_unref)
        GETDLSYM(gtk_window_add_accel_group)
        GETDLSYM(gtk_window_get_type)
        GETDLSYM(gtk_window_new)
        GETDLSYM(gtk_window_set_default_icon)
        GETDLSYM(gtk_window_set_position)
        GETDLSYM(gtk_window_set_resizable)
        GETDLSYM(gtk_window_set_title)
        GETDLSYM(gtk_window_set_type_hint)
        GETDLSYM(gtk_window_set_default)
        GETDLSYM(g_utf8_collate)
    }
    while (0);

    if (err)
    {
        //printf("Failed fetching symbol %s from GTK lib\n", failsym);
        dlclose(handle);
        handle = NULL;
        return -1;
    }
    return 0;
}

void dynamicgtk_uninit(void)
{
    if (handle) dlclose(handle);
    handle = NULL;
    memset(&dynamicgtksyms, 0, sizeof(dynamicgtksyms));
}

