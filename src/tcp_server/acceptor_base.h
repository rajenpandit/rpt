#ifndef __ACCEPTOR_BASE_H__
#define __ACCEPTOR_BASE_H__
#include <memory>
namespace rpt{
class acceptor_base{
public:
	enum acceptor_status_t{ACCEPT_SUCCESS,ACCEPT_FAILED,DUPLICATE_CLIENT_SOCKET};
public:
	virtual std::shared_ptr<client_iostream> get_new_client() = 0;
	virtual void notify_accept(std::shared_ptr<client_iostream> client, acceptor_status_t status) = 0;
	virtual ~acceptor_base(){
	}
};
}
#endif
