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
#define recent_contacts_VERSION	"0.9"

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
#include <prefs.h>
#include <network.h>
#include <conversation.h>

/* global preference */
static const char * PREF_NONE = "/plugins/core/recent_contacts";
static const char * PREF_SIZE = "/plugins/core/recent_contacts/size";
static const char * GROUP_NAME = "Recent Contacts";
static const char * NODE_GROUP_KEY = "buddy_orig_group";
static const char * NODE_OFFLINE_KEY = "show_offline";
static const char * NODE_ORIG_OFFLINE_KEY = "show_offline_orig";
static const int DEFAULT_SIZE = 10;

int g_size;

void rc_push_contact(PurpleAccount *, const char *);
void rc_pop_contacts(PurpleGroup *);

PurplePluginPrefFrame* get_plugin_pref_frame(PurplePlugin*);
static PurplePluginUiInfo plugin_prefs = {
  get_plugin_pref_frame,
  0,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

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

  purple_debug_info(recent_contacts_ID, "%s\n", buf);
  g_free(buf);
}

void rc_push_contact(PurpleAccount *acct,
    const char * buddyname)
{
  PurpleGroup * grp = purple_find_group(GROUP_NAME);
  if (!grp) {
    trace("Group %s Not Found. Create one.", GROUP_NAME);
    grp = purple_group_new(GROUP_NAME);
  }

  PurpleBuddy * buddy;
  // if we can find it in 'Recent Contacts', skip
  if ((buddy = purple_find_buddy_in_group(acct, buddyname, grp)) != NULL) {
    trace("Buddy %s is already in %s", buddyname, GROUP_NAME);
    purple_blist_add_buddy(buddy, NULL, grp, NULL);
    return;
  }

  buddy = purple_find_buddy(acct, buddyname);
  if (!buddy) {
    trace("Buddy %s Not Found. You SUCK!", buddyname);
    return;
  }

  PurpleBlistNode * node = PURPLE_BLIST_NODE(buddy); 

  // back up group info
  PurpleGroup * orig_grp = 	purple_buddy_get_group(buddy);
  purple_blist_node_set_string(node, NODE_GROUP_KEY, orig_grp->name);

  // back up offline info
  gboolean offline = purple_blist_node_get_bool(node, NODE_OFFLINE_KEY);
  purple_blist_node_set_bool(node, NODE_ORIG_OFFLINE_KEY, offline);
  purple_blist_node_set_bool(node, NODE_OFFLINE_KEY, TRUE);

  // Add to Recent Contacts Group
  trace(">>>>>>> Add %s", buddyname);
  purple_blist_add_buddy(buddy, NULL, grp, NULL);

  // Clean up old group if needed
  rc_pop_contacts(grp);
}

void rc_pop_contacts(PurpleGroup * grp)
{
  if (!grp) return;

  PurpleBlistNode * gnode = PURPLE_BLIST_NODE(grp);
  PurpleBlistNode * n = NULL;
  PurpleBuddy * b = NULL;
  int total;
  gboolean offline;

  //XXX group->totalsize is unreliable!!!

  for (n=gnode->child, total=0; n!=NULL; total++, n=n->next);
  trace("Total Group Count %d", total);

  while (total > g_size) {

    n = gnode->child;

    if (PURPLE_BLIST_NODE_IS_CONTACT(n)) {
      trace("Child Contact : %s", (PURPLE_CONTACT(n)->priority->name));
      b = PURPLE_CONTACT(n)->priority;
    } else if (PURPLE_BLIST_NODE_IS_BUDDY(n)) {
      trace("Child Buddy : %s", (PURPLE_BUDDY(n)->name));
      b = PURPLE_BUDDY(n);
    }

    n = PURPLE_BLIST_NODE(b);
    const char *name = purple_blist_node_get_string(n, NODE_GROUP_KEY);
    if (!name) {  // if cannot find orig group name, put back to Buddies
      trace("ERROR!!! cannot find original group name"); 
      name = "Buddies"; 
    }
    PurpleGroup * g = purple_find_group(name);
    if (!g) {
      trace("Group %s Not Found. Create one.", name);
      g = purple_group_new(name);
    }
    trace("<<<<<<< Remove %s", b->name);

    offline = purple_blist_node_get_bool(n, NODE_ORIG_OFFLINE_KEY);
    purple_blist_node_set_bool(n, NODE_OFFLINE_KEY, offline);
    purple_blist_add_buddy(b, NULL, g, NULL);

    total--;
  }
  
}

static void
pref_size_on_change(const char *name, PurplePrefType type, gconstpointer val,
			  gpointer user_data)
{
  g_size = purple_prefs_get_int(PREF_SIZE);
  PurpleGroup * grp = purple_find_group(GROUP_NAME);
  if (!grp) return;
  rc_pop_contacts(grp);
}

/*******************
 * Signal Handlers *
 *******************/
static void rc_at_conversation_event(PurpleConversation *conv)
{
  PurpleAccount *acct = purple_conversation_get_account(conv);
  const char * proto = purple_account_get_protocol_name(acct);
  const char * proto_id = purple_account_get_protocol_id(acct);
  const char * my_acct = purple_account_get_username(acct);
  const char * recv_acct = purple_conversation_get_name(conv);

  trace("Conversation event.. [%s: %s] account: %s to %s", proto_id,
      proto, my_acct, recv_acct); 

  rc_push_contact(acct, recv_acct);

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
  g_size = purple_prefs_get_int(PREF_SIZE);
  void *handle = purple_prefs_get_handle();
  purple_prefs_connect_callback(handle, PREF_SIZE, pref_size_on_change, NULL);
  void *conv_handle = purple_conversations_get_handle();
  purple_signal_connect(conv_handle, "conversation-created", plugin,
  		PURPLE_CALLBACK(rc_at_conversation_event), NULL);
  purple_signal_connect(conv_handle, "deleting-conversation", plugin, 
  		PURPLE_CALLBACK(rc_at_conversation_event), NULL);
  return TRUE;
}

PurplePluginPrefFrame *get_plugin_pref_frame(PurplePlugin *plugin) {
  PurplePluginPrefFrame *frame;
  PurplePluginPref *pref;

  frame = purple_plugin_pref_frame_new();

  pref = purple_plugin_pref_new_with_name_and_label(PREF_SIZE, "Size of Recent Contacts Group:");
  purple_plugin_pref_set_bounds(pref, 0, 100);
  purple_plugin_pref_frame_add(frame, pref);

  return frame;
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
  "http://github.com/interskh/Recent-Contacts-Plugin-for-Pidgin",


  plugin_load,
  NULL,
  NULL,

  NULL,
  NULL,
	&plugin_prefs,
  NULL,

  NULL,
  NULL,
  NULL,
  NULL
};

static void
init_plugin (PurplePlugin * plugin)
{
	purple_prefs_add_none(PREF_NONE);
	purple_prefs_add_int(PREF_SIZE, DEFAULT_SIZE);
}

PURPLE_INIT_PLUGIN(recent_contacts, init_plugin, info)
