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
class condition_t{
public:
	condition_t() : _is_true(false){
	}
public:
	virtual bool is_true() = 0;
	virtual std::vector<char>::const_iterator check(const std::vector<char>& data,int last_size)=0;
	virtual unsigned int read_size() = 0;
protected:
	bool _is_true;
};

class datasize_t : public condition_t{
public:
	datasize_t(size_t size){
		_size = size;
	}
public:
	virtual std::vector<char>::const_iterator check(const std::vector<char>& data, __attribute__((unused)) int last_size){
		if(data.size() >= _size)	
		{
			_is_true = true;
			return data.cbegin()+_size;
		}
		_is_true = false;
		return data.cend();
	}
	virtual bool is_true(){
		return _is_true;
	}
	virtual unsigned int read_size(){
		return _size;
	}
private:
	size_t _size;
};

class string_found_t : public condition_t{
public:
	string_found_t(const std::string &s) : _string(s){
	}
public:
	virtual std::vector<char>::const_iterator check(const std::vector<char>& data, __attribute__((unused)) int last_size){
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
	virtual bool is_true(){
		return _is_true;
	}
	virtual unsigned int read_size(){
		return _string.length();
	}
private:
	std::string _string;
};


class buffered_client_iostream : public client_iostream{
	public:
		buffered_client_iostream(std::unique_ptr<socket_base> soc,
				std::function<void(const std::vector<char>& data)> notify=nullptr) : 
			client_iostream(std::move(soc),notify){
			init();
		}
		buffered_client_iostream(const buffered_client_iostream&) = delete;
		void operator = (const buffered_client_iostream&) = delete;
		buffered_client_iostream(buffered_client_iostream&& bc_ios) : 
			client_iostream(std::forward<buffered_client_iostream&&>(bc_ios)){
		}
		void operator = (buffered_client_iostream && bc_ios){
			swap(*this, bc_ios);
		}
		void init(){
			_data.clear();
		}
		friend void swap(buffered_client_iostream& bc_ios1, buffered_client_iostream& bc_ios2){
			std::lock(bc_ios1._mutex, bc_ios2._mutex);
			std::lock_guard<std::mutex> lk1(bc_ios1._mutex, std::adopt_lock);
			std::lock_guard<std::mutex> lk2(bc_ios2._mutex, std::adopt_lock);

			std::swap(bc_ios1._conditions, bc_ios2._conditions);
			std::swap(bc_ios1._data, bc_ios2._data);
			//std::swap(bc_ios1._segmented_data, bc_ios2._segmented_data);
		//	std::swap(bc_ios1._last_check, bc_ios2._last_check);
		}
	protected:
		virtual bool rcv_condition(){
			if( !_conditions.empty() ){
				if( !_data.empty() ){
					for(auto &condition : _conditions){
						condition->check(_data,0);
						if(condition->is_true())
							return true;
					}
					return false;
				}
			}
			else{
				return !_data.empty();
			}
			return false;
		}
		virtual void get_data(std::vector<char>& data){
			if( !_conditions.empty() ){
				if( !_data.empty() ){
					for(auto &condition : _conditions)
					{
						auto cit = condition->check(_data,0);
						if( condition->is_true()){
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
		virtual void get_data(std::string& data){
			std::vector<char> v;
			get_data(v);
			data.insert(data.end(),v.begin(),v.end());
		}
	public:
		void set_condition(std::unique_ptr<condition_t> condition){
			_conditions.push_back(std::move(condition));
		}
		void clear_conditions(){
			_conditions.clear();
		}
	public:
		using client_iostream::read;
		std::string read(std::unique_ptr<condition_t> condition){
			clear_conditions();
			set_condition(std::move(condition));		
			return read();
		}
//		virtual void notify(const std::vector<char>& data) = 0 ;
	private:
		std::vector<std::unique_ptr<condition_t>> _conditions;
//		std::vector<char> _segmented_data;
	//	int _last_check;
};

#if 1
inline
std::unique_ptr<condition_t> datasize(size_t size){
	return std::unique_ptr<condition_t>(new datasize_t(size));
}
inline
std::unique_ptr<condition_t> string_found(const std::string& str){
	return std::unique_ptr<condition_t>(new string_found_t(str));
}
}
#endif
#endif
