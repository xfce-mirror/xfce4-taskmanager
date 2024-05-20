/*
 * Copyright (c) 2024 Jehan-Antoine Vayssade, <javayss@sleek-think.ovh>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef INMODE_TO_SOCK_H
#define INMODE_TO_SOCK_H

#include <glib.h>

typedef struct _XtmInodeToSock XtmInodeToSock;
struct _XtmInodeToSock
{
	// inode to port
	GHashTable *hash;
	// inode to pid (freebsd)
	GHashTable *pid;
};

XtmInodeToSock *xtm_create_inode_to_sock (void);
void addtoconninode(XtmInodeToSock *its, char *buffer);
void xtm_refresh_inode_to_sock (XtmInodeToSock *);
void xtm_destroy_inode_to_sock (XtmInodeToSock *);
XtmInodeToSock *xtm_inode_to_sock_get_default (void);

#endif
