#ifndef __SERVER_SOCKET_H_05032016_RAJEN__
#define __SERVER_SOCKET_H_05032016_RAJEN__

#include "fdbase.h"
#include "socket_base.h"
#include <memory>
namespace rpt{
class server_socket : public fdbase{
public:
	server_socket(std::unique_ptr<socket_base> soc) : fdbase(soc->get_fd()), _socket(std::move(soc)){
		
	}
	server_socket(const server_socket&) = delete;
	void operator = (const server_socket&) = delete;
	server_socket(server_socket&& s_socket) : fdbase(s_socket.get_fd()){
		swap(*this, s_socket);
	}
	void operator = (server_socket&& s_socket){
		swap(*this, s_socket);
	}
	~server_socket(){
	}
public:
	friend void swap(server_socket& s_socket1, server_socket& s_socket2){
		std::lock(s_socket1._mutex, s_socket2._mutex);	
		std::lock_guard<std::mutex> lk1(s_socket1._mutex, std::adopt_lock);
		std::lock_guard<std::mutex> lk2(s_socket2._mutex, std::adopt_lock);
		
		std::swap(s_socket1._socket,s_socket2._socket);
	}
public:
	socket_base* operator -> (){
		return _socket.get();
	}
private:
	std::unique_ptr<socket_base> _socket;
};
}
#endif
