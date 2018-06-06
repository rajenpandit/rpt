#include "epoll_reactor.h"
#include <iostream>
#include <chrono>
#include <thread>
using namespace rpt;
bool epoll_reactor::register_descriptor(std::shared_ptr<fdbase> fdb, std::function<void(std::shared_ptr<fdbase>, unsigned int)> call_back_fun){
	struct epoll_event event;
	int s;
	int fd=fdb->get_fd();

	{
		std::lock_guard<std::mutex> lk(_mutex);
		auto it = _callback_map.find(fd);
		if(it != _callback_map.end()){
			_callback_map.erase(it);
		}
		auto call_back = std::make_shared<epoll_call_back>(fdb,call_back_fun);
		_callback_map.emplace(fd,call_back);
		//event.data.ptr = call_back.get();
		event.data.fd = fd;
	}
	event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
	s = epoll_ctl (_efd, EPOLL_CTL_ADD, fd, &event);
	if (s == -1)
	{
		return false;
	}
	return true;
}
std::shared_ptr<fdbase> epoll_reactor::remove_descriptor(int fd){
	std::lock_guard<std::mutex> lk(_mutex);
	/* In  kernel  versions before 2.6.9, the EPOLL_CTL_DEL operation required a non-NULL pointer in event, even though this argument is ignored. */
	std::shared_ptr<fdbase> fdb;
	epoll_ctl (_efd, EPOLL_CTL_DEL, fd, NULL);

	auto it = _callback_map.find(fd);
	if(it != _callback_map.end()){
		fdb = it->second->get_fd();
		_callback_map.erase(it);
	}
	return fdb;
}
bool epoll_reactor::run(bool block){
	if(_is_running)
		return false;
	_is_running=true;
	if(block)
		run_impl();
	else
		_thread = std::thread(&epoll_reactor::run_impl,this);
	return _is_running;
}
void epoll_reactor::run_impl(){
	struct epoll_event* events = static_cast<epoll_event*>(calloc (_max_events, sizeof(epoll_event)));
	while(true)
	{
		int n = epoll_wait (_efd, events, _max_events, 100);
#ifdef POLL_SLEEP
		int waiting_time = 100;
		std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
#endif
		for (int i = 0; i < n; i++)
		{
/*
			epoll_call_back* epoll_call_back_p = static_cast<epoll_call_back*>(events[i].data.ptr);
			int fd = epoll_call_back_p->get_fd()->get_fd();
			(*epoll_call_back_p)(events[i].events);
*/
			int fd = events[i].data.fd;
			std::shared_ptr<epoll_call_back> call_back_ptr;
			{
				std::lock_guard<std::mutex> lk(_mutex);
				auto it = _callback_map.find(fd);
				if(it != _callback_map.end()){
					call_back_ptr = it->second;
				}
			}
			if(call_back_ptr)
				(*call_back_ptr)(events[i].events);
/*
			if(events[i].events & (EPOLLRDHUP | EPOLLHUP)){
				remove_descriptor(fd);
			}
*/
		}
		if(_is_running == false){
			free(events);
			return;
		}
#ifdef POLL_SLEEP
		std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count();
		if( duration < waiting_time){
			int sleep_time = waiting_time - duration;
			std::this_thread::sleep_for(std::chrono::microseconds(sleep_time));
		}
#endif
	}
//	is_running=false;
}
