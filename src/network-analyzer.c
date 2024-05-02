#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "network-analyzer.h"

void packet_callback(u_char *args, const struct pcap_pkthdr *header, const u_char *packet);
void* network_analyzer_thread(void *ptr);
void increament_packet_count(char*, char*, GHashTable* hash_table, long int port);
int mac_get_binary_from_file(const char *filename, uint8_t mac[6]);

void increament_packet_count(char *mac, char *direction, GHashTable* hash_table, long int port)
{
    int *key = g_new0(gint, 1);
    *key = port;
    gpointer value = g_hash_table_lookup(hash_table, key);
    g_hash_table_replace(hash_table, key, (gpointer)(((long int)value)+1));
    printf("%s -> %s %ld: %ld\n", mac, direction, port, ((long int)value)+1);
}

void packet_callback(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
    XtmNetworkAnalyzer *analyzer = (XtmNetworkAnalyzer*)args;

    // Extract source and destination IP addresses and ports from the packet
    struct ether_header *eth_header = (struct ether_header*)packet;
    struct ip *ip_header = (struct ip*)(packet + sizeof(struct ether_header));
    struct tcphdr *tcp_header = (struct tcphdr*)(packet + sizeof(struct ether_header) + sizeof(struct ip));

    // Dropped non-ip packet
	if (eth_header->ether_type != 8 || ip_header->ip_p != 6)
        return;

    long int src_port = ntohs(tcp_header->source);
    long int dst_port = ntohs(tcp_header->dest);

    // directly use strcmp on analyzer->mac, eth_header->ether_shost doesnt work

    char local_mac[18];
    char src_mac[18];
    char dst_mac[18];

    sprintf(local_mac,
        "%02X:%02X:%02X:%02X:%02X:%02X",
        analyzer->mac[0], analyzer->mac[1],
        analyzer->mac[2], analyzer->mac[3],
        analyzer->mac[4], analyzer->mac[5]
    );

    sprintf(src_mac,
        "%02X:%02X:%02X:%02X:%02X:%02X",
        eth_header->ether_shost[0], eth_header->ether_shost[1],
        eth_header->ether_shost[2], eth_header->ether_shost[3],
        eth_header->ether_shost[4], eth_header->ether_shost[5]
    );

    sprintf(dst_mac,
        "%02X:%02X:%02X:%02X:%02X:%02X",
        eth_header->ether_dhost[0], eth_header->ether_dhost[1],
        eth_header->ether_dhost[2], eth_header->ether_dhost[3],
        eth_header->ether_dhost[4], eth_header->ether_dhost[5]
    );

    // Debug
    //pthread_mutex_lock(&analyzer->lock);

    if(strcmp(local_mac, src_mac) == 0)
        increament_packet_count(local_mac, "in ", analyzer->packetin, src_port);
    
    if(strcmp(local_mac, dst_mac) == 0)
        increament_packet_count(local_mac, "out", analyzer->packetout, dst_port);

    //pthread_mutex_unlock(&analyzer->lock);
}

void *network_analyzer_thread( void *ptr )
{
    XtmNetworkAnalyzer *analyzer = (XtmNetworkAnalyzer*)ptr;
    pcap_loop(analyzer->handle, -1, packet_callback, (void*)analyzer);
    return NULL;
}

int mac_get_binary_from_file(const char *filename, uint8_t mac[6])
{
    int status = 1;
    char buf[256];
    FILE *fp = fopen(filename, "rt");
    memset(buf, 0, 256);

    if (!fp)
        return 1;

    if (fgets(buf, sizeof buf, fp) != NULL)
    {
        sscanf(
            buf, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
            &mac[0], &mac[1],
            &mac[2], &mac[3],
            &mac[4], &mac[5]
        );
        status = 0;
    }

    fclose(fp);
    return status;
}

XtmNetworkAnalyzer*
xtm_create_network_analyzer(void)
{
    char device_path[2048];
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
        sprintf(device_path, "/sys/class/net/%s/address", device);

        //! todo check pcap lib ?
        if (mac_get_binary_from_file(device_path, mac) == 0)
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

        current->mac[0] = mac[0];
        current->mac[1] = mac[1];
        current->mac[2] = mac[2];
        current->mac[3] = mac[3];
        current->mac[4] = mac[4];
        current->mac[5] = mac[5];

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