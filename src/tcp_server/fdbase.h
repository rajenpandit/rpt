#ifndef __FDBASE_H__05032016_H__
#define __FDBASE_H__05032016_H__
#include <mutex>
namespace rpt{
class fdbase{
public:
//	virtual socket& get_socket() = 0;	
	fdbase(const int& fd):_fd(fd){
	}
	fdbase(const fdbase&) = default;
	fdbase(fdbase&&) = default;
	virtual ~fdbase(){
	}
public:
	std::mutex& get_mutex(){
		return _mutex;
	}
	int get_fd(){
		return _fd;
	}	
public:
	operator std::mutex& (){
		return _mutex;
	}

protected:
	std::mutex _mutex;
	const int& _fd;
};
}
#endif
