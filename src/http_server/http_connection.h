#ifndef __HTTP_CONNECTION_H__26092016_RAJEN__
#define __HTTP_CONNECTION_H__26092016_RAJEN__
#include "buffered_client_iostream.h"
#include "socket_factory.h"
#include "tcp_socket.h"
#include "thread_pool.h"
#include "http_servlet_request.h"
#include "http_servlet_response.h"
#include "http_servlet.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <memory>
#include <mutex>

namespace rpt{
class http_connection : public buffered_client_iostream,
	public virtual http_servlet_request,
	public virtual http_servlet_response,
	public std::enable_shared_from_this<http_connection>
{
	public:
		http_connection(const std::shared_ptr<socket_factory>& sf) :
			buffered_client_iostream(sf->get_socket(),
					std::bind(&http_connection::decode_header,this,std::placeholders::_1)){
				set_condition(string_found("\r\n\r\n"));
				set_condition(string_found("\n\n"));
				_resrecvd = false;
			}
	public:
		virtual bool write_to_stream(const void* data, size_t size) override;
		virtual const std::string& get_client_addr() const override;
	public:
		std::shared_ptr<http_request> get_request(){
			return shared_from_this();
		}
		std::shared_ptr<http_response> get_response(){
			if(_thread_pool==nullptr){
				std::unique_lock<std::mutex> lk(_cv_mutex);
				_cv.wait(lk, [this]{return _resrecvd;});
			}
			else{
				_thread_pool->context_yield([this]{return _resrecvd;});
			}
			_resrecvd = false;
			return shared_from_this();
		}
		void register_callback(std::function<void(std::shared_ptr<http_servlet_request>,
					std::shared_ptr<http_servlet_response>)> callback){
			_callback = callback;
		}
		void register_client_callback(std::function<void(std::shared_ptr<http_response>)> callback){
			_client_callback = callback;
		}
		void reset_callback(){
			_callback=nullptr;
			_client_callback=nullptr;
		}
	private:
		void decode_header(const std::vector<char>& data);
		void decode_body(const std::vector<char>& data);
		void decode_chunked_body(const std::vector<char>& data);
		void set_resrecvd(){
			_resrecvd = true;
			_cv.notify_one();
		}
	private:
		struct chunked_status{
			chunked_status(){
				check_length=true;
				skip_crlf=false;
				length=0;
			};
			bool check_length;	
			bool skip_crlf;
			int length;
		};
		chunked_status _chunked_status;
	private:
		std::function<void(std::shared_ptr<http_servlet_request>,std::shared_ptr<http_servlet_response>)> _callback;
		std::function<void(std::shared_ptr<http_response>)> _client_callback;
		bool _resrecvd;
		std::condition_variable _cv;
		std::mutex _cv_mutex;
};
}
#endif
