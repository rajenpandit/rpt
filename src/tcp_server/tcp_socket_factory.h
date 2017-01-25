#ifndef __TCP_SOCKET_FACTORY_H__30052016_RAJEN__
#define __TCP_SOCKET_FACTORY_H__30052016_RAJEN__
#include "socket_factory.h"
#include "tcp_socket.h"
#include "tcp_sslsocket.h"
#include <memory>
#include <iostream>
namespace rpt{
class tcp_socket_factory : public socket_factory{
public:
	tcp_socket_factory() : _is_ssl(false){
	}
	tcp_socket_factory(const std::string& cert_path, const std::string& key_path) 
		: _cert_path(cert_path), _key_path(key_path), _is_ssl(true){
	}
public:
	std::unique_ptr<socket_base> get_socket() const override{
		if(_is_ssl)
			return std::make_unique<tcp_sslsocket>(_cert_path,_key_path);
		return std::make_unique<tcp_socket>();
	}
private:
	bool _is_ssl;
	std::string _cert_path;
	std::string _key_path;
};
}
#endif

