/*
 * Copyright (c) 2024 Jehan-Antoine Vayssade, <javayss@sleek-think.ovh>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netpacket/packet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

#include "network-analyzer.h"
#include "task-manager.h"

void* network_analyzer_thread(void *ptr);
void increament_packet_count(char*, char*, GHashTable* hash_table, long int port);
int get_mac_address(const char *device, uint8_t mac[6]);

void
increament_packet_count(char *mac, char *direction, GHashTable* hash_table, long int port)
{
    int *key = g_new0(gint, 1);
    *key = port;
    gpointer value = g_hash_table_lookup(hash_table, key);
    g_hash_table_replace(hash_table, key, (gpointer)(((long int)value)+1));
    printf("%s -> %s %ld: %ld\n", mac, direction, port, ((long int)value)+1);
}

void*
network_analyzer_thread( void *ptr )
{
    XtmNetworkAnalyzer *analyzer = (XtmNetworkAnalyzer*)ptr;
    pcap_loop(analyzer->handle, -1, packet_callback, (void*)analyzer);
    return NULL;
}

int
get_mac_address(const char *device, uint8_t mac[6])
{
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1)
        return -1;

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        int family = ifa->ifa_addr->sa_family;

        // Check if the interface is a network interface (AF_PACKET for Linux)
        if (family == AF_PACKET && strcmp(device, ifa->ifa_name) == 0)
        {
            // Check if the interface has a hardware address (MAC address)
            if (ifa->ifa_data != NULL)
            {
                struct sockaddr_ll *sll = (struct sockaddr_ll *)ifa->ifa_addr;
                memcpy(mac, sll->sll_addr, sizeof(uint8_t) * 6);
                freeifaddrs(ifaddr);
                return 0;
            }
        }
    }

    freeifaddrs(ifaddr);
    return -1;
}

XtmNetworkAnalyzer*
xtm_create_network_analyzer(void)
{
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t *it = NULL;

    if (pcap_findalldevs(&it, errbuf) != 0)
    {
        fprintf(stderr, "Error finding device: %s\n", errbuf);
        return NULL;
    }

    XtmNetworkAnalyzer *analyzer = NULL;
    XtmNetworkAnalyzer *current = NULL;

    while (it)
    {
        guint8 mac[6];
        char *device = it->name;

        //! todo check pcap lib ?
        if (get_mac_address(device, mac) == 0)
        {
            printf(
                "%s, MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                device, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
            );
        }
        else
        {
            fprintf(stderr, "No mac adresse for %s\n", device);
            it = it->next;
            continue;
        }

        // Open a pcap session
        pcap_t *handle = pcap_open_live(device, BUFSIZ, 1, 1000, errbuf);

        if(handle == NULL)
        {
            fprintf(stderr, "Could not open any device %s: %s\n", device, errbuf);
            it = it->next;
            continue;
        }

        if(analyzer == NULL)
        {
            analyzer = (XtmNetworkAnalyzer*)malloc(sizeof(XtmNetworkAnalyzer));
            current = analyzer;
        }
        else
        {
            current->next = (XtmNetworkAnalyzer*)malloc(sizeof(XtmNetworkAnalyzer));
            current = current->next;
        }

        current->handle = handle;
        current->iface = it;
        current->packetin = g_hash_table_new_full (g_int_hash, g_int_equal, g_free, NULL);
        current->packetout = g_hash_table_new_full (g_int_hash, g_int_equal, g_free, NULL);

        memcpy(current->mac, mac, sizeof(uint8_t) * 6);

        pthread_mutex_init(&current->lock, NULL);
        pthread_create(&current->thread, NULL, network_analyzer_thread, (void*)current);

        it = it->next;
    }

    if(analyzer == NULL)
    {
        fprintf(stderr, "Could not open any device %s\n", errbuf);
        pcap_freealldevs(it);
    }

    return analyzer;
}

XtmNetworkAnalyzer *
xtm_network_analyzer_get_default (void)
{
	static XtmNetworkAnalyzer *analyzer = NULL;
	if (analyzer == NULL)
		analyzer = xtm_create_network_analyzer();
	return analyzer;
}

void xtm_destroy_network_analyzer(XtmNetworkAnalyzer *analyzer)
{
    if(analyzer == NULL)
        return;

    XtmNetworkAnalyzer *previous = analyzer;
    pcap_if_t *it = analyzer->iface;

    while (analyzer)
    {
        pthread_cancel(analyzer->thread);
        pcap_close(analyzer->handle);
        g_hash_table_destroy(analyzer->packetin);
        g_hash_table_destroy(analyzer->packetout);
        pthread_mutex_destroy(&analyzer->lock);
        analyzer = analyzer->next;
        free(previous);
        previous = analyzer;
    }

    pcap_freealldevs(it);
}