#ifndef __HTTP_REQUEST_H__11062016_RAJEN__
#define __HTTP_REQUEST_H__11062016_RAJEN__
#include "http_packet.h"
namespace rpt{
class http_request{
public:
	bool write(const std::string& data){
		return write(data.c_str(),data.size());
	}
public:
	bool write(const void* data, size_t size){
		auto &v = _request.encode_request(std::string(static_cast<const char*>(data),size));
		return write_to_stream(&v[0],v.size());
	}
	virtual bool write_to_stream(const void* data, size_t size) = 0;
public:
	void set_status_code(const std::string& status_code){
		_request.set_status_code(status_code);
        }
	void set_content_type(const std::string& content_type){
		_request.set_content_type(content_type);
	}
	void set_method(const std::string& method){
		_request.set_method(method);
	}
	void set_uri(const std::string& uri){
		_request.set_uri(uri);
	}
	void set_version(const std::string& version){
		_request.set_version(version);
	}
	void set_header(const std::string& name, const std::string& value){
		_request.set_header(name,value);
	}
	const char* get_data() const{
		return _request.get_data();
	}
protected:
	http_packet _request;

};
};
#endif
