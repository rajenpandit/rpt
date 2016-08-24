#include "thread_pool.h"
#include <ctime>
#include <iomanip>

using namespace rpt;
thread_pool::thread_pool(int pool_size):
	_pool_size(pool_size),
	_task_queue([](const std::shared_ptr<task_base>& lhs, const std::shared_ptr<task_base>& rhs)->bool{
				return (lhs->get_execution_time_point() > rhs->get_execution_time_point());
			})
{
	if(_pool_size==0)
		_pool_size=1;
	_stop_threads=false;

}
void thread_pool::start(){
	std::lock_guard<std::mutex> lk(_queue_mutex);
	if(_is_started == false)
	{
		for(unsigned int i=0; i< _pool_size; ++i){
			_threads.push_back(std::thread(&thread_pool::run,this));
		}
		_is_started = true;
	}
}
void thread_pool::stop(){
	{
		std::lock_guard<std::mutex> lk(_queue_mutex);
		_stop_threads = true;;
		_is_started = false;
	}
	_cv.notify_all();
	for(auto& t : _threads){
		t.join();
	}
}
void thread_pool::add_task(std::shared_ptr<task_base> task,std::chrono::milliseconds waiting_period){
	{
		std::lock_guard<std::mutex> lk(_queue_mutex);
		task->set_waiting_period(waiting_period);
		_task_queue.push(task);
	}
	_cv.notify_one();
}
void thread_pool::run(){
	while(true)
	{
		std::shared_ptr<task_base> t;
		{
			std::unique_lock<std::mutex> lk(_cv_mutex);
			_cv.wait(lk, [this]{return (!_task_queue.empty() || _stop_threads);});	
			std::lock_guard<std::mutex> lkg(_queue_mutex);
			if(_stop_threads){
				while(!_task_queue.empty()){
					_task_queue.pop();
				}
				return;
			}
			t = _task_queue.top();
			_task_queue.pop();
		}
		{
			if(t->get_execution_time_point() > std::chrono::system_clock::now()){
				std::unique_lock<std::mutex> lk(_cv_mutex);
				auto duration =  t->get_execution_time_point()-std::chrono::system_clock::now();
				{
					std::lock_guard<std::mutex> lk(_queue_mutex);
					_task_queue.push(t);
				}
				_cv.wait_for(lk,duration);
				continue;

			}

		}
		try{
/*
			std::time_t exc_c = std::chrono::system_clock::to_time_t(t.get_execution_time_point());
			std::time_t now_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			std::cout<<"Time Point:"<< std::put_time(std::localtime(&exc_c), "%F %T")<<std::endl;
			std::cout<<"Now:"<< std::put_time(std::localtime(&now_c), "%F %T")<<std::endl;

*/
			if(t!=nullptr)
				(*t)();
		}
		catch(std::exception & e){
		}
	}
}
