#ifndef __SOCKET_H__04032016_RAJEN__
#define __SOCKET_H__04032016_RAJEN__

#include <memory>
#include <iostream>
#include <mutex>
#include <unistd.h>
#include <fcntl.h>
#include "socket_base.h"
namespace rpt{
class socket{
public:
#if 0
	socket(socket_base&& soc) : _socket(std::make_shared<socket_base>(std::move(soc))){
	}
#endif
	socket(const std::unique_ptr<socket_base>& soc) : _socket(std::move(soc)){
	}
	socket(const socket& socket){
	}
	void operator = (const socket& socket){
	}
	socket(socket&& socket){
	}
	void operator = (socket&& socket){
	}
	~socket(){
		//std::cout<<"Destructor called"<<std::endl;
	}
	operator int (){
		return _socket->get_fd();
	}
public:
	socket_base& operator * (){
		return *_socket;
	}

public: //NOTE: All  function must be inherited by derived class
	const int get_fd(){
		return _socket->get_fd();	
	}
	bool create(int port){
		return _socket->create(port);
	}
	bool create(int port,const char *ip){
		return _socket->create(port,ip);
	}
	bool bind(bool reuse_add=true, bool keep_alive=true, bool no_delay=true){
		return _socket->bind(reuse_add,keep_alive,no_delay);
	}
	bool listen(int backlog){
		return _socket->listen(backlog);
	}
	bool accept(socket &socket){
		return _socket->accept(*socket);
	}
	bool connect(){
		return _socket->connect();
	}
	int send(const void *buffer, size_t size, int flags=0){
		return _socket->send(buffer,size,flags);
	}
	int receive(void *buffer,size_t size,bool block=true){
		return _socket->receive(buffer, size, block);
	}
	bool close(){
		return _socket->close();
	}
	bool is_connected(){
		return _socket->is_connected();
	}
public:
	std::mutex _mutex;
	bool _is_connected;
private:
	std::unique_ptr<socket_base> _socket;
};
}
#endif
