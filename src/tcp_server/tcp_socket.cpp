#include "tcp_socket.h"
#include <iostream>
#include <string>
#include <cstring>
using namespace rpt;
bool tcp_socket::create(int port){
	return create(port,NULL);
}
bool tcp_socket::create(int port,const char* ip){
	addrinfo* result;
	addrinfo* rp;
	memset (&_addrinfo, 0, sizeof (_addrinfo));
	_addrinfo.ai_family = AF_UNSPEC; /*Returns IPV4 and IPV6 choices*/
	_addrinfo.ai_socktype = SOCK_STREAM; /* A TCP Socket */
	_addrinfo.ai_flags = AI_PASSIVE; /* All Interfaces */
	std::string s_port = std::to_string(port);
	
	if(getaddrinfo (ip, s_port.c_str(), &_addrinfo, &result) != 0)
		return false;
	for(rp = result; rp != NULL ; rp = rp->ai_next){
		int fd;
		if((fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) != -1){
			memcpy(&_addrinfo,rp, sizeof(_addrinfo));
			_fd = fd;
			break;
		}
	}
	if(result)
		freeaddrinfo(result);
	return true;
}
bool tcp_socket::bind(__attribute__((unused)) bool reuse_add, __attribute__((unused)) bool keep_alive, __attribute__((unused)) bool no_delay){
	int enable = 1;
	if(reuse_add)
		setsockopt(_fd,SOL_SOCKET,SO_REUSEADDR,&enable,sizeof(enable));
	if(::bind(_fd, _addrinfo.ai_addr, _addrinfo.ai_addrlen) != 0)
		return false;
	return true;
}

bool tcp_socket::listen(int backlog){
	if(::listen (_fd, backlog) == -1)
		return false;
	return true;
}

bool tcp_socket::accept(socket_base &socket){
	try{
		struct sockaddr in_addr;
		socklen_t in_len;
		int infd;
		char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

		tcp_socket& client_socket = dynamic_cast<tcp_socket&>(socket);
		in_len = sizeof in_addr;
		infd = ::accept(_fd, &in_addr, &in_len);
		if(infd == -1)
			return false;
		if(getnameinfo (&in_addr, in_len,hbuf, sizeof hbuf,
					sbuf, sizeof sbuf,NI_NUMERICHOST | NI_NUMERICSERV) != 0){
			return false;
		}
		client_socket._fd = infd;
		client_socket._is_connected = true;
		return true;
	}
	catch(const std::bad_cast& e){
		return false;
	}	
}
bool tcp_socket::connect(){
	if(::connect(_fd,_addrinfo.ai_addr,_addrinfo.ai_addrlen) == -1){
		return false;
	}
	_is_connected = true;
	return true;
}
bool tcp_socket::send(const void *buffer, size_t size){
	return send(buffer,size,0);
}
bool tcp_socket::send(const void *buffer, size_t size, int flags){
	if(::send(_fd,buffer,size,flags) == -1)
		return false;
	return true;
}
bool tcp_socket::receive(void *buffer,size_t size,bool block){
	if(block){
		if(recv(_fd,buffer,size,MSG_WAITALL) <= 0)
			return false;
	}
	else{
		if(recv(_fd,buffer,size,MSG_DONTWAIT) <= 0)
			return false;
	}
	return true;
}
bool tcp_socket::close(){
	if(::close(_fd)==-1)
		return false;
	_is_connected=false;	
	return true;
}
