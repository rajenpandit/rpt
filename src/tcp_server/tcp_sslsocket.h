#ifndef __TCP__SSLSOCKET_H_05032016_RAJEN_H__
#define __TCP__SSLSOCKET_H_05032016_RAJEN_H__
#include "tcp_socket.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <iostream>
namespace rpt{
	template <bool T=true>
		class ssl_init{
			public:
				ssl_init(){
#                       if OPENSSL_VERSION_NUMBER < 0x10100000L
					SSL_load_error_strings();
					SSL_library_init();
					OpenSSL_add_all_algorithms();
#                       else
					OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS,NULL);
					OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CRYPTO_STRINGS,NULL);
#                       endif
				}
				~ssl_init(){
#                       if OPENSSL_VERSION_NUMBER < 0x10100000L
					ERR_free_strings();
					EVP_cleanup();
#                       else
					OPENSSL_cleanup();
#                       endif
				}
		};
class tcp_sslsocket : public tcp_socket{
	
public:
	tcp_sslsocket(const std::string& cert_path, const std::string& key_path)
		: _cert_path(cert_path), _key_path(key_path){
		_ssl=nullptr;
		do_init();
	}
private:
	void do_init(){
		static ssl_init<true> _init;	
	}
public:
	virtual bool accept(socket_base &socket) override;
	virtual bool connect(long timeout=0) override;
	virtual int send(const void *buffer, size_t size, int flags) override;
	virtual int receive(void *buffer,size_t size,bool block=true) override;
	virtual bool close() override;
private:
        SSL *_ssl;
	std::string _cert_path;	
	std::string _key_path;
};
inline
bool tcp_sslsocket::accept(socket_base &socket){
	if(tcp_socket::accept(socket))
	{
		tcp_sslsocket& client_socket = dynamic_cast<tcp_sslsocket&>(socket);
		SSL_CTX *sslctx = nullptr;
#	if OPENSSL_VERSION_NUMBER < 0x10100000L
		sslctx = SSL_CTX_new(SSLv23_method());
#	else
		sslctx = SSL_CTX_new( TLS_method());
#	endif
		SSL_CTX_set_options(sslctx, SSL_OP_SINGLE_DH_USE);
		SSL_CTX_use_certificate_file(sslctx, _cert_path.c_str() , SSL_FILETYPE_PEM);
		SSL_CTX_use_PrivateKey_file(sslctx, _key_path.c_str(), SSL_FILETYPE_PEM);
		if ( SSL_CTX_check_private_key(sslctx) != 1) 
		{
			SSL_CTX_free(sslctx);
			return false;
		}
		client_socket._ssl = SSL_new(sslctx);
		SSL_set_fd(client_socket._ssl, client_socket._fd );
		if(SSL_accept(client_socket._ssl) <= 0)
		{
			SSL_free(client_socket._ssl);
			client_socket._ssl=nullptr;
			return false;
		}
		return true;
	}
	return false;
}
inline
bool tcp_sslsocket::connect(long timeout=0){
	if(tcp_socket::connect(timeout)==true)
	{
		SSL_CTX *sslctx = nullptr;
#	if OPENSSL_VERSION_NUMBER < 0x10100000L
		sslctx = SSL_CTX_new(SSLv23_client_method());
#	else
		sslctx = SSL_CTX_new( TLS_client_method() );
#	endif
		SSL_CTX_set_options(sslctx, SSL_OP_SINGLE_DH_USE);
		SSL_CTX_use_certificate_file(sslctx, _cert_path.c_str() , SSL_FILETYPE_PEM);
		SSL_CTX_use_PrivateKey_file(sslctx, _key_path.c_str(), SSL_FILETYPE_PEM);
		if ( !SSL_CTX_check_private_key(sslctx)) 
		{
			SSL_CTX_free(sslctx);
			return false;
		}
		_ssl = SSL_new(sslctx);
		SSL_set_fd(_ssl, _fd );
		if(SSL_connect(_ssl) <= 0)
		{
			SSL_free(_ssl);
			_ssl=nullptr;
			return false;
		}

		return true;

	}
}

inline
int tcp_sslsocket::send(const void *buffer, size_t size, int flags){
	return SSL_write(_ssl,buffer,size);
}
inline
int tcp_sslsocket::receive(void *buffer,size_t size,bool block){
	if(_ssl)
		return SSL_read(_ssl, buffer, size);
	return -1;
}
inline
bool tcp_sslsocket::close(){
	bool rv = tcp_socket::close();
	if(_ssl!=nullptr)
		SSL_free(_ssl);
	return rv;
}

}
#endif
