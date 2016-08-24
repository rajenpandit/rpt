#ifndef __TCP_SOCKET_FACTORY_H__30052016_RAJEN__
#define __TCP_SOCKET_FACTORY_H__30052016_RAJEN__
#include "socket_factory.h"
#include "tcp_socket.h"
#include <memory>
#include <iostream>
namespace rpt{
class tcp_socket_factory : public socket_factory{
public:
	std::unique_ptr<socket_base> get_socket() const override{
		return std::make_unique<tcp_socket>();
	}
};
}
#endif

