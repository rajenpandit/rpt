#ifndef __HTTP_SERVLET_RESPONSE_H__11062016_RAJEN__
#define __HTTP_SERVLET_RESPONSE_H__11062016_RAJEN__
#include "http_request.h"
namespace rpt{
class http_servlet_response : public http_request{
public:
	bool write(const void* data, size_t size){
		auto &v = _request.encode_response(std::string(static_cast<const char*>(data),size));
		return write_to_stream(&v[0],v.size());
	}
};
}
#endif
