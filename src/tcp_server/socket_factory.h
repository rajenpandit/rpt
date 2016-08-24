#ifndef __SOCKET_FACTORY_H__30052016_RAJEN__
#define	__SOCKET_FACTORY_H__30052016_RAJEN__
#include "socket_base.h"
namespace rpt{
class socket_factory{
public:
	virtual	std::unique_ptr<socket_base> get_socket() const = 0;	
};
}
#endif	
