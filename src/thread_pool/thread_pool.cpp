#include "thread_pool.h"
#include <ctime>
#include <iomanip>

#pragma GCC push_options
#pragma GCC optimize ("O0")
using namespace rpt;
std::mutex thread_pool::context_yield_helper::_mutex;
thread_pool::thread_pool(int pool_size):
	_pool_size(pool_size),
	_task_queue([](const std::shared_ptr<task_base>& lhs, const std::shared_ptr<task_base>& rhs)->bool{
				return (lhs->get_execution_time_point() > rhs->get_execution_time_point());
			}),
	_internal_task_queue([](const std::shared_ptr<task_base>& lhs, const std::shared_ptr<task_base>& rhs)->bool{
				return (lhs->get_execution_time_point() > rhs->get_execution_time_point());
			})
{
	if(_pool_size==0)
		_pool_size=1;
	_stop_threads=false;
	_is_started=false;
	_is_cy_event=false;
	start();
}
void thread_pool::start(){
	std::lock_guard<std::mutex> lk(_queue_mutex);
	if(_is_started == false)
	{
		for(unsigned int i=0; i< _pool_size; ++i){
			std::thread t(&thread_pool::run,this);
			_thread_ids.push_back(t.get_id());
			_threads.push_back(std::move(t));
		}
		_is_started = true;
	}
}
void thread_pool::join(){
	{
		std::lock_guard<std::mutex> lk(_queue_mutex);
		if( !_is_started )
			return;
		_stop_threads = true;;
		_is_started = false;
	}
	_is_cy_event=true;
	_cv.notify_one();
	_is_cy_event=false;
	_cv.notify_all();
	for(auto& t : _threads){
		t.join();
	}
}
bool thread_pool::has_thread_ownership(){
	std::thread::id this_id = std::this_thread::get_id();
	for(auto id : _thread_ids){
		if(id==this_id)
			return true;	
	}
	return false;
}
bool thread_pool::context_yield(std::shared_ptr<context_yield_helper> context_yield_data){
	long int sp,si,di,bx,cx;
	PUSH_REGS(sp,si,di,bx,cx);
	bool retVal = context_yield_impl(context_yield_data);
	POP_REGS(sp,si,di,bx,cx);
	return retVal;
}
bool thread_pool::context_yield_impl(std::shared_ptr<context_yield_helper> context_yield_data_p){
	{
		std::lock_guard<std::mutex> lk(_cy_mutex2);
		std::this_thread::sleep_for(1ms);
	}
	std::shared_ptr<context_yield_helper> context_yield_data = context_yield_data_p;
	long int x=0, y=0,sp=0,bp=0;
	PUSH(x,y);
	GET_PTR(sp,bp);	
	context_yield_data->lock();
	if(context_yield_data->get_status() == -1){
		context_yield_data->unlock();
		return true;
	}
	if(context_yield_data->get_status()==1){
		context_yield_data->swap_ra(x);
		context_yield_data->swap_ba(y);
		context_yield_data->swap_sp(sp);
		context_yield_data->swap_bp(bp);

		context_yield_data->unlock();
	}
	else if(has_thread_ownership()==false)
	{
		context_yield_data->unlock();
		while(true){
			if(!(*context_yield_data)())
			{

				if(!invoke_task(1ns))
				{
					break;
				}
			}
			else{
				break;
			}
		}
	}
	else
	{
		context_yield_data->set_ra(x);
		context_yield_data->set_ba(y);
		context_yield_data->set_sp(sp);	
		context_yield_data->set_bp(bp);	

		context_yield_data->unlock();
#ifdef __STACK_IMPL__
		{
			std::lock_guard<std::mutex> lk(_cy_mutex2);
			std::this_thread::sleep_for(1ms);
		}
		char* sp;
		char* bp;
		char* p=new char[STACK_SIZE];
		GET_PTR(sp,bp);
		char* top_p=(p+STACK_SIZE-1)-((bp-sp)+8);
		std::memcpy(top_p,sp,(bp-sp)+8);
		bp=top_p+(bp-sp);
		sp=top_p;
		SET_PTR(sp,bp);
#endif
#if 0
		add_task(make_task(&thread_pool::check_cy_condition,this));
		using namespace std::chrono_literals;
		{
			std::lock_guard<std::mutex> lk(_cy_mutex);	
			_cy_wait_que.push(context_yield_data);
		}
		while(true){
			if(!context_yield_data->get_status())
			{

				if(!invoke_task(100ms))
				{
					break;
				}
			}
			else{
				break;
			}
		}
#else
		using namespace std::chrono_literals;
		{
			std::lock_guard<std::mutex> lk(_cy_mutex);	
			_cy_wait_que.push(context_yield_data);
			internal_add_task(make_task(&thread_pool::check_cy_condition,this,std::this_thread::get_id()));
		}
		while(true){
			context_yield_data->lock();
			if(context_yield_data->get_status() == 0 && _stop_threads==false)
			{
				{
					std::unique_lock<std::mutex> lk(_cy_cvmutex);
					_cv.wait(lk,[this,context_yield_data]{
							bool result =  ((*context_yield_data)() || 
								!_task_queue.empty() ||
								_stop_threads);
							return result;
							});
					_is_cy_event=false;
				}
				if(_stop_threads || context_yield_data->get_status()==1)
				{
					context_yield_data->set_status(-1);
					context_yield_data->unlock();
					break;
				}
				if(!_task_queue.empty())
				{
					context_yield_data->unlock();
					if(!invoke_task(10ms))
					{
						break;
					}
				}
				else	
					context_yield_data->unlock();
			}
			else{
				context_yield_data->set_status(-1);
				context_yield_data->unlock();
				break;
			}
		}
#endif
#ifdef __STACK_IMPL__
		std::lock_guard<std::mutex> lk(_cy_mutex2);
		GET_PTR(sp,bp);
		char* bp_temp=reinterpret_cast<char*>(context_yield_data->get_bp());
		char* sp_temp=bp_temp - (bp-sp);
		std::memcpy(sp_temp,sp,(bp-sp));
		SET_PTR(sp_temp,bp_temp);
		delete[] p;
#endif
		context_yield_data->lock();
		x = context_yield_data->get_ra();
		y = context_yield_data->get_ba();
		context_yield_data->set_status(-1);
		context_yield_data->unlock();
	}
	std::lock_guard<std::mutex> lk(_cy_mutex2);
	POP(x,y);
	return true;
}
void thread_pool::check_cy_condition(std::thread::id tid){
	if(std::this_thread::get_id() == tid)
		return;
	std::shared_ptr<context_yield_helper> cy_data;
	{
		std::lock_guard<std::mutex> lk(_cy_mutex);
		auto size = _cy_wait_que.size();
		for(size_t i = 0; i<size; ++i)
		{
			cy_data = _cy_wait_que.front();
			_cy_wait_que.pop();
			cy_data->lock();
			if(cy_data->get_status()==0 )
			{	
				if((*cy_data)()==false){
					cy_data->unlock();
					_cy_wait_que.push(cy_data);
					cy_data=nullptr;
				}
				else{
					cy_data->unlock();
					context_yield_notify(); // notify to check if another event is  present
					break;  //Continue with this thread
				}
			}
			else{
				cy_data->unlock();
				cy_data=nullptr;
			}

		}
		if(cy_data==nullptr){
			return;
		}

	}
	{
		std::lock_guard<std::mutex> lk(_cy_mutex2);
		std::this_thread::sleep_for(1ms);
	}
	long int sp,si,di,bx,cx;
	PUSH_REGS(sp,si,di,bx,cx);
	context_yield_impl(cy_data);	
	POP_REGS(sp,si,di,bx,cx);
}
void thread_pool::add_task(std::shared_ptr<task_base> task,std::chrono::milliseconds waiting_period){
	if(!is_started())
		return;
	{
		std::lock_guard<std::mutex> lk(_queue_mutex);
		task->set_waiting_period(waiting_period);
		_task_queue.push(task);
	}
	_cv.notify_one();
}
void thread_pool::internal_add_task(std::shared_ptr<task_base> task,std::chrono::milliseconds waiting_period){
	if(!is_started())
		return;
	{
		std::lock_guard<std::mutex> lk(_internal_queue_mutex);
		task->set_waiting_period(waiting_period);
		_internal_task_queue.push(task);
	}
	//_cv.notify_one();
}
bool thread_pool::invoke_task(std::chrono::nanoseconds waiting_period)
{
	bool is_waiting_defined=false;
	std::shared_ptr<task_base> t;
	{
			
		std::unique_lock<std::mutex> lk(_cv_mutex);
		if(waiting_period > 0ms)
		{
			_cv.wait_for(lk,waiting_period);
			if(_task_queue.empty()){
				return true;
			}
			is_waiting_defined=true;
		}
		else
		{
			_cv.wait(lk, [this]{
					std::lock_guard<std::mutex> lkg(_queue_mutex);
					return (!(_task_queue.empty() || _is_cy_event)
						|| _stop_threads);
					});	
		}
		std::lock_guard<std::mutex> lkg(_queue_mutex);
#if 0
		if(_stop_threads){
			while(!_task_queue.empty()){
				_task_queue.pop();
			}
			return false;
		}
#else
		if(_stop_threads && _task_queue.empty() ){
			return false;
		}
#endif
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
			return true;

		}

	}
	try{
		if(t!=nullptr)
		{
			if(is_waiting_defined)
			{
				(*t)();
			}
			else
			{
				(*t)();
			}
			
		}
	}
	catch(std::exception & e){
		std::cout<<"Excetion during thread execution:"<<e.what()<<std::endl;
	}
	return true;
}
bool thread_pool::invoke_task_helper(){
	bool result = invoke_task();
	return result;
}
void thread_pool::run(){
	while(true)
	{
		if(!invoke_task_helper())
		{
			break;
		}	
	}
}
#pragma GCC pop_options
