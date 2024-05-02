#ifndef NETWORK_ANALYZER_H
#define NETWORK_ANALYZER_H

#include <glib.h>
#include <pcap.h>
#include <pthread.h>

typedef struct _XtmNetworkAnalyzer XtmNetworkAnalyzer;
struct _XtmNetworkAnalyzer
{
    // interface mac adress
    pcap_if_t *iface;
    guint8 mac[6];
    
    pthread_t thread;
    pthread_mutex_t lock;

    pcap_t* handle;
    
    // map port and number of packets
    GHashTable* packetin;
    GHashTable* packetout;

    // next interface
    XtmNetworkAnalyzer *next;
};

XtmNetworkAnalyzer* xtm_create_network_analyzer(void);
void xtm_destroy_network_analyzer(XtmNetworkAnalyzer *analyzer);
XtmNetworkAnalyzer *xtm_network_analyzer_get_default (void);

#endif