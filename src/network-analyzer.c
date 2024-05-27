/*
 * Copyright (c) 2024 Jehan-Antoine Vayssade, <javayss@sleek-think.ovh>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "network-analyzer.h"
#include "task-manager.h"

void *network_analyzer_thread (void *ptr);

void
increament_packet_count (char *mac, char *direction, GHashTable *hash_table, long int port)
{
	int *key = g_new0 (gint, 1);
	*key = port;
	gpointer value = g_hash_table_lookup (hash_table, key);
	g_hash_table_replace (hash_table, key, (gpointer)(((long int)value) + 1));
	// printf ("%s -> %s %ld: %ld\n", mac, direction, port, ((long int)value) + 1);
}

void *
network_analyzer_thread (void *ptr)
{
#ifdef HAVE_LIBPCAP
	XtmNetworkAnalyzer *analyzer = (XtmNetworkAnalyzer *)ptr;
	pcap_loop (analyzer->handle, -1, packet_callback, (void *)analyzer);
#endif
	return NULL;
}

XtmNetworkAnalyzer *
xtm_create_network_analyzer (void)
{
	XtmNetworkAnalyzer *analyzer = NULL;

#ifdef HAVE_LIBPCAP
	XtmNetworkAnalyzer *current = NULL;
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_if_t *it = NULL;

	if (pcap_findalldevs (&it, errbuf) != 0)
	{
		fprintf (stderr, "Error finding device: %s\n", errbuf);
		return NULL;
	}


	while (it)
	{
		guint8 mac[6];
		char *device = it->name;

		//! todo check pcap lib ?
		if (get_mac_address (device, mac) == 0)
		{
			printf (
				"%s, MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
				device, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		}
		else
		{
			fprintf (stderr, "No mac adresse for %s\n", device);
			it = it->next;
			continue;
		}

		// Open a pcap session
		pcap_t *handle = pcap_open_live (device, BUFSIZ, 1, 1000, errbuf);

		if (handle == NULL)
		{
			fprintf (stderr, "Could not open device %s: %s\n", device, errbuf);
			it = it->next;
			continue;
		}

		if (analyzer == NULL)
		{
			analyzer = (XtmNetworkAnalyzer *)malloc (sizeof (XtmNetworkAnalyzer));
			analyzer->next = NULL;
			current = analyzer;
		}
		else
		{
			current->next = (XtmNetworkAnalyzer *)malloc (sizeof (XtmNetworkAnalyzer));
			current = current->next;
			current->next = NULL;
		}

		current->handle = handle;
		current->iface = it;
		current->packetin = g_hash_table_new_full (g_int_hash, g_int_equal, g_free, NULL);
		current->packetout = g_hash_table_new_full (g_int_hash, g_int_equal, g_free, NULL);

		memcpy (current->mac, mac, sizeof (uint8_t) * 6);

		pthread_mutex_init (&current->lock, NULL);
		pthread_create (&current->thread, NULL, network_analyzer_thread, (void *)current);

		it = it->next;
	}

	if (analyzer == NULL)
	{
		fprintf (stderr, "Could not open any device %s\n", errbuf);
		pcap_freealldevs (it);
	}
#endif

	return analyzer;
}

XtmNetworkAnalyzer *
xtm_network_analyzer_get_default (void)
{
	static int initialized = FALSE;
	static XtmNetworkAnalyzer *analyzer = NULL;

	if (initialized == FALSE)
	{
		initialized = TRUE;
		// analyzer may be NULL if no device can be opened
		analyzer = xtm_create_network_analyzer ();
	}

	return analyzer;
}

void
xtm_destroy_network_analyzer (XtmNetworkAnalyzer *analyzer)
{
#ifdef HAVE_LIBPCAP
	if (analyzer == NULL)
		return;

	XtmNetworkAnalyzer *previous = analyzer;
	pcap_if_t *it = analyzer->iface;

	while (analyzer)
	{
		pthread_cancel (analyzer->thread);
		pcap_close (analyzer->handle);
		g_hash_table_destroy (analyzer->packetin);
		g_hash_table_destroy (analyzer->packetout);
		pthread_mutex_destroy (&analyzer->lock);
		analyzer = analyzer->next;
		free (previous);
		previous = analyzer;
	}

	pcap_freealldevs (it);
#endif
}
