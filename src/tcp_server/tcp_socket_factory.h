#ifndef __TCP_SOCKET_FACTORY_H__30052016_RAJEN__
#define __TCP_SOCKET_FACTORY_H__30052016_RAJEN__
#include "socket_factory.h"
#include "tcp_socket.h"
#ifdef __SSL_SOCKET__
#	include "tcp_sslsocket.h"
#endif
#include <memory>
#include <iostream>
namespace rpt{
class tcp_socket_factory : public socket_factory{
public:
#ifdef __SSL_SOCKET__
	tcp_socket_factory() : _is_ssl(false){
	}
	tcp_socket_factory(const std::string& cert_path, const std::string& key_path) 
		: _cert_path(cert_path), _key_path(key_path), _is_ssl(true){
	}
#endif
public:
	std::unique_ptr<socket_base> get_socket() const override{
#ifdef __SSL_SOCKET__
		if(_is_ssl)
			return std::make_unique<tcp_sslsocket>(_cert_path,_key_path);
#endif
		return std::make_unique<tcp_socket>();
	}
private:
#ifdef __SSL_SOCKET__
	bool _is_ssl;
#endif
	std::string _cert_path;
	std::string _key_path;
};
}
#endif

