/* vi:set ts=4 sts=0 sw=4:
 *
 * gEdit About Box
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
#include <assert.h>
#include <gtk/gtk.h>
#include <config.h>
#include <gnome.h>
#include "main.h"

void gE_about_box(GtkWidget *w, gpointer cbdata)
{

	GtkWidget *about;
	
	const gchar *authors[] = {
	
		"Alex Roberts",
		"Evan Lawrence",
		"http://gedit.pn.org",
		"",
		N_("With special thanks to:"),
		"     Will LaShell, Chris Lahey, Andy Kahn,",
		"     Miguel de Icaza, Martin Baulig,",
		"     Thomas Holmgren, Martijn van Beers",
		NULL
		
	};

			

	about = gnome_about_new ("gEdit", VERSION,
			_("(C) 1998, 1999 Alex Roberts and Evan Lawrence"),
			authors,
			_("gEdit is a small and lightweight text editor for GNOME/Gtk+"),
			"gedit-logo.png");
			
	gtk_widget_show (about);
	
	/*gtk_widget_destroy (about);
	g_free (authors);	*/
}
