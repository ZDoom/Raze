#ifndef __dynamicgtk_h__
#define __dynamicgtk_h__

#include <gdk-pixbuf/gdk-pixdata.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "compat.h"

#if !GTK_CHECK_VERSION(2,4,0)
#error You need at least 2.4.0 version of GTK+
#endif

#if !defined(LINKED_GTK)

#ifndef G_GNUC_NULL_TERMINATED
/* this is a glib-2.8.x thing: */
#define G_GNUC_NULL_TERMINATED
#endif

struct _dynamicgtksyms {

    // glib.h
void        (*g_free)                           (gpointer      mem);
	// gobject.h
gpointer    (*g_object_get_data)                 (GObject        *object,
					       const gchar    *key);
void        (*g_object_set_data)                 (GObject        *object,
					       const gchar    *key,
					       gpointer        data);
void        (*g_object_set_data_full)            (GObject        *object,
					       const gchar    *key,
					       gpointer        data,
					       GDestroyNotify  destroy);
void        (*g_object_unref)                    (gpointer        object);

	// gsignal.h
gulong	 (*g_signal_connect_data)		      (gpointer		  instance,
					       const gchar	 *detailed_signal,
					       GCallback	  c_handler,
					       gpointer		  data,
					       GClosureNotify	  destroy_data,
					       GConnectFlags	  connect_flags);
guint	 (*g_signal_handlers_block_matched)      (gpointer		  instance,
					       GSignalMatchType	  mask,
					       guint		  signal_id,
					       GQuark		  detail,
					       GClosure		 *closure,
					       gpointer		  func,
					       gpointer		  data);
guint	 (*g_signal_handlers_unblock_matched)    (gpointer		  instance,
					       GSignalMatchType	  mask,
					       guint		  signal_id,
					       GQuark		  detail,
					       GClosure		 *closure,
					       gpointer		  func,
					       gpointer		  data);

	// gtype.h
GTypeInstance*   (*g_type_check_instance_cast)     (GTypeInstance      *instance,
						 GType               iface_type);

	// gdk-pixdata.h
GdkPixbuf*	(*gdk_pixbuf_from_pixdata)	(const GdkPixdata	*pixdata,
					 gboolean		 copy_pixels,
					 GError		       **error);

	// gdk-pixbuf-core.h
GdkPixbuf *(*gdk_pixbuf_new_from_data) (const guchar *data,
				     GdkColorspace colorspace,
				     gboolean has_alpha,
				     int32_t bits_per_sample,
				     int32_t width, int32_t height,
				     int32_t rowstride,
				     GdkPixbufDestroyNotify destroy_fn,
				     gpointer destroy_fn_data);

	// gtkaccelgroup.h
GtkAccelGroup* (*gtk_accel_group_new)	      	  (void);

	// gtkalignment.h
GtkWidget* (*gtk_alignment_new)        (gfloat             xalign,
				     gfloat             yalign,
				     gfloat             xscale,
				     gfloat             yscale);

	// gtkbox.h
GType	   (*gtk_box_get_type)	       (void) G_GNUC_CONST;
void	   (*gtk_box_pack_start)	       (GtkBox	     *box,
					GtkWidget    *child,
					gboolean      expand,
					gboolean      fill,
					guint	      padding);

	// gtkbbox.h
GType (*gtk_button_box_get_type) (void) G_GNUC_CONST;
void              (*gtk_button_box_set_layout)          (GtkButtonBox      *widget,
						      GtkButtonBoxStyle  layout_style);

	// gtkbutton.h
GtkWidget*     (*gtk_button_new)               (void);

	// gtkcelllayout.h
GType (*gtk_cell_layout_get_type)           (void) G_GNUC_CONST;
void  (*gtk_cell_layout_pack_start)         (GtkCellLayout         *cell_layout,
                                          GtkCellRenderer       *cell,
                                          gboolean               expand);
void  (*gtk_cell_layout_set_attributes)     (GtkCellLayout         *cell_layout,
                                          GtkCellRenderer       *cell,
                                          ...) G_GNUC_NULL_TERMINATED;

	// gtkcellrenderertext.h
GtkCellRenderer *(*gtk_cell_renderer_text_new)      (void);

	// gtkcheckbutton.h
GtkWidget* (*gtk_check_button_new_with_mnemonic) (const gchar *label);

	// gtkcombobox.h
gint          (*gtk_combo_box_get_active)       (GtkComboBox     *combo_box);
gboolean      (*gtk_combo_box_get_active_iter)  (GtkComboBox     *combo_box,
                                              GtkTreeIter     *iter);
gchar        (*gtk_combo_box_get_active_text)  (GtkComboBox     *combo_box);
GtkTreeModel *(*gtk_combo_box_get_model)        (GtkComboBox     *combo_box);
GType         (*gtk_combo_box_get_type)         (void) G_GNUC_CONST;
GtkWidget    *(*gtk_combo_box_new_text)         (void);
GtkWidget    *(*gtk_combo_box_new_with_model)   (GtkTreeModel    *model);
void          (*gtk_combo_box_set_active)       (GtkComboBox     *combo_box,
                                              gint             index_);
void          (*gtk_combo_box_set_active_iter)  (GtkComboBox     *combo_box,
                                              GtkTreeIter     *iter);

	// gtkcontainer.h
void    (*gtk_container_add)		 (GtkContainer	   *container,
					  GtkWidget	   *widget);
void     (*gtk_container_foreach)      (GtkContainer       *container,
				     GtkCallback         callback,
				     gpointer            callback_data);
GType   (*gtk_container_get_type)		 (void) G_GNUC_CONST;
void    (*gtk_container_set_border_width)	 (GtkContainer	   *container,
					  guint		    border_width);

	// gtkdialog.h
GType      (*gtk_dialog_get_type) (void) G_GNUC_CONST;
gint (*gtk_dialog_run)                (GtkDialog *dialog);

	// gtkhbox.h
GtkWidget* (*gtk_hbox_new)	     (gboolean homogeneous,
			      gint spacing);

	// gtkhbbox.h
GtkWidget* (*gtk_hbutton_box_new)      (void);

	// gtkimage.h
GtkWidget* (*gtk_image_new_from_pixbuf)    (GdkPixbuf       *pixbuf);
GtkWidget* (*gtk_image_new_from_stock)     (const gchar     *stock_id,
                                         GtkIconSize      size);

	// gtkmain.h
gboolean (*gtk_init_check)           (int32_t    *argc,
                                   char ***argv);

	// gtklabel.h
GType                 (*gtk_label_get_type)          (void) G_GNUC_CONST;
GtkWidget*            (*gtk_label_new)               (const gchar   *str);
GtkWidget*            (*gtk_label_new_with_mnemonic) (const gchar   *str);
void     (*gtk_label_set_mnemonic_widget)            (GtkLabel         *label,
						   GtkWidget        *widget);

	// gtkliststore.h
void          (*gtk_list_store_append)           (GtkListStore *list_store,
					       GtkTreeIter  *iter);
void          (*gtk_list_store_clear)            (GtkListStore *list_store);
GType         (*gtk_list_store_get_type)         (void) G_GNUC_CONST;
GtkListStore *(*gtk_list_store_new)              (gint          n_columns,
					       ...);
void          (*gtk_list_store_set)              (GtkListStore *list_store,
					       GtkTreeIter  *iter,
					       ...);

	// gtkmain.h
void	   (*gtk_main)		   (void);
gboolean   (*gtk_main_iteration_do)   (gboolean blocking);
void	   (*gtk_main_quit)	   (void);

	// gtkmessagedialog.h
GtkWidget* (*gtk_message_dialog_new)      (GtkWindow      *parent,
                                        GtkDialogFlags  flags,
                                        GtkMessageType  type,
                                        GtkButtonsType  buttons,
                                        const gchar    *message_format,
                                        ...) G_GNUC_PRINTF (5, 6);

	// gtkmisc.h
GType   (*gtk_misc_get_type)      (void) G_GNUC_CONST;
void	(*gtk_misc_set_alignment) (GtkMisc *misc,
				gfloat	 xalign,
				gfloat	 yalign);

	// gtknotebook.h
GtkWidget* (*gtk_notebook_get_nth_page)     (GtkNotebook *notebook,
					  gint         page_num);
GType   (*gtk_notebook_get_type)       (void) G_GNUC_CONST;
GtkWidget * (*gtk_notebook_new)        (void);
void       (*gtk_notebook_set_current_page) (GtkNotebook *notebook,
					  gint         page_num);
void (*gtk_notebook_set_tab_label)           (GtkNotebook *notebook,
					   GtkWidget   *child,
					   GtkWidget   *tab_label);

	// gtkobject.h
GtkType	(*gtk_object_get_type)		(void) G_GNUC_CONST;

	// gtkscrolledwindow.h
GType          (*gtk_scrolled_window_get_type)          (void) G_GNUC_CONST;
GtkWidget*     (*gtk_scrolled_window_new)               (GtkAdjustment     *hadjustment,
						      GtkAdjustment     *vadjustment);
void           (*gtk_scrolled_window_set_policy)        (GtkScrolledWindow *scrolled_window,
						      GtkPolicyType      hscrollbar_policy,
						      GtkPolicyType      vscrollbar_policy);
void           (*gtk_scrolled_window_set_shadow_type)   (GtkScrolledWindow *scrolled_window,
                                                      GtkShadowType      type);
	// gtktable.h
GType	   (*gtk_table_get_type)	      (void) G_GNUC_CONST;
GtkWidget* (*gtk_table_new)	      (guint		rows,
				       guint		columns,
				       gboolean		homogeneous);
void	   (*gtk_table_attach)	      (GtkTable	       *table,
				       GtkWidget       *child,
				       guint		left_attach,
				       guint		right_attach,
				       guint		top_attach,
				       guint		bottom_attach,
				       GtkAttachOptions xoptions,
				       GtkAttachOptions yoptions,
				       guint		xpadding,
				       guint		ypadding);


	// gtktextbuffer.h
gboolean (*gtk_text_buffer_backspace)          (GtkTextBuffer *buffer,
					     GtkTextIter   *iter,
					     gboolean       interactive,
					     gboolean       default_editable);
GtkTextMark   *(*gtk_text_buffer_create_mark) (GtkTextBuffer     *buffer,
                                            const gchar       *mark_name,
                                            const GtkTextIter *where,
                                            gboolean           left_gravity);
void           (*gtk_text_buffer_delete_mark) (GtkTextBuffer     *buffer,
                                            GtkTextMark       *mark);
void (*gtk_text_buffer_get_end_iter)            (GtkTextBuffer *buffer,
                                              GtkTextIter   *iter);
void (*gtk_text_buffer_insert)            (GtkTextBuffer *buffer,
                                        GtkTextIter   *iter,
                                        const gchar   *text,
                                        gint           len);

	// gtktextiter.h
	// FIXME: should I put a #if !GTK_CHECK_VERSION(2,6,0)
	// around these three, or should I not care?
gboolean (*gtk_text_iter_backward_cursor_position)  (GtkTextIter *iter);
gboolean (*gtk_text_iter_equal)			    (const GtkTextIter *lhs,
						     const GtkTextIter *rhs);
gboolean (*gtk_text_buffer_delete_interactive)	    (GtkTextBuffer *buffer,
						     GtkTextIter   *start_iter,
						     GtkTextIter   *end_iter,
						     gboolean       default_editable);

	// gtktextview.h
GtkTextBuffer *(*gtk_text_view_get_buffer)            (GtkTextView   *text_view);
GType          (*gtk_text_view_get_type)              (void) G_GNUC_CONST;
GtkWidget *    (*gtk_text_view_new)                   (void);
void           (*gtk_text_view_scroll_to_mark)        (GtkTextView   *text_view,
                                                    GtkTextMark   *mark,
                                                    gdouble        within_margin,
                                                    gboolean       use_align,
                                                    gdouble        xalign,
                                                    gdouble        yalign);
void           (*gtk_text_view_set_cursor_visible)    (GtkTextView   *text_view,
                                                    gboolean       setting);
void             (*gtk_text_view_set_editable)           (GtkTextView      *text_view,
                                                       gboolean          setting);
void             (*gtk_text_view_set_left_margin)        (GtkTextView      *text_view,
                                                       gint              left_margin);
void             (*gtk_text_view_set_right_margin)       (GtkTextView      *text_view,
                                                       gint              right_margin);
void             (*gtk_text_view_set_wrap_mode)          (GtkTextView      *text_view,
                                                       GtkWrapMode       wrap_mode);

	// gtktogglebutton.h
gboolean   (*gtk_toggle_button_get_active)        (GtkToggleButton *toggle_button);
GType      (*gtk_toggle_button_get_type)          (void) G_GNUC_CONST;
void       (*gtk_toggle_button_set_active)        (GtkToggleButton *toggle_button,
                                                gboolean         is_active);

	// gtktreemodel.h
void              (*gtk_tree_model_get)             (GtkTreeModel *tree_model,
						  GtkTreeIter  *iter,
						  ...);
gboolean          (*gtk_tree_model_get_iter)        (GtkTreeModel *tree_model,
						  GtkTreeIter  *iter,
						  GtkTreePath  *path);
GtkTreePath *     (*gtk_tree_model_get_path)        (GtkTreeModel *tree_model,
						  GtkTreeIter  *iter);
GType             (*gtk_tree_model_get_type)        (void) G_GNUC_CONST;
gint        *(*gtk_tree_path_get_indices)      (GtkTreePath       *path);
GtkTreePath *(*gtk_tree_path_new_from_indices) (gint               first_index,
					     ...);
    // gtktreeselection.h
gboolean         (*gtk_tree_selection_get_selected)        (GtkTreeSelection            *selection,
							 GtkTreeModel               **model,
							 GtkTreeIter                 *iter);
void             (*gtk_tree_selection_select_iter)         (GtkTreeSelection            *selection,
							 GtkTreeIter                 *iter);
GType            (*gtk_tree_selection_get_type)            (void) G_GNUC_CONST;

void             (*gtk_tree_selection_set_mode)            (GtkTreeSelection            *selection,
							 GtkSelectionMode             type);

    // gtktreesortable.h
GType    (*gtk_tree_sortable_get_type)              (void) G_GNUC_CONST;

void     (*gtk_tree_sortable_set_sort_column_id)    (GtkTreeSortable        *sortable,
						  gint                    sort_column_id,
						  GtkSortType             order);
void     (*gtk_tree_sortable_set_sort_func)         (GtkTreeSortable        *sortable,
						  gint                    sort_column_id,
						  GtkTreeIterCompareFunc  sort_func,
						  gpointer                user_data,
						  GtkDestroyNotify        destroy);

    // gtktreeview.h
GType                  (*gtk_tree_view_get_type)                      (void) G_GNUC_CONST;
GtkWidget             *(*gtk_tree_view_new_with_model)                (GtkTreeModel              *model);

GtkTreeModel          *(*gtk_tree_view_get_model)                     (GtkTreeView               *tree_view);

GtkTreeSelection      *(*gtk_tree_view_get_selection)                 (GtkTreeView               *tree_view);
gint                   (*gtk_tree_view_append_column)                 (GtkTreeView               *tree_view,
								    GtkTreeViewColumn         *column);
void                       (*gtk_tree_view_set_enable_search)     (GtkTreeView                *tree_view,
								gboolean                    enable_search);
void                   (*gtk_tree_view_set_headers_visible)           (GtkTreeView               *tree_view,
								    gboolean                   headers_visible);

    // gtktreeviewcolumn.h
GtkTreeViewColumn      *(*gtk_tree_view_column_new_with_attributes) (const gchar             *title,
								  GtkCellRenderer         *cell,
								  ...) G_GNUC_NULL_TERMINATED;
void                    (*gtk_tree_view_column_set_expand)          (GtkTreeViewColumn       *tree_column,
								  gboolean                 expand);
void                    (*gtk_tree_view_column_set_min_width)       (GtkTreeViewColumn       *tree_column,
								  gint                     min_width);


	// gtkvbox.h
GtkWidget* (*gtk_vbox_new)	     (gboolean homogeneous,
			      gint spacing);

	// gtkwidget.h
void	   (*gtk_widget_add_accelerator)	  (GtkWidget           *widget,
					   const gchar         *accel_signal,
					   GtkAccelGroup       *accel_group,
					   guint                accel_key,
					   GdkModifierType      accel_mods,
					   GtkAccelFlags        accel_flags);
void	   (*gtk_widget_destroy)		  (GtkWidget	       *widget);
GtkWidget* (*gtk_widget_ref)		  (GtkWidget	       *widget);
void                  (*gtk_widget_set_sensitive)          (GtkWidget    *widget,
							 gboolean      sensitive);
void       (*gtk_widget_set_size_request)    (GtkWidget           *widget,
                                           gint                 width,
                                           gint                 height);
void	   (*gtk_widget_show_all)		  (GtkWidget	       *widget);
void	   (*gtk_widget_unref)		  (GtkWidget	       *widget);

	// gtkwindow.h
void       (*gtk_window_add_accel_group)          (GtkWindow           *window,
						GtkAccelGroup	    *accel_group);
GType      (*gtk_window_get_type)                 (void) G_GNUC_CONST;
GtkWidget* (*gtk_window_new)                      (GtkWindowType        type);
void       (*gtk_window_set_default_icon)             (GdkPixbuf  *icon);
void       (*gtk_window_set_position)             (GtkWindow           *window,
						GtkWindowPosition    position);
void       (*gtk_window_set_resizable)            (GtkWindow           *window,
                                                gboolean             resizable);
void       (*gtk_window_set_title)                (GtkWindow           *window,
						const gchar         *title);
void       (*gtk_window_set_type_hint)            (GtkWindow           *window, 
						GdkWindowTypeHint    hint);
void       (*gtk_window_set_default)              (GtkWindow           *window,
						GtkWidget           *default_widget);

    // gunicode.h
gint   (*g_utf8_collate)     (const gchar *str1,
                           const gchar *str2);

};
extern struct _dynamicgtksyms dynamicgtksyms;

int32_t dynamicgtk_init(void);
void dynamicgtk_uninit(void);

#ifndef __dynamicgtkfoo__

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

#endif	/* LINKED_GTK */

#endif	/* __dynamicgtk_h__ */

