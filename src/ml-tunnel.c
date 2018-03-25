#include <stdio.h> 			//for printf
#include <string.h> 		//memset
#include <sys/socket.h>    	//for socket ofcourse
#include <stdlib.h> 		//for exit(0);
#include <libgen.h>
#include <errno.h> 			//For errno - the error number
#include <netinet/udp.h>   	//Provides declarations for udp header
#include <netinet/ip.h>    	//Provides declarations for ip header
#include <arpa/inet.h>		//for inet_addr
#include <unistd.h>			//for close
#include <fcntl.h>			//for open
#include <sys/ioctl.h>		//for ioctl
#include <net/if.h>			//for struct ifreq
#include <linux/if_tun.h>	//for tun devices
#include <linux/route.h>	//for rtentry
#include <ev.h>				//for ev

#include "ml-tunnel_tools.h"	//tools

// This callback is called when data is readable on the tun interface.
static void tun_cb(EV_P_ ev_io *w, int revents) {
	
	char buffer[BUF_SIZE];	
    
	fprintf(stdout, "tun interface has become readable, hex dump :\n");
	ssize_t nbytes = read(w->fd, buffer, sizeof(buffer));
    hexdump(buffer, nbytes);
	packet_decode(buffer, nbytes);
}

int main(int argc, char **argv) {
	
	/*
	 * Runs as server by default
	 */
	int client = 0;
	/*
	 * Tun if descriptor and name
	 */
	int stun = 0;
	const char tun_dev[] = "tun0";
	/*
	 * List of WAN ifs
	 */
	const char* wan_ifs[] = { "ppp0", "ppp1", "wlan0" };
	/*
	 * IP Address on the client side of the tunnel
	 */
	const char client_ip_address[] = "10.0.0.1";
	
	char progname[FNAME_MAX_SIZE+1];
	memset(progname, '\0', FNAME_MAX_SIZE);
	
	if ( argv[0] != NULL ) {
		strncpy(progname, basename(argv[0]), FNAME_MAX_SIZE);
		printf("Start of program %s\n", progname);
		if ( argc > 1 ) {
			fprintf(stdout, "Argument : %s\n", argv[1]);
			if ( argv[1][0] ==  'c' ) {
				fprintf(stdout, "Running in client mode\n");
				client = 1;
			}
			else {
				fprintf(stdout, "Running in server mode\n");
				client = 0;
			}
		}
	}
	else {
		perror("Bad program name. Exiting\n");
		exit(-1);
	}
	
	// Opening TUN If
	stun = tun_open(tun_dev);
	if (stun < 0) {
		fprintf(stderr, "Error opening tun %s : %s\n", tun_dev, strerror(errno));
		exit(-1);
	}	

	// Client mode
	if (client) {
		fprintf(stdout, "Client started\n");
		// Finish the tun configuration
		ipv4_set_addr(tun_dev, client_ip_address);
		ipv4_route_add(tun_dev, "1.2.3.4", "255.255.255.255", HOST);
		fprintf(stdout, "Enabled WAN interfaces : ");
		int iwan_if = 0;
		while (iwan_if < sizeof(wan_ifs)) {
			fprintf(stdout, "%s ", wan_ifs[i++]);
		}
		fprintf(stdout, "\n");
		
		// Do the libev stuff.
		struct ev_loop *loop = ev_default_loop(0);
		ev_io tun_watcher;
		ev_io_init(&tun_watcher, tun_cb, stun, EV_READ);
		ev_io_start(loop, &tun_watcher);
		ev_loop(loop, 0);
	}
	// Server mode
	else {
		fprintf(stdout, "Server started\n");
		ipv4_route_add(tun_dev, client_ip_address, "255.255.255.255", HOST);
		while(1);
	}
	fprintf(stdout, "End of program %s\n", progname);
	exit(0);
}
