
#ifndef __THREAD_POOL__29022016_RAJEN__
#define __THREAD_POOL__29022016_RAJEN__
#include <thread>
#include <mutex>
#include <queue>
#include <list>
#include <memory>
#include <condition_variable>
#include "task.h"
#include <functional>
#include <cstring>
#include "asm_defines.h"
#define __STACK_IMPL__
#define STACK_SIZE 1024 * 1024 * 8
namespace rpt{
/**
 * Provides interface to create a pool of threads.
 * All threads will be idle untill #rpt::task is added by using #add_task.
 * @author Rajendra Pandit (rajenpandit)
 * @bug No known bugs
 */

#pragma GCC push_options
#pragma GCC optimize ("O0")
class thread_pool
{
public:
	thread_pool(int pool_size); 
	
	thread_pool(thread_pool &) = delete;
	void operator = (thread_pool &) = delete;
	thread_pool(thread_pool&& tp){
		swap(*this, tp);
	}
	void operator = (thread_pool&& tp){
		swap(*this, tp);
	}

	~thread_pool(){
		join();
	}
public:
/**
 * Keeps track of idle and running threads 
 */
	struct counter{
		counter(int &c, std::mutex& m) : _counter(c),_mutex(m){
			_mutex.lock();
			++_counter;	
			_mutex.unlock();
		}	
		~counter(){
			_mutex.lock();
			--_counter;
			_mutex.unlock();
		}
	private:
		int& _counter;
		std::mutex& _mutex;
	};
public:
/**
 * context_yield_helper stores registers info used by a thread.
 * It helps to interchange regsiters data from one thread to another.
 */
	struct context_yield_helper{
		context_yield_helper(){
			_ra=0;
			_ba=0;
			_sp=0;
			_bp=0;
			_status=0;
		}
		void lock(){_mutex.lock();}
		void unlock(){_mutex.unlock();}
		long int get_sp(){return _sp;}
		long int get_bp(){return _bp;}
		long int get_ra(){return _ra;}
		long int get_ba(){return _ba;}
		void set_sp(long int sp){_sp = sp;}
		void set_bp(long int bp){_bp = bp;}
		void set_ra(long int ra){_ra = ra;}
		void set_ba(long int ba){_ba = ba;}
		void swap_ra(long int& ra){std::swap(_ra,ra);}
		void swap_ba(long int& ba){std::swap(_ba,ba);}
		void swap_sp(long int& sp){std::swap(_sp,sp);}
		void swap_bp(long int& bp){std::swap(_bp,bp);}
		void set_status(int s){_status=s;}
		int get_status(){return _status;}
		virtual bool operator()() = 0;
	private:
		long int _sp;
		long int _bp;
		long int _ra;
		long int _ba;
		static std::mutex _mutex;
	protected:
		int _status;
	};
/**
 * A thread context can be yield by calling context_yield function, 
 * this will make the thread free to execute some other task.
 * When @param func, retruns true, then a free thread will continue the process from where it is yield.
 * returns false, if context_yield is failed.
 * @param func: a function pointer which points to a function that returns a bool.
 * @param args: arguments to be passed to the fuction during call. This parameter is optional.
 */
	template<typename F, typename... Args,typename R = typename std::result_of<F(Args...)>::type
		,typename X = typename std::enable_if<std::is_same<R,bool>::value>::type>
	bool context_yield(F&& func, Args... args)
	{
		if(!is_started())
			return false;
		auto context_yield_data = std::make_shared<context_yield_helper_impl<F,Args...>>
			(std::forward<F>(func),std::forward<Args>(args)...);
		
		return context_yield(context_yield_data);
	}
	void context_yield_notify(){
		std::lock_guard<std::mutex> lk(_internal_queue_mutex);
		using namespace std::chrono_literals;
		_is_cy_event = true;
		_cv.notify_one();
		//_cv_context_yield.notify_one();
		//_is_cy_event = false;
		if(!_internal_task_queue.empty())
		{
			_is_cy_event = false;
			add_task(_internal_task_queue.top(),0ms);
			_internal_task_queue.pop();
		}
//		add_task(make_task(&thread_pool::check_cy_condition,this));
	}
private:
	bool context_yield(std::shared_ptr<context_yield_helper> context_yield_data_p);
	bool context_yield_impl(std::shared_ptr<context_yield_helper> context_yield_data_p);

public:

	/*!
	 * stops all threads one by one after waiting on join and keep inside thread pool.
	 */
	void join();
	/*! 
	 * add task to task_que, which will be picked and invoked by a thread from thread_pool.
	 * @param task: Its a std::unique_ptr to task object which will be moved and kept in std::priority_queue
	 * @param priority: priority value will be added as waiting period (value in milliseconds).
	 */
	template<typename T>
	task_future<typename T::result_type> add_task(std::unique_ptr<T>&& task, unsigned int priority = 0){
		auto future = task->get_future();
		auto task_b = std::make_shared<task_base>(std::move(task));
		auto f = task_future<typename T::result_type>(task_b,std::move(future));
		add_task(task_b,std::chrono::milliseconds(priority));
		return f;
	}
	/*! 
	 * add task to task_que, which will be picked and invoked by a thread from thread_pool.
	 * @param task: Its a std::unique_ptr to task object which will be moved and kept in std::priority_queue
	 * @param priority: waiting period tells thread_pool to invoke the function once waiting period is over.
	 */
	template<typename T>
	task_future<typename T::result_type> add_task(std::unique_ptr<T>&& task,
			std::chrono::milliseconds waiting_period ){
		auto future = task->get_future();
		auto task_b = std::make_shared<task_base>(std::move(task));
		auto f = task_future<typename T::result_type>(task_b,std::move(future));
		add_task(task_b,waiting_period);
		return f;
	}
	bool is_started() const{
		return _is_started;
	}
private:
	/*! 
	 * add internal task to task_que, which will be picked and invoked by a thread from thread_pool.
	 * @param task: Its a std::unique_ptr to task object which will be moved and kept in std::priority_queue
	 * @param priority: priority value will be added as waiting period (value in milliseconds).
	 */
	template<typename T>
	task_future<typename T::result_type> internal_add_task(std::unique_ptr<T>&& task, unsigned int priority = 0){
		auto future = task->get_future();
		auto task_b = std::make_shared<task_base>(std::move(task));
		auto f = task_future<typename T::result_type>(task_b,std::move(future));
		internal_add_task(task_b,std::chrono::milliseconds(priority));
		return f;
	}

	/*! 
	 * add task to task_que, which will be picked and invoked by a thread from thread_pool.
	 * @param task: Its a std::unique_ptr to task object which will be moved and kept in std::priority_queue
	 * @param priority: waiting period tells thread_pool to invoke the function once waiting period is over.
	 */
	template<typename T>
	task_future<typename T::result_type> internal_add_task(std::unique_ptr<T>&& task,
			std::chrono::milliseconds waiting_period ){
		auto future = task->get_future();
		auto task_b = std::make_shared<task_base>(std::move(task));
		auto f = task_future<typename T::result_type>(task_b,std::move(future));
		internal_add_task(task_b,waiting_period);
		return f;
	}
	/*!
	 * Checks whether caller thread is from this thread_pool.
	 */
	bool has_thread_ownership();
	/*!
	 * starts all threads one by one from the thread pool.
	 */
	void start();
	/*!
	 * Implementation of run
	 */
	bool invoke_task(std::chrono::nanoseconds waiting_period=0ns);
	bool invoke_task_helper();
	/*!
	 * start all the threads one by one and make the thread_pool ready for use.
	 */
	void run();
	/*!
	 * Adding task to std::prority_queue
	 */
	void add_task(std::shared_ptr<task_base> task,std::chrono::milliseconds waiting_period );
	/*!
	 * Adding task to std::prority_queue
	 */
	void internal_add_task(std::shared_ptr<task_base> task,std::chrono::milliseconds waiting_period );

	/*!
	 * swap function is used to swap the members from one to other 
	 */
	friend void swap(thread_pool& tp1, thread_pool& tp2){
		std::lock_guard<std::mutex> lk1(tp1._queue_mutex);
		std::lock_guard<std::mutex> lk2(tp2._queue_mutex);
		std::lock_guard<std::mutex> lk3(tp1._internal_queue_mutex);
		std::lock_guard<std::mutex> lk4(tp2._internal_queue_mutex);
		std::swap(tp1._pool_size, tp2._pool_size);
		std::swap(tp1._stop_threads, tp2._stop_threads);
		std::swap(tp1._is_started, tp2._is_started);
		std::swap(tp1._task_queue, tp2._task_queue);
		std::swap(tp1._internal_task_queue, tp2._internal_task_queue);
		std::swap(tp1._threads, tp2._threads);
		{
			std::lock_guard<std::mutex> lk1(tp1._cy_mutex);
			std::lock_guard<std::mutex> lk2(tp2._cy_mutex);
			std::swap(tp1._cy_wait_que, tp2._cy_wait_que);
			std::swap(tp1._cy_ready_que, tp2._cy_ready_que);
		}
	}
private:
	/*!
	 * The function checks for the given condition as argument to context_yield.
	 * When condition becames true, this function will start executing the task from 
	 * the same place, where context_yield is called.
	 */
	void check_cy_condition(std::thread::id tid);

	template<typename F,typename... Args>
	struct context_yield_helper_impl : public context_yield_helper{
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
		template <typename... FArgs, std::size_t... Is>
			bool helper_func(std::tuple<FArgs...>& tup, helper_index<Is...>){
				return _func(std::get<Is>(tup)...);
			}

		template <typename... FArgs>
			bool helper_func(std::tuple<FArgs...>& tup){
				return helper_func(tup, helper_gen_seq<sizeof...(FArgs)>{});
			}

		context_yield_helper_impl(std::function<bool(Args...)> fun, Args&&... args) :
			_func(fun),_args(std::forward<Args>(args)...){
		}
		public:
		bool operator()() override{
			_status = helper_func(_args);
			return _status;
		}
	private:	
		std::function<bool(Args...)> _func;
		std::tuple<Args...> _args;
	};
private:
	unsigned int _pool_size; /*! < Define thread_pool size. */
	bool _stop_threads;/*! < Informs thread pool manager that all threads need to stop. */
	bool _is_started; /*! < Informs thread pool manager that threads are started. */
	std::priority_queue<std::shared_ptr<task_base>,std::vector<std::shared_ptr<task_base>>,
		std::function<bool(const std::shared_ptr<task_base>& lhs, const std::shared_ptr<task_base>& rhs)>> _task_queue; 
	/*! < holds assigned rpt::task objects. */
	std::priority_queue<std::shared_ptr<task_base>,std::vector<std::shared_ptr<task_base>>,
		std::function<bool(const std::shared_ptr<task_base>& lhs, const std::shared_ptr<task_base>& rhs)>> _internal_task_queue; 
	/*! < holds assigned internal rpt::task objects. */
	std::mutex _queue_mutex; /*! < to protect _task_queue from being simultaneously accessed by multiple threads. */
	std::mutex _internal_queue_mutex; /*! < to protect _internal_task_queue from being simultaneously accessed by multiple threads. */
	std::mutex _cv_mutex;
	std::condition_variable _cv; /*! < a condition variable which informs idle threads once a task is assigned.*/
	std::condition_variable _cv_context_yield; /*! < a condition variable which informs idle threads once a task is assigned.*/
	std::mutex _cy_cvmutex;  /*! < context_yield thread waits in _cv in condition variables by using this lock. */
	bool _is_cy_event;
	std::vector<std::thread> _threads; /*! < Container to keep all the threads. */
	std::vector<std::thread::id> _thread_ids; /*! < Container to keep all the thread ids. */
	std::mutex _cy_mutex;
	std::mutex _cy_mutex2; /* ! < Used inside contexy_yield implementation. */
	std::queue<std::shared_ptr<context_yield_helper>> _cy_wait_que; /*! waiting function will be queued. */
	std::queue<std::shared_ptr<context_yield_helper>> _cy_ready_que; /*! ready to return to caller function. */
};
#pragma GCC pop_options
}
#endif
