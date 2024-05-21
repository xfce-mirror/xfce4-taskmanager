/*
 * Copyright (c) 2024 Jehan-Antoine Vayssade, <javayss@sleek-think.ovh>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef NETWORK_ANALYZER_H
#define NETWORK_ANALYZER_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>

#ifdef HAVE_LIBPCAP
#include <pcap.h>
#include <pthread.h>
#endif

typedef struct _XtmNetworkAnalyzer XtmNetworkAnalyzer;
struct _XtmNetworkAnalyzer
{
	guint8 mac[6];

#ifdef HAVE_LIBPCAP
	// interface mac adress
	pcap_if_t *iface;

	pthread_t thread;
	pthread_mutex_t lock;

	pcap_t *handle;
#endif

	// map port and number of packets
	GHashTable *packetin;
	GHashTable *packetout;

	// next interface
	XtmNetworkAnalyzer *next;
};

XtmNetworkAnalyzer *xtm_create_network_analyzer (void);
void increament_packet_count (char *mac, char *direction, GHashTable *hash_table, long int port);
void xtm_destroy_network_analyzer (XtmNetworkAnalyzer *analyzer);
XtmNetworkAnalyzer *xtm_network_analyzer_get_default (void);

#endif
