#ifndef __ACCEPTOR_BASE_H__
#define __ACCEPTOR_BASE_H__
#include <memory>
#include "client_iostream.h"
namespace rpt{
/*!
 * acceptor_base class provides interface to user's acceptor class,
 * acceptor class provided by user will be used to get client_iostream 
 * or its child class object. The object will be required to 
 * communicate with remote client. 
 * @author Rajendra Pandit (rajenpandit)
 * @bug No known bugs
 */
class acceptor_base{
public:
	enum acceptor_status_t{ACCEPT_SUCCESS,ACCEPT_FAILED,DUPLICATE_CLIENT_SOCKET};
public:
	/*!
	 * A virtual function which must be defined by user.
	 * This function should return a std::shared_ptr to client_iostream or its child object.
	 */
	virtual std::shared_ptr<client_iostream> get_new_client() = 0;
	/*!
	 * Upon acceptence of remote client's connection, tcp_connection class calls 
	 * notify_accept function.
	 * @param client: a std::shared_ptr to an object associated with remote client.
	 * @param status: status of remote client's connection.
	 */
	virtual void notify_accept(std::shared_ptr<client_iostream> client, acceptor_status_t status) = 0;
	virtual ~acceptor_base(){
	}
};
}
#endif
