#ifndef __HTTP_PACKET_H__09032016_RAJEN__
#define __HTTP_PACKET_H__09032016_RAJEN__

#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <experimental/string_view>
namespace rpt{
class http_packet{
        /************************************************************************/
        /* Comparator for case-insensitive comparison in STL assos. containers  */
        /************************************************************************/
        struct ci_less : std::binary_function<std::string, std::string, bool>
        {
                // case-independent (ci) compare_less binary function
                struct nocase_compare : public std::binary_function<unsigned char,unsigned char,bool>
                {
                        bool operator() (const unsigned char& c1, const unsigned char& c2) const {
                                return tolower (c1) < tolower (c2);
                        }
                };
                bool operator() (const std::string & s1, const std::string & s2) const {
                        return std::lexicographical_compare
                                (s1.begin (), s1.end (),   // source range
                                 s2.begin (), s2.end (),   // dest range
                                 nocase_compare ());  // comparison
                }
        };
public:
	enum http_method{NONE,GET,POST,PUT,DELETE};
public:
	http_packet() = default;
	http_packet(const std::vector<char>& data){
		_http_packet_data = data;
	}
public:
	/*Assigning data to container*/
	void assign(const std::vector<char>& data){
                _http_packet_data = data;
        }
	void assign(const std::string& data){
		_http_packet_data.assign(data.begin(), data.end());
	}
	template<std::size_t size>
	void assign(const char (&arr)[size]){
		_http_packet_data.assign(std::begin(arr),std::end(arr)-1);
	}
	void assign(const char* p, std::size_t size){
		_http_packet_data.clear();
		_http_packet_data.assign(p, p+size);
	}

	/*Appending data to container*/
	void append(const std::vector<char>& data){
                _http_packet_data.insert(_http_packet_data.end(),data.begin(),data.end());
		set_header_body();
        }
	void append(const std::string& data){
		_http_packet_data.insert(_http_packet_data.end(),data.begin(), data.end());
		set_header_body();
	}
	template<std::size_t size>
	void append(const char (&arr)[size]){
                _http_packet_data.insert(_http_packet_data.end(),std::begin(arr),std::end(arr)-1);
		set_header_body();
	}
	void append(const char* p, std::size_t size){
		_http_packet_data.insert(_http_packet_data.end(), p, p+size);
		set_header_body();
	}

#if 1
	void display(){
		std::cout<<(char*)&_http_packet_data[0]<<std::endl;
	}
	void display_headers(){
		std::cout<<_method << " " <<_uri << " " << _version << std::endl;
		for( auto h : _headers)
			std::cout << h.first << ": " << h.second << std::endl;
		std::cout<<"\n"<<_http_body<<std::endl;
	}
#endif
public:
	void clear(){
		_http_packet_data.clear();
		_status_code.clear();
		_method.clear();
		_uri.clear();
		_version.clear();
		_headers.clear();
	}

	bool decode();
	bool reset_header_body(){
		return set_header_body();
	}
/*
	const std::vector<char>& encode_request(){
		return encode_request("");
	}
	const std::vector<char>& encode_response(){
		return encode_response("");
	}
	const std::vector<char>& encode_response(){
		return encode_response("");
	}
*/
	const std::vector<char>& encode_request(const std::string& body="");
	const std::vector<char>& encode_response(const std::string& body="");
	const std::vector<char>& encode(const std::string& body="");
public:
	const std::map<std::string,std::string>& get_headers(){
		if(_duplicate_headers.size() != _headers.size()){
			_duplicate_headers.clear();
			for(auto pair : _headers){
				_duplicate_headers.emplace(pair.first,pair.second);
			}
		}
		return _duplicate_headers;
	}
	void set_headers(const std::map<std::string,std::string>& headers){
		for(auto pair : headers){
			_headers.emplace(pair.first,pair.second);
		}
	}
	const std::string& get_header(const std::string& name) const {
		auto it = _headers.find(name);
		if(it != _headers.end())
			return it->second;
		return _empty;
	}
	void set_header(const std::string& name, const std::string& value){
		_headers[name] = value;
	}
	const std::string& get_content_type() const{
		return get_header("Content-Type");
	}
	void set_content_type(const std::string& value){
		set_header("Content-Type",value);
	}
	unsigned int get_content_length() const{
		std::string content_length = get_header("Content-Length");
		if(!content_length.empty()){
			return std::stoi(content_length);
		}
		return 0;
	}
	
	const std::string& get_status_code() const{
		return _status_code;
	}
	void set_status_code(const std::string& status_code){
		_status_code = status_code;
	}
	const std::string& get_method() const{
		return _method;
	}
	void set_method(const std::string& method){
		_method = method;
	}
	const std::string& get_transfer_encoding() const{
		return get_header("Transfer-Encoding");
	}
	void set_transfer_encoding(const std::string& transfer_encoding){
		set_header("Transfer-Encoding",transfer_encoding);
	}
	const std::string& get_uri() const{
		return _uri;
	}
	void set_uri(const std::string& uri){
		_uri = uri;
	}
	const std::string& get_version() const{
		return _version;
	}
	void set_version(const std::string& version){
		_version = version;
	}
	const std::string get_body() const{
		return _http_body.to_string();
	}
	const char* get_data() const{
		if(_http_packet_data.size())
			return &_http_packet_data[0];
		else
			return nullptr;
	}
private:
	void set_content_length(unsigned int length){
		set_header("Content-Length",std::to_string(length));
	}
public:
	bool set_header_body();
	bool decode_http_header(const std::string& http_header);
	bool decode_headers(std::string http_header);
	bool decode_response_info(const std::string& http_header);
	bool decode_request_info(const std::string& http_header);
protected:
	std::vector<char> _http_packet_data;
	std::string _empty; //helps to returnning empty reference to empty string 
	std::string _status_code;
	std::string _method;
	std::string _uri;
	std::string _version;
	std::map<std::string,std::string,ci_less> _headers;	
	std::map<std::string,std::string> _duplicate_headers;	
	std::experimental::string_view _http_header;
	std::experimental::string_view _http_body;
};
}

#endif
