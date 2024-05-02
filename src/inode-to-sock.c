#include "inode-to-sock.h"
#include "stdio.h"

void addtoconninode(XtmInodeToSock *its, char *buffer);

XtmInodeToSock*
xtm_create_inode_to_sock(void)
{
    XtmInodeToSock *its = (XtmInodeToSock*)malloc(sizeof(XtmInodeToSock));
    its->hash = g_hash_table_new_full (g_int_hash, g_int_equal, g_free, NULL);
    return its;
}

void
addtoconninode(XtmInodeToSock *its, char *buffer)
{
	char rem_addr[128], local_addr[128];
	int local_port, rem_port;
    int *inode = g_new0(gint, 1);

	sscanf(
        buffer,
        "%*d: %64[0-9A-Fa-f]:%X %64[0-9A-Fa-f]:%X %*X %*lX:%*lX %*X:%*lX %*lX %*d %*d %ld %*512s\n",
		local_addr, &local_port, rem_addr, &rem_port, inode
    );

    g_hash_table_replace(its->hash, inode, (gpointer)local_port);
}

void
xtm_refresh_inode_to_sock(XtmInodeToSock *its)
{
	char buffer[8192];
	FILE * procinfo = fopen ("/proc/net/tcp", "r");
	if (procinfo)
	{
		fgets(buffer, sizeof(buffer), procinfo);
		do
		{
			if (fgets(buffer, sizeof(buffer), procinfo))
				addtoconninode(its, buffer); 
		}
        while (!feof(procinfo));
		fclose(procinfo);
	}

	procinfo = fopen ("/proc/net/tcp6", "r");
	if (procinfo != NULL)
    {
		fgets(buffer, sizeof(buffer), procinfo);
		do
        {
			if (fgets(buffer, sizeof(buffer), procinfo))
				addtoconninode(its, buffer);
		}
        while (!feof(procinfo));
		fclose (procinfo);
	}
}

void
xtm_destroy_inode_to_sock(XtmInodeToSock *its)
{
    if(its == NULL)
        return;

    g_hash_table_destroy(its->hash);
    free(its);
}

XtmInodeToSock*
xtm_inode_to_sock_get_default (void)
{
	static XtmInodeToSock *its = NULL;
	if (its == NULL)
		its = xtm_create_inode_to_sock();
	return its;
}