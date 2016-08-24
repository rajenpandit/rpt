#ifndef __TASK_H_25022016_RAJEN__
#define __TASK_H_25022016_RAJEN__
#include <tuple>
#include <functional>
#include <iostream>
#include <memory>
#include <future>
#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>

/*!
 * @namespace rpt
 * @author Rajendra Pandit (rajenpandit)
 * @bug No know bugs.
*/
namespace rpt{

using namespace std::chrono_literals;
template <typename _Res> class task_future;
/** 
 * task_base class provides wrapper to its templated child clasess.
 * @author Rajendra Pandit (rajenpandit)
 * @bug No know bugs.
*/
class task_base
{
public:
	
	task_base(){/// empty constructore is provided to make few containers happy
	};


	task_base(std::unique_ptr<task_base>&& task) : _task(std::move(task)),_time_point(0ms){
	/*!
		@param task is a rvalue reference to unique_ptr pointing to a task object created by using rpt::make_task function
		@see rpt::make_task
	 */
	}

	/*! deleted copy constructor to make object non copyable */
	task_base(task_base& tb)=delete;
	/*! deleted assignment operator to make object non assignable */
	void operator=(task_base& tb)=delete; 

	/*! move constructor allow task_base to move instead of being copied */
	task_base(task_base&& tb){ 
		_task=std::move(tb._task);
		_time_point = tb._time_point;
	}
	/*! move assignment operator allow task_base object to move the assignee instead of making a copy */
	void operator=(task_base&& tb){ 
		_task=std::move(tb._task);
		_time_point = tb._time_point;
	}
	/*! Sets a waiting time period during #rpt::task assignment to #rpt::thread_pool. 
	 * The assigned task will be invoked once waiting period is over.
	 * This function is used by #rpt::thread_pool.
	 * @param waiting_period %A waiting period value in milliseconds.
	 */
	void set_waiting_period(std::chrono::milliseconds waiting_period){
		_time_point  =  std::chrono::system_clock::now() + waiting_period;

	}
	/*!
	 * Returns a std::chrono::system_clock::time_point
	 * which informs #rpt::thread_pool that when a #rpt::task has to be invoked.
	 */
	std::chrono::system_clock::time_point get_execution_time_point() const{
		return _time_point;
	}
public:
	virtual ~task_base(){
	}
	/*!
	 * %A functor, which invokes the #rpt::task's operator ().
	 */
	virtual void operator () (){
		(*_task)();
	};
protected:
	std::unique_ptr<task_base> _task;
	std::chrono::system_clock::time_point _time_point;
};

/*!
 * class task wraps a callable object along with its argument which need to be passed during function call.
 * The callable object will be invoked by using operator(). 
 */
template<typename R, typename... Ts>
class task : public task_base
{
public:
	using type = task<R,Ts...>;
	using result_type = R;
private:
/*! 
	Helper class to extract arguments from tuple and pass during function call
*/
	template <std::size_t... T>
		struct helper_index{
		};

	template <std::size_t N, std::size_t... T>
		struct helper_gen_seq : helper_gen_seq<N-1, N-1, T...>{
		};

	template <std::size_t... T>
		struct helper_gen_seq<0, T...> : helper_index<T...>{
		};
	struct call_helper{
		call_helper(task::type& p) : _p(p)
		{}
		void operator()()
		{_p.helper_func(_p._args);}
		private:
		task::type &_p;
	};
public:	
	/*! templated constructor, takes two parameters:
	 * @param fun, a function of type F
	 * @param args, arguments need to be passed to functions during invocation
	 */ 
	template <typename F>
	task(F&& func, Ts&&... args) : _pt(std::forward<F>(func)), _args(std::forward<Ts>(args)...){
	}
	/*! task object is non copyable.*/
	task(const task& t)=delete;
	/*! task object is non assignable.*/
	task& operator = (const task& t) = delete;
	/*! task object can be moved from one to another */
	task(task&& t){
		swap(*this, t);
	}
	/*! task object can be moved and assgined from one to another */
	task& operator = (task&& t){
		swap(*this, t);
	}
	~task(){
	}
private:
	template <typename... Args, std::size_t... Is>
	void helper_func(std::tuple<Args...>& tup, helper_index<Is...>){
		_pt(std::get<Is>(tup)...);
	}
	
	template <typename... Args>
	void helper_func(std::tuple<Args...>& tup){
		helper_func(tup, helper_gen_seq<sizeof...(Args)>{});
	}
public:
	std::future<R> get_future(){
		return _pt.get_future();
	}
public:
	/*! Invokes callable object */
	void operator () (){
		call_helper h(*this);
		std::call_once(_flag,h);
	}
private:
	/*! swaps two given rpt::task objects */
	friend void swap(task& a, task& b){
		//std::swap(a._function, b._function);
		std::swap(a._args, b._args);	
		std::swap(a._pt, b._pt);
		std::swap(a._flag, b._flag);
	}
private:
	std::packaged_task<R(Ts...)> _pt;
	std::tuple<Ts...> _args;
	std::once_flag _flag;
};


/*!
 * thread_pool provides asynchronous execution of assigned task. 
 * Once the task is assgined to thread_pool there is no option to
 * capture the value return by the task once it is executed.
 * task_future class enables caller to captrue the retrun value once executed.
 * thread_pool returns a task_future object, which provides interface to get the return value.
 * When task_future.get
 */
template <typename _Res>
class task_future
{
public:
	task_future(const std::shared_ptr<task_base>& task_b, std::future<_Res>&& f): _task(task_b), _future(std::move(f)){
	}
	task_future(const task_future& ) = delete;
	task_future(task_future&& tf){
		swap(*this,tf);	
	}
	void operator = (const task_future&) = delete;
	void operator = (task_future&& tf){
		swap(*this,tf);
	}
	~task_future(){
	}
	_Res get(){
		(*_task)();
		return _future.get();
	}
	std::shared_future<_Res> share(){
		return _future.share();
	}
	bool valid() const{
		return _future.valid();
	}
	void wait() const{
		_future.wait();
	}
	template< class Rep, class Period >
		std::future_status wait_for( const std::chrono::duration<Rep,Period>& timeout_duration ) const{
		return _future.wait_for(timeout_duration);
	}
	template< class Clock, class Duration >
		std::future_status wait_until( const std::chrono::time_point<Clock,Duration>& timeout_time ) const{
		return _future.wait_until(timeout_time);
	}
private:
	friend void swap(task_future& a, task_future& b){
		std::swap(a._task,b._task);
		std::swap(a._future,b._future);
	}
	std::shared_ptr<task_base> _task;
	std::future<_Res> _future;
};

/*!
 * To execute a function by using #rpt::thread_pool, The function has to be wrapped as a #rpt::task object
 * which further need to be added by using #add_task method of #rpt::thread_pool class.
 * #make_task creates #rpt::task object by wrapping funtion pointer and its arguments (if any), 
 * returns a unique_ptr to it.
 * @param fun: a function pointer, a member function or a callable object
 * @param args: arguments need to be passed during function call
 */
template<typename F, typename... Args, typename R=typename std::result_of<F(Args...)>::type>
std::unique_ptr<task<R,Args...>> make_task(F&& fun, Args... args){

        return std::unique_ptr<task<R,Args...>>(new task<R,Args...>(std::forward<F>(fun),std::forward<Args>(args)...));
}


}
#endif
