#include "MetaType.h"
namespace rpt{
//----------------------ObjectType Class Impl-------------------------------

const MetaType& ObjectType::operator [](const std::string& name) const{
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
                        throw MetaTypeException("Data Type not defined");
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
                        throw MetaTypeException("Data Type not defined");
                }
        }
}
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
const ObjectType& ObjectType::append(const ObjectType& obj){
	/**
	 * Traverse through each elements and insert into _Index and _DataTypes if new element is found.
	 * In case if element name already exist in _DataTypes, then replace old value with new if
	 * - element type is not ArrayType and ObjectType
	 * - if element is of ArrayType then push_back each element from obj Array.
	 * - if element is of ObjectType then call recursive append.
	 * - if source element and destination element type is not matches then replace with source type. 
	 */
	for(auto node : obj._Index){
		auto it = _Index.find(node.first);
		if(it != _Index.end()){
			if(obj._DataTypes[node.second]->get_type_name() == _DataTypes[it->second]->get_type_name()){
				if(_DataTypes[it->second]->get_type_name() == ObjectType::get_type_name()){
					_DataTypes[it->second].append(obj.get<ObjectType>(node.first));
				}	
				else if(_DataTypes[it->second]->get_type_name() == ArrayType::get_type_name()){
					_DataTypes[it->second].append(obj.get<ArrayType>(node.first));

				}
				else{
					(*this)[node.first]=obj[node.first];
				}
			}
			else{
				(*this)[node.first]=obj[node.first];
			}
		}
		else{ //found new node
			(*this)[node.first]=obj[node.first];
		}
	}
	return *this;
}


std::ostream& ObjectType::write_to(std::ostream& os) const{
        if(_DataTypes.size() == 0)
        {
                os << "{}";
                return os;
        }
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

std::ostream& ArrayType::write_to(std::ostream& os) const
{
	os<<"[";	
	if(_DataTypes.size()){
		std::size_t i=0;
		for(;i<_DataTypes.size()-1; ++i){
			os<<_DataTypes[i]<<", ";
		}
		os<<_DataTypes[i];
	}
	os<<"]";	
	return os;
}

} //rpt
