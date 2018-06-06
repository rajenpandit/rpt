#include "tcp_socket.h"
#include <iostream>
#include <string>
#include <cstring>
#include <arpa/inet.h>
using namespace rpt;
bool tcp_socket::create(int port){
	return create(port,NULL);
}
bool tcp_socket::create(int port,const char* ip){
	addrinfo* rp;
	memset (&_addrinfo, 0, sizeof (_addrinfo));
	_addrinfo.ai_family = AF_UNSPEC; /*Returns IPV4 and IPV6 choices*/
	_addrinfo.ai_socktype = SOCK_STREAM; /* A TCP Socket */
	_addrinfo.ai_flags = AI_PASSIVE; /* All Interfaces */
	std::string s_port = std::to_string(port);
	if(_result)
	{
		freeaddrinfo(_result);
		_result = nullptr;
	}
	if(getaddrinfo (ip, s_port.c_str(), &_addrinfo, &_result) != 0)
		return false;
	for(rp = _result; rp != NULL ; rp = rp->ai_next){
		int fd;
		if((fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) != -1){
			if(fd > FD_SETSIZE)
				return false;
			memcpy(&_addrinfo,rp, sizeof(_addrinfo));
			_fd = fd;
			break;
		}
	}
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
		char ipstr[INET6_ADDRSTRLEN];
		struct sockaddr_in *s = (struct sockaddr_in *)&in_addr;
		inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
		std::string client_addr(ipstr);
		client_addr.append(":");
		client_addr.append(std::to_string(ntohs(s->sin_port)));
		client_socket._client_addr = client_addr;
		
		client_socket._fd = infd;
		client_socket._is_connected = true;
		return true;
	}
	catch(const std::bad_cast& e){
		return false;
	}	
}
bool tcp_socket::connect(long timeout){
	if(timeout > 0)
	{
		if(!set_block_state(false))
			return false;
	}
	int status = ::connect(_fd,_addrinfo.ai_addr,_addrinfo.ai_addrlen);
	if( status == -1 )
	{
		if ( errno != EINPROGRESS )
			return false;
		if(timeout > 0)
		{
			fd_set set;
			FD_ZERO(&set);
			FD_SET(_fd, &set);
			int t=0;
			for(t=0; t<timeout ; ++t){
				struct timeval tv;
				int error;
				socklen_t len = sizeof (error);
				tv.tv_sec = 1;
				tv.tv_usec = 0;
				int res=select(FD_SETSIZE,NULL,&set, NULL, &tv);
				if(res != 0)
				{
					if(getsockopt(_fd,SOL_SOCKET,SO_ERROR,&error,&len) == -1 )
						return false;
					if(error == 0)
						break;
					else
						return false;
				}
			}
			if(t == timeout)
				return false;
		}
		else{
			return false;
		}
	}
	_is_connected = true;
	return true;
}

int tcp_socket::send(const void *buffer, size_t size, int flags){
	return ::send(_fd,buffer,size,flags);
}
int tcp_socket::receive(void *buffer,size_t size,bool block){
	if(block){
		return recv(_fd,buffer,size,MSG_WAITALL);
	}
	else{
		return recv(_fd,buffer,size,MSG_DONTWAIT);
	}
}
bool tcp_socket::close(){
	if(::close(_fd)==-1)
		return false;
	_is_connected=false;	
	_fd = 0;
	return true;
}
const std::string& tcp_socket::get_client_addr() const{
	return _client_addr;
}
