#ifndef __CLIENT_IOSTREAM_H__05032016_RAJEN_H__
#define __CLIENT_IOSTREAM_H__05032016_RAJEN_H__
#include "fdbase.h"
#include "vector"
#include "socket_base.h"
#include <memory>
namespace rpt{
class client_iostream : public fdbase{
public:
	client_iostream(std::unique_ptr<socket_base> soc) : fdbase(soc->get_fd()), _socket(std::move(soc)){
	}
	client_iostream(const client_iostream&) = delete;
	void operator = (const client_iostream&) = delete;
	client_iostream(client_iostream&& c_ios) : fdbase(c_ios.get_fd()){
		swap(*this, c_ios);
	}
	void operator = (client_iostream&& c_ios){
		swap(*this, c_ios);
	}
	virtual ~client_iostream(){
	}
public:
	friend void swap(client_iostream& c_ios1, client_iostream& c_ios2){
		std::lock(c_ios1._mutex, c_ios2._mutex);
		std::lock_guard<std::mutex> lk1(c_ios1._mutex, std::adopt_lock);
		std::lock_guard<std::mutex> lk2(c_ios2._mutex, std::adopt_lock);
		
		std::swap(c_ios1._socket, c_ios2._socket);
	}
public:
	socket_base* operator -> (){
		return _socket.get();
	}
public:
	bool close(){
		for(auto& close_handler : _close_handlers)
		{
			close_handler(_socket->get_fd());
		}
		_close_handlers.clear();
		return _socket->close();
	}
	virtual bool write(const void* data, size_t size){
		if(_socket->is_connected())
			return _socket->send(data, size);
		return false;
	}
	virtual void notify_read(__attribute__((unused)) unsigned int events){
		while(true){
			std::vector<char> _data;
			char ch;
			while(_socket->receive(&ch, 1, false)){
				_data.push_back(ch);
			}
			if(_data.empty())
				break;
			read(&_data[0],_data.size());
		}
	}
public:
	virtual void read(void* data, size_t size) = 0;
public:
	operator socket_base& (){
		return *_socket;
	}
	void register_close_handler(std::function<void(int)> fun){
		_close_handlers.push_back(fun);
	}
	const std::string& get_id() const{
		return _id;
	}
	void set_id(const std::string& id){
		_id=id;
	}
private:
	std::string _id;
protected:
	std::unique_ptr<socket_base> _socket;
	std::vector<std::function<void(int)>> _close_handlers;
};
}
#endif
