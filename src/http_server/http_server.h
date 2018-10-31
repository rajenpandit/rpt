#ifndef __HTTP_SERVER_H__07032016_RAJEN__
#define __HTTP_SERVER_H__07032016_RAJEN__

#include "buffered_client_iostream.h"
#include "tcp_connection.h"
#include "socket_factory.h"
#include "tcp_socket.h"
#include "thread_pool.h"
#include "acceptor_base.h"
#include "http_servlet_request.h"
#include "http_servlet_response.h"
#include "http_servlet.h"
#include "http_connection.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <memory>
#include <mutex>
namespace rpt{
class http_server{
public:
	class http_acceptor : public acceptor_base{
		public:
			http_acceptor(std::map<std::string, std::shared_ptr<http_servlet> >& servlets_map, 
					const std::shared_ptr<socket_factory>& sf, http_server* server) :
				_servlets_map(servlets_map),_socket_factory(sf),_server(server){
			}
		public:
			virtual std::shared_ptr<client_iostream> get_new_client() override{
				auto client = std::make_shared<http_connection>(_socket_factory);
				client->register_callback(std::bind(&http_server::notify,_server,
							std::placeholders::_1,std::placeholders::_2));
				return client;
			}
			virtual void notify_accept(std::shared_ptr<client_iostream> client, acceptor_status_t status) override{
				if(status == acceptor_base::ACCEPT_SUCCESS){
					std::lock_guard<std::mutex> lk(_client_map_mutex);
					_client_map[client->get_fd()] = client;
					client->register_close_handler(std::bind(&http_acceptor::close,this,std::placeholders::_1));
				}
			}
		public:
			void close(int fd){
				std::lock_guard<std::mutex> lk(_client_map_mutex);
				auto it = _client_map.find(fd);	
				if(it != _client_map.end()){
					_client_map.erase(it);
				}	
			}
		private:
			std::map<int,std::shared_ptr<client_iostream>> _client_map;	
			std::mutex _client_map_mutex;
			std::map<std::string, std::shared_ptr<http_servlet> >& _servlets_map;
			std::shared_ptr<socket_factory> _socket_factory;
			http_server* _server;
	};


	class endpoint{
	public:
		endpoint(int port, const std::string& ip="", int backlog=5){
			this->port = port;
			this->ip = ip;
			this->backlog = backlog;
		}
	private:
		int port;
		std::string ip;
		int backlog;
	friend class http_server;
	};
public:
	http_server(const std::shared_ptr<socket_factory>& sf, unsigned int max_thread) : _socket_factory(sf),
	_tcp_connection(sf, max_thread){
	}
	http_server(const std::shared_ptr<socket_factory>& sf, std::shared_ptr<thread_pool> threads) : _socket_factory(sf),
	_tcp_connection(sf, threads){
	}
public:
	void register_servlet(const std::string& pattern, std::shared_ptr<http_servlet> servlet){
		_servlets_map[pattern] = servlet;
	}
	void start(const endpoint &e,unsigned int max_connection=std::numeric_limits<unsigned int>::max(),bool block=true){
		_tcp_connection.start(std::make_shared<http_acceptor>(_servlets_map,_socket_factory,this),
				tcp_connection::endpoint(e.port,e.ip,e.backlog),max_connection,block);
	}	
private:
	void notify(std::shared_ptr<http_servlet_request> request, std::shared_ptr<http_servlet_response> response);	
	std::shared_ptr<http_servlet> find_servlet(const std::string& request_uri);
private:
	std::map<std::string, std::shared_ptr<http_servlet> > _servlets_map;
	std::shared_ptr<socket_factory> _socket_factory;
	tcp_connection _tcp_connection;
};
}
#endif
