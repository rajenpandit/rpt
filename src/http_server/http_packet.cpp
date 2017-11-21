#include "http_packet.h"
#include "utils.h"
#include <algorithm>
#include <cstdio>
#if 0
#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)
#else
#define likely(x)     	x 
#define unlikely(x)     x
#endif
using namespace rpt;
bool http_packet::set_header_body(){
	const char *h_end="\r\n\r\n";
	std::experimental::string_view s(&_http_packet_data[0], _http_packet_data.size());
	auto it = s.find(h_end);
	if(it != std::string::npos){
		it+=4;
	}
	else{
		h_end="\n\n";
		it = s.find(h_end);
		if(it == std::string::npos)
			it = s.size();
		else
			it+=2;
	}
	_http_header = s.substr(0,it);
	if(it < s.size())
		_http_body = s.substr(it);
	return true;
}
bool http_packet::decode(){
	if(!set_header_body()){
		return false;
	}
	if(!decode_http_header(_http_header.to_string()))
	{
		return false;
	}
	return true;
}
bool http_packet::decode_http_header(const std::string& http_header){
	std::string::size_type pos;
	if( (pos=http_header.find("HTTP/")) == std::string::npos) 
	{
		return false;
	}
	if( (pos = http_header.find("\n",pos)) == std::string::npos)
	{
		return false;
	}
	std::string::size_type start;
	const std::string& http_header_info = http_header.substr(0,pos);
#define FIND_METHOD(x) ((start=http_header_info.find(#x))!=std::string::npos)
        if((FIND_METHOD(GET) || FIND_METHOD(POST) || FIND_METHOD(PUT) || FIND_METHOD(DELETE))){
                if(!decode_request_info( std::string(http_header.begin() + start, http_header.begin() + pos) ))
                        return false;
        }
        else if(!decode_response_info(std::string(http_header.begin(), http_header.begin() + pos))){
                return false;
        }
#undef FIND_METHOD
	return decode_headers(http_header.substr(pos+1));
}

bool http_packet::decode_response_info(const std::string& http_info){
	std::string::size_type start_pos;
	if( (start_pos=http_info.find("HTTP/")) == std::string::npos) 
		return false;
	auto it = http_info.find(" ",start_pos);
	_version=http_info.substr(start_pos,it);
	start_pos = it+1;
	it = http_info.find("\r\n",start_pos);
	if(it==std::string::npos)
		it = http_info.find("\n",start_pos);
	_status_code = http_info.substr(start_pos,it);			
	return true;
}
bool http_packet::decode_request_info(const std::string& http_info){
#if 0
	std::stringstream ss(http_info);
	//ss.write(http_info.c_str(), http_info.length());
	ss >> _method; //Method: GET/POST
	ss >> _uri; //Request URI
	ss >> _version;
#else
	std::string::size_type start_pos = 0;
	auto it = http_info.find(" ");
	if(likely(it != std::string::npos)){
		_method=http_info.substr(start_pos,it);
		start_pos = it+1;
		it = http_info.find(" ",start_pos);
		if(unlikely(it == std::string::npos))
			return false;
		_uri=http_info.substr(start_pos,it-start_pos);
		_version=http_info.substr(it+1);
	}
	else{	
		return false;
	}
#endif
	return true;
}
bool http_packet::decode_headers(std::string http_header){
	std::string::size_type pos;
	while(true){
		std::string header;
		if((pos = http_header.find("\r\n")) != std::string::npos){
			header = http_header.substr(0,pos);
			http_header.erase(0,pos+2);
		}
		else if((pos = http_header.find("\n")) != std::string::npos){
			header = http_header.substr(0,pos);
			http_header.erase(0,pos+1);
		}
		else{
			break;
		}
		auto pair = utils::get_name_value_pair(header,":");	
		if(!pair.first.empty()){
			_headers[utils::trim(pair.first)] = utils::trim(pair.second);	
		}
	}
	return true;
}

const std::vector<char>& http_packet::encode_request(const std::string& body){
	if(_method.empty()){
		if(body.length() )
			_method="POST";
		else
			_method="GET";
	}
	if(_uri.empty())
		_uri="/";
	if(_version.empty())
		_version="HTTP/1.1";
	const std::string& request_line = _method + " " + _uri + " " + _version + "\r\n";
	_http_packet_data.assign(request_line.begin(),request_line.end());
	return encode(body);
}
const std::vector<char>& http_packet::encode_response(const std::string& body){
	if(_version.empty())
		_version="HTTP/1.1";
	if(_status_code.empty())
		_status_code.assign("200 OK");
	const std::string& response_line = _version +" " + _status_code + "\r\n";
	_http_packet_data.assign(response_line.begin(),response_line.end());
	return encode(body);
}
const std::vector<char>& http_packet::encode(const std::string& body){
	if(!body.empty()){
		set_content_length(body.length());
	}
	for(auto &h : _headers){
		const std::string& header = h.first + ": " + h.second + "\r\n";
		_http_packet_data.insert(_http_packet_data.end(), header.begin(), header.end());	
	}	
	const std::string& delimiter = "\r\n";
	_http_packet_data.insert(_http_packet_data.end(), delimiter.begin(), delimiter.end());	
	_http_packet_data.insert(_http_packet_data.end(), body.begin(), body.end());
	set_header_body();
	return _http_packet_data;
}
