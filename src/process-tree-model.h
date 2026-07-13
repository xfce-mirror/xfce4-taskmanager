/*
 * Copyright (c) 2014 Peter de Ridder, <petert@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef PROCESS_TREE_MODEL_H
#define PROCESS_TREE_MODEL_H

#include <glib-object.h>
#include <gtk/gtk.h>

#define XTM_TYPE_PROCESS_TREE_MODEL (xtm_process_tree_model_get_type ())
G_DECLARE_FINAL_TYPE (XtmProcessTreeModel, xtm_process_tree_model, XTM, PROCESS_TREE_MODEL, GObject)

GtkTreeModel *xtm_process_tree_model_new (GtkTreeModel *model);

#endif /* !PROCESS_TREE_MODEL_H */
