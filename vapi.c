/* vapi.c -- Verification API Interface test side
   Copyright (C) 2001, Marko Mlinar, markom@opencores.org

This file is part of OpenRISC 1000 Architectural Simulator.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#include "vapi.h"

int vapi_fd;
unsigned long first_id, last_id;

static int vapi_write_stream(int fd, void* buf, int len)
{
  int n;
  char* w_buf = (char*)buf;
  struct pollfd block;

  while(len) {
    if((n = write(fd,w_buf,len)) < 0) {
      switch(errno) {
      case EWOULDBLOCK: /* or EAGAIN */
        /* We've been called on a descriptor marked
           for nonblocking I/O. We better simulate
           blocking behavior. */
        block.fd = fd;
        block.events = POLLOUT;
        block.revents = 0;
        poll(&block,1,-1);
        continue;
      case EINTR:
        continue;
      case EPIPE:
        close(fd);
        vapi_fd = 0;
        return -1;
      default:
        return -1;
      }
    } else {
      len -= n;
      w_buf += n;
    }
  }
  return 0;
}

static int vapi_read_stream(int fd, void* buf, int len)
{
  int n;
  char* r_buf = (char*)buf;
  struct pollfd block;

  while(len) {
    if((n = read(fd,r_buf,len)) < 0) {
      switch(errno) {
      case EWOULDBLOCK: /* or EAGAIN */
        /* We've been called on a descriptor marked
           for nonblocking I/O. We better simulate
           blocking behavior. */
        block.fd = fd;
        block.events = POLLIN;
        block.revents = 0;
        poll(&block,1,-1);
        continue;
      case EINTR:
        continue;
      default:
        return -1;
      }
    } else if(n == 0) {
      close(fd);
      fd = 0;
      return -1;
    } else {
      len -= n;
      r_buf += n;
    }
  }
  return 0;
}

static int write_packet (unsigned long id, unsigned long data) {
  id = htonl (id);
  if (vapi_write_stream(vapi_fd, &id, sizeof (id)) < 0)
    return 1;
  data = htonl (data);
  if (vapi_write_stream(vapi_fd, &data, sizeof (data)) < 0)
    return 1;
  return 0;
}

static int read_packet (unsigned long *id, unsigned long *data) {
  if (vapi_read_stream(vapi_fd, id, sizeof (unsigned long)) < 0)
    return 1;
  *id = htonl (*id);
  if (vapi_read_stream(vapi_fd, data, sizeof (unsigned long)) < 0)
    return 1;
  *data = htonl (*data);
  return 0;
}

/* Added by CZ 24/05/01 */
static int connect_to_server(char* hostname,char* name)
{
  struct hostent *host;
  struct sockaddr_in sin;
  struct servent *service;
  struct protoent *protocol;
  int sock,flags;
  char sTemp[256],sTemp2[256];
  char* proto_name = "tcp";
  int port = 0;
  int on_off = 0; /* Turn off Nagel's algorithm on the socket */
  char *s;

  if(!(protocol = getprotobyname(proto_name))) {
    sprintf(sTemp,"Protocol \"%s\" not available.\n",
      proto_name);
    error(sTemp);
    return 0;
  }

  /* Convert name to an integer only if it is well formatted.
     Otherwise, assume that it is a service name. */

  port = strtol(name, &s, 10);
  if(*s)
    port = 0;

  if(!port) {
    if(!(service = getservbyname(name, protocol->p_name))) {
  	  sprintf(sTemp,"Unknown service \"%s\".\n",name);
  	  error(sTemp);
  	  return 0;
  	}
    port = ntohs(service->s_port);
  }

  if(!(host = gethostbyname(hostname))) {
    sprintf(sTemp,"Unknown host \"%s\"\n",hostname);
    error(sTemp);
    return 0;
  }

  if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    sprintf(sTemp, "can't create socket errno = %d\n", errno);
    sprintf(sTemp2, "%s\n",strerror(errno));
    strcat(sTemp, sTemp2);
    error(sTemp);
    return 0;
  }

  if(fcntl(sock, F_GETFL, &flags) < 0) {
    sprintf(sTemp, "Unable to get flags for VAPI proxy socket %d", sock);
    error(sTemp);
    close(sock);
    return 0;
  }
  
  if(fcntl(sock,F_SETFL, flags & ~O_NONBLOCK) < 0) {
    sprintf(sTemp, "Unable to set flags for VAPI proxy socket %d to value 0x%08x", sock,flags | O_NONBLOCK);
    error(sTemp);
    close(sock);
    return 0;
  }

  memset(&sin,0,sizeof(sin));
  sin.sin_family = host->h_addrtype;
  memcpy(&sin.sin_addr, host->h_addr_list[0], host->h_length);
  sin.sin_port = htons(port);

  if((connect(sock, (struct sockaddr*)&sin, sizeof(sin)) < 0) 
     && errno != EINPROGRESS) {
    
    sprintf(sTemp, "connect failed  errno = %d\n", errno);
    sprintf(sTemp2, "%s\n", strerror(errno));
    close(sock);
    strcat(sTemp, sTemp2);
    error(sTemp); 
    return 0;
  }

  if(fcntl(sock,F_SETFL, flags | O_NONBLOCK) < 0) {
    sprintf(sTemp, "Unable to set flags for VAPI proxy socket %d to value 0x%08x", sock, flags | O_NONBLOCK);
    error(sTemp);
    close(sock);
    return 0;
  }

  if(setsockopt(sock,protocol->p_proto,TCP_NODELAY,&on_off,sizeof(int)) < 0) {
    sprintf(sTemp,"Unable to disable Nagel's algorithm for socket %d.\nsetsockopt", sock);
    error(sTemp);
    close(sock);
    return 0;
  }

  return sock;
}

/* Check if a vapi id is "good" */
static inline int good_id (unsigned long id)
{
  return ((id >= first_id) && (id <= last_id));
}


/* Initialize a new connection to the or1k board, and make sure we are
   really connected.  */

int
vapi_init (char *port_name, unsigned long id)
{
  first_id = id;
  last_id = id + num_vapi_ids - 1;
  /* CZ 24/05/01 - Check to see if we have specified a remote
     VAPI interface or a local one. It is remote if it follows
     the URL naming convention vapi://<hostname>:<port> */
  if(!strncmp(port_name,"vapi://",7)) {
    char *port;
    char hostname[256];

    port = strchr(&port_name[7], ':');
    if(port) {
  	  int len = port - port_name - 7;
  	  strncpy(hostname,&port_name[7],len);
  	  hostname[len] = '\0';
  	  port++;
  	} else
      strcpy(hostname,&port_name[7]);

    /* Interface is remote */
    if(!(vapi_fd = connect_to_server(hostname,port))) {
  	  char sTemp[256];
  	  sprintf(sTemp,"Can not access VAPI Proxy Server at \"%s\"",
  		  &port_name[5]);
  	  error(sTemp);
  	}
    printf("Remote or1k testing using %s, id 0x%x\n", port_name, id);
    if (vapi_write_stream (vapi_fd, &id, sizeof (id)))
      return 1;
  } else
    return 1;
  return 0;
}

void
vapi_done ()  /* CZ */
{
  int flags = 0;
  struct linger linger;
  char sTemp[256];

  linger.l_onoff = 0;
  linger.l_linger = 0;

  /* First, make sure we're non blocking */
  if(fcntl(vapi_fd, F_GETFL,&flags) < 0) {
    sprintf(sTemp,"Unable to get flags for VAPI proxy socket %d", vapi_fd);
    error(sTemp);
  }
  if(fcntl(vapi_fd, F_SETFL, flags & ~O_NONBLOCK) < 0) {
    sprintf(sTemp,"Unable to set flags for VAPI proxy socket %d to value 0x%08x", vapi_fd, flags | O_NONBLOCK);
    error(sTemp);
  }
  
  /* Now, make sure we don't linger around */
  if(setsockopt(vapi_fd,SOL_SOCKET,SO_LINGER,&linger,sizeof(linger)) < 0) {
    sprintf(sTemp,"Unable to disable SO_LINGER for VAPI proxy socket %d.", vapi_fd);
    error(sTemp);
  }

  close(vapi_fd);
}

/* Writes an unsigned long to server */
void vapi_write(unsigned long data)
{
  vapi_write_with_id (0, data);
}

/* Writes an unsigned long to server with relative ID */
void vapi_write_with_id(unsigned long relative_id, unsigned long data)
{
  printf ("WRITE [%08x, %08x]\n", first_id + relative_id, data);
  if (write_packet (first_id + relative_id, data))
    perror ("write packet");
}

/* Reads an unsigned long from server */
unsigned long vapi_read()
{
  unsigned long relative_id, data;
  vapi_read_with_id (&relative_id, &data);
  return data;
}

/* Reads an unsigned long and vapi id from server */
void vapi_read_with_id(unsigned long *relative_id, unsigned long *data) 
{
  unsigned long id;
  if(read_packet (&id, data)) {
    if(vapi_fd) {
      perror("vapi read");
      close(vapi_fd);
      vapi_fd = 0;
    }
    exit (2);
  }
  if (!good_id (id)) {
    if (last_id > first_id)
      fprintf (stderr, "ERROR: Invalid id %x, expected %x..%x", id, first_id, last_id);
    else
      fprintf (stderr, "ERROR: Invalid id %x, expected %x.", id, first_id);
    exit (1);
  }
  printf ("READ [%08x, %08x]\n", id, *data);
  *relative_id = id - first_id;
}

/* Polls the port if anything is available and if do_read is set something is read from port. */
int vapi_waiting ()
{
  struct pollfd fds[1];

  if(vapi_fd) {
    fds[0].fd = vapi_fd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
  } else
    return;

  while(1) {
    switch(poll(fds, 1, 0)) {
    case -1:
      if(errno == EINTR)
        continue;
      perror("poll");
      break;
    case 0: /* Nothing interesting going on */
      return 0;
    default:
      return 1;
    } /* End of switch statement */
  } /* End of while statement */
}

int main (int argc, char *argv[]) {
  unsigned long id, data;
  if (argc != 3) {
    printf ("Usage: %s URL ID\n", argv[0]);
    printf ("%s vapi://localhost:9998 0x12345678\n", argv[0]);
    return 2;
  }
  id = atol (argv[2]);
  if (sscanf (argv[2], "0x%x", &id)) {
    if (vapi_init(argv[1], id))
      return 1;
  } else {
    fprintf (stderr, "Invalid vapi_id\n", argv[2]);
    return 2;
  }

  if (vapi_main ()) {
    fprintf (stderr, "TEST FAILED.\n");
    return 1;
  }
  printf ("Test passed.\n");

  vapi_done();
  return 0;
}
