#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <sys/time.h>
#include <stdarg.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#define __FAVOR_BSD
#include <netinet/tcp.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <openssl/err.h>

#include "tcpcrypt_divert.h"
#include "tcpcrypt.h"
#include "tcpcryptd.h"
#include "profile.h"
#include "test.h"
#include "crypto.h"

int _s;

void open_raw()
{       
        int one = 1;

        _s= socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
        if (_s == -1)
                err(1, "socket()");

        if (setsockopt(_s, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one))
	    == -1)
                err(1, "IP_HDRINCL");
}

void raw_inject(void *data, int len)
{
        int rc;
        struct ip *ip = data;
        struct tcphdr *tcp = (struct tcphdr*) ((char*) ip + (ip->ip_hl << 2));
        struct sockaddr_in s_in;

	if (_s == 0)
		open_raw();

	memset(&s_in, 0, sizeof(s_in));

        s_in.sin_family = PF_INET;
        s_in.sin_addr   = ip->ip_dst;
        s_in.sin_port   = tcp->th_dport;

#if defined(__FreeBSD__)
#include <osreldate.h>
#if __FreeBSD_version < 1000022
       #define HO_LEN
#endif
#endif
#ifdef __DARWIN_UNIX03
	#define HO_LEN
#endif
#ifdef HO_LEN
	ip->ip_len = ntohs(ip->ip_len);
	ip->ip_off = ntohs(ip->ip_off);
#endif

        rc = sendto(_s, data, len, 0, (struct sockaddr*) &s_in,
		    sizeof(s_in));
        if (rc == -1)
                err(1, "sendto(raw)");

        if (rc != len)
                errx(1, "wrote %d/%d", rc, len);

#ifdef HO_LEN
	ip->ip_len = htons(ip->ip_len);
	ip->ip_off = htons(ip->ip_off);
#endif
}

void divert_cycle(void)
{
}
