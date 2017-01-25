#ifndef __TCP_SOCKET_H_05032016_RAJEN_H__
#define __TCP_SOCKET_H_05032016_RAJEN_H__
#include "socket_base.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
namespace rpt{
class tcp_socket : public socket_base{
public:
	virtual bool create(int port);
	virtual bool create(int port,const char *ip);
	virtual bool bind(bool reuse_add=true, bool keep_alive=true, bool no_delay=true);
	virtual bool listen(int backlog);
	virtual bool accept(socket_base &socket);
	virtual bool connect();
	virtual int send(const void *buffer, size_t size, int flags);
	virtual int receive(void *buffer,size_t size,bool block=true);
	virtual bool close();
private:
	addrinfo _addrinfo;
};
}
#endif
