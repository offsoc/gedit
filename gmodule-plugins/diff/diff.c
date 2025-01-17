/* diff.c - diff plugin.
 *
 * Copyright (C) 1998 Chris Lahey.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
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
#include <stdlib.h>
#include <unistd.h>
#include <config.h>
#include <gnome.h>
#include "client.h"

static GtkWidget *entry1;
static GtkWidget *entry2;
static gint context;

void call_diff( GtkWidget *widget, gpointer data );

static void set_entry( GtkWidget *widget, gpointer data )
{
  GtkWidget *file_sel = gtk_widget_get_toplevel( widget );

  gtk_entry_set_text( GTK_ENTRY( data ), gtk_file_selection_get_filename( GTK_FILE_SELECTION( file_sel ) ) );
  gtk_widget_destroy( file_sel );
}

static void open_file_sel( GtkWidget *widget, gpointer data )
{
  GtkWidget *file_sel = gtk_file_selection_new( "Choose a file" );
  
  gtk_signal_connect( GTK_OBJECT( GTK_FILE_SELECTION( file_sel )->ok_button ), "clicked", GTK_SIGNAL_FUNC( set_entry ), data );
  gtk_signal_connect_object( GTK_OBJECT( GTK_FILE_SELECTION( file_sel )->cancel_button ),
			     "clicked",
			     GTK_SIGNAL_FUNC( gtk_widget_destroy ), GTK_OBJECT( file_sel ) );
  
  gtk_widget_show_all( file_sel );
}

static void done( GtkWidget *widget, gpointer data )
{
  client_finish(context);
  gtk_main_quit();
}

int main( int argc, char *argv[] )
{
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *dialog;
  client_info info = empty_info;

  info.menu_location = "[Plugins]Diff";

  context = client_init( &argc, &argv, &info );
  
  /* gtk_init( &argc, &argv ); */
  bindtextdomain(PACKAGE, GNOMELOCALEDIR);
  textdomain(PACKAGE);

  gnome_init("diff-plugin", VERSION, argc, argv);

  dialog = gtk_dialog_new();
  gtk_window_set_title( GTK_WINDOW( dialog ), "Choose files to diff" );
  gtk_signal_connect( GTK_OBJECT( dialog ), "destroy", GTK_SIGNAL_FUNC( done ), NULL );
  gtk_container_set_border_width( GTK_CONTAINER( GTK_DIALOG( dialog )->vbox ), 10 );

  label = gtk_label_new( "Choose files to diff:" );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->vbox ), label, FALSE, FALSE, 0 );

  hbox = gtk_hbox_new( FALSE, 0 );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->vbox ), hbox, FALSE, FALSE, 0 );
  
  entry1 = gtk_entry_new();
  gtk_box_pack_start( GTK_BOX( hbox ), entry1, TRUE, TRUE, 0 );

  button = gtk_button_new_with_label( "Browse..." );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked", GTK_SIGNAL_FUNC( open_file_sel ), entry1 );
  gtk_box_pack_start( GTK_BOX( hbox ), button, FALSE, FALSE, 0 );
  
  entry2 = gtk_entry_new();
  gtk_box_pack_start( GTK_BOX( hbox ), entry2, TRUE, TRUE, 0 );

  button = gtk_button_new_with_label( "Browse..." );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked", GTK_SIGNAL_FUNC( open_file_sel ), entry2 );
  gtk_box_pack_start( GTK_BOX( hbox ), button, FALSE, FALSE, 0 );

  button = gtk_button_new_with_label( "Calculate Diff" );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked", GTK_SIGNAL_FUNC( call_diff ), NULL );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->action_area ), button, FALSE, TRUE, 0 );

  button = gnome_stock_button (GNOME_STOCK_BUTTON_CANCEL);
  gtk_signal_connect( GTK_OBJECT( button ), "clicked", GTK_SIGNAL_FUNC( done ), NULL );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->action_area ), button, FALSE, TRUE, 0 );

  gtk_widget_show_all( dialog );

  gtk_main();

  exit( 0 );
}

void call_diff( GtkWidget *widget, gpointer data )
{
  char buff[1025];
  int fdpipe[2];
  int pid;
  char *filenames[2] = { NULL, NULL };
  int docid;
  int length;

  filenames[0] = gtk_entry_get_text( GTK_ENTRY( entry1 ) );
  filenames[1] = gtk_entry_get_text( GTK_ENTRY( entry2 ) );

  if( pipe( fdpipe ) == -1 )
    {
      _exit( 1 );
    }
  
  pid = fork();
  if ( pid == 0 )
    {
      /* New process. */
      char *argv[4];

      close( 1 );
      dup( fdpipe[1] );
      close( fdpipe[0] );
      close( fdpipe[1] );
      
      argv[0] = g_strdup( "diff" );
      argv[1] = filenames[0];
      argv[2] = filenames[1];
      argv[3] = NULL;
      execv( "/usr/bin/diff", argv );
      /* This is only reached if something goes wrong. */
      _exit( 1 );
    }
  close( fdpipe[1] );

  docid = client_document_new( context, "difference" );
  
  length = 1;
  while( length > 0 )
    {
      buff[ length = read( fdpipe[0], buff, 1024 ) ] = 0;
      if( length > 0 )
	{
	  client_text_append( docid, buff, length );
	}
    }
  client_document_show( docid );
  client_finish( context );
  gtk_main_quit();
}
