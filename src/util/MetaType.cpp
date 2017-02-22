#include "MetaType.h"

namespace rpt{
//----------------------ObjectType Class Impl-------------------------------

MetaType& ObjectType::operator [](const std::string& name){
	auto pos = name.find(".");
	if( pos!=std::string::npos)
	{
		const std::string& name_ref = name.substr(0,pos);
		const std::string& name_rem = name.substr(pos+1);
		auto it=_Index.find(name_ref);
		if(it != _Index.end()){
			return _DataTypes[it->second][name_rem];	
		}	
		else{
			_DataTypes.push_back(MetaType(name_ref,std::make_unique<ObjectType>()));
			std::size_t index = _DataTypes.size() -1 ;
			_Index.insert(std::pair<std::string,std::size_t>(name_ref,index));
			return _DataTypes[index][name_rem];
		}
	}
	else
	{
		auto it=_Index.find(name);
		if(it != _Index.end()){
			return _DataTypes[it->second];	
		}	
		else
		{
			_DataTypes.push_back(MetaType(name,std::make_unique<ObjectType>()));
			std::size_t index = _DataTypes.size() -1 ;
			_Index.insert(std::pair<std::string,std::size_t>(name,index));
			return _DataTypes[index];
		}
	}
}
std::ostream& ObjectType::writeTo(std::ostream& os) {
	std::vector<std::string> names(_DataTypes.size());
	for(auto const &i : _Index){
		names[i.second] = i.first;
	}
	os << "{";
	std::size_t i=0;
	for(; i<_DataTypes.size()-1; ++i)
		os<<"\""<<names[i]<<"\"" <<": " << _DataTypes[i] <<", ";
	if(_DataTypes.size())
		os<<"\""<<names[i]<<"\"" <<": " << _DataTypes[i];
	os<<"}";
	return os;
}

//----------------------Array Type------------------------

std::ostream& ArrayType::writeTo(std::ostream& os)
{
	os<<"[";	
	std::size_t i=0;
	for(;i<_DataTypes.size()-1; ++i){
		os<<_DataTypes[i]<<", ";
	}
	if(_DataTypes.size())
		os<<_DataTypes[i];
	os<<"]";	
	return os;
}
}
