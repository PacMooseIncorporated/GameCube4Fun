#ifndef __BBA_DEBUG_H
#define __BBA_DEBUG_H

typedef enum out {
    BBA_PRINTF, KPRINTF, PRINTF
} out_t;

int     setup_bba_logging(int port, char* ip_address, out_t output, bool keep_existing_out, char ** bba_config);
void    close_bba_logging();
int     bba_printf(const char *fmt, ...);

#endif /* __BBA_DEBUG_H */