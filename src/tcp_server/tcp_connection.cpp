#include "tcp_connection.h"
#include "server_socket.h"
#include <mutex>
#include <experimental/string_view>
#include <chrono>
using namespace rpt;
void tcp_connection::remove_client(__attribute__((unused)) int fd){
	++_max_connection;
	_reactor.remove_descriptor(fd); //moved to epoll_reactor
}
/* 
 * tcp_connection::client_handler function will be called whenever client socket will receive an event.
 */
void tcp_connection::client_handler(std::shared_ptr<fdbase> fdb, unsigned int events){
	std::shared_ptr<client_iostream> client = std::dynamic_pointer_cast<client_iostream>(fdb);
	if(client != nullptr){
		if(events & (EPOLLRDHUP | EPOLLHUP)){
			std::lock_guard<std::mutex> lk(*client, std::adopt_lock);
			client->close();
		}
		else if(client->get_mutex().try_lock())
		{
			_threads->add_task(make_task(std::bind([=](){
						//std::lock_guard<std::mutex> lk(client);
						std::lock_guard<std::mutex> lk(*client, std::adopt_lock);
						client->notify_read(events);
						})));
		}
	}
}
void tcp_connection::release_connection(client_iostream* client){
	if(client != nullptr){
		std::string key = client->get_id();
		auto it = _client_map.find(key);
		if( it != _client_map.end()){
			{
				std::lock_guard<std::mutex> lk(_client_map_mutex);
				auto& connection_que = it->second;
				std::shared_ptr<fdbase> fd = _reactor.remove_descriptor(client->get_fd());
				if(fd != nullptr){
					connection_que.push(fd);
				}
			}
		}
	}
}
/* tcp_connection::accept funtion will be called when server accepts a new connection from client 
 * once connection is accepted, the new connection client's socket will be passed as a parameter by
 * user's callback function _accept.
 */
void tcp_connection::accept(std::shared_ptr<fdbase> fdb, unsigned int events){
	//socket_base client_socket=_socket.get_new_socket();
		if(fdb->get_mutex().try_lock())
		{
			_threads->add_task(make_task(std::bind([=](){
						std::lock_guard<std::mutex> lk(fdb->get_mutex(), std::adopt_lock);
						accept_impl(fdb,events);
						})));
		}
	
}
void tcp_connection::accept_impl(std::shared_ptr<fdbase> fdb,__attribute__((unused)) unsigned int events){

	try{
		while(true){
			//	std::lock_guard<std::mutex> lk(*fdb);
			server_socket& ssocket = (dynamic_cast<server_socket&>(*fdb));
			std::shared_ptr<client_iostream> client=_acceptor->get_new_client();
			if(!ssocket->accept(*client))
			{
				break;
			}
			//_threads->add_task(make_task([=](){
				if(_max_connection == 0){
					std::experimental::string_view msg = "Connection Denied, Server busy!!!\r\n";
					client->write(msg.data(),msg.length());
					client->close();
					return;
				}
				using std::placeholders::_1;
				using std::placeholders::_2;
				client->set_thread_pool(_threads);
				client->register_close_handler(std::bind(&tcp_connection::remove_client,this,_1));
				_reactor.register_descriptor(client,std::bind(&tcp_connection::client_handler,this,_1,_2));
				_acceptor->notify_accept(client, acceptor_base::ACCEPT_SUCCESS);
				--_max_connection;
		//	}));

		//	std::this_thread::sleep_for(std::chrono::seconds(5));
		}
	}
	catch(const std::bad_cast& e){
	}
}
void tcp_connection::set_error(connection_status_t status, error_t error){
	_connection_status=status;
	_connection_error=error;
	if(_error_handler)
		_error_handler(_connection_status, _connection_error);
}

void tcp_connection::start_listening(std::shared_ptr<acceptor_base> acceptor,const endpoint &e,unsigned int max_connection,
		bool block) noexcept{
	_acceptor = acceptor;	
	_max_connection = max_connection;
	std::unique_ptr<socket_base> socket = _socket_factory->get_socket();
	if(e.ip.length()){
		if(!socket->create(e.port,e.ip.c_str())){	
			set_error(SYNC_SENT,CREATE_FAILED);	
		}
	}	
	else{
		if(!socket->create(e.port)){
			set_error(SYNC_SENT,CREATE_FAILED);	
		}
	}
	if(!socket->bind()){
		set_error(SYNC_SENT,BIND_FAILED);	
	}	
	if(!socket->listen(e.backlog)){
		set_error(SYNC_SENT,LISTEN_FAILED);	
	}
	socket->set_block_state(false);

	using std::placeholders::_1;
	using std::placeholders::_2;
	_reactor.register_descriptor(std::make_shared<server_socket>(std::move(socket)),std::bind(&tcp_connection::accept,this,_1,_2));
	start_reactor(block);
}

