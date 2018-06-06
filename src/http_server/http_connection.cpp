#include "http_connection.h"
#include "utils.h"
#include <vector>
#include <sstream>
#include <iostream>
using namespace rpt;
void http_connection::decode_header(const std::vector<char>& data){
	if(data.empty())
		return;
	if(_resrecvd == false){
		_response.clear();
	}
	if((_response.get_content_length() == 0)){
		_response.assign(data);
		if(_response.decode()){
			if(size_t l = _response.get_content_length()){
				clear_conditions();
				set_condition(datasize(l));
				notify_to(std::bind(&http_connection::decode_body,this,std::placeholders::_1));
			}
			else if(_response.get_transfer_encoding().find("chunked") != std::string::npos){
				clear_conditions();
				set_condition(string_found("\r\n"));
				set_condition(string_found("\n"));
				notify_to(std::bind(&http_connection::decode_chunked_body,this,std::placeholders::_1));
			}
			else{
				if(_callback != nullptr){
					_callback(get_shared_from_this(),get_shared_from_this());
				}
				else if(_client_callback != nullptr){
					_client_callback(get_shared_from_this());
				}
				else{
					set_resrecvd();
				}
			}
		}
		else{
			std::cout<<"Error:["<<__FILE__<<":"<<__LINE__<<"] Decode Failed"<<std::endl;
		}
	}
}

void http_connection::decode_body(const std::vector<char>& data){
	if(data.empty())
	{
		std::cout<<"Error:["<<__FILE__<<":"<<__LINE__<<"] Empty Body"<<std::endl;
		return;
	}
	_response.append(data);
	clear_conditions();
	set_condition(string_found("\r\n\r\n"));
	set_condition(string_found("\n\n"));
	notify_to(std::bind(&http_connection::decode_header,this,std::placeholders::_1));

	if(_callback != nullptr){
		_callback(get_shared_from_this(),get_shared_from_this());
	}
	else if(_client_callback != nullptr){
		_client_callback(get_shared_from_this());
	}
	else{
		set_resrecvd();
	}
}
void http_connection::decode_chunked_body(const std::vector<char>& data){
	if(data.empty())
	{
		return;
	}
	std::experimental::string_view s(&data[0], data.size());
	if(_chunked_status.check_length){
		auto pos = s.find("\r\n");
		if(pos == std::string::npos)
			pos = s.find("\n");
		if(pos != std::string::npos)
		{
			std::size_t p;
			_chunked_status.length=std::stoi(s.substr(0,pos).to_string(),&p,16);
			if(_chunked_status.length){
				clear_conditions();
				set_condition(datasize(_chunked_status.length));
			}
			else{
				_chunked_status.skip_crlf = true;
			}
			_chunked_status.check_length = false;
		}
		else	
			std::cout<<"Error:["<<__FILE__<<":"<<__LINE__<<"] Decode Failed"<<std::endl;
	}	
	else if(_chunked_status.skip_crlf){
		_chunked_status.skip_crlf=false;
		if(_chunked_status.length)
		{
			_chunked_status.check_length = true;
		}
		else
		{
			_chunked_status.check_length = true;
			clear_conditions();
			set_condition(string_found("\r\n\r\n"));
			set_condition(string_found("\n\n"));
			notify_to(std::bind(&http_connection::decode_header,this,std::placeholders::_1));

			if(_callback != nullptr){
				_callback(get_shared_from_this(),get_shared_from_this());
			}
			else if(_client_callback != nullptr){
				_client_callback(get_shared_from_this());
			}
			else{
				set_resrecvd();
			}
		}
		return;
	}
	else{
		_response.append(s.to_string());
		clear_conditions();
		set_condition(string_found("\r\n"));
		set_condition(string_found("\n"));
		_chunked_status.skip_crlf=true;
	}
}

//------------------------- http_server_encode---------------------------------
bool http_connection::write_to_stream(const void* data, size_t size){
	return buffered_client_iostream::write(data,size);
}

const std::string& http_connection::get_client_addr() const {
	return buffered_client_iostream::get_client_addr();
}
