#ifndef __CLIENT_IOSTREAM_H__05032016_RAJEN_H__
#define __CLIENT_IOSTREAM_H__05032016_RAJEN_H__
#include "fdbase.h"
#include "vector"
#include "socket_base.h"
#include "thread_pool.h"
#include <memory>
#include <sys/ioctl.h>
namespace rpt{
class client_iostream : public fdbase{
public:
	client_iostream(std::unique_ptr<socket_base> soc,std::function<void(const std::vector<char>& data)> notify) : 
		fdbase(soc->get_fd()),_socket(std::move(soc)),_notify(notify){
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
	virtual bool write(const std::string& data){
		return write(data.c_str(),data.size());
	}
	virtual bool write(const std::vector<char>& data){
		if(data.size())
			return write(&data[0],data.size());
		return false;
	}
	virtual void notify_read(__attribute__((unused)) unsigned int events){
		int fd = _socket->get_fd();
		while(true){
			int len=1;
			ioctl(fd, FIONREAD, &len);
			std::vector<char> data(len);
			if(!_socket->receive(&data[0], len, false)){
				break;
			}
			if(data.empty())
				break;
			notify(data);
		}
	}
	void notify(const std::vector<char>& v){
		_data.insert(_data.end(),v.begin(),v.end());
		if(_notify!=nullptr){
			if(rcv_condition())
			{
				std::vector<char> data;	
				get_data(data);
				_notify(data);
			}
		}
		else
			_cv.notify_one();
	}
protected:
	virtual bool rcv_condition(){
		return !_data.empty();
	}
	virtual void get_data(std::vector<char>& data){
		data.insert(data.end(),_data.begin(),_data.end());
		_data.clear();
	}
	virtual void get_data(std::string& data){
		data.insert(data.end(),_data.begin(),_data.end());
		_data.clear();
	}
private:
	bool check_condition(){
		return rcv_condition();
	}
public:
	std::string read(){
		if(_thread_pool==nullptr){
			std::unique_lock<std::mutex> lk(_cv_mutex);
			_cv.wait(lk, [this]{return rcv_condition();});
			std::string data;
			get_data(data);
			return data;
		}	
		else
		{
			_thread_pool->context_yield(&client_iostream::check_condition,this);	
			std::string data;
			get_data(data);
			return data;
		}
	}	
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
	void set_thread_pool(const std::shared_ptr<thread_pool>& tp){
		_thread_pool = tp;
	}

protected:
	std::vector<char> _data;
	std::unique_ptr<socket_base> _socket;
private:
	std::string _id;
	std::shared_ptr<thread_pool> _thread_pool;
	std::function<void(const std::vector<char>& data)> _notify;
	std::vector<std::function<void(int)>> _close_handlers;
	std::condition_variable _cv;
	std::mutex _cv_mutex;
};
}
#endif
