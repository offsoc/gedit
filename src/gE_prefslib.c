/* gEdit
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


#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gtk/gtk.h>
#include <fcntl.h>
#include <unistd.h>
#include <gnome.h>

#include "gE_prefslib.h"
GList *gE_prefs = NULL;

FILE *gE_prefs_open_temp (char **filename)
{
	FILE *fp;
	char *fn;
	
	if ((fn = gE_prefs_get_char ("Tempfile")) == NULL)
		if ((fn = getenv ("GETEMPFILE")) == NULL)
			fn = g_strdup ("/tmp/gE_temp");

	if ((fp = fopen (fn, "w")) == NULL)
	{
		printf ("gE_prefs_open_temp: Unable to open temporary file %s for writing.\n", fn);
		return NULL;
	}
	
	*filename = fn;
	return fp;
}

/* 
   char *gE_prefs_open_file (char *filename)
   Prepends the path of the rc file directory to the filename specified by
   char *filename, creates the rc file directory if need be, and returns a pointer
   to the full pathname.
*/
 
char *gE_prefs_open_file (char *filename, char *rw)
{
	FILE *f;
	char *homedir, *gedit_dir, *gedit_dir_old, *fn;
	struct stat gedit_stats;
	mode_t mode = 484;

	/*homedir = gedit_dir = gedit_dir_old = fn = NULL;*/
	
	if ((gedit_dir = getenv ("GEPREFSDIR")) == NULL)
	{
		if ((homedir = getenv ("HOME")) == NULL)
		{
			printf ("gE_prefs_open_file: Couldn't get home directory path.\n");
			return NULL;
		}
		gedit_dir = g_malloc0 (strlen (homedir) + strlen ("/.gedit") + 1);
		sprintf (gedit_dir, "%s/.gedit", homedir);
	}

	if (stat (gedit_dir, &gedit_stats) == -1)
	{
		printf ("Creating preferences directory...\n");
		if (mkdir (gedit_dir, mode) == -1)
		{
			printf ("gE_prefs_open_file: Couldn't create preferences directory.\n");
			return NULL;
		}
		stat (gedit_dir, &gedit_stats);
	}

	if (!S_ISDIR (gedit_stats.st_mode))
	{
		gedit_dir_old = g_malloc0 (strlen (gedit_dir) + strlen (".OLD"));
		sprintf (gedit_dir_old, "%s.OLD", gedit_dir);
		printf ("Moving outdated preferences file away...\n");
		if (rename (gedit_dir, gedit_dir_old) == -1)
		{
			printf ("gE_prefs_open_file: Couldn't rename outdated preferences file, please move %s out of the way.\n", gedit_dir);
			g_free (gedit_dir);
			g_free (gedit_dir_old);
			return NULL;
		}
		g_free (gedit_dir_old);
	}

	/* We need to do this twice in case .gedit wasn't a directory */

	if (stat (gedit_dir, &gedit_stats) == -1)
	{
		printf ("Creating preferences directory...\n");
		if (mkdir (gedit_dir, mode) == -1)
		{
			printf ("gE_prefs_open_file: Couldn't create preferences directory.\n");
			return NULL;
		}
	}

	fn = g_malloc0 (strlen (gedit_dir) + strlen (filename) + 2);
	sprintf (fn, "%s/%s", gedit_dir, filename);
	g_free (gedit_dir);

	if (stat (fn, &gedit_stats) == -1)
	{
		printf ("Creating new rc file \"%s\"...\n", filename);
		if ((f = fopen (fn, "w")) == NULL)
		{
			printf ("gE_prefs_open_file: Couldn't create rc file.\n");
			return NULL;
		}
		else
			fclose (f);
	}
	/* If we reach this point, the directory and rc file either exist
	   or have just been created */


	return fn;
}



int gE_prefs_open ()
{
	gE_pref *new = NULL;
	FILE *fp;
	char line[256];
	gchar *ptr, *ptr2, *filename;

	if ((filename = gE_prefs_open_file ("geditrc", "r")) == NULL)
	{
		g_print ("gE_prefs_open: There was an error openning the prefs file.\n");
		return -1;
	}

	if ((fp = fopen (filename, "r")) == NULL)
	{
		printf ("gE_prefs_open: There was an error openning the prefs file.\n");
		g_free (filename);
		return -1;
	}
	g_free (filename);

	while (fgets(line, 256, fp))
	{
		new = g_malloc0(sizeof(gE_pref));
		ptr = ptr2 = g_strdup (line);

		while (*ptr2 != '\0' && *ptr2 != '=' && *ptr2 != '\n')
			ptr2++;

		if (*ptr2 == '\0')
		{
			fclose(fp);
			printf("gE_prefs_open: Error in config file.\n");
			return -1;
		}
		else
		{
			*ptr2 = '\0';
			ptr2++;

			new->value = g_malloc0 (strlen(ptr2));
			new->name = g_strdup (ptr);
			
			strncpy(new->value, ptr2, strlen (ptr2) - 1);
		}
         	
		gE_prefs = g_list_append (gE_prefs, new);
	}

	fclose(fp);
	return 0;
}

void gE_prefs_close ()
{
	FILE *temp_fp;
	int i;
	gE_pref *pref;
 	char *temp_fn, *prefs_fn, *command;
 	
	if ((temp_fp = gE_prefs_open_temp (&temp_fn)) == NULL)
	{
		printf ("gE_prefs_close: Couldn't open temporary file for writing.\n");
		return;
	}

	for (i = 0; i < g_list_length (gE_prefs); i++)
	{
		pref = g_list_nth_data (gE_prefs, i);
		fprintf(temp_fp, "%s=%s\n", pref->name, pref->value);
	}

	for (i = g_list_length (gE_prefs) - 1; i >= 0; i--)
	{
		pref = g_list_nth_data (gE_prefs, i);
		gE_prefs = g_list_remove (gE_prefs, pref);
		g_free(pref);
	}

	fclose(temp_fp);
	
	if ((prefs_fn = gE_prefs_open_file ("geditrc", "w")) == NULL)
	{
		printf ("gE_prefs_close: Couldn't open prefs file for writing.\n");
		return;
	}
	
	if (unlink (prefs_fn) == -1)
	{
		printf ("gE_prefs_close: Couldn't delete old temporary file.\n");
		g_free (temp_fn);
		g_free (prefs_fn);
		return;
	}
	/*if (rename (temp_fn, prefs_fn) == -1)
	{*/
		command = g_malloc0 (strlen (temp_fn) + strlen (prefs_fn) + 6);
		sprintf (command, "cp %s %s", temp_fn, prefs_fn);
		if (system (command) == -1)
		{
			printf ("gE_prefs_close: Couldn't move temporary file %s to %s.\n", temp_fn, prefs_fn);
			g_free (temp_fn);
			g_free (prefs_fn);
			g_free (command);
			return;
		}
		g_free (command);
		unlink (temp_fn);
	/*}*/
	/*g_free (temp_fn);*/
	/*g_free (prefs_fn);*/

}


char *gE_prefs_get_data (char *name)
{

	char *value;
	gnome_config_push_prefix("/gEdit/Global/");
	value = gnome_config_get_string (name);
	gnome_config_pop_prefix ();
	gnome_config_sync ();
	if (value == NULL)
		value = gE_prefs_get_default (name);
	return value;

}

void gE_prefs_set_data (char *name, char *value)
{

	gnome_config_push_prefix ("/gEdit/Global/");
	gnome_config_set_string (name, value);
	gnome_config_pop_prefix ();
	gnome_config_sync ();
	
}


char *gE_prefs_get_default (char *name)
{
	int i;
	char *value;

	static gE_pref gE_prefs_defaults [] = {
		{ "tab pos", "2" },
		{ "auto indent", "0" },
		{ "toolbar", "1" },
		{ "tb pix", "1" },
		{ "tb text", "1" },
		{ "show statusbar", "1" },
		{ "tb relief", "0" },
		{ "splitscreen", "0" },
		{ "mdi mode", "0" },
		{ NULL, NULL }
	};

	i = 0;
	while (gE_prefs_defaults[i].name != NULL)
	{
		if (strcmp (name, gE_prefs_defaults[i].name) == 0)
		{
			value = g_strdup (gE_prefs_defaults[i].value);
			return value;
		}
		i++;
	}
	return NULL;
}


char *gE_prefs_get_char(char *name) 
{
	return gE_prefs_get_data (name);
}

void gE_prefs_set_char(char *name, char *value) 
{
	gE_prefs_set_data (name, value);
}

int gE_prefs_get_int(char *name)
{

	int i;
	char *value;
	gnome_config_push_prefix ("/gEdit/Global/");
	i = gnome_config_get_int (name);
	gnome_config_pop_prefix ();
	gnome_config_sync ();
/*	if (i == NULL)
	  {
		value = gE_prefs_get_data (name);
		i = atoi (value);
		g_free(value);
	  }
*/
	return i;
	
}

void gE_prefs_set_int(char *name, int value) 
{

	gnome_config_push_prefix ("/gEdit/Global/");
	gnome_config_set_int (name, value);
	gnome_config_pop_prefix ();
	gnome_config_sync ();

}

