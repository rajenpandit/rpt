#ifndef __BUFFERED_CLIENT_IOSTREAM_H__06032016_RAJEN_H__
#define __BUFFERED_CLIENT_IOSTREAM_H__06032016_RAJEN_H__
#include <vector>
#include <algorithm>
#include "client_iostream.h"
#include <iostream>
#include <limits.h>
#include <sys/ioctl.h>
#include <experimental/string_view>
namespace rpt{

/*!
 * class condition_t is a base class, which is used by its child to 
 * implement different kind of condition. A condition object notifies 
 * buffered_client_iostream when to return data receieved from socket.
 * This class is for internal use by library.
 * @author Rajendra Pandit (rajenpandit)
 * @bug No known bugs
 */
class condition_t{
public:
	condition_t() : _is_true(false){
	}
	/*!
	 * Checks for the given condition. 
	 * @return: It returns iterator to a position into vector upto which data need to be returned from buffer. 
	 */
	std::vector<char>::const_iterator check(const std::vector<char>& data){
		_iterator = check_condition(data);
		if(_iterator != data.cend())
		{
			_is_true = true; 
		}
		return _iterator;
	}
	bool is_true(){
		return _is_true;
	}
	std::vector<char>::const_iterator get_iterator(){
		return _iterator;
	}
public:
	virtual std::vector<char>::const_iterator check_condition(const std::vector<char>& data) = 0;
private:
	std::vector<char>::const_iterator _iterator;
protected:
	bool _is_true;
};
/*!
 * This class defines a condition, which will be true when receieved data
 * from socket is greater or equal to a given size.
 * @author Rajendra Pandit (rajenpandit)
 * @bug No known bugs
 */
class datasize_t : public condition_t{
public:
	datasize_t(size_t size){
		_size = size;
	}
public:
	virtual std::vector<char>::const_iterator check_condition(const std::vector<char>& data) override{
		if(data.size() >= _size)	
		{
			_is_true = true;
			return data.cbegin()+_size;
		}
		_is_true = false;
		return data.cend();
	}
private:
	size_t _size;
};

/*!
 * This class defines a condition, which will be true when receieved data
 * from socket contains a given string.
 * In case of multiple match, it returns a iterator which points to end of first matched string. 
 * @author Rajendra Pandit (rajenpandit)
 * @bug No known bugs
 */
class string_found_t : public condition_t{
public:
	string_found_t(const std::string &s) : _string(s){
	}
public:
	virtual std::vector<char>::const_iterator check_condition(const std::vector<char>& data) override{
		#if 0
		auto it = data.cbegin() + last_size;
		it = std::search(it , data.end(), _string.begin(),_string.end());		
		if(it != data.cend()){
			_is_true = true;
			return it+_string.length();
		}
		else{
			_is_true = false;
			return it;
		}
		#else
		std::experimental::string_view sv(&data[0],data.size());
		auto it = sv.find(_string);
		if(it != std::string::npos){
			_is_true = true;
			return data.cbegin() + it + _string.length();
		}
		else{
			_is_true = false;
			return data.cend();
		}
		#endif
	}
private:
	std::string _string;
};

/*!
 * The class buffered_client_iostream is used to store data receieved from socket into a buffered memory.
 * The stored data can be retrieved by providing conditions.
 * A condition object is used to define how much data need to be returned from buffered memory.
 * If condition object is not provided before calling read/notify(a callback function), then all 
 * the data received from socket will be returned at once.
 * The class provides two types of interface to get stored data from buffer.
 * -# By passing a callback function or a callable object. In this case when given condition is met,
 * then callback function will be called and required data will be passed to user.
 * -# By calling read method, if callback function is not provided, user can call read method to
 * receive data from buffer.	
 * @author Rajendra Pandit (rajenpandit)
 * @bug No known bugs
 */
class buffered_client_iostream : public client_iostream{
	public:
		/*!
		 * @param soc: A socket, which will be used by tcp_connection to connect with remote client.
		 * @param notify: A callback function where received data will be posted. 
		 * This is an optional parameter and it can also be set after object construction 
		 * by using notify_to function.
		 */
		buffered_client_iostream(std::unique_ptr<socket_base> soc,
				std::function<void(const std::vector<char>& data)> notify_function=nullptr) : 
			client_iostream(std::move(soc),notify_function){
			init();
		}
		buffered_client_iostream(const buffered_client_iostream&) = delete;
		void operator = (const buffered_client_iostream&) = delete;
		buffered_client_iostream(buffered_client_iostream&& bc_ios) : 
			client_iostream(std::forward<buffered_client_iostream&&>(bc_ios)){
			swap(*this, bc_ios);
		}
		void operator = (buffered_client_iostream && bc_ios){
			client_iostream::operator = (std::forward<buffered_client_iostream>(bc_ios));
			swap(*this, bc_ios);
		}
		virtual ~buffered_client_iostream(){
		}
		/*!
		 * Clears buffered data and re-initialize object.
		 */
		void init(){
			_data.clear();
		}
		friend void swap(buffered_client_iostream& bc_ios1, buffered_client_iostream& bc_ios2){
			std::lock(bc_ios1._mutex, bc_ios2._mutex);
			std::lock_guard<std::mutex> lk1(bc_ios1._mutex, std::adopt_lock);
			std::lock_guard<std::mutex> lk2(bc_ios2._mutex, std::adopt_lock);

			std::swap(bc_ios1._conditions, bc_ios2._conditions);
			std::swap(bc_ios1._data, bc_ios2._data);
		}
	protected:
		/*!
		 * Check whether a condtion is met for received data.
		 */
		virtual bool rcv_condition(){
			if( !_conditions.empty() ){
				if( !_data.empty() ){
					for(auto &condition : _conditions){
						condition->check(_data);
						if(condition->is_true())
						{
							return true;
						}
					}
					return false;
				}
			}
			else{
				return !_data.empty();
			}
			return false;
		}
		/*!
		 * Returns data from buffer if a condtion is true.
		 * This function should be called if rcv_condition returns true.
		 */
		virtual void get_data(std::vector<char>& data){
			if( !_conditions.empty() ){
				if( !_data.empty() ){
					for(auto &condition : _conditions)
					{
						if( condition->is_true()){
							auto cit = condition->get_iterator();
							data.insert(data.end(),_data.cbegin(),cit);	
							_data.erase(_data.cbegin(),cit);
							break;
						}
					}
				}
			}
			else{
				if(!_data.empty()){
					data.insert(data.end(),_data.begin(),_data.end());	
					_data.clear();
				}
			}
		}

		/*!
		 * Returns data from buffer if a condtion is true.
		 * This function should be called if rcv_condition returns true.
		 */
		virtual void get_data(std::string& data){
			std::vector<char> v;
			get_data(v);
			data.insert(data.end(),v.begin(),v.end());
		}
	public:
		/*!
		 * An interface to set a condition when a data need to be retrieved from the buffer.
		 * This function can be called multiple times to set multiple conditions.
		 * @param condtion A condition object returned by datasize/string_found functions.
		 */
		void set_condition(std::unique_ptr<condition_t> condition){
			_conditions.push_back(std::move(condition));
		}
		/*!
		 * Clears all the conditions set by user.
		 */
		void clear_conditions(){
			_conditions.clear();
		}
	public:
		using client_iostream::read;
		/*!
		 * Retrieve stored data by using a condition. This call will be blocked if provided condition is match.
		 * @param condtion A condition object returned by datasize/string_found functions. 
		 * @note if a condition is used, all other conditions will be removed which is set by using set_condition.
		 * To use multiple condition with by read method, use #set_condition followed by client_iostream::read() function call. 
		 */
		std::string read(std::unique_ptr<condition_t> condition){
			clear_conditions();
			set_condition(std::move(condition));		
			return read();
		}
	private:
		std::vector<std::unique_ptr<condition_t>> _conditions;
};

/*!
 * Returns a std::unique_ptr to a condition object of type datasize_t,
 * which is required by buffered_client_iostream::set_condition function.
 */
inline
std::unique_ptr<condition_t> datasize(size_t size){
	return std::unique_ptr<condition_t>(new datasize_t(size));
}

/*!
 * Returns a std::unique_ptr to a condition object of type string_found_t,
 * which is required by buffered_client_iostream::set_condition function.
 */
inline
std::unique_ptr<condition_t> string_found(const std::string& str){
	return std::unique_ptr<condition_t>(new string_found_t(str));
}
}
#endif
