#ifndef dynamicgtk_h_
#define dynamicgtk_h_

#include <gdk-pixbuf/gdk-pixdata.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "compat.h"

#if !GTK_CHECK_VERSION(2,4,0)
#error You need at least 2.4.0 version of GTK+
#endif

#ifndef G_GNUC_NULL_TERMINATED
/* this is a glib-2.8.x thing: */
#define G_GNUC_NULL_TERMINATED
#endif


// glib.h
typedef void (*g_free_ptr) (gpointer mem);

// gobject.h
typedef gpointer (*g_object_get_data_ptr) (GObject *object, const gchar *key);
typedef void (*g_object_set_data_ptr) (GObject *object, const gchar *key, gpointer data);
typedef void (*g_object_set_data_full_ptr) (GObject *object, const gchar *key, gpointer data, GDestroyNotify destroy);
typedef void (*g_object_unref_ptr) (gpointer object);

// gsignal.h
typedef gulong (*g_signal_connect_data_ptr) (gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data, GClosureNotify destroy_data, GConnectFlags connect_flags);
typedef guint (*g_signal_handlers_block_matched_ptr) (gpointer instance, GSignalMatchType mask, guint signal_id, GQuark detail, GClosure *closure, gpointer func, gpointer data);
typedef guint (*g_signal_handlers_unblock_matched_ptr) (gpointer instance, GSignalMatchType mask, guint signal_id, GQuark detail, GClosure *closure, gpointer func, gpointer data);

// gtype.h
typedef GTypeInstance* (*g_type_check_instance_cast_ptr) (GTypeInstance *instance, GType iface_type);

// gdk-pixdata.h
typedef GdkPixbuf* (*gdk_pixbuf_from_pixdata_ptr) (const GdkPixdata *pixdata, gboolean copy_pixels, GError **error);

// gdk-pixbuf-core.h
typedef GdkPixbuf *(*gdk_pixbuf_new_from_data_ptr) (const guchar *data, GdkColorspace colorspace, gboolean has_alpha, int32_t bits_per_sample, int32_t width, int32_t height, int32_t rowstride, GdkPixbufDestroyNotify destroy_fn, gpointer destroy_fn_data);

// gtkaccelgroup.h
typedef GtkAccelGroup* (*gtk_accel_group_new_ptr) (void);

// gtkalignment.h
typedef GtkWidget* (*gtk_alignment_new_ptr) (gfloat xalign, gfloat yalign, gfloat xscale, gfloat yscale);

// gtkbox.h
typedef GType (*gtk_box_get_type_ptr) (void) G_GNUC_CONST;
typedef void (*gtk_box_pack_start_ptr) (GtkBox *box, GtkWidget *child, gboolean expand, gboolean fill, guint padding);

// gtkbbox.h
typedef GType (*gtk_button_box_get_type_ptr) (void) G_GNUC_CONST;
typedef void (*gtk_button_box_set_layout_ptr) (GtkButtonBox *widget, GtkButtonBoxStyle layout_style);

// gtkbutton.h
typedef GtkWidget* (*gtk_button_new_ptr) (void);

// gtkcelllayout.h
typedef GType (*gtk_cell_layout_get_type_ptr) (void) G_GNUC_CONST;
typedef void (*gtk_cell_layout_pack_start_ptr) (GtkCellLayout *cell_layout, GtkCellRenderer *cell, gboolean expand);
typedef void (*gtk_cell_layout_set_attributes_ptr) (GtkCellLayout *cell_layout, GtkCellRenderer *cell, ...) G_GNUC_NULL_TERMINATED;

// gtkcellrenderertext.h
typedef GtkCellRenderer *(*gtk_cell_renderer_text_new_ptr) (void);

// gtkcheckbutton.h
typedef GtkWidget* (*gtk_check_button_new_with_mnemonic_ptr) (const gchar *label);

// gtkcombobox.h
typedef gint (*gtk_combo_box_get_active_ptr) (GtkComboBox *combo_box);
typedef gboolean (*gtk_combo_box_get_active_iter_ptr) (GtkComboBox *combo_box, GtkTreeIter *iter);
typedef gchar (*gtk_combo_box_get_active_text_ptr) (GtkComboBox *combo_box);
typedef GtkTreeModel *(*gtk_combo_box_get_model_ptr) (GtkComboBox *combo_box);
typedef GType (*gtk_combo_box_get_type_ptr) (void) G_GNUC_CONST;
typedef GtkWidget *(*gtk_combo_box_new_text_ptr) (void);
typedef GtkWidget *(*gtk_combo_box_new_with_model_ptr) (GtkTreeModel *model);
typedef void (*gtk_combo_box_set_active_ptr) (GtkComboBox *combo_box, gint index_);
typedef void (*gtk_combo_box_set_active_iter_ptr) (GtkComboBox *combo_box, GtkTreeIter *iter);

// gtkcontainer.h
typedef void (*gtk_container_add_ptr) (GtkContainer *container, GtkWidget *widget);
typedef void (*gtk_container_foreach_ptr) (GtkContainer *container, GtkCallback callback, gpointer callback_data);
typedef GType (*gtk_container_get_type_ptr) (void) G_GNUC_CONST;
typedef void (*gtk_container_set_border_width_ptr) (GtkContainer *container, guint border_width);

// gtkdialog.h
typedef GType (*gtk_dialog_get_type_ptr) (void) G_GNUC_CONST;
typedef gint (*gtk_dialog_run_ptr) (GtkDialog *dialog);

// gtkhbox.h
typedef GtkWidget* (*gtk_hbox_new_ptr) (gboolean homogeneous, gint spacing);

// gtkhbbox.h
typedef GtkWidget* (*gtk_hbutton_box_new_ptr) (void);

// gtkimage.h
typedef GtkWidget* (*gtk_image_new_from_pixbuf_ptr) (GdkPixbuf *pixbuf);
typedef GtkWidget* (*gtk_image_new_from_stock_ptr) (const gchar *stock_id, GtkIconSize size);

// gtkmain.h
typedef gboolean (*gtk_init_check_ptr) (int32_t *argc, char ***argv);

// gtklabel.h
typedef GType (*gtk_label_get_type_ptr) (void) G_GNUC_CONST;
typedef GtkWidget* (*gtk_label_new_ptr) (const gchar *str);
typedef GtkWidget* (*gtk_label_new_with_mnemonic_ptr) (const gchar *str);
typedef void (*gtk_label_set_mnemonic_widget_ptr) (GtkLabel *label, GtkWidget *widget);

// gtkliststore.h
typedef void (*gtk_list_store_append_ptr) (GtkListStore *list_store, GtkTreeIter *iter);
typedef void (*gtk_list_store_clear_ptr) (GtkListStore *list_store);
typedef GType (*gtk_list_store_get_type_ptr) (void) G_GNUC_CONST;
typedef GtkListStore *(*gtk_list_store_new_ptr) (gint n_columns, ...);
typedef void (*gtk_list_store_set_ptr) (GtkListStore *list_store, GtkTreeIter *iter, ...);

// gtkmain.h
typedef void (*gtk_main_ptr) (void);
typedef gboolean (*gtk_main_iteration_do_ptr) (gboolean blocking);
typedef void (*gtk_main_quit_ptr) (void);

// gtkmessagedialog.h
typedef GtkWidget* (*gtk_message_dialog_new_ptr) (GtkWindow *parent, GtkDialogFlags flags, GtkMessageType type, GtkButtonsType buttons, const gchar *message_format, ...) G_GNUC_PRINTF (5, 6);

// gtkmisc.h
typedef GType (*gtk_misc_get_type_ptr) (void) G_GNUC_CONST;
typedef void (*gtk_misc_set_alignment_ptr) (GtkMisc *misc, gfloat xalign, gfloat yalign);

// gtknotebook.h
typedef GtkWidget* (*gtk_notebook_get_nth_page_ptr) (GtkNotebook *notebook, gint page_num);
typedef GType (*gtk_notebook_get_type_ptr) (void) G_GNUC_CONST;
typedef GtkWidget * (*gtk_notebook_new_ptr) (void);
typedef void (*gtk_notebook_set_current_page_ptr) (GtkNotebook *notebook, gint page_num);
typedef void (*gtk_notebook_set_tab_label_ptr) (GtkNotebook *notebook, GtkWidget *child, GtkWidget *tab_label);

// gtkobject.h
typedef GtkType (*gtk_object_get_type_ptr) (void) G_GNUC_CONST;

// gtkscrolledwindow.h
typedef GType (*gtk_scrolled_window_get_type_ptr) (void) G_GNUC_CONST;
typedef GtkWidget* (*gtk_scrolled_window_new_ptr) (GtkAdjustment *hadjustment, GtkAdjustment *vadjustment);
typedef void (*gtk_scrolled_window_set_policy_ptr) (GtkScrolledWindow *scrolled_window, GtkPolicyType hscrollbar_policy, GtkPolicyType vscrollbar_policy);
typedef void (*gtk_scrolled_window_set_shadow_type_ptr) (GtkScrolledWindow *scrolled_window, GtkShadowType type);

// gtktable.h
typedef GType (*gtk_table_get_type_ptr) (void) G_GNUC_CONST;
typedef GtkWidget* (*gtk_table_new_ptr) (guint rows, guint columns, gboolean homogeneous);
typedef void (*gtk_table_attach_ptr) (GtkTable *table, GtkWidget *child, guint left_attach, guint right_attach, guint top_attach, guint bottom_attach, GtkAttachOptions xoptions, GtkAttachOptions yoptions, guint xpadding, guint ypadding);

// gtktextbuffer.h
typedef gboolean (*gtk_text_buffer_backspace_ptr) (GtkTextBuffer *buffer, GtkTextIter *iter, gboolean interactive, gboolean default_editable);
typedef GtkTextMark *(*gtk_text_buffer_create_mark_ptr) (GtkTextBuffer *buffer, const gchar *mark_name, const GtkTextIter *where, gboolean left_gravity);
typedef void (*gtk_text_buffer_delete_mark_ptr) (GtkTextBuffer *buffer, GtkTextMark *mark);
typedef void (*gtk_text_buffer_get_end_iter_ptr) (GtkTextBuffer *buffer, GtkTextIter *iter);
typedef void (*gtk_text_buffer_insert_ptr) (GtkTextBuffer *buffer, GtkTextIter *iter, const gchar *text, gint len);

// gtktextiter.h
// FIXME: should I put a #if !GTK_CHECK_VERSION(2,6,0)
// around these three, or should I not care?
typedef gboolean (*gtk_text_iter_backward_cursor_position_ptr) (GtkTextIter *iter);
typedef gboolean (*gtk_text_iter_equal_ptr) (const GtkTextIter *lhs, const GtkTextIter *rhs);
typedef gboolean (*gtk_text_buffer_delete_interactive_ptr) (GtkTextBuffer *buffer, GtkTextIter *start_iter, GtkTextIter *end_iter, gboolean default_editable);

// gtktextview.h
typedef GtkTextBuffer *(*gtk_text_view_get_buffer_ptr) (GtkTextView *text_view);
typedef GType (*gtk_text_view_get_type_ptr) (void) G_GNUC_CONST;
typedef GtkWidget * (*gtk_text_view_new_ptr) (void);
typedef void (*gtk_text_view_scroll_to_mark_ptr) (GtkTextView *text_view, GtkTextMark *mark, gdouble within_margin, gboolean use_align, gdouble xalign, gdouble yalign);
typedef void (*gtk_text_view_set_cursor_visible_ptr) (GtkTextView *text_view, gboolean setting);
typedef void (*gtk_text_view_set_editable_ptr) (GtkTextView *text_view, gboolean setting);
typedef void (*gtk_text_view_set_left_margin_ptr) (GtkTextView *text_view, gint left_margin);
typedef void (*gtk_text_view_set_right_margin_ptr) (GtkTextView *text_view, gint right_margin);
typedef void (*gtk_text_view_set_wrap_mode_ptr) (GtkTextView *text_view, GtkWrapMode wrap_mode);

// gtktogglebutton.h
typedef gboolean (*gtk_toggle_button_get_active_ptr) (GtkToggleButton *toggle_button);
typedef GType (*gtk_toggle_button_get_type_ptr) (void) G_GNUC_CONST;
typedef void (*gtk_toggle_button_set_active_ptr) (GtkToggleButton *toggle_button, gboolean is_active);

// gtktreemodel.h
typedef void (*gtk_tree_model_get_ptr) (GtkTreeModel *tree_model, GtkTreeIter *iter, ...);
typedef gboolean (*gtk_tree_model_get_iter_ptr) (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path);
typedef GtkTreePath * (*gtk_tree_model_get_path_ptr) (GtkTreeModel *tree_model, GtkTreeIter *iter);
typedef GType (*gtk_tree_model_get_type_ptr) (void) G_GNUC_CONST;
typedef gint *(*gtk_tree_path_get_indices_ptr) (GtkTreePath *path);
typedef GtkTreePath *(*gtk_tree_path_new_from_indices_ptr) (gint first_index, ...);

// gtktreeselection.h
typedef gboolean (*gtk_tree_selection_get_selected_ptr) (GtkTreeSelection *selection, GtkTreeModel **model, GtkTreeIter *iter);
typedef void (*gtk_tree_selection_select_iter_ptr) (GtkTreeSelection *selection, GtkTreeIter *iter);
typedef GType (*gtk_tree_selection_get_type_ptr) (void) G_GNUC_CONST;
typedef void (*gtk_tree_selection_set_mode_ptr) (GtkTreeSelection *selection, GtkSelectionMode type);

// gtktreesortable.h
typedef GType (*gtk_tree_sortable_get_type_ptr) (void) G_GNUC_CONST;
typedef void (*gtk_tree_sortable_set_sort_column_id_ptr) (GtkTreeSortable *sortable, gint sort_column_id, GtkSortType order);
typedef void (*gtk_tree_sortable_set_sort_func_ptr) (GtkTreeSortable *sortable, gint sort_column_id, GtkTreeIterCompareFunc sort_func, gpointer user_data, GtkDestroyNotify destroy);

// gtktreeview.h
typedef GType (*gtk_tree_view_get_type_ptr) (void) G_GNUC_CONST;
typedef GtkWidget *(*gtk_tree_view_new_with_model_ptr) (GtkTreeModel *model);
typedef GtkTreeModel *(*gtk_tree_view_get_model_ptr) (GtkTreeView *tree_view);
typedef GtkTreeSelection *(*gtk_tree_view_get_selection_ptr) (GtkTreeView *tree_view);
typedef gint (*gtk_tree_view_append_column_ptr) (GtkTreeView *tree_view, GtkTreeViewColumn *column);
typedef void (*gtk_tree_view_set_enable_search_ptr) (GtkTreeView *tree_view, gboolean enable_search);
typedef void (*gtk_tree_view_set_headers_visible_ptr) (GtkTreeView *tree_view, gboolean headers_visible);

// gtktreeviewcolumn.h
typedef GtkTreeViewColumn *(*gtk_tree_view_column_new_with_attributes_ptr) (const gchar *title, GtkCellRenderer *cell, ...) G_GNUC_NULL_TERMINATED;
typedef void (*gtk_tree_view_column_set_expand_ptr) (GtkTreeViewColumn *tree_column, gboolean expand);
typedef void (*gtk_tree_view_column_set_min_width_ptr) (GtkTreeViewColumn *tree_column, gint min_width);

// gtkvbox.h
typedef GtkWidget* (*gtk_vbox_new_ptr) (gboolean homogeneous, gint spacing);

// gtkwidget.h
typedef void (*gtk_widget_add_accelerator_ptr) (GtkWidget *widget, const gchar *accel_signal, GtkAccelGroup *accel_group, guint accel_key, GdkModifierType accel_mods, GtkAccelFlags accel_flags);
typedef void (*gtk_widget_destroy_ptr) (GtkWidget *widget);
typedef GtkWidget* (*gtk_widget_ref_ptr) (GtkWidget *widget);
typedef void (*gtk_widget_set_sensitive_ptr) (GtkWidget *widget, gboolean sensitive);
typedef void (*gtk_widget_set_size_request_ptr) (GtkWidget *widget, gint width, gint height);
typedef void (*gtk_widget_show_all_ptr) (GtkWidget *widget);
typedef void (*gtk_widget_unref_ptr) (GtkWidget *widget);

// gtkwindow.h
typedef void (*gtk_window_add_accel_group_ptr) (GtkWindow *window, GtkAccelGroup *accel_group);
typedef GType (*gtk_window_get_type_ptr) (void) G_GNUC_CONST;
typedef GtkWidget* (*gtk_window_new_ptr) (GtkWindowType type);
typedef void (*gtk_window_set_default_icon_ptr) (GdkPixbuf *icon);
typedef void (*gtk_window_set_position_ptr) (GtkWindow *window, GtkWindowPosition position);
typedef void (*gtk_window_set_resizable_ptr) (GtkWindow *window, gboolean resizable);
typedef void (*gtk_window_set_title_ptr) (GtkWindow *window, const gchar *title);
typedef void (*gtk_window_set_type_hint_ptr) (GtkWindow *window, GdkWindowTypeHint hint);
typedef void (*gtk_window_set_default_ptr) (GtkWindow *window, GtkWidget *default_widget);

// gunicode.h
typedef gint (*g_utf8_collate_ptr) (const gchar *str1, const gchar *str2);


struct _dynamicgtksyms {
    g_free_ptr g_free;
    g_object_get_data_ptr g_object_get_data;
    g_object_set_data_ptr g_object_set_data;
    g_object_set_data_full_ptr g_object_set_data_full;
    g_object_unref_ptr g_object_unref;
    g_signal_connect_data_ptr g_signal_connect_data;
    g_signal_handlers_block_matched_ptr g_signal_handlers_block_matched;
    g_signal_handlers_unblock_matched_ptr g_signal_handlers_unblock_matched;
    g_type_check_instance_cast_ptr g_type_check_instance_cast;
    gdk_pixbuf_from_pixdata_ptr gdk_pixbuf_from_pixdata;
    gdk_pixbuf_new_from_data_ptr gdk_pixbuf_new_from_data;
    gtk_accel_group_new_ptr gtk_accel_group_new;
    gtk_alignment_new_ptr gtk_alignment_new;
    gtk_box_get_type_ptr gtk_box_get_type;
    gtk_box_pack_start_ptr gtk_box_pack_start;
    gtk_button_box_get_type_ptr gtk_button_box_get_type;
    gtk_button_box_set_layout_ptr gtk_button_box_set_layout;
    gtk_button_new_ptr gtk_button_new;
    gtk_cell_layout_get_type_ptr gtk_cell_layout_get_type;
    gtk_cell_layout_pack_start_ptr gtk_cell_layout_pack_start;
    gtk_cell_layout_set_attributes_ptr gtk_cell_layout_set_attributes;
    gtk_cell_renderer_text_new_ptr gtk_cell_renderer_text_new;
    gtk_check_button_new_with_mnemonic_ptr gtk_check_button_new_with_mnemonic;
    gtk_combo_box_get_active_ptr gtk_combo_box_get_active;
    gtk_combo_box_get_active_iter_ptr gtk_combo_box_get_active_iter;
    gtk_combo_box_get_active_text_ptr gtk_combo_box_get_active_text;
    gtk_combo_box_get_model_ptr gtk_combo_box_get_model;
    gtk_combo_box_get_type_ptr gtk_combo_box_get_type;
    gtk_combo_box_new_text_ptr gtk_combo_box_new_text;
    gtk_combo_box_new_with_model_ptr gtk_combo_box_new_with_model;
    gtk_combo_box_set_active_ptr gtk_combo_box_set_active;
    gtk_combo_box_set_active_iter_ptr gtk_combo_box_set_active_iter;
    gtk_container_add_ptr gtk_container_add;
    gtk_container_foreach_ptr gtk_container_foreach;
    gtk_container_get_type_ptr gtk_container_get_type;
    gtk_container_set_border_width_ptr gtk_container_set_border_width;
    gtk_dialog_get_type_ptr gtk_dialog_get_type;
    gtk_dialog_run_ptr gtk_dialog_run;
    gtk_hbox_new_ptr gtk_hbox_new;
    gtk_hbutton_box_new_ptr gtk_hbutton_box_new;
    gtk_image_new_from_pixbuf_ptr gtk_image_new_from_pixbuf;
    gtk_image_new_from_stock_ptr gtk_image_new_from_stock;
    gtk_init_check_ptr gtk_init_check;
    gtk_label_get_type_ptr gtk_label_get_type;
    gtk_label_new_ptr gtk_label_new;
    gtk_label_new_with_mnemonic_ptr gtk_label_new_with_mnemonic;
    gtk_label_set_mnemonic_widget_ptr gtk_label_set_mnemonic_widget;
    gtk_list_store_append_ptr gtk_list_store_append;
    gtk_list_store_clear_ptr gtk_list_store_clear;
    gtk_list_store_get_type_ptr gtk_list_store_get_type;
    gtk_list_store_new_ptr gtk_list_store_new;
    gtk_list_store_set_ptr gtk_list_store_set;
    gtk_main_ptr gtk_main;
    gtk_main_iteration_do_ptr gtk_main_iteration_do;
    gtk_main_quit_ptr gtk_main_quit;
    gtk_message_dialog_new_ptr gtk_message_dialog_new;
    gtk_misc_get_type_ptr gtk_misc_get_type;
    gtk_misc_set_alignment_ptr gtk_misc_set_alignment;
    gtk_notebook_get_nth_page_ptr gtk_notebook_get_nth_page;
    gtk_notebook_get_type_ptr gtk_notebook_get_type;
    gtk_notebook_new_ptr gtk_notebook_new;
    gtk_notebook_set_current_page_ptr gtk_notebook_set_current_page;
    gtk_notebook_set_tab_label_ptr gtk_notebook_set_tab_label;
    gtk_object_get_type_ptr gtk_object_get_type;
    gtk_scrolled_window_get_type_ptr gtk_scrolled_window_get_type;
    gtk_scrolled_window_new_ptr gtk_scrolled_window_new;
    gtk_scrolled_window_set_policy_ptr gtk_scrolled_window_set_policy;
    gtk_scrolled_window_set_shadow_type_ptr gtk_scrolled_window_set_shadow_type;
    gtk_table_get_type_ptr gtk_table_get_type;
    gtk_table_new_ptr gtk_table_new;
    gtk_table_attach_ptr gtk_table_attach;
    gtk_text_buffer_backspace_ptr gtk_text_buffer_backspace;
    gtk_text_buffer_create_mark_ptr gtk_text_buffer_create_mark;
    gtk_text_buffer_delete_mark_ptr gtk_text_buffer_delete_mark;
    gtk_text_buffer_get_end_iter_ptr gtk_text_buffer_get_end_iter;
    gtk_text_buffer_insert_ptr gtk_text_buffer_insert;
    // FIXME: should I put a #if !GTK_CHECK_VERSION(2,6,0)
    // around these three, or should I not care?
    gtk_text_iter_backward_cursor_position_ptr gtk_text_iter_backward_cursor_position;
    gtk_text_iter_equal_ptr gtk_text_iter_equal;
    gtk_text_buffer_delete_interactive_ptr gtk_text_buffer_delete_interactive;
    //
    gtk_text_view_get_buffer_ptr gtk_text_view_get_buffer;
    gtk_text_view_get_type_ptr gtk_text_view_get_type;
    gtk_text_view_new_ptr gtk_text_view_new;
    gtk_text_view_scroll_to_mark_ptr gtk_text_view_scroll_to_mark;
    gtk_text_view_set_cursor_visible_ptr gtk_text_view_set_cursor_visible;
    gtk_text_view_set_editable_ptr gtk_text_view_set_editable;
    gtk_text_view_set_left_margin_ptr gtk_text_view_set_left_margin;
    gtk_text_view_set_right_margin_ptr gtk_text_view_set_right_margin;
    gtk_text_view_set_wrap_mode_ptr gtk_text_view_set_wrap_mode;
    gtk_toggle_button_get_active_ptr gtk_toggle_button_get_active;
    gtk_toggle_button_get_type_ptr gtk_toggle_button_get_type;
    gtk_toggle_button_set_active_ptr gtk_toggle_button_set_active;
    gtk_tree_model_get_ptr gtk_tree_model_get;
    gtk_tree_model_get_iter_ptr gtk_tree_model_get_iter;
    gtk_tree_model_get_path_ptr gtk_tree_model_get_path;
    gtk_tree_model_get_type_ptr gtk_tree_model_get_type;
    gtk_tree_path_get_indices_ptr gtk_tree_path_get_indices;
    gtk_tree_path_new_from_indices_ptr gtk_tree_path_new_from_indices;
    gtk_tree_selection_get_selected_ptr gtk_tree_selection_get_selected;
    gtk_tree_selection_select_iter_ptr gtk_tree_selection_select_iter;
    gtk_tree_selection_set_mode_ptr gtk_tree_selection_set_mode;
    gtk_tree_sortable_get_type_ptr gtk_tree_sortable_get_type;
    gtk_tree_sortable_set_sort_column_id_ptr gtk_tree_sortable_set_sort_column_id;
    gtk_tree_sortable_set_sort_func_ptr gtk_tree_sortable_set_sort_func;
    gtk_tree_view_append_column_ptr gtk_tree_view_append_column;
    gtk_tree_view_column_new_with_attributes_ptr gtk_tree_view_column_new_with_attributes;
    gtk_tree_view_column_set_expand_ptr gtk_tree_view_column_set_expand;
    gtk_tree_view_column_set_min_width_ptr gtk_tree_view_column_set_min_width;
    gtk_tree_view_get_model_ptr gtk_tree_view_get_model;
    gtk_tree_view_get_selection_ptr gtk_tree_view_get_selection;
    gtk_tree_view_get_type_ptr gtk_tree_view_get_type;
    gtk_tree_view_new_with_model_ptr gtk_tree_view_new_with_model;
    gtk_tree_view_set_enable_search_ptr gtk_tree_view_set_enable_search;
    gtk_tree_view_set_headers_visible_ptr gtk_tree_view_set_headers_visible;
    gtk_vbox_new_ptr gtk_vbox_new;
    gtk_widget_add_accelerator_ptr gtk_widget_add_accelerator;
    gtk_widget_destroy_ptr gtk_widget_destroy;
    gtk_widget_ref_ptr gtk_widget_ref;
    gtk_widget_set_sensitive_ptr gtk_widget_set_sensitive;
    gtk_widget_set_size_request_ptr gtk_widget_set_size_request;
    gtk_widget_show_all_ptr gtk_widget_show_all;
    gtk_widget_unref_ptr gtk_widget_unref;
    gtk_window_add_accel_group_ptr gtk_window_add_accel_group;
    gtk_window_get_type_ptr gtk_window_get_type;
    gtk_window_new_ptr gtk_window_new;
    gtk_window_set_default_icon_ptr gtk_window_set_default_icon;
    gtk_window_set_position_ptr gtk_window_set_position;
    gtk_window_set_resizable_ptr gtk_window_set_resizable;
    gtk_window_set_title_ptr gtk_window_set_title;
    gtk_window_set_type_hint_ptr gtk_window_set_type_hint;
    gtk_window_set_default_ptr gtk_window_set_default;
    g_utf8_collate_ptr g_utf8_collate;
};
extern struct _dynamicgtksyms dynamicgtksyms;


int32_t dynamicgtk_init(void);
void dynamicgtk_uninit(void);

#ifndef dynamicgtkfoo__

// glib.h
#define g_free dynamicgtksyms.g_free

// gobject.h
#define g_object_get_data dynamicgtksyms.g_object_get_data
#define g_object_set_data dynamicgtksyms.g_object_set_data
#define g_object_set_data_full dynamicgtksyms.g_object_set_data_full
#define g_object_unref dynamicgtksyms.g_object_unref

// gsignal.h
#define g_signal_connect_data dynamicgtksyms.g_signal_connect_data
#define g_signal_handlers_block_matched dynamicgtksyms.g_signal_handlers_block_matched
#define g_signal_handlers_unblock_matched dynamicgtksyms.g_signal_handlers_unblock_matched

// gtype.h
#define g_type_check_instance_cast dynamicgtksyms.g_type_check_instance_cast

// gdk-pixdata.h
#define gdk_pixbuf_from_pixdata dynamicgtksyms.gdk_pixbuf_from_pixdata

// gdk-pixbuf-core.h
#define gdk_pixbuf_new_from_data dynamicgtksyms.gdk_pixbuf_new_from_data

// gtkaccelgroup.h
#define gtk_accel_group_new dynamicgtksyms.gtk_accel_group_new

// gtkalignment.h
#define gtk_alignment_new dynamicgtksyms.gtk_alignment_new

// gtkbox.h
#define gtk_box_get_type dynamicgtksyms.gtk_box_get_type
#define gtk_box_pack_start dynamicgtksyms.gtk_box_pack_start

// gtkbbox.h
#define gtk_button_box_get_type dynamicgtksyms.gtk_button_box_get_type
#define gtk_button_box_set_layout dynamicgtksyms.gtk_button_box_set_layout

// gtkbutton.h
#define gtk_button_new dynamicgtksyms.gtk_button_new

// gtkcelllayout.h
#define gtk_cell_layout_get_type dynamicgtksyms.gtk_cell_layout_get_type
#define gtk_cell_layout_pack_start dynamicgtksyms.gtk_cell_layout_pack_start
#define gtk_cell_layout_set_attributes dynamicgtksyms.gtk_cell_layout_set_attributes

// gtkcellrenderertext.h
#define gtk_cell_renderer_text_new dynamicgtksyms.gtk_cell_renderer_text_new

// gtkcheckbutton.h
#define gtk_check_button_new_with_mnemonic dynamicgtksyms.gtk_check_button_new_with_mnemonic

// gtkcombobox.h
#define gtk_combo_box_get_active dynamicgtksyms.gtk_combo_box_get_active
#define gtk_combo_box_get_active_iter dynamicgtksyms.gtk_combo_box_get_active_iter
#define gtk_combo_box_get_active_text dynamicgtksyms.gtk_combo_box_get_active_text
#define gtk_combo_box_get_model dynamicgtksyms.gtk_combo_box_get_model
#define gtk_combo_box_get_type dynamicgtksyms.gtk_combo_box_get_type
#define gtk_combo_box_new_text dynamicgtksyms.gtk_combo_box_new_text
#define gtk_combo_box_new_with_model dynamicgtksyms.gtk_combo_box_new_with_model
#define gtk_combo_box_set_active dynamicgtksyms.gtk_combo_box_set_active
#define gtk_combo_box_set_active_iter dynamicgtksyms.gtk_combo_box_set_active_iter

// gtkcontainer.h
#define gtk_container_add dynamicgtksyms.gtk_container_add
#define gtk_container_foreach dynamicgtksyms.gtk_container_foreach
#define gtk_container_get_type dynamicgtksyms.gtk_container_get_type
#define gtk_container_set_border_width dynamicgtksyms.gtk_container_set_border_width

// gtkdialog.h
#define gtk_dialog_get_type dynamicgtksyms.gtk_dialog_get_type
#define gtk_dialog_run dynamicgtksyms.gtk_dialog_run

// gtkhbox.h
#define gtk_hbox_new dynamicgtksyms.gtk_hbox_new

// gtkhbbox.h
#define gtk_hbutton_box_new dynamicgtksyms.gtk_hbutton_box_new

// gtkimage.h
#define gtk_image_new_from_pixbuf dynamicgtksyms.gtk_image_new_from_pixbuf
#define gtk_image_new_from_stock dynamicgtksyms.gtk_image_new_from_stock

// gtkmain.h
#define gtk_init_check dynamicgtksyms.gtk_init_check

// gtklabel.h
#define gtk_label_get_type dynamicgtksyms.gtk_label_get_type
#define gtk_label_new dynamicgtksyms.gtk_label_new
#define gtk_label_new_with_mnemonic dynamicgtksyms.gtk_label_new_with_mnemonic
#define gtk_label_set_mnemonic_widget dynamicgtksyms.gtk_label_set_mnemonic_widget

// gtkliststore.h
#define gtk_list_store_append dynamicgtksyms.gtk_list_store_append
#define gtk_list_store_clear dynamicgtksyms.gtk_list_store_clear
#define gtk_list_store_get_type dynamicgtksyms.gtk_list_store_get_type
#define gtk_list_store_new dynamicgtksyms.gtk_list_store_new
#define gtk_list_store_set dynamicgtksyms.gtk_list_store_set

// gtkmain.h
#define gtk_main dynamicgtksyms.gtk_main
#define gtk_main_iteration_do dynamicgtksyms.gtk_main_iteration_do
#define gtk_main_quit dynamicgtksyms.gtk_main_quit

// gtkmessagedialog.h
#define gtk_message_dialog_new dynamicgtksyms.gtk_message_dialog_new

// gtkmisc.h
#define gtk_misc_get_type dynamicgtksyms.gtk_misc_get_type
#define gtk_misc_set_alignment dynamicgtksyms.gtk_misc_set_alignment

// gtknotebook.h
#define gtk_notebook_get_nth_page dynamicgtksyms.gtk_notebook_get_nth_page
#define gtk_notebook_get_type dynamicgtksyms.gtk_notebook_get_type
#define gtk_notebook_new dynamicgtksyms.gtk_notebook_new
#define gtk_notebook_set_current_page dynamicgtksyms.gtk_notebook_set_current_page
#define gtk_notebook_set_tab_label dynamicgtksyms.gtk_notebook_set_tab_label

// gtkobject.h
#define gtk_object_get_type dynamicgtksyms.gtk_object_get_type

// gtkscrolledwindow.h
#define gtk_scrolled_window_get_type dynamicgtksyms.gtk_scrolled_window_get_type
#define gtk_scrolled_window_new dynamicgtksyms.gtk_scrolled_window_new
#define gtk_scrolled_window_set_policy dynamicgtksyms.gtk_scrolled_window_set_policy
#define gtk_scrolled_window_set_shadow_type dynamicgtksyms.gtk_scrolled_window_set_shadow_type

// gtktable.h
#define gtk_table_get_type dynamicgtksyms.gtk_table_get_type
#define gtk_table_new dynamicgtksyms.gtk_table_new
#define gtk_table_attach dynamicgtksyms.gtk_table_attach

// gtktextbuffer.h
#define gtk_text_buffer_backspace dynamicgtksyms.gtk_text_buffer_backspace
#define gtk_text_buffer_create_mark dynamicgtksyms.gtk_text_buffer_create_mark
#define gtk_text_buffer_delete_mark dynamicgtksyms.gtk_text_buffer_delete_mark
#define gtk_text_buffer_get_end_iter dynamicgtksyms.gtk_text_buffer_get_end_iter
#define gtk_text_buffer_insert dynamicgtksyms.gtk_text_buffer_insert

// gtktextiter.h
#define gtk_text_iter_backward_cursor_position dynamicgtksyms.gtk_text_iter_backward_cursor_position
#define gtk_text_iter_equal dynamicgtksyms.gtk_text_iter_equal
#define gtk_text_buffer_delete_interactive dynamicgtksyms.gtk_text_buffer_delete_interactive

// gtktextview.h
#define gtk_text_view_get_buffer dynamicgtksyms.gtk_text_view_get_buffer
#define gtk_text_view_get_type dynamicgtksyms.gtk_text_view_get_type
#define gtk_text_view_new dynamicgtksyms.gtk_text_view_new
#define gtk_text_view_scroll_to_mark dynamicgtksyms.gtk_text_view_scroll_to_mark
#define gtk_text_view_set_cursor_visible dynamicgtksyms.gtk_text_view_set_cursor_visible
#define gtk_text_view_set_editable dynamicgtksyms.gtk_text_view_set_editable
#define gtk_text_view_set_left_margin dynamicgtksyms.gtk_text_view_set_left_margin
#define gtk_text_view_set_right_margin dynamicgtksyms.gtk_text_view_set_right_margin
#define gtk_text_view_set_wrap_mode dynamicgtksyms.gtk_text_view_set_wrap_mode

// gtktogglebutton.h
#define gtk_toggle_button_get_active dynamicgtksyms.gtk_toggle_button_get_active
#define gtk_toggle_button_get_type dynamicgtksyms.gtk_toggle_button_get_type
#define gtk_toggle_button_set_active dynamicgtksyms.gtk_toggle_button_set_active

// gtktreemodel.h
#define gtk_tree_model_get dynamicgtksyms.gtk_tree_model_get
#define gtk_tree_model_get_iter dynamicgtksyms.gtk_tree_model_get_iter
#define gtk_tree_model_get_path dynamicgtksyms.gtk_tree_model_get_path
#define gtk_tree_model_get_type dynamicgtksyms.gtk_tree_model_get_type
#define gtk_tree_path_get_indices dynamicgtksyms.gtk_tree_path_get_indices
#define gtk_tree_path_new_from_indices dynamicgtksyms.gtk_tree_path_new_from_indices

// gtktreeselection.h
#define gtk_tree_selection_get_selected dynamicgtksyms.gtk_tree_selection_get_selected
#define gtk_tree_selection_select_iter dynamicgtksyms.gtk_tree_selection_select_iter
#define gtk_tree_selection_set_mode dynamicgtksyms.gtk_tree_selection_set_mode

// gtktreesortable.h
#define gtk_tree_sortable_get_type dynamicgtksyms.gtk_tree_sortable_get_type
#define gtk_tree_sortable_set_sort_column_id dynamicgtksyms.gtk_tree_sortable_set_sort_column_id
#define gtk_tree_sortable_set_sort_func dynamicgtksyms.gtk_tree_sortable_set_sort_func

// gtktreeview.h
#define gtk_tree_view_append_column dynamicgtksyms.gtk_tree_view_append_column
#define gtk_tree_view_get_model dynamicgtksyms.gtk_tree_view_get_model
#define gtk_tree_view_get_selection dynamicgtksyms.gtk_tree_view_get_selection
#define gtk_tree_view_get_type dynamicgtksyms.gtk_tree_view_get_type
#define gtk_tree_view_new_with_model dynamicgtksyms.gtk_tree_view_new_with_model
#define gtk_tree_view_set_enable_search dynamicgtksyms.gtk_tree_view_set_enable_search
#define gtk_tree_view_set_headers_visible dynamicgtksyms.gtk_tree_view_set_headers_visible

// gtktreeviewcolumn.h
#define gtk_tree_view_column_new_with_attributes dynamicgtksyms.gtk_tree_view_column_new_with_attributes
#define gtk_tree_view_column_set_expand dynamicgtksyms.gtk_tree_view_column_set_expand
#define gtk_tree_view_column_set_min_width dynamicgtksyms.gtk_tree_view_column_set_min_width


// gtkvbox.h
#define gtk_vbox_new dynamicgtksyms.gtk_vbox_new

// gtkwidget.h
#define gtk_widget_add_accelerator dynamicgtksyms.gtk_widget_add_accelerator
#define gtk_widget_destroy dynamicgtksyms.gtk_widget_destroy
#define gtk_widget_ref dynamicgtksyms.gtk_widget_ref
#define gtk_widget_set_sensitive dynamicgtksyms.gtk_widget_set_sensitive
#define gtk_widget_set_size_request dynamicgtksyms.gtk_widget_set_size_request
#define gtk_widget_show_all dynamicgtksyms.gtk_widget_show_all
#define gtk_widget_unref dynamicgtksyms.gtk_widget_unref

// gtkwindow.h
#define gtk_window_add_accel_group dynamicgtksyms.gtk_window_add_accel_group
#define gtk_window_get_type dynamicgtksyms.gtk_window_get_type
#define gtk_window_new dynamicgtksyms.gtk_window_new
#define gtk_window_set_default_icon dynamicgtksyms.gtk_window_set_default_icon
#define gtk_window_set_position dynamicgtksyms.gtk_window_set_position
#define gtk_window_set_resizable dynamicgtksyms.gtk_window_set_resizable
#define gtk_window_set_title dynamicgtksyms.gtk_window_set_title
#define gtk_window_set_type_hint dynamicgtksyms.gtk_window_set_type_hint
#define gtk_window_set_default dynamicgtksyms.gtk_window_set_default

// gunicode.h
#define g_utf8_collate dynamicgtksyms.g_utf8_collate

#endif	/* __dynamicgtkfoo__ */

#endif	/* dynamicgtk_h_ */
