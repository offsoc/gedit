/* 	- New Document interface  
 *
 * gEdit
 * Copyright (C) 1999 Alex Roberts and Evan Lawrence
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <gnome.h>
#include <config.h>
#include "gE_view.h"
#include "main.h"
#include "gE_undo.h"
#include "gE_mdi.h"
#include "commands.h"
#include "gE_prefs.h"
#include "gE_window.h"
#include "gE_print.h"

#define GE_DATA		1

GnomeUIInfo popup_menu [] = {

	GNOMEUIINFO_MENU_CUT_ITEM(edit_cut_cb, (gpointer) GE_DATA),

        GNOMEUIINFO_MENU_COPY_ITEM(edit_copy_cb, (gpointer) GE_DATA),

	GNOMEUIINFO_MENU_PASTE_ITEM(edit_paste_cb, (gpointer) GE_DATA),

	GNOMEUIINFO_MENU_SELECT_ALL_ITEM(edit_selall_cb, (gpointer) GE_DATA),


	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_ITEM_STOCK (N_("Save"),NULL,file_save_cb,GNOME_STOCK_MENU_SAVE),
	GNOMEUIINFO_ITEM_STOCK (N_("Close"), NULL, file_close_cb, GNOME_STOCK_MENU_CLOSE),
	GNOMEUIINFO_ITEM_STOCK (N_("Print"), NULL, file_print_cb, GNOME_STOCK_MENU_PRINT),
	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_ITEM_STOCK (N_("Open (swap) .c/.h file"),NULL,doc_swaphc_cb,GNOME_STOCK_MENU_REFRESH),
	
	GNOMEUIINFO_END

};

enum {
	
	CURSOR_MOVED_SIGNAL,
	LAST_SIGNAL
	
};

void line_pos_cb(GtkWidget *w, gE_data *data);
void doc_insert_text_cb(GtkWidget *editable, const guchar *insertion_text, int length, int *pos, gE_view *view);
static gint gE_view_signals[LAST_SIGNAL] = { 0 };

GtkVBoxClass *parent_class = NULL;



/* Private Internal stuff */

/* Signal Releated stuffs */


/* handles changes in the text widget... */
void
view_changed_cb(GtkWidget *w, gpointer cbdata)
{

	gchar MOD_label[255];
	gchar *str;

	gE_view *view = (gE_view *) cbdata;

	if (view->document->changed)
	  return;
	
	view->document->changed = TRUE;
	gtk_signal_disconnect (GTK_OBJECT(view->text), (gint) view->changed_id);
	view->changed_id = FALSE;
	
	str = g_malloc (strlen (GNOME_MDI_CHILD(view->document)->name) + 2);
	
	sprintf(str, "*%s", GNOME_MDI_CHILD(view->document)->name);
	/*gtk_label_set(GTK_LABEL(doc->tab_label), MOD_label);*/
	gnome_mdi_child_set_name (GNOME_MDI_CHILD (view->document), str);
	
	g_free (str);
	
	str = g_malloc0 (strlen (GEDIT_ID) +
				 strlen (GNOME_MDI_CHILD (gE_document_current())->name) + 4);
	sprintf (str, "%s - %s", GNOME_MDI_CHILD (gE_document_current())->name, GEDIT_ID);
	gtk_window_set_title(GTK_WINDOW(mdi->active_window), str);
	g_free(str);

}

/* 
 * Text insertion and deletion callbacks - used for Undo/Redo (not yet implemented) and split screening
 */

void gE_view_list_insert (gE_view *view, gE_data *data)
{

	gE_view *nth_view = NULL;
	gint n, p1, p2;
	gchar *buffer = (guchar *)data->temp2;
	gint position = (gint)data->temp1;
	gint length = strlen (buffer);
	gE_document *doc;
	
	
	if (view != GE_VIEW(mdi->active_view)) {

	  gtk_text_freeze (GTK_TEXT (view->text));
		
	  p1 = gtk_text_get_point (GTK_TEXT (view->text));
		
	  gtk_text_set_point (GTK_TEXT(view->text), position);

	  gtk_text_insert (GTK_TEXT (view->text), NULL,
					 &view->text->style->black,
					 NULL, buffer, length);
	  gtk_text_set_point (GTK_TEXT (view->text), p1);
		
	  gtk_text_thaw (GTK_TEXT (view->text));
		
		
	  gtk_text_freeze (GTK_TEXT (view->split_screen));
	  p1 = gtk_text_get_point (GTK_TEXT (view->split_screen));
	  gtk_text_set_point (GTK_TEXT(view->split_screen), (position));

	  gtk_text_insert (GTK_TEXT (view->split_screen), NULL,
					 &view->split_screen->style->black,
					 NULL, buffer, length);
	  gtk_text_set_point (GTK_TEXT (view->text), p1);
	  gtk_text_thaw (GTK_TEXT (view->split_screen));

	}	

}

void view_list_erase (gE_view *view, gE_data *data)
{


}

gint insert_into_buffer (gE_document *doc, gchar *buffer, gint position)
{

	  		if ((doc->buf->len > 0) && (position < doc->buf->len) && (position)) {
	
#ifdef DEBUG
	  g_message ("g_string_insert");
#endif
	  doc->buf = g_string_insert (doc->buf, position, buffer);
	
	} else if (position == 0) {

#ifdef DEBUG		  
	  g_message ("g_string_prepend");
#endif
	  doc->buf = g_string_prepend (doc->buf, buffer);
	  
	} else {
	  
#ifdef DEBUG
	  g_message ("g_string_append");
#endif
	  doc->buf = g_string_append (doc->buf, buffer);
	
	}
	
	return 0;		

}

void
doc_insert_text_cb(GtkWidget *editable, const guchar *insertion_text, int length,
	int *pos, gE_view *view)
{

	GtkWidget *significant_other;
	guchar *buffer;
	gchar buf[96];
	gint position = *pos;
	gint n, p1, p2;
	gE_document *doc;
	GnomeMDIChild *child;
	/*GnomeMDIChild *temp = NULL;*/
	gE_data *data;
	gE_view *nth_view = NULL;
	GtkWidget *text;

	if (!view->split_screen)
	  return;
	
	if (view->flag == editable) {
	
	  view->flag = NULL;
	  
	  return;
	
	}
	
	
	if (editable == view->text)
	  significant_other = view->split_screen;
	else if (editable == view->split_screen)
	  significant_other = view->text;
	else
	  return;
	

	view->flag = significant_other;	
	/*buffer = g_strdup (insertion_text);
	
	if (length <= 96)
	  buffer = buf;
	else
	  buffer = g_new (gchar, length);*/
	
	buffer = g_new0 (guchar, length + 1);
	  
	strncpy (buffer, insertion_text, length);

	doc = view->document;
/*
#ifdef DEBUG
 	g_message ("and the buffer is: %s, %d, %d.. buf->len %d", buffer, length, position, view->document->buf->len);
#endif

	insert_into_buffer (doc, buffer, position);
*/
	gE_undo_add (buffer, position, (position + length), INSERT, doc);

	data = g_malloc0 (sizeof (gE_data));
	
	data->temp1 = (gint*) position;
	data->temp2 = (guchar*) buffer;
	
	g_list_foreach (doc->views, (GFunc) gE_view_list_insert, data);
 
	gtk_text_freeze (GTK_TEXT (significant_other));
	gtk_editable_insert_text (GTK_EDITABLE (significant_other), buffer, length,
						  &position);
	gtk_text_thaw (GTK_TEXT (significant_other));

	gtk_text_set_point (GTK_TEXT (significant_other), position);
	
	g_free (data);


	/*if (length > 96)  */
	  g_free (buffer);

}

void
doc_delete_text_cb(GtkWidget *editable, int start_pos, int end_pos,
	gE_view *view)
{

	GtkWidget *significant_other;
	gE_document *doc;
	gE_view *nth_view = NULL;
	gchar *buffer;
	gint n;

	if (!view->split_screen)
	  return;

	if (view->flag == editable) {

	  view->flag = NULL;

	  return;
	
	}
	
	if (editable == view->text)
	  significant_other = view->split_screen;
	else if (editable == view->split_screen)
	  significant_other = view->text;
	else
	  return;
	
	doc = view->document;
/*
#ifdef DEBUG	
	g_message ("start: %d end: %d len: %d", start_pos, end_pos, doc->buf->len);
#endif	
	if ((start_pos + end_pos) < doc->buf->len)
	if (end_pos + (end_pos - start_pos) <= doc->buf->len) {
	
#ifdef DEBUG
	  g_message ("g_string_erase");
#endif
	  doc->buf = g_string_erase (doc->buf, start_pos, (end_pos - start_pos));
	  
	} else {
	  
#ifdef DEBUG
	  g_message ("g_string_truncate");
#endif
	  doc->buf = g_string_truncate (doc->buf, start_pos);
	
	}
*/
	buffer = gtk_editable_get_chars (GTK_EDITABLE(editable), start_pos, end_pos);
	gE_undo_add (buffer, start_pos, end_pos, DELETE, doc);

	view->flag = significant_other;
	gtk_text_freeze (GTK_TEXT (significant_other));
	gtk_editable_delete_text (GTK_EDITABLE (significant_other), start_pos, end_pos);
	gtk_text_thaw (GTK_TEXT (significant_other));

	
	for (n = 0; n < g_list_length (doc->views); n++) {

	  nth_view = g_list_nth_data (doc->views, n);
	  
	  if (nth_view != GE_VIEW(mdi->active_view)) {

	    /* Disconnect the signals so we can safely update the other views */
	    gtk_signal_disconnect (GTK_OBJECT (nth_view->text), 
							(gint)nth_view->delete);
	    gtk_signal_disconnect (GTK_OBJECT (nth_view->split_screen),
						 	(gint)nth_view->s_delete);
		
	    gtk_text_freeze (GTK_TEXT (nth_view->text));
	    gtk_editable_delete_text (GTK_EDITABLE (nth_view->text), start_pos, end_pos);
	    gtk_text_thaw (GTK_TEXT (nth_view->text));
		
	    gtk_text_freeze (GTK_TEXT (nth_view->split_screen));
	    gtk_editable_delete_text (GTK_EDITABLE (nth_view->split_screen),
							start_pos, end_pos);
	    gtk_text_thaw (GTK_TEXT (nth_view->split_screen));

	    /* Reconnect the signals, now the views have been updated */
	    nth_view->delete = gtk_signal_connect (GTK_OBJECT (nth_view->text),
										 	"delete_text",
										 	GTK_SIGNAL_FUNC(doc_delete_text_cb),
										 	view);
							                   
	    nth_view->s_delete = gtk_signal_connect (GTK_OBJECT (nth_view->split_screen),
											"delete_text",
											GTK_SIGNAL_FUNC(doc_delete_text_cb),
											view);	
	  }
	  
	}

}


/* ---- Auto-indent Callback(s) --- */
/*
void
auto_indent_toggle_cb(GtkWidget *w, gpointer cbdata)
{

	gE_data *data = (gE_data *)cbdata;

	gE_window_set_auto_indent (!settings->auto_indent);

}
*/

gint
auto_indent_cb(GtkWidget *text, char *insertion_text, int length,
	int *pos, gpointer cbdata)
{

	int i, newlines, newline_1 = 0;
	gchar *buffer, *whitespace;
	gE_view *view = (gE_view *)cbdata;
	gE_document *doc;
	gE_data *data;	
	
	
	if ((length != 1) || (insertion_text[0] != '\n'))
		return FALSE;
	if (gtk_text_get_length (GTK_TEXT (text)) <=1)
		return FALSE;
	if (!settings->auto_indent)
		return FALSE;

	doc = view->document;

	newlines = 0;
	for (i = *pos; i > 0; i--) {

		buffer = gtk_editable_get_chars (GTK_EDITABLE (text), i-1, i);
		
		if (buffer == NULL)
			continue;
		if (buffer[0] == 10) {

			if (newlines > 0) {
			
				g_free (buffer);
				buffer = NULL;
				break;
				
			} else {
				
				newlines++;
				newline_1 = i;
				g_free (buffer);
				buffer = NULL;
				
			}
			
		}
		
		g_free (buffer);
		buffer = NULL;
	}

	whitespace = g_malloc0 (newline_1 - i + 2);

	for (i = i; i <= newline_1; i++) {

		buffer = gtk_editable_get_chars (GTK_EDITABLE (text), i, i+1);
		
		if ((buffer[0] != 32) & (buffer[0] != 9)) {
		
			g_free (buffer);
			buffer = NULL;
			break;
		
		}
		
		strncat (whitespace, buffer, 1);
		g_free (buffer);
		
	}

	if (strlen(whitespace) > 0) {

		i = *pos;
		gtk_text_set_point (GTK_TEXT (text), i);
		gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL,
			whitespace, strlen (whitespace));
	
/*		insert_into_buffer (doc, whitespace, i);
*/
		

	}
	
	g_free (whitespace);

	return TRUE;

}


void
line_pos_cb(GtkWidget *w, gE_data *data)
{

	static char col [32];
	GtkWidget *label;
	GnomeApp *app;
	
	app = gnome_mdi_get_active_window  (mdi);
	
	sprintf (col, "Column: %d",
	 GTK_TEXT(GE_VIEW(mdi->active_view)->text)->cursor_pos_x/6);
	
	gnome_appbar_set_status (GNOME_APPBAR(app->statusbar), col);
	
}


gint gE_event_button_press (GtkWidget *w, GdkEventButton *event)
{

	gE_data *data;
	data = g_malloc0 (sizeof (gE_data));

	line_pos_cb(NULL, data);

#ifdef DEBUG
	g_print ("you pressed a button\n");
#endif

	return FALSE;

}

gint gE_event_key_press (GtkWidget *w, GdkEventKey *event)
{

	gint mask;
	gE_data *data = g_malloc0 (sizeof (gE_data));

	line_pos_cb (NULL, NULL);
	
	mask = GDK_CONTROL_MASK | GDK_MOD1_MASK | GDK_MOD2_MASK |
		GDK_MOD3_MASK | GDK_MOD4_MASK | GDK_MOD5_MASK;
	
	/* Control key related */
	if (event->state == GDK_CONTROL_MASK) {
	  
	  switch (event->keyval) {
	    
	    case 's':
	    		file_save_cb (w, NULL);
	    		break;
	    
	    case 'p':
	    		file_print_cb (w, (gpointer)data);
	    		break;

	    case 'w':
	    		file_close_cb (w, NULL);
	    		break;
	    
	    case 'z':
	    		gE_undo_do (w, NULL);
	    		break;
	    
	    case 'r':
	    		gE_undo_redo (w, NULL);
	    		break;
	    
	    default:
	    		return TRUE;
	    		break;
	  
	  }
	
	}
	
	
	return TRUE;

}

/* The Widget Stuff */

static void gE_view_realize (GtkWidget *w)
{

	if (GTK_WIDGET_CLASS (parent_class)->realize)
				(* GTK_WIDGET_CLASS(parent_class)->realize)(w);  	

}

static void gE_view_finalize (GtkObject *o)
{

		if (GTK_OBJECT_CLASS(parent_class)->finalize)
			(* GTK_OBJECT_CLASS(parent_class)->finalize)(o);

}

static gint gE_view_expose (GtkWidget *w, GdkEventExpose *event)
{

	if (GTK_WIDGET_CLASS (parent_class)->expose_event)
		(* GTK_WIDGET_CLASS (parent_class)->expose_event)(w,event);
	
	return TRUE;
}

static void gE_view_size_allocate (GtkWidget *w, GtkAllocation *alloc)
{

	gE_view *view = GE_VIEW (w);
	GtkAllocation my_alloc;
	
	w->allocation = *alloc;
	
	if (GTK_WIDGET_REALIZED (w))
	  gdk_window_move_resize (w->window, alloc->x, alloc->y,
	  						alloc->width, alloc->height);
	

	my_alloc.x = 1;
	my_alloc.y = 1;
	my_alloc.height = alloc->height - 2;
	my_alloc.width = alloc->width - 2;
	gtk_widget_size_allocate (view->vbox, &my_alloc);
	
}

static void gE_view_size_request (GtkWidget *w, GtkRequisition *req)
{

	gE_view *view = GE_VIEW (w);
	GtkRequisition sb_req;
	
	GTK_WIDGET_CLASS(parent_class)->size_request(w, req);

}

static void gE_view_class_init (gE_view_class *klass)
{

	GtkObjectClass *object_class;
	/*GtkWidgetClass *widget_class;
	GtkFixedClass *fixed_class;*/
	
	object_class = (GtkObjectClass *)klass;
	/*widget_class = (GtkWidgetClass *)klass;*/
	
	gE_view_signals[CURSOR_MOVED_SIGNAL] = gtk_signal_new ("cursor_moved",
														GTK_RUN_FIRST,
														object_class->type,
														GTK_SIGNAL_OFFSET (gE_view_class, cursor_moved),
														gtk_signal_default_marshaller,
														GTK_TYPE_NONE,
														0);
	
	gtk_object_class_add_signals (object_class, gE_view_signals, LAST_SIGNAL);
	
	klass->cursor_moved = NULL;
	
	/*widget_class->size_allocate = gE_view_size_allocate;
	widget_class->size_request = gE_view_size_request;
	widget_class->expose_event = gE_view_expose;
	widget_class->realize = gE_view_realize;
	object_class->finalize = gE_view_finalize;
	
	parent_class = gtk_type_class (gtk_vbox_get_type ());*/
}

static void gE_view_init (gE_view *view)
{

	GtkWidget *vpaned, *menu;
	GtkStyle *style;
	
	view->vbox = gtk_vbox_new (TRUE, TRUE);
	gtk_container_add (GTK_CONTAINER (view), view->vbox);
	gtk_widget_show (view->vbox);
	
	/* Create the upper split screen */
	view->scrwindow[0] = gtk_scrolled_window_new (NULL, NULL);
	
	gtk_box_pack_start (GTK_BOX (view->vbox), view->scrwindow[0], TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (view->scrwindow[0]),
				      GTK_POLICY_NEVER,
				      GTK_POLICY_AUTOMATIC);
      	gtk_widget_show (view->scrwindow[0]);

	view->line_wrap = 1;

	view->text = gtk_text_new(NULL, NULL);
	gtk_text_set_editable(GTK_TEXT(view->text), !view->read_only);
	gtk_text_set_word_wrap(GTK_TEXT(view->text), settings->word_wrap);
	gtk_text_set_line_wrap(GTK_TEXT(view->text), view->line_wrap);

	
	/* - Signals - */

	gtk_signal_connect_after(GTK_OBJECT(view->text), "button_press_event",
		GTK_SIGNAL_FUNC(gE_event_button_press), NULL);

	gtk_signal_connect_after (GTK_OBJECT (view->text), "key_press_event",
					GTK_SIGNAL_FUNC (gE_event_key_press), 0);


	/* Handle Auto Indent */
	view->indent = gtk_signal_connect_after (GTK_OBJECT(view->text), "insert_text",
										 GTK_SIGNAL_FUNC(auto_indent_cb), view);

	/*	
	I'm not even sure why these are here.. i'm sure there are much easier ways
	of implementing undo/redo... 
	*/
	view->insert = gtk_signal_connect (GTK_OBJECT (view->text), "insert_text",
		                           GTK_SIGNAL_FUNC(doc_insert_text_cb), view);
	view->delete = gtk_signal_connect (GTK_OBJECT (view->text), "delete_text",
		                           GTK_SIGNAL_FUNC(doc_delete_text_cb),
		                           (gpointer) view);



/*	gtk_signal_connect_after (GTK_OBJECT(view->text), "key_press_event",
		GTK_SIGNAL_FUNC(gE_event_button_press), NULL);*/


	gtk_container_add (GTK_CONTAINER (view->scrwindow[0]), view->text);

	style = gtk_style_new();
	gtk_widget_set_style(GTK_WIDGET(view->text), style);

	gtk_widget_set_rc_style(GTK_WIDGET(view->text));
	gtk_widget_ensure_style(GTK_WIDGET(view->text));
	g_free (style);
		
/*	doc->changed = FALSE; */
	view->changed_id = gtk_signal_connect(GTK_OBJECT(view->text), "changed",
		GTK_SIGNAL_FUNC(view_changed_cb), view);


	gtk_widget_show(view->text);
	gtk_text_set_point(GTK_TEXT(view->text), 0);

	
	/* Create the bottom split screen */
	view->scrwindow[1] = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (view->vbox), view->scrwindow[1], TRUE, TRUE, 1);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (view->scrwindow[1]),
				      GTK_POLICY_NEVER,
				      GTK_POLICY_AUTOMATIC);
	/*gtk_fixed_put (GTK_FIXED(view), view->scrwindow[1], 0, 0);*/
      	gtk_widget_show (view->scrwindow[1]);

	view->split_screen = gtk_text_new(NULL, NULL);
	gtk_text_set_editable(GTK_TEXT(view->split_screen), !view->read_only);
	gtk_text_set_word_wrap(GTK_TEXT(view->split_screen), settings->word_wrap);
	gtk_text_set_line_wrap(GTK_TEXT(view->split_screen), view->line_wrap);

	
	/* - Signals - */

	gtk_signal_connect_after(GTK_OBJECT(view->split_screen), "button_press_event",
		GTK_SIGNAL_FUNC(gE_event_button_press), NULL);

	gtk_signal_connect_after (GTK_OBJECT (view->split_screen), "key_press_event",
					GTK_SIGNAL_FUNC (gE_event_key_press), NULL);

	
	view->s_insert = gtk_signal_connect (GTK_OBJECT (view->split_screen), "insert_text",
		                             GTK_SIGNAL_FUNC(doc_insert_text_cb),
		                             (gpointer) view);
		                             
	view->s_delete = gtk_signal_connect (GTK_OBJECT (view->split_screen), "delete_text",
				                     GTK_SIGNAL_FUNC(doc_delete_text_cb),
				                     (gpointer) view);

	view->s_indent = gtk_signal_connect_after (GTK_OBJECT(view->split_screen), "insert_text",
	                                    GTK_SIGNAL_FUNC(auto_indent_cb),
	                                    (gpointer) view);
				
	gtk_container_add (GTK_CONTAINER (view->scrwindow[1]), view->split_screen);

	view->split_parent = GTK_WIDGET (view->split_screen)->parent;

	style = gtk_style_new();
  	gdk_font_unref (style->font);

	if (use_fontset) {
	
		style->font = view->font ? gdk_fontset_load (view->font) : NULL;
		
		if (style->font == NULL) {
		
			style->font = gdk_fontset_load (DEFAULT_FONTSET);
			view->font = DEFAULT_FONTSET;
		
		} 

	} else {
		
		style->font = view->font ? gdk_font_load (view->font) : NULL;
		
		if (style->font == NULL) {
		
			style->font = gdk_font_load (DEFAULT_FONT);
			view->font = DEFAULT_FONT;
		
		} 

	}
  	 
 	gtk_widget_push_style (style);     
	gtk_widget_set_style(GTK_WIDGET(view->split_screen), style);
   	gtk_widget_set_style(GTK_WIDGET(view->text), style);
   	gtk_widget_pop_style ();
   	

	gtk_widget_show(view->split_screen);
	gtk_text_set_point(GTK_TEXT(view->split_screen), 0);

	/* Popup Menu */
	menu = gnome_popup_menu_new (popup_menu);
	gnome_popup_menu_attach (menu, view->text, view);
	gnome_popup_menu_attach (menu, view->split_screen, view);
		
	
        gnome_config_push_prefix ("/gEdit/Global/");
	view->splitscreen = gnome_config_get_int("splitscreen");
	gnome_config_pop_prefix ();
	gnome_config_sync ();
	
	if (!view->splitscreen)
	  gtk_widget_hide (GTK_WIDGET (view->split_screen)->parent);

	/*gtk_fixed_put (GTK_FIXED (view), view->vbox, 0, 0);
	gtk_widget_show (view->vbox);*/

	gtk_widget_grab_focus(view->text);
	
}

guint gE_view_get_type ()
{

	static guint gE_view_type = 0;
	
	if (!gE_view_type) {

	  GtkTypeInfo gE_view_info = {
	  		"gE_view",
	  		sizeof (gE_view),
	  		sizeof (gE_view_class),
	  		(GtkClassInitFunc) gE_view_class_init,
	  		(GtkObjectInitFunc) gE_view_init,
	  		(GtkArgSetFunc) NULL,
	  		(GtkArgGetFunc) NULL,
	  };
	    
	  gE_view_type = gtk_type_unique (gtk_vbox_get_type (), &gE_view_info);
	  
	}
	 
	return gE_view_type;

}

GtkWidget *gE_view_new (gE_document *doc)
{

	gE_view *view = gtk_type_new (gE_view_get_type ());
	
	view->document = doc;
	
	if (view->document->buf) {
	
#ifdef DEBUG
	  	g_print ("gE_view_init: inserting buffer..\n");
#endif	  	
	  	gtk_text_freeze (GTK_TEXT (view->text));
	  	gtk_text_insert (GTK_TEXT (view->text), NULL,
						 &view->text->style->black,
						 NULL, view->document->buf->str,
						 view->document->buf->len);

	  	gtk_text_insert (GTK_TEXT (view->split_screen), NULL,
						 &view->split_screen->style->black,
						 NULL, view->document->buf->str,
						 view->document->buf->len);
		gtk_text_thaw (GTK_TEXT (view->text));
		
		gE_view_set_position (view, 0);
	
	}
	
	view->document->views = g_list_append (doc->views, view);
	
	return GTK_WIDGET (view);

}



/* Public Functions */

void gE_view_set_group_type (gE_view *view, guint type)
{

	view->group_type = type;
	
	gtk_widget_queue_resize (GTK_WIDGET (view));

}

void gE_view_set_split_screen (gE_view *view, gint split_screen)
{

	if (!view->split_parent)
		return;

	if (split_screen) {

	   	gtk_widget_show (view->split_parent);
	
	} else {
	
		gtk_widget_hide (view->split_parent);
	
	}
 
   	view->splitscreen = split_screen;

}

void gE_view_set_word_wrap (gE_view *view, gint word_wrap)
{

	view->word_wrap = word_wrap;

	gtk_text_set_word_wrap (GTK_TEXT (view->text), word_wrap);
	gtk_text_set_word_wrap (GTK_TEXT (view->split_screen), word_wrap);

}

void gE_view_set_line_wrap (gE_view *view, gint line_wrap)
{

	view->line_wrap = line_wrap;

	gtk_text_set_line_wrap (GTK_TEXT (view->text), view->line_wrap);

}

void gE_view_set_read_only (gE_view *view, gint read_only)
{
	gchar RO_label[255];
	gchar *fname;

	view->read_only = read_only;
	gtk_text_set_editable (GTK_TEXT (view->text), !view->read_only);
	
	if (read_only) {

	  sprintf(RO_label, "RO - %s", GNOME_MDI_CHILD(view->document)->name);
	  gnome_mdi_child_set_name (GNOME_MDI_CHILD (view->document), RO_label);

	} else {
	 
	 if (view->document->filename)
	   gnome_mdi_child_set_name (GNOME_MDI_CHILD(view->document),
	   					     g_basename(view->document->filename));
	 else
	   gnome_mdi_child_set_name (GNOME_MDI_CHILD(view->document), _(UNTITLED));

	}
	 
	if (view->split_screen)
		gtk_text_set_editable (GTK_TEXT (view->split_screen), !view->read_only);

}

void gE_view_set_font (gE_view *view, gchar *font)
{

	GtkStyle *style;
	
	style = gtk_style_new();
  	gdk_font_unref (style->font);
  	
  	if (use_fontset)
	  style->font = gdk_fontset_load (font);
  	else
	  style->font = gdk_font_load (font);
  
  	gtk_widget_push_style (style);    

  	gtk_widget_set_style(GTK_WIDGET((view)->split_screen), style);

  	gtk_widget_set_style(GTK_WIDGET((view)->text), style);


	gtk_widget_pop_style ();

}

void gE_view_set_position (gE_view *view, gint pos)
{

	gtk_text_set_point (GTK_TEXT (view->text), pos);

	gtk_text_set_point (GTK_TEXT (view->split_screen), pos);

}

guint gE_view_get_position (gE_view *view)
{

	return gtk_text_get_point (GTK_TEXT (view->text));

}

guint gE_view_get_length (gE_view *view)
{

	return gtk_text_get_length (GTK_TEXT (view->text));

}

void gE_view_set_selection (gE_view *view, gint start, gint end)
{

	gtk_editable_select_region (GTK_EDITABLE (view->text), start, end);
	
	gtk_editable_select_region (GTK_EDITABLE (view->text), start, end);

}

/* Sync the itnernal document buffer with what is visible in the text box */
void gE_view_buffer_sync (gE_view *view) 
{
	gchar *buf;
	gE_document *doc = view->document;

	buf = g_strdup (gtk_editable_get_chars (GTK_EDITABLE (view->text),
	   									  0, gE_view_get_length (view)));
	
	doc->buf = g_string_new (buf);
	
	g_free (buf);				   									  
	
}

void gE_view_refresh (gE_view *view)
{


	gint i = gE_view_get_length (view);

	if (i > 0) {
	
	  gE_view_set_position (view, i);
	  gtk_text_backward_delete (GTK_TEXT(view->text), i);
	  gtk_text_backward_delete (GTK_TEXT(view->split_screen), i);
		
	}

	if (view->document->buf) {
	
#ifdef DEBUG
	  	g_print ("gE_view_refresh: inserting buffer..\n");
#endif	  	
	  	gtk_text_freeze (GTK_TEXT (view->text));
	  	gtk_text_insert (GTK_TEXT (view->text), NULL,
						 &view->text->style->black,
						 NULL, view->document->buf->str,
						 view->document->buf->len);

	  	gtk_text_insert (GTK_TEXT (view->split_screen), NULL,
						 &view->split_screen->style->black,
						 NULL, view->document->buf->str,
						 view->document->buf->len);
		gtk_text_thaw (GTK_TEXT (view->text));
		
		gE_view_set_position (view, 0);
	
	}
}
