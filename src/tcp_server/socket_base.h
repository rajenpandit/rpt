#ifndef __SOCKET_BASE_H__02032016_RAJEN__
#define __SOCKET_BASE_H__02032016_RAJEN__

#include <memory>
#include <iostream>
#include <mutex>
#include <unistd.h>
#include <fcntl.h>
namespace rpt{
class socket_base{

public: //NOTE: All virtual function must be inherited by derived class

	virtual bool create(int port) = 0;
	virtual bool create(int port,const char *ip) = 0;
	virtual bool bind(bool reuse_add=true, bool keep_alive=true, bool no_delay=true) = 0;
	virtual bool listen(int backlog) = 0;
	virtual bool accept(socket_base &socket) = 0;
	virtual bool connect(long timeout=0) = 0;
	virtual int send(const void *buffer, size_t size, int flags=0) = 0;
	virtual int receive(void *buffer,size_t size,bool block=true) = 0;
	virtual bool close() = 0;
	virtual const std::string& get_client_addr() const = 0;
	virtual ~socket_base() {}
public:
	socket_base() : _fd(0),_is_connected(false){
        }
public:
	const int& get_fd(){
		return _fd;
	};
	bool is_connected(){
		return _is_connected;
	}
	bool set_block_state(bool blocking){
		int flags = fcntl(_fd, F_GETFL, 0);
		if (flags < 0) 
			return false;
		flags = blocking ? (flags&~O_NONBLOCK) : (flags|O_NONBLOCK);
		return (fcntl(_fd, F_SETFL, flags) == 0) ? true : false;
	}	
public:
	std::mutex _mutex;
protected:
	bool _is_connected;
	int _fd;
private:
	std::shared_ptr<socket_base> _socket;
	//std::function<void(socket_base& , unsigned int)> _event_handler;
	//std::function<void(socket_base&)> _close_event_handler;
};
}
#endif
