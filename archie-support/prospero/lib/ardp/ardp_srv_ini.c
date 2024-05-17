/*
 * Copyright (c) 1991       by the University of Washington
 * Copyright (c) 1992, 1993 by the University of Southern California
 *
 * For copying and distribution information, please see the files
 * <uw-copyright.h> and <usc-copyr.h>.
 *
 * Written  by bcn 1991     as part of rdgram.c in Prospero distribution 
 * Modified by bcn 1/93     modularized and incorporated into new ardp library
 */

#include <uw-copyright.h>
#include <usc-copyr.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>

#include <ardp.h>
#include <plog.h>

int (*ardp_pri_func)() = NULL;  /* Function to compare priorities       */
int ardp_pri_override = 0;	/* If 1, then oveeride value in request */

int ardp_srvport = -1;
int ardp_prvport = -1;


/* 
 * ardp_set_queueing_plicy - set function for queueing policy 
 *
 *    ardp_set_queuing_policy allows one to provide a function that will set 
 *    priorities for requests.  When passed two req structures, r1 and r2, the 
 *    function should  return a negative number if r1 should be executed first 
 *    (i.e. r1 has a lower numerical priority) and positive if r2 should be 
 *    executed first.  If the function returns 0, it means the two have the 
 *    same priority and should be executed FCFS.  If override is non-zero, then
 *    the priority function is to be applied to all requests.  If non-zero,
 *    it is only applied to those with identical a priori priorities (as
 *    specified in the datagram itself.
 */
int
ardp_set_queuing_policy(pf,override)
    int (*pf)(); 		/* Function to compare priorities       */
    int	override;		/* If 1, then oveeride value in request */
    {
	ardp_pri_func = pf;
	ardp_pri_override = override;
	return(ARDP_SUCCESS);
    }

int
ardp_set_prvport(port)
    int		port;
    {
	ardp_prvport = port;
	return(ARDP_SUCCESS);
    }

int
ardp_bind_port(portname)
    char	*portname;
{
    struct sockaddr_in	s_in = {AF_INET};
    struct servent 	*sp;
    int     		on = 1;
    int			port_no = 0;

    assert(P_IS_THIS_THREAD_MASTER()); /*getpwuid MT-Unsafe*/
    if(*portname == '#') {
	sscanf(portname+1,"%d",&port_no);
	if(port_no == 0) {
	    fprintf(stderr, "ardp_bind_port: port number must follow #\n");
	    exit(1);
	}
	s_in.sin_port = htons((ushort) port_no);
    }
    else if((sp = getservbyname(portname, "udp")) != NULL) {
	s_in.sin_port = sp->s_port;
    }
    else if(strcmp(portname,ARDP_DEFAULT_PEER) == 0) {
	fprintf(stderr, "ardp_bind_port: udp/%s unknown service - using %d\n", 
		ARDP_DEFAULT_PEER, ARDP_DEFAULT_PORT);
	s_in.sin_port = htons((ushort) ARDP_DEFAULT_PORT);
    }
    else {
	fprintf(stderr, "ardp_bind_port: udp/%s unknown service\n",portname);
	exit(1);
    }
    
    if ((ardp_srvport = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	plog(L_STATUS,NOREQ,"Startup - Can't open socket",0);
	fprintf(stderr, "ardp_bind_port: Can't open socket\n");
	exit(1);
    }
    if (setsockopt(ardp_srvport, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
	fprintf(stderr, "dirsrv: setsockopt (SO_REUSEADDR)\n");
    
    if (bind(ardp_srvport, (struct sockaddr *) &s_in, S_AD_SZ) < 0) {
	plog(L_STATUS,NOREQ,"Startup - Can't bind socket",0);
	fprintf(stderr, "dirsrv: Can not bind socket\n");
	exit(1);
    }
    return(ntohs(s_in.sin_port));
}

