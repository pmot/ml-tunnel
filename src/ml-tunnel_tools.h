#ifndef __TUNNEL_TOOLS__
#define __TUNNEL_TOOLS__

#include <stdio.h> 			// for printf
#include <ctype.h>			// for isprint
#include <string.h> 		// for memset
#include <sys/socket.h>    	// for socket ofcourse
#include <stdlib.h> 		// for exit(0);
#include <errno.h> 			// for errno - the error number
#include <netinet/udp.h>   	// provides declarations for udp header
#include <netinet/ip.h>    	// provides declarations for ip header
#include <arpa/inet.h>		// for inet_addr
#include <unistd.h>			// for close
#include <fcntl.h>			// for open
#include <sys/ioctl.h>		// for ioctl
#include <net/if.h>			// for struct ifreq
#include <linux/if_tun.h>	// for tun devices
#include <linux/route.h>	// for rtentry

/*
 * Consts
 */
#define FNAME_MAX_SIZE	256
#define DEFAULT_MTU		1500
#define TUN_MTU			DEFAULT_MTU - sizeof(struct proto_t) \
									- sizeof(struct ip) \
									- sizeof(struct udphdr)
#define BUF_SIZE        4096
#define HEXDUMP_COLS	12

#define HOST			1
#define NET				0

/*
 * Packets
 */
typedef struct proto_t {
	uint32_t seq;
	char secret_key[20];
} __attribute__ ((packed)) proto_t;

typedef struct packet_t {
    struct ip       ip;
    struct udphdr   udp;
	struct proto_t	proto;
    char            payload[TUN_MTU];
} __attribute__ ((packed)) packet_t;


/*
 *   Ouverture de l'interface tun
 *    device (eg. tun0)
*/
int tun_open(const char*);

/*
 *   Set Ip address of the interface
 *    interface name, ip v4 address
 */
int ipv4_set_addr(const char*, const char*);

/*
 *   Add route :
 *    device, dest ip, mask, bool true if host
 */
int ipv4_route_add(const char*, const char*, const char*, int);

/*
 *   Dump hexa + ascii d'un buffer
 *    buffer, buffer's size
 */
void hexdump(char*, uint32_t);

/*
 *   Decode IP packet and print some stuff
 *    buffer, buffer's size
 */
void packet_decode(char*, uint32_t);

#endif