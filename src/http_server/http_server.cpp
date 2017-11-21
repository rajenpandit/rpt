#include "http_server.h"
#include "utils.h"
#include <vector>
#include <sstream>
#include <iostream>
using namespace rpt;


void http_server::notify(std::shared_ptr<http_servlet_request> request, std::shared_ptr<http_servlet_response> response){
	std::shared_ptr<http_servlet> servlet = find_servlet(request->get_uri());
	if(servlet == nullptr)
		servlet = find_servlet("/");
	if(servlet != nullptr){
		if(request->get_method() == "GET")
			servlet->do_get(request,response);
		else
			servlet->do_post(request,response);
	}
	else
		std::cout<<"No Servlet found"<<std::endl;
}

std::shared_ptr<http_servlet> http_server::find_servlet(const std::string& request_uri)
{
	auto it = _servlets_map.find(request_uri);
	if( it == _servlets_map.end() ){
		it = _servlets_map.find(request_uri + "*");
		if( it == _servlets_map.end() ){
			it = _servlets_map.find(request_uri + "/*");
		}
	}
	if( it != _servlets_map.end() ){
		return it->second->get_new_instance();
	}
	return nullptr;
}

