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
	virtual bool create(int port) override;
	virtual bool create(int port,const char *ip) override;
	virtual bool bind(bool reuse_add=true, bool keep_alive=true, bool no_delay=true) override;
	virtual bool listen(int backlog) override;
	virtual bool accept(socket_base &socket) override;
	virtual bool connect() override;
	virtual int send(const void *buffer, size_t size, int flags) override;
	virtual int receive(void *buffer,size_t size,bool block=true) override;
	virtual const std::string& get_client_addr() const override;
	virtual bool close() override;
private:
	addrinfo _addrinfo;
	std::string _client_addr;
};
}
#endif
