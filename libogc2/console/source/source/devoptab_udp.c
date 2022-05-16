#include <sys/iosupport.h>
#include <ogcsys.h>

#include "bba_debug.h"

int netudp_open(struct _reent *r, void *fileStruct, const char *path, int flags, int mode);
int netudp_close(struct _reent *r, void *fd);
int netudp_write(struct _reent *r, void *fd, const char *ptr, size_t len);

extern s32 sock;
bool is_enabled = false;

//---------------------------------------------------------------------------------
const devoptab_t dotab_stdudp = {
//---------------------------------------------------------------------------------
    "stdudp",     // device name
    0,            // size of file structure
    netudp_open,  // device open
    netudp_close, // device close
    netudp_write, // device write
    NULL,         // device read
    NULL,         // device seek
    NULL,         // device fstat
    NULL,         // device stat
    NULL,         // device link
    NULL,         // device unlink
    NULL,         // device chdir
    NULL,         // device rename
    NULL,         // device mkdir
    0,            // dirStateSize
    NULL,         // device diropen_r
    NULL,         // device dirreset_r
    NULL,         // device dirnext_r
    NULL,         // device dirclose_r
    NULL          // device statvfs_r
};

//---------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------
int enable_bba_traces()
{
    if (configure_bba() < 0)
        return -1;

    // update newlib devoptab_list with this devoptab
    devoptab_list[STD_OUT] = &dotab_stdudp;
    devoptab_list[STD_ERR] = &dotab_stdudp;
    is_enabled = true;

    return 0;
}

//---------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------
int netudp_open(struct _reent *r, void *fileStruct, const char *path, int flags, int mode)
{
    if (!is_enabled)
        return -1;

    // assuming path is a string containing the IP address and not a hostname
    if (open_udp_socket(TRACE_PORT, (char *)path) < 0)
        return -1;

    return sock;
}

//---------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------
int netudp_close(struct _reent *r, void *fd)
{
    if (!is_enabled)
        return -1;

    close_udp_socket();
    // net_close(fd);

    return 0;
}

//---------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------
int netudp_write(struct _reent *r, void *fd, const char *ptr, size_t len)
{
    int ret;

    if (!is_enabled)
        return -1;
    if (fd < 0)
        return -1;

    ret = bba_printf(ptr);
    // ret = net_write(fd, (void*)ptr, len);

    return ret;
}