/*
 * Copyright (c) 2024 Jehan-Antoine Vayssade, <javayss@sleek-think.ovh>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "inode-to-sock.h"
#include "stdio.h"

XtmInodeToSock *
xtm_create_inode_to_sock (void)
{
	XtmInodeToSock *its = g_new (XtmInodeToSock, 1);
	its->hash = g_hash_table_new_full (g_int_hash, g_int_equal, g_free, NULL);
	// freebsd only
	its->pid = g_hash_table_new_full (g_int_hash, g_int_equal, g_free, NULL);
	return its;
}

void
xtm_destroy_inode_to_sock (XtmInodeToSock *its)
{
	if (its == NULL)
		return;

	g_hash_table_destroy (its->hash);
	g_hash_table_destroy (its->pid);

	free (its);
}

XtmInodeToSock *
xtm_inode_to_sock_get_default (void)
{
	static XtmInodeToSock *its = NULL;
	if (its == NULL)
		its = xtm_create_inode_to_sock ();
	return its;
}
