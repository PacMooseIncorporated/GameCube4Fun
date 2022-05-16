#ifndef __BBA_DEBUG_H
#define __BBA_DEBUG_H

// custom
int     setup_bba_logging(int port, char* ip_address, bool use_kprintf, bool use_printf);
void    close_bba_logging();
int     bba_printf(const char *fmt, ...);

#endif /* __BBA_DEBUG_H */