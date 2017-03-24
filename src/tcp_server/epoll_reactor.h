#ifndef __EPOLL_REACTOR_H__26022016_RAJEN__
#define __EPOLL_REACTOR_H__26022016_RAJEN__
#include <exception>
#include <string>
#include <sstream>
#include <sys/epoll.h>
#include <functional>
#include <map>
#include <memory>
#include <iostream>
#include <thread>
#include "fdbase.h"
namespace rpt{
/*!
 * epoll_call_back class provides an interface to epoll_reactor 
 * to maintain lifetime for a socket / fdbase.
 * @author Rajendra Pandit (rajenpandit)
 * @bug No known bugs
 */
class epoll_call_back
{
public:
	epoll_call_back( std::shared_ptr<fdbase> fd, std::function<void(std::shared_ptr<fdbase>fdbase, unsigned int)> call_back_fun ) : _fd(fd), 
	_call_back_fun(call_back_fun){
	}
	~epoll_call_back(){
	}
public:
	void reset(const std::shared_ptr<fdbase>& fd, const std::function<void(std::shared_ptr<fdbase>fdbase, unsigned int)>& call_back_fun){
		_fd = fd;	
		_call_back_fun = call_back_fun;
	}
	void operator () (unsigned int events){
		_call_back_fun(_fd, events );
	}
	std::shared_ptr<fdbase> get_fd(){
		return _fd;
	}
private:
	std::shared_ptr<fdbase> _fd;
	std::function<void(std::shared_ptr<fdbase>, unsigned int)> _call_back_fun;
};









/*!
* epoll_reactor_exception class inherites property of std::exception,
* which shall be used by epoll_reactor to handle runtime exceptions.
*/
class epoll_reactor_exception : public std::exception{
public:
	epoll_reactor_exception(const std::string& error) : _error(error){}
	virtual const char* what() const noexcept override{
		return _error.c_str();
	}
private:
	std::string _error;
};








#include <sys/resource.h>
#include <limits>
/*! 
* epoll_reactor class provides an interface to register a file descriptor (fd)
* along with a associated call back function.
* When run methode is invoked, this will start monitoring registered file descriptors
* for any read/write operations. When a file descriptor is ready for read/write, 
* this will invoke callback function associated to the file descriptor.
*/

class epoll_reactor{
public:
	epoll_reactor(unsigned int max_events):_max_events(max_events){
		if(_max_events == 0)
			_max_events = 1;
		_efd = epoll_create1 (0);
		if (_efd == -1){
			throw epoll_reactor_exception("Error:"+std::to_string(_efd));
		}
		_is_running=false;
	}
	~epoll_reactor(){
	}
public:
	/*! associates a callback function to a file descriptor, 
	   epoll_reactor invokes call_back function once file descriptor 
	   is ready for read/write*/
	bool register_descriptor(std::shared_ptr<fdbase> fdb, std::function<void(std::shared_ptr<fdbase>, unsigned int)> call_back_fun);
	std::shared_ptr<fdbase> remove_descriptor(int fd);
	/*! Starts epoll_reactor */
	bool run(bool block=true);
	void run_impl();
	/*! Stops epoll_reactor */
	void stop(){
		_is_running=false;
		if(_thread.joinable()){
			_thread.join();
		}
	}
private:
	int getfdlimit(){
		struct rlimit rl;
		if(getrlimit(RLIMIT_NOFILE, &rl)==-1)
			return std::numeric_limits<int>::max();
		return rl.rlim_max;
		
	}
private:
	int _efd;
	bool _is_running;
	unsigned int _max_events;
	std::thread _thread;
	std::map<int,std::shared_ptr<epoll_call_back>> _callback_map;
	std::mutex _mutex;
};
}
#endif
