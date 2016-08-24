#ifndef __CLIENT_H_05032016_H__
#define __CLIENT_H_05032016_H__
#include "client_iostream.h"
namespace rpt{
class client : public client_iostream {
public:
	virtual void read( void* data, size_t size)
	{
		std::cout<<"data["<<size<<"]:"<<(char*)data<<std::endl;
	} 
};
}
#endif
