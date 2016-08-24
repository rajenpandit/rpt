#ifndef __UTILS_H__07032016_RAJEN__
#define __UTILS_H__07032016_RAJEN__
namespace rpt{
struct utils{
	/* trim a string */
	static std::string ltrim(const std::string& s){
		std::string str = s;
		std::string::size_type pos = str.find_first_not_of(' ');
		if( pos != std::string::npos)
			str.erase(0,pos);
		return str;
	}
	static std::string rtrim(const std::string& s){
		std::string str = s;
		std::string::size_type pos = str.find_last_not_of(' ');
		if( pos != std::string::npos)
			str.erase(pos+1);
		return str;
	}
	static std::string trim(const std::string& str){
		return rtrim(ltrim(str));
	}
	/* ---------------------- */
	
	static std::pair<std::string,std::string> get_name_value_pair(const std::string& str, const std::string& sep){
		std::string::size_type pos;
		if((pos = str.find(sep)) != std::string::npos){
			std::string name = str.substr(0,pos);
			std::string value = str.substr(pos+1);	
			return std::make_pair(name,value);
		}
		return std::pair<std::string,std::string>(); //returning empty pair, with default value (empty) of string 
	}
};
}
#endif
