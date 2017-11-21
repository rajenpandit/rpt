#ifndef __HTTP_RESPONSE_H__11062016_RAJEN__
#define __HTTP_RESPONSE_H__11062016_RAJEN__
#include "http_packet.h"
namespace rpt{
class http_response{
public:
	const std::string&  get_content_type(){
		return _response.get_content_type();
	}
	int get_content_length(){
		return _response.get_content_length();
	}
	const std::string& get_header(const std::string& name){
		return _response.get_header(name);
	}
	const std::map<std::string,std::string>& get_headers(){
		return _response.get_headers();
	}
	const std::string& get_method() const{
		return _response.get_method();
	}
	const std::string& get_uri() const{
		return _response.get_uri();
	}
	const std::string& get_version() const{
		return _response.get_version();
	}
	const std::string& get_status_code() const{
                return _response.get_status_code();
        }
	const std::string get_body() const{
		return _response.get_body();
	}
	const char* get_data() const{
		return _response.get_data();
	}
public:
	virtual const std::string& get_client_addr() const = 0;
protected:
	http_packet _response;

};
}
#endif
