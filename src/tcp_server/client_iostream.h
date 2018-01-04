#ifndef __CLIENT_IOSTREAM_H__05032016_RAJEN_H__
#define __CLIENT_IOSTREAM_H__05032016_RAJEN_H__
#include "fdbase.h"
#include "vector"
#include "socket_base.h"
#include "thread_pool.h"
#include <memory>
#include <sys/ioctl.h>
namespace rpt{
/*!
 * class client_iostream, provides interfaces to read/write data from/to socket.
 * tcp_connection class notifies when data is present and need to be read data from socket.
 */
class client_iostream : public fdbase{
public:
	/*!
	 * clietn_iostream object will be constructed by providing a std::unique_ptr of socket_base,
	 * and a funtion pointer where received data will be notified. 
	 * @param soc: A std::uniqe_ptr to a socket of socket_base family.
	 * @param notify_function: A function pointer where received data will be posted. This is an
	 * optional parameter. By default read() memeber function can be used to get the data retrieved 
	 * from socket.
	 */
	client_iostream(std::unique_ptr<socket_base> soc,
			std::function<void(const std::vector<char>& data)> notify_function=nullptr) : 
		fdbase(soc->get_fd()),_socket(std::move(soc)),_notify(notify_function){
		_status = std::make_shared<char>(1);
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
	/*!
	 * this <CODE>operator->()</CODE> of the class provides direct interface to socket_base.
	 */
	socket_base* operator -> (){
		return _socket.get();
	}
public:
	/*!
	 * An interface to terminate session with remote client, and close the file descriptor.
	 */
	bool close(){
		*_status=0;
		if(!_socket->is_connected())
			return true;
		for(auto& close_handler : _close_handlers)
		{
			close_handler(_socket->get_fd());
		}
		_close_handlers.clear();
		_socket->close();
		if(!_socket->is_connected())
		{
			return true;;
		}
	}
	/*!
	 * An interface to get the client address(ip:port).
	 */
	const std::string& get_client_addr() const{
		return _socket->get_client_addr();
	}

	/*!
	 * An interface to read data from socket. If data is not available, this function call will be blocked.\n
	 * If thread_pool object is provided (generally it is set by tcp_connection), then caller thread will 
	 * will be freed to perform other task by invoking thread_pool::context_yield function.\n
	 * If thread_pool object is not set, then caller thread will be put into waiting state untill data is
	 * available in socket. 
	 */
	std::string read(bool block_thread=false){
		if(_thread_pool==nullptr || block_thread){
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
	/*!
	 * An interface to write data to socket.
	 * @param data: a \c void pointer to a memory location from where data has to be read.
	 * @param size: number of bytes to be read from memory location pointed by \b data.
	 */
	virtual bool write(const void* data, size_t size){
		if(_socket->is_connected())
			return _socket->send(data, size);
		return false;
	}

	/*!
	 * An interface to write a string to socket.
	 * @param data: a \c const reference to a string data, which need to be written into socket.
	 */
	virtual bool write(const std::string& data){
		return write(data.c_str(),data.size());
	}

	/*!
	 * An interface to write a binary data to socket.
	 * @param data: a \c const reference to a std::vector<char> which may contain binary data.
	 */
	virtual bool write(const std::vector<char>& data){
		if(data.size())
			return write(&data[0],data.size());
		return false;
	}
public:
	operator socket_base& (){
		return *_socket;
	}
	/*!
	 * An interface to register a callback function, where data will be posted after reading 
	 * from socket.
	 * @param notify_function: is a function pointer or a callable object.
	 * @note There is an alternative method to get the data which is retreived from socket.
	 * @see read()
	 */
	void notify_to(std::function<void(const std::vector<char>& data)> notify_function){
		_notify = notify_function;
	}
	/*!
	 * register_close_handler function registers a function pointer or a callable object.
	 * All the registered function will be invoked upon closing socket connection.
	 */
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
	/*!
	 * When remote client writes some data to socket, then tcp_connection invokes notify_read,
	 * here, data will be read from socket and store into buffer memory, which can be retrieved
	 * by using read method or providing a callback function.
	 */
#if 0
	virtual void notify_read(__attribute__((unused)) unsigned int events){
		if(!_socket->is_connected())
			return;
		int fd = _socket->get_fd();
		while(true){
			int len=1;
			ioctl(fd, FIONREAD, &len);
			if(len == 0)
				break;
			std::vector<char> data(len);
			int nbytes=0;
			if((nbytes = _socket->receive(&data[0], len, false))<=0){
				_socket->close();
				break;
			}
			data.resize(nbytes);
			if(data.empty())
				break;
			notify(data);
			if(!_socket->is_connected())
			{
				return;
			}
		}
	}
#else
	virtual void notify_read(__attribute__((unused)) unsigned int events){
		if(!_socket->is_connected())
			return;
		int fd = _socket->get_fd();
		while(true){
			int len=1;
			ioctl(fd, FIONREAD, &len);
			if(len == 0)
				break;
			std::vector<char> data(len);
			int nbytes=0;
			if((nbytes = _socket->receive(&data[0], len, false))<=0){
				_socket->close();
				break;
			}
			data.resize(nbytes);
			if(!data.empty())
			{
				std::lock_guard<std::mutex> lk(_notify_mutex);
				_data.insert(_data.end(),data.begin(),data.end());
			}
			else{
				break;
			}
			if(_thread_pool!=nullptr){
				_thread_pool->add_task(make_task([status=this->_status](std::function<void()> f){
							if( *status== 0)
							return;
							else
							{
							f();
							}
							},
							std::bind(&client_iostream::notify,this)));
			}
			else{
				notify();
			}

			if(!_socket->is_connected())
			{
				return;
			}
		}
	}
#endif
	/*!
	 * rcv_condition function checks for the condition provided by user,
	 * which tells when data need to be retrieved from buffer. 
	 * returns true, when a given condition is satisfied.
	 * returns true, if condition is not set and there is some data in buffer.
	 */
	virtual bool rcv_condition(){
		return (!_data.empty() || (!_socket->is_connected()));
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
	/*!
	 * stores data into buffer, and checks for the condition.
	 * If given condition is match and callback function is registered,
	 * then pass the data to user.
	 */
	void notify(){
		std::lock_guard<std::mutex> lk (_notify_mutex);
		while(true){
			if(_notify!=nullptr){
				if(rcv_condition()){
					std::vector<char> data;	
					get_data(data);
					_notify(data);
				}
				else{
					break;
				}
			}
			else{
				if(_thread_pool!=nullptr)
					_thread_pool->context_yield_notify();
				else
					_cv.notify_one();
				break;
			}
		}
	}

	bool check_condition(){
		return rcv_condition();
	}
protected:
	std::vector<char> _data;
	std::unique_ptr<socket_base> _socket;
	std::shared_ptr<thread_pool> _thread_pool;
private:
	std::string _id;
	std::function<void(const std::vector<char>& data)> _notify;
	std::vector<std::function<void(int)>> _close_handlers;
	std::condition_variable _cv;
	std::mutex _cv_mutex;
	std::mutex _notify_mutex;
	std::shared_ptr<char> _status;
	friend class tcp_connection;
};
}
#endif
