#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <network.h>
#include <debug.h>
#include <errno.h>

#include "exi.h"
#include "sidestep.h"

// --------------------------------------------------------------------------------
//  DECLARATIONS
//---------------------------------------------------------------------------------
void * init();
void * gclink(void *arg);

int parse_command(int csock, char * packet);
int execute_dol(int csock, int size);
int execute_elf(int csock, int size);
int say_hello(int csock);

// --------------------------------------------------------------------------------
//  GLOBALS
//---------------------------------------------------------------------------------
static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

static lwp_t gclink_handle = (lwp_t)NULL;

int PORT = 23;
const static char gclink_exec_dol[] = "EXECDOL";
const static char gclink_exec_elf[] = "EXECELF";
const static char gclink_hello[] = "HELLO";


//---------------------------------------------------------------------------------
// init
//---------------------------------------------------------------------------------
void * init() {

	void *framebuffer;

	VIDEO_Init();
	PAD_Init();

	rmode = VIDEO_GetPreferredMode(NULL);
	framebuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	console_init(framebuffer,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(framebuffer);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	return framebuffer;
}

//---------------------------------------------------------------------------------
// Main
//---------------------------------------------------------------------------------
int main(int argc, char **argv) {

	s32 ret;

	char localip[16] = {0};
	char gateway[16] = {0};
	char netmask[16] = {0};

	xfb = init();

	printf("\ngclink by Shazz/TRSi\n");
	printf("Configuring network...\n");

    if(exi_bba_exists()) {

        // Configure the network interface
        ret = if_config( localip, netmask, gateway, TRUE, 20);
        if (ret>=0) {
            printf("Network configured IP: %s, GW: %s, MASK: %s\n", localip, gateway, netmask);

            LWP_CreateThread(	&gclink_handle,	/* thread handle */
                                gclink,			/* code */
                                localip,		/* arg pointer for thread */
                                NULL,			/* stack base */
                                16*1024,		/* stack size */
                                50				/* thread priority */ );
        }
        else {
            printf("Network configuration failed!\n");
			printf("Press START to exit\n");
        }
    }
    else {
        printf("Broadband Adapter not found!\n");
		printf("Press START to exit\n");
    }

	while(1) {

		VIDEO_WaitVSync();
		PAD_ScanPads();

		int buttonsDown = PAD_ButtonsDown(0);

		if(buttonsDown & PAD_BUTTON_START) {
			exit(0);
		}
	}

	return 0;
}

//---------------------------------------------------------------------------------
// Thread entry point
//---------------------------------------------------------------------------------
void * gclink(void *arg) {

	int sock, csock;
	int ret;
	u32	clientlen;
	struct sockaddr_in client;
	struct sockaddr_in server;
	char temp[1026];

	clientlen = sizeof(client);

	sock = net_socket (AF_INET, SOCK_STREAM, IPPROTO_IP);

	if(sock == INVALID_SOCKET) {
    	printf ("Cannot create a socket!\n");
    }
	else {

		memset(&server, 0, sizeof (server));
		memset(&client, 0, sizeof (client));

		server.sin_family = AF_INET;
		server.sin_port = htons (PORT);
		server.sin_addr.s_addr = INADDR_ANY;
		ret = net_bind (sock, (struct sockaddr *) &server, sizeof (server));

		if(ret) {
			printf("Error %d binding socket!\n", ret);
		}
		else {

			if((ret = net_listen(sock, 5))) {
				printf("Error %d listening!\n", ret);
			}
			else {

				while(1) {

					csock = net_accept (sock, (struct sockaddr *) &client, &clientlen);

					if (csock < 0) {
						printf("Error connecting socket %d!\n", csock);
						while(1);
					}

					printf("Connecting port %d from %s\n", client.sin_port, inet_ntoa(client.sin_addr));
					memset (temp, 0, 1026);
					ret = net_recv(csock, temp, 1024, 0);
					printf("Received %d bytes\n", ret);

                    ret = parse_command(csock, temp);
					if (!ret) {
						printf("Commmand failed!");
					}

					net_close (csock);
				}
			}
		}
	}
	return NULL;
}

//---------------------------------------------------------------------------------
// Command Parser
//---------------------------------------------------------------------------------
int parse_command(int csock, char * packet) {

    char response[200];

	printf("Parsing command\n");

    if(!strncmp(packet, gclink_exec_dol, strlen(gclink_exec_dol))) {
		char filename[50];
        int size = 0;
		char * token;

		token = strtok(&packet[strlen(gclink_exec_dol)], " ");
		if(token != NULL) {
			sprintf(filename, token);
			token = strtok(NULL, " ");
			if(token != NULL) {
				size = atoi(token);
			}
		}
		else {
			sprintf(response, "Incomplete %s command, missing filename", gclink_exec_dol);
			net_send(csock, response, strlen(response), 0);
			return 1;
		}

		if(size != 0) {
			printf("Executing DOL of size %d\n", size);
			execute_dol(csock, size);

			sprintf(response, "DOL executed");
			net_send(csock, response, strlen(response), 0);
		}
		else {
			sprintf(response, "Incomplete %s command, missing size", gclink_exec_dol);
			net_send(csock, response, strlen(response), 0);
			return 1;
		}

    }
    else if(!strncmp(packet, gclink_exec_elf, strlen(gclink_exec_elf))) {
		char filename[50];
        int size = 0;
		char * token;

		token = strtok(&packet[strlen(gclink_exec_elf)], " ");
		if(token != NULL) {
			sprintf(filename, token);
			token = strtok(NULL, " ");
			if(token != NULL) {
				size = atoi(token);
			}
		}
		else {
			sprintf(response, "Incomplete %s command, missing filename", gclink_exec_dol);
			net_send(csock, response, strlen(response), 0);
			return 1;
		}

		if(size != 0) {
			printf("Executing ELF of size %d\n", size);
			execute_elf(csock, size);

			sprintf(response, "ELF executed");
			net_send(csock, response, strlen(response), 0);
		}
		else {
			sprintf(response, "Incomplete %s command, missing size", gclink_exec_elf);
			net_send(csock, response, strlen(response), 0);
			return 1;
		}
    }
	else if(!strncmp(packet, gclink_hello, strlen(gclink_hello))) {

		printf("Saying hello\n");
		say_hello(csock);

        sprintf(response, "HELLO said!");
		net_send(csock, response, strlen(response), 0);
    }
    else {
        sprintf(response, "Unknown command");
        net_send(csock, response, strlen(response), 0);
		return 1;
    }

    return 0;
}

//---------------------------------------------------------------------------------
// HELLO execute command
//---------------------------------------------------------------------------------
int say_hello(int csock) {

	char * response = "OLLEH";
	net_send(csock, response, strlen(response), 0);

    return 0;
}

//---------------------------------------------------------------------------------
// DOL execute command
//---------------------------------------------------------------------------------
int execute_dol(int csock, int size) {

    u8 * data = (u8*) memalign(32, size);
    int ret;

    ret = net_recv(csock, data, size, 0);
    printf("Received %d bytes of DOL payload\n", ret);

    ret = DOLtoARAM(data, 0, NULL);

    return ret;
}

//---------------------------------------------------------------------------------
// ELF execute command
//---------------------------------------------------------------------------------
int execute_elf(int csock, int size) {

    u8 * data = (u8*) memalign(32, size);
    int ret;

    ret = net_recv(csock, data, size, 0);
    printf("Received %d bytes of ELF payload\n", ret);

    ret = ELFtoARAM(data, 0, NULL);

    return ret;
}