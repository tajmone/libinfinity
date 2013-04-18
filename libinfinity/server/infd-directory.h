/* libinfinity - a GObject-based infinote implementation
 * Copyright (C) 2007-2010 Armin Burgmeier <armin@arbur.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef __INFD_DIRECTORY_H__
#define __INFD_DIRECTORY_H__

#include <libinfinity/server/infd-storage.h>
#include <libinfinity/server/infd-note-plugin.h>
#include <libinfinity/server/infd-session-proxy.h>
#include <libinfinity/common/inf-browser.h>
#include <libinfinity/communication/inf-communication-manager.h>

#include <glib-object.h>

G_BEGIN_DECLS

#define INFD_TYPE_DIRECTORY                 (infd_directory_get_type())
#define INFD_DIRECTORY(obj)                 (G_TYPE_CHECK_INSTANCE_CAST((obj), INFD_TYPE_DIRECTORY, InfdDirectory))
#define INFD_DIRECTORY_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST((klass), INFD_TYPE_DIRECTORY, InfdDirectoryClass))
#define INFD_IS_DIRECTORY(obj)              (G_TYPE_CHECK_INSTANCE_TYPE((obj), INFD_TYPE_DIRECTORY))
#define INFD_IS_DIRECTORY_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE((klass), INFD_TYPE_DIRECTORY))
#define INFD_DIRECTORY_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS((obj), INFD_TYPE_DIRECTORY, InfdDirectoryClass))

typedef struct _InfdDirectory InfdDirectory;
typedef struct _InfdDirectoryClass InfdDirectoryClass;

struct _InfdDirectoryClass {
  GObjectClass parent_class;

#if 0
  /* Signals */
  void (*node_added)(InfdDirectory* directory,
                     InfBrowserIter* iter);

  void (*node_removed)(InfdDirectory* directory,
                       InfBrowserIter* iter);

  void (*add_session)(InfdDirectory* directory,
                      InfBrowserIter* iter,
                      InfdSessionProxy* session);

  void (*remove_session)(InfdDirectory* directory,
                         InfBrowserIter* iter,
                         InfdSessionProxy* session);
#endif
};

struct _InfdDirectory {
  GObject parent;
};

typedef void(*InfdDirectoryForeachConnectionFunc)(InfXmlConnection*,
                                                  gpointer);

GType
infd_directory_get_type(void) G_GNUC_CONST;

InfdDirectory*
infd_directory_new(InfIo* io,
                   InfdStorage* storage,
                   InfCommunicationManager* comm_manager);

InfIo*
infd_directory_get_io(InfdDirectory* directory);

InfdStorage*
infd_directory_get_storage(InfdDirectory* directory);

InfCommunicationManager*
infd_directory_get_communication_manager(InfdDirectory* directory);

gboolean
infd_directory_add_plugin(InfdDirectory* directory,
                          const InfdNotePlugin* plugin);

const InfdNotePlugin*
infd_directory_lookup_plugin(InfdDirectory* directory,
                             const gchar* note_type);

gboolean
infd_directory_add_connection(InfdDirectory* directory,
                              InfXmlConnection* connection);

void
infd_directory_foreach_connection(InfdDirectory* directory,
                                  InfdDirectoryForeachConnectionFunc func,
                                  gpointer user_data);

#if 0
const gchar*
infd_directory_iter_get_name(InfdDirectory* directory,
                             InfBrowserIter* iter);

gchar*
infd_directory_iter_get_path(InfdDirectory* directory,
                             InfBrowserIter* iter);

void
infd_directory_iter_get_root(InfdDirectory* directory,
                             InfBrowserIter* iter);

gboolean
infd_directory_iter_get_next(InfdDirectory* directory,
                             InfBrowserIter* iter);

gboolean
infd_directory_iter_get_prev(InfdDirectory* directory,
                             InfBrowserIter* iter);

gboolean
infd_directory_iter_get_parent(InfdDirectory* directory,
                               InfBrowserIter* iter);

gboolean
infd_directory_iter_get_child(InfdDirectory* directory,
                              InfBrowserIter* iter,
                              GError** error);

gboolean
infd_directory_iter_get_explored(InfdDirectory* directory,
                                 InfBrowserIter* iter);

gboolean
infd_directory_add_subdirectory(InfdDirectory* directory,
                                InfBrowserIter* parent,
                                const gchar* name,
                                InfBrowserIter* iter,
                                GError** error);

gboolean
infd_directory_add_note(InfdDirectory* directory,
                        InfBrowserIter* parent,
                        const gchar* name,
                        const InfdNotePlugin* plugin,
                        InfBrowserIter* iter,
                        GError** error);

gboolean
infd_directory_remove_node(InfdDirectory* directory,
                           InfBrowserIter* iter,
                           GError** error);

InfdStorageNodeType
infd_directory_iter_get_node_type(InfdDirectory* directory,
                                  InfBrowserIter* iter);

const InfdNotePlugin*
infd_directory_iter_get_plugin(InfdDirectory* directory,
                               InfBrowserIter* iter);

InfdSessionProxy*
infd_directory_iter_get_session(InfdDirectory* directory,
                                InfBrowserIter* iter,
                                GError** error);

InfdSessionProxy*
infd_directory_iter_peek_session(InfdDirectory* directory,
                                 InfBrowserIter* iter);
#endif

gboolean
infd_directory_iter_save_session(InfdDirectory* directory,
                                 InfBrowserIter* iter,
                                 GError** error);

void
infd_directory_enable_chat(InfdDirectory* directory,
                           gboolean enable);

InfdSessionProxy*
infd_directory_get_chat_session(InfdDirectory* directory);

G_END_DECLS

#endif /* __INFD_DIRECTORY_H__ */

/* vim:set et sw=2 ts=2: */
