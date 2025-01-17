/* vi:set ts=8 sts=0 sw=8:
 *
 * gEdit
 * Copyright (C) 1998 Alex Roberts and Evan Lawrence
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

#include <stdio.h>
#include <string.h>
#include <config.h>
#include <gnome.h>
#include <gtk/gtk.h>
#include <glib.h>

#include "main.h"
#include "gE_window.h"
#include "gE_view.h"
#include "gE_files.h"
#include "gE_prefs_box.h"
#include "gE_plugin_api.h"
#include "commands.h"
#include "gE_mdi.h"
#include "gE_print.h"
#include "menus.h"
/*#include "toolbar.h"*/
#include "gE_prefs.h"
#include "search.h"
#include "gE_icon.xpm"

extern GList *plugins;
gE_window *window;
extern GtkWidget  *col_label;

GtkWidget *search_result_window;
GtkWidget *search_result_clist;

/* Prototype for setting the window icon */
void gE_window_set_icon(GtkWidget *window, char *icon);


/*
 *  Create the find in all files search result window
 *  but dont show it. packer is the widget to which it attaches too.
 *
 *  I know the code is a little ugly but its the idea that Im trying to get 
 *  acrross. Cleaning up the code would be nice.
 */
static GtkWidget*
create_find_in_files_result_window ()
{
	GtkWidget *wnd, *btn, *top, *frame, *image, *label, *hsep;
	gchar *titles[] = {"Filename", "Line", "Contents"};
	int i;
	
	search_result_clist = gtk_clist_new_with_titles(3, titles);
	gtk_signal_connect(GTK_OBJECT(search_result_clist), 
					"select_row",
					GTK_SIGNAL_FUNC(search_result_clist_cb),
					NULL);
	for (i = 0; i < 3; i++) 
	  gtk_clist_set_column_auto_resize ((GtkCList *)search_result_clist, i, TRUE);

	wnd = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy ((GtkScrolledWindow *)wnd,
								GTK_POLICY_NEVER,
								GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport ( GTK_SCROLLED_WINDOW(wnd),
										search_result_clist); 
	gtk_widget_show(search_result_clist);
	gtk_widget_show(wnd);

	btn = gtk_button_new();

	/*image = gnome_pixmap_new_from_file_at_size ("../xpm/tb_cancel.xpm", 15, 15);*/
	image = gnome_stock_new_with_icon (GNOME_STOCK_PIXMAP_CLOSE);
	
	gtk_container_add (GTK_CONTAINER (btn), image);

	gtk_button_set_relief ( GTK_BUTTON(btn), GTK_RELIEF_NONE);
	
	gtk_signal_connect (GTK_OBJECT(btn), "clicked",
					GTK_SIGNAL_FUNC(remove_search_result_cb),
					NULL);

	gtk_widget_show (image);
	gtk_widget_show (btn);
	
	frame = gtk_vbox_new (FALSE, 0);
	top = gtk_hbox_new (FALSE, 0);	
	
	label = gtk_label_new ( " Search Results " );

	gtk_label_set_justify ( GTK_LABEL(label), GTK_JUSTIFY_LEFT );
	hsep = gtk_hseparator_new();

	gtk_box_pack_start (GTK_BOX (top), label, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (top), hsep, TRUE, TRUE, 0);		 	
	gtk_box_pack_start (GTK_BOX (top), btn, FALSE, TRUE, 0);	 	
	
	gtk_box_pack_start (GTK_BOX (frame), top, FALSE, FALSE, 0);	 	
	gtk_box_pack_start (GTK_BOX (frame), wnd, TRUE, TRUE, 0);	 		

	gtk_widget_show (hsep);
	gtk_widget_show (label);
	gtk_widget_show (top);

	return frame;
}


/*gE_window */
void gE_window_new(GnomeMDI *mdi, GnomeApp *app)
{

	GtkWidget *statusbar;
	gint *ptr; /* For Plugin Stuff. */
	
	static GtkTargetEntry drag_types[] =
	{
		{ "text/uri-list", 0, 0 },
	};

	static gint n_drag_types = sizeof (drag_types) / sizeof (drag_types [0]);

	gtk_drag_dest_set (GTK_WIDGET(app),
		GTK_DEST_DEFAULT_MOTION |
		GTK_DEST_DEFAULT_HIGHLIGHT |
		GTK_DEST_DEFAULT_DROP,
		drag_types, n_drag_types,
		GDK_ACTION_COPY);
		
	gtk_signal_connect (GTK_OBJECT (app),
		"drag_data_received",
		GTK_SIGNAL_FUNC (filenames_dropped), NULL);

	ptr = g_new (int, 1);
	*ptr = ++last_assigned_integer;
	g_hash_table_insert (win_int_to_pointer, ptr, app);
        g_hash_table_insert (win_pointer_to_int, app, ptr);

	gE_window_set_icon(GTK_WIDGET(app), "gE_icon");

	gtk_window_set_default_size (GTK_WINDOW(app), settings->width, settings->height);
	gtk_window_set_policy (GTK_WINDOW (app), TRUE, TRUE, FALSE);

	/*gE_get_settings ();*/
	
	/* find in files result window  dont show it.*/
	search_result_window = create_find_in_files_result_window();

	gtk_box_pack_start (GTK_BOX (app->vbox), search_result_window, TRUE, TRUE, 0); 

	/* gtk_widget_hide(search_result_window); */


	g_list_foreach(plugins, (GFunc) add_plugins_to_window, app);


	settings->num_recent = 0;
	recent_update(GNOME_APP(app));

	/* statusbar */
	statusbar = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_USER);
	gnome_app_set_statusbar (GNOME_APP(app),GTK_WIDGET (statusbar));
		
	gnome_app_install_menu_hints(app, gnome_mdi_get_menubar_info(app));
	
} /* gE_window_new */

void gE_window_set_auto_indent (gint auto_indent)
{

	settings->auto_indent = auto_indent;

}

/* set the a window icon */
void gE_window_set_icon(GtkWidget *window, char *icon)
{

	GdkPixmap *pixmap;
        GdkBitmap *mask;

	gtk_widget_realize (window);
	
	pixmap = gdk_pixmap_create_from_xpm_d (window->window, &mask,
                                		&window->style->bg[GTK_STATE_NORMAL],
                                		(char **)gE_icon);
	
	gdk_window_set_icon (window->window, NULL, pixmap, mask);
	
	/* Not sure about this.. need to test in E 
	gtk_widget_unrealize (window);*/

}


void gE_window_set_status_bar (gint show_status)
{

	settings->show_status = show_status;

	if (show_status)
	  gtk_widget_show (GTK_WIDGET (GNOME_APP(mdi->active_window)->statusbar));
	else
	  gtk_widget_hide (GTK_WIDGET (GNOME_APP(mdi->active_window)->statusbar));

}

void
child_switch (GnomeMDI *mdi, gE_document *doc)
{

	gchar *title;

	if (gE_document_current()) {
	
	  gtk_widget_grab_focus(GE_VIEW(mdi->active_view)->text);
	  title = g_malloc0 (strlen (GEDIT_ID) +
					 strlen (GNOME_MDI_CHILD (gE_document_current())->name) + 4);
	  sprintf (title, "%s - %s", GNOME_MDI_CHILD (gE_document_current())->name,
			GEDIT_ID);
	  
	  gtk_window_set_title(GTK_WINDOW(mdi->active_window), title);
	  g_free(title);
	
	}

}

/*	umm.. FIXME?
static gint
gE_destroy_window (GtkWidget *widget, GdkEvent *event, gE_data *data)
{
	window_close_cb(widget, data);
	return TRUE;
}
*/

/*
 * PRIVATE: doc_swaphc_cb
 *
 * if .c file is open open .h file 
 *
 * TODO: if a .h file is open, do we swap to a .c or a .cpp?  we should put a
 * check in there.  if both exist, then probably open both files.
 */
void
doc_swaphc_cb(GtkWidget *wgt, gpointer cbdata)
{

	size_t len;
	char *newfname;
	gE_document *doc;
	
	doc = gE_document_current();
	if (!doc || !doc->filename)
		return;

	newfname = NULL;
	len = strlen(doc->filename);
	
	while (len) {
		if (doc->filename[len] == '.')
			break;
		len--;
	}

	len++;
	if (doc->filename[len] == 'h') {

		newfname = g_strdup(doc->filename);
		newfname[len] = 'c';

	} else if (doc->filename[len] == 'H') {

		newfname = g_strdup(doc->filename);
		newfname[len] = 'C';

	} else if (doc->filename[len] == 'c') {

		newfname = g_strdup(doc->filename);
		newfname[len] = 'h';

		if (len < strlen(doc->filename) && strcmp(doc->filename + len, "cpp") == 0)
			newfname[len+1] = '\0';
	
	} else if (doc->filename[len] == 'C') {

		newfname = g_strdup(doc->filename);

		if (len < strlen(doc->filename) && strcmp(doc->filename + len, "CPP") == 0) {

			newfname[len] = 'H';
			newfname[len+1] = '\0';

		} else

			newfname[len] = 'H';

	}

	if (!newfname)
		return;

	/* hmm maybe whe should check if the file exist before we try
	 * to open.  this will be fixed later.... */
	doc = gE_document_new_with_file (newfname);
	gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
	gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));

} /* doc_swaphc_cb */

/* the end */
