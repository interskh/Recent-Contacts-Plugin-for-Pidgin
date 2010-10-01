/*
 recent contact - plugin to have a list of ur recent contacts
 Copyright (C) 2010  Kyle Sun <interskh@gmail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc.

*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef PURPLE_PLUGINS
# define PURPLE_PLUGINS
#endif

#define recent_contacts_ID   "core-recent_contacts"
#define recent_contacts_NAME	"Recent Contact"
#define recent_contacts_VERSION	"0.01"

#include <glib.h>
#include <assert.h>
#include <string.h>

#ifndef G_GNUC_NULL_TERMINATED
# if __GNUC__ >= 4
#  define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
# else
#  define G_GNUC_NULL_TERMINATED
# endif
#endif

#include <notify.h>
#include <plugin.h>
#include <version.h>
#include <debug.h>
#include <status.h>
#include <savedstatuses.h>
#include <prefs.h>
#include <network.h>

/* global preference */
static const char * const PREF_NONE = "/plugins/core/recent_contacts";

/********************
 * helper functions *
 ********************/
/* Trace a debugging message. Writes to log file as well as purple
 * debug sink.
 */
void
trace(const char *str, ...)
{
	va_list ap;
	va_start(ap, str);
	char *buf = g_strdup_vprintf(str, ap);
	va_end(ap);

  FILE *log = fopen("/tmp/pidgin_recent_contact.log", "a");
  assert(log);
  time_t t;
  time(&t);
  fprintf(log, "%s: %s\n", ctime(&t), buf);
  fclose(log);

	purple_debug_info(recent_contacts_ID, "%s\n", buf);
	g_free(buf);
}


/* we're adding this here and assigning it in plugin_load because we need
 * a valid plugin handle for our call to purple_notify_message() in the
 * plugin_action_test_cb() callback function */
PurplePlugin *recent_contacts_plugin = NULL;

static gboolean
plugin_load (PurplePlugin * plugin)
{

  trace("plugin loading");

  recent_contacts_plugin = plugin; /* assign this here so we have a
                                      valid handle later */
	return TRUE;
}

/* For specific notes on the meanings of each of these members, consult the C Plugin Howto
 * on the website. */
static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,
	NULL,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,

  recent_contacts_ID,
	recent_contacts_NAME,
	recent_contacts_VERSION, 

	"Recent Contacts Plugin",
	"Have a list of your recent Contacts",
	"Kyle Sun <interskh@gmail.com>", /* correct author */
	"http://code.google.com/p/pidgin-recent_contacts/",


	plugin_load,
	NULL,
	NULL,

	NULL,
	NULL,
	NULL,
  NULL,

	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin (PurplePlugin * plugin)
{
}

PURPLE_INIT_PLUGIN (hello_world, init_plugin, info)
