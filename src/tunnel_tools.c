#include "tunnel_tools.h"

/*
    Ouverture de l'interface tun
*/
int tun_open(const char *devname)
{
    struct ifreq ifr;
    int fd, err;

    if ( (fd = open("/dev/net/tun", O_RDWR)) == -1 ) {
		perror("open /dev/net/tun");
		return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;		// TUN device, do not add the 4 extra bytes (flags and protocol)
    strncpy(ifr.ifr_name, devname, IFNAMSIZ);	// devname = "tun0" or "tun1", etc 

    /* ioctl will use ifr.if_name as the name of TUN 
     * interface to open: "tun0", etc. */
    if ( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) == -1 ) {
		perror("ioctl TUNSETIFF");
		close(fd);
		return -1;
    }

    /* After the ioctl call the fd is "connected" to tun device specified
     * by devname ("tun0", "tun1", etc)*/
    int sfd;
    if ( (sfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1 ) {
		perror("socket");
		close(fd);
		return -1;
    }

    if ( (err = ioctl(sfd, SIOCGIFFLAGS, &ifr)) == -1 ) {
		perror("ioctl SIOCGIFFLAGS");
		close(sfd);
		close(fd);
		return -1;
    }

    ifr.ifr_flags |= IFF_UP;
    ifr.ifr_flags |= IFF_RUNNING;

    if ( (err = ioctl(sfd, SIOCSIFFLAGS, (void *) &ifr)) == -1 ) {
		perror("ioctl SIOCGIFFLAGS");
		close(sfd);
		close(fd);
		return -1;
    }

    ifr.ifr_mtu = 1400;
    if ( (err = ioctl(sfd, SIOCSIFMTU, (void *) &ifr)) == -1 ) {
		perror("ioctl SIOCGIFFLAGS");
		close(sfd);
		close(fd);
		return -1;
    }

    close(sfd);
    return fd;
}

/*
 * Set IP Address of the interface
 */
int ipv4_set_addr(const char *ifname, const char *ipv4addr) {

	struct ifreq ifr;
	struct sockaddr_in sin;
	struct in_addr inaddr;
	
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	
	if ( sockfd == -1 ) {
		fprintf(stderr, "Unable to open socket : %s\n", strerror(errno));
		return -1;
	}

	sin.sin_family = AF_INET;

	// inet_aton(ipv4addr, &sin.sin_addr.s_addr);
	if ( inet_aton(ipv4addr, &inaddr) == 0 ) {
		fprintf(stderr, "Bad IP address : %s\n", ipv4addr);
		return -1;
	}
	sin.sin_addr = inaddr;
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ioctl(sockfd, SIOCGIFFLAGS, &ifr);
	memcpy(&ifr.ifr_addr, &sin, sizeof(struct sockaddr)); 
	// Set interface address
	if (ioctl(sockfd, SIOCSIFADDR, &ifr) < 0) {
		fprintf(stderr, "Cannot set IP address %s on %s : %s\n", ipv4addr, ifname, strerror(errno));
		return -1;
	}             
	return 0;
}

/*
  Ajout de la route via l'interface tun
  https://github.com/robclark/core/blob/master/libnetutils/ifc_utils.c
  L. 505 / ifc_act_on_ipv4_route
  L. 218 / init_sockaddr_in
*/
int ipv4_route_add(const char *devname, const char *dest, const char *mask, int is_host)
{
	char *ldev = malloc( strlen( devname ) + 1 );
	if (! ldev) {
		fprintf(stdout, "Out of memory\n");
		return -1;
	}
    struct rtentry route;
    memset( &route, 0, sizeof( route ) );
	memset ( ldev, 0, strlen( devname ) + 1 );
	strncpy ( ldev, devname, strlen( devname ) );

    route.rt_dst.sa_family = AF_INET;
	route.rt_dev = ldev; // devname;

    struct sockaddr_in *s_addr;

    //Netmask
    s_addr = (struct sockaddr_in*) &route.rt_genmask;
    s_addr->sin_family = AF_INET;
    s_addr->sin_addr.s_addr = inet_addr ( mask );
    // s_addr->sin_addr.s_addr = INADDR_ANY;			// Route Net Default

    //Destination
    s_addr = (struct sockaddr_in*) &route.rt_dst;
    s_addr->sin_family = AF_INET;
    s_addr->sin_addr.s_addr = inet_addr ( dest );		// Route Host
    // s_addr->sin_addr.s_addr = INADDR_ANY;			// Route Net Default

    //Flags
    route.rt_flags = RTF_UP;
    if (is_host == 1)
		route.rt_flags |= RTF_HOST;	// Si Route Host

    int sfd, err;
    if ( (sfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1 ) {
		perror("add route : socket");
		free( ldev );
		return -1;
    }

    if ( (err = ioctl(sfd, SIOCADDRT, &route )) == -1 ) {
		close(sfd);
		free ( ldev );
		if ( errno == EEXIST ) {
			return 0;
		}
		else
		{
			perror("SIOCADDRT");
			return -1;
		}
    }

	free( ldev );
    return 0;
}

/*
 * Pretty version of hex dump
 * http://grapsus.net/blog/post/Hexadecimal-dump-in-C
 */
void hexdump(char *mem, uint32_t len)
{
	uint32_t i, j;

	for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++)
	{
		/* print offset */
		if(i % HEXDUMP_COLS == 0)
		{
			printf("0x%06x: ", i);
		}
 
		/* print hex data */
		if(i < len)
		{
			printf("%02x ", 0xFF & ((char*)mem)[i]);
		}
		else /* end of block, just aligning for ASCII dump */
		{
			printf("   ");
		}
                
		/* print ASCII dump */
		if(i % HEXDUMP_COLS == (HEXDUMP_COLS - 1))
		{
			for(j = i - (HEXDUMP_COLS - 1); j <= i; j++)
			{
				if(j >= len) /* end of block, not really printing */
				{
					putchar(' ');
				}
				else if(isprint(((char*)mem)[j])) /* printable char */
				{
					putchar(0xFF & ((char*)mem)[j]);        
				}
				else /* other char */
				{
					putchar('.');
				}
			}
			putchar('\n');
		}
	}
}

/*
 * Decode The IP header
 * buffer, size of data
 */
void packet_decode(char *buffer, uint32_t n) {
	// struct ip *pip = NULL;
	
	struct ip *ip = (struct ip *)(buffer);
	
	printf("\nIP Header :\n");
	printf("\t|-Version      : %d\n", ip->ip_v);
	printf("\t|-ToS          : %d\n", ip->ip_tos);
	printf("\t|-Total length : %u\n", htons(ip->ip_len));
	printf("\t|-Packet Id    : %u\n", htons(ip->ip_id));
	printf("\t|-Offset       : %u\n", htons(ip->ip_off));
	printf("\t|-TTL          : %d\n", ip->ip_ttl);
	printf("\t|-Protocol     : %d\n", ip->ip_p);
	printf("\t|-Checksum     : %d\n", htons(ip->ip_sum));
	printf("\t|-Source Addr  : %s\n", inet_ntoa(ip->ip_src));
	printf("\t|-Dest Addr    : %s\n", inet_ntoa(ip->ip_dst));

}