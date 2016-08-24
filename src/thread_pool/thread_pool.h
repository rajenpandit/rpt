
#ifndef __THREAD_POOL__29022016_RAJEN__
#define __THREAD_POOL__29022016_RAJEN__
#include <thread>
#include <mutex>
#include <queue>
#include <list>
#include <condition_variable>
#include "task.h"
namespace rpt{
/**
 * Provides interface to create a pool of threads.
 * All threads will be idle untill #rpt::task is added by using #add_task.
 * @author Rajendra Pandit (rajenpandit)
 * @bug No known bugs
 */
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
public:
	/*!
	 * starts all threads one by one from the thread pool.
	 */
	void start();

	/*!
	 * stops all threads one by one after waiting on join and keep inside thread pool.
	 */
	void stop();
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
	 * start all the threads one by one and make the thread_pool ready for use.
	 */
	void run();
	/*!
	 * Adding task to std::prority_queue
	 */
	void add_task(std::shared_ptr<task_base> task,std::chrono::milliseconds waiting_period );
	/*!
	 * swap function is used to swap the members from one to other 
	 */
	friend void swap(thread_pool& tp1, thread_pool& tp2){
		std::lock_guard<std::mutex> lk1(tp1._queue_mutex);
		std::lock_guard<std::mutex> lk2(tp2._queue_mutex);
		std::swap(tp1._pool_size, tp2._pool_size);
		std::swap(tp1._stop_threads, tp2._stop_threads);
		std::swap(tp1._task_queue, tp2._task_queue);
		std::swap(tp1._threads, tp2._threads);
	}
private:
	unsigned int _pool_size; /*! < Define thread_pool size. */
	bool _stop_threads;/*! < Informs thread pool manager that all threads need to stop. */
	bool _is_started; /*! < Informs thread pool manager that threads are started. */
	std::priority_queue<std::shared_ptr<task_base>,std::vector<std::shared_ptr<task_base>>,
		std::function<bool(const std::shared_ptr<task_base>& lhs, const std::shared_ptr<task_base>& rhs)>> _task_queue; 
	/*! < holds assigned rpt::task objects. */
	std::mutex _queue_mutex; /*! < to protect _task_queue from being simultaneously accessed by multiple threads. */
	std::mutex _cv_mutex;
	std::condition_variable _cv; /*! < a condition variable which informs idle threads once a task is assigned.*/
	std::vector<std::thread> _threads; /*! < Container to keep all the threads. */
};
}
#endif
