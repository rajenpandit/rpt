//--------------------- StrinToType -----------------

template<>
struct StringToType<int>{
	int operator() (const std::string& s){
		return std::stoi(s);
	}
};
template<>
struct StringToType<long>{
	long operator() (const std::string& s){
		return std::stol(s);
	}
};
template<>
struct StringToType<long long>{
	long long operator() (const std::string& s){
		return std::stoll(s);
	}
};
template<>
struct StringToType<unsigned long>{
	unsigned long operator() (const std::string& s){
		return std::stoul(s);
	}
};
template<>
struct StringToType<unsigned long long>{
	unsigned long long operator() (const std::string& s){
		return std::stoull(s);
	}
};
template<>
struct StringToType<float>{
	float operator() (const std::string& s){
		return std::stof(s);
	}
};
template<>
struct StringToType<double>{
	double operator() (const std::string& s){
		return std::stod(s);
	}
};
template<>
struct StringToType<long double>{
	long double operator() (const std::string& s){
		return std::stold(s);
	}
};
//--------------------- MetaType ---------------------
template<class T>
struct Type<T,typename std::enable_if_t<std::is_arithmetic<T>::value>>{
	using type = GenericType<T> ;
};

template<>
struct Type<const char*,void>{
	using type = StringType;
};
template<>
struct Type<std::string,void>{
	using type = StringType;
};
template<class T>
struct Type<std::reference_wrapper<T>,void>{
	using type = ReferenceType<std::reference_wrapper<T>>;
};
template<>
struct Type<ArrayType,void>{
	using type = ArrayType;
};
template<>
struct Type<ArrayType&,void>{
	using type = ArrayType&;
};
template<>
struct Type<ObjectType,void>{
	using type = ObjectType;
};
template<>
struct Type<ObjectType&,void>{
	using type = ObjectType&;
};
template<class T>
struct Node<T,typename std::enable_if_t<std::is_arithmetic<T>::value>>{
	using type = GenericType<T> ;
	Node(T&& val) : value(std::forward<T>(val)){
	}
	T value;
};
template<>
struct Node<const char*,void>{
	using type = StringType;
	Node(const char*& val) : value(val){
	}
	const std::string& value;
};
template<>
struct Node<std::string,void>{
	using type = StringType;
	Node(const std::string& val) : value(val){
	}
	const std::string& value;
};
template<class T>
struct Node<std::reference_wrapper<T>,void>{
	using type = ReferenceType<std::reference_wrapper<T>>;
	Node(const std::reference_wrapper<T>& val) : value(val){
	}
	const std::reference_wrapper<T>& value;
};
template<>
struct Node<ArrayType,void>{
	using type = ArrayType;
	Node(const ArrayType& val) : value(val){
	}
	const ArrayType& value;
};

template<>
struct Node<ObjectType,void>{
	using type = ObjectType;
	Node(const ObjectType& val) : value(val){
	}
	const ObjectType& value;
};
template<>
struct MetaType::Assign<MetaType,false>
{
	auto makeMetaTypePtr(MetaType val){
		return std::move(val._MetaTypePtr);
	}
};
inline
bool MetaType::set_default(const std::string& name){
	return _MetaTypePtr->set_default(name);
}
inline
const MetaType& MetaType::operator [] (const std::string& name) const
{
	return (*_MetaTypePtr)[name];
}
inline
const MetaType& MetaType::operator [] (std::size_t index) const
{
	return (*_MetaTypePtr)[index];
}
inline
MetaType& MetaType::operator [] (const std::string& name)
{
	return (*_MetaTypePtr)[name];
}
inline
MetaType& MetaType::operator [] (std::size_t index)
{
	return (*_MetaTypePtr)[index];
}
inline
const ObjectType& MetaType::append(const ObjectType& obj)
{
	return _MetaTypePtr->append(obj);
} 
inline
const ArrayType& MetaType::append(const ArrayType& arr)
{
	return _MetaTypePtr->append(arr);
}


inline
std::string MetaType::get_type_name() const{
	return _MetaTypePtr->get_type_name();
}
template<class T>
inline
void MetaType::push_back(T val){
	_MetaTypePtr->push_back(Assign<typename Node<T>::type>().makeMetaTypePtr(val));
}
template<class T>
inline
void MetaType::push_back(const std::string& name,T val){
	ObjectType Obj;
	Obj[name]=val;
	push_back(Obj);
}

inline
void MetaType::operator = (const MetaType& Obj){
	_MetaTypePtr = std::move(Obj._MetaTypePtr->clone());
	_NodeName = Obj._NodeName;
}
inline
std::ostream& operator << (std::ostream& os, const MetaType& Obj){
	if(Obj._MetaTypePtr != nullptr)
		os << (*Obj._MetaTypePtr);
	return os;
}

//--------------------- GenericType ---------------------
template<class T>
inline
std::ostream& GenericType<T>::write_to(std::ostream& os) const
{
	os << _value;
	return os;
}
template<>
inline
std::ostream& GenericType<char>::write_to(std::ostream& os) const
{
	os << "\""<< _value <<"\"";
	return os;
}

//--------------------- ObjectType ---------------------
template<class T>
inline
T ObjectType::getHelper<T,true>::operator()(const ObjectType* obj,const std::string& path) const
{
	auto pos = path.find(".");
	if( pos!=std::string::npos)
	{
		const std::string& name = path.substr(0,pos);
		auto it=obj->_Index.find(name);
		if(it != obj->_Index.end()){
			int index = it->second;
			auto x = dynamic_cast<ObjectType*>( obj->_DataTypes[index].get() );
			if(x != nullptr)
				return x->get<T>(path.substr(pos+1));
		}
		else{
			std::cout<<"Invalide Generic Object"<<std::endl;
			throw MetaTypeException("Invalid Generic Object");
		}
	}
	else
	{
		auto it=obj->_Index.find(path);
		if(it != obj->_Index.end()){
			int index = it->second;
#			define CHECK_DATA_TYPE(TYPE) \
			if(obj->_DataTypes[index]->get_type_name() == #TYPE){\
				auto x = dynamic_cast<GenericType<TYPE>*>( obj->_DataTypes[index].get() );\
				return *x;\
			}else 
#			include "GenericType.def"				//else 
			if(obj->_DataTypes[index]->get_type_name() == "StringType")
			{
				auto x = dynamic_cast<StringType*>( obj->_DataTypes[index].get() );\
				std::string s = *x;
				return StringToType<T>{}(s);\
			}
			else if((!std::is_same<T,ObjectType>::value) && 
					(obj->_DataTypes[index]->get_type_name() == ObjectType::get_type_name())){
				const MetaType& metaType = obj->_DataTypes[index];
				auto x = dynamic_cast<ObjectType*>( metaType.get() );
				if(x != nullptr && (x->_Default >= 0)){
					return x->get<T>(x->_Default).second;
				}
				throw MetaTypeException("Data Type not defined");
			}
			else
			{
				throw MetaTypeException("Data Type not defined");
				//: data type not found
			}
#			undef CHECK_DATA_TYPE
		}	
		else{
			//Data not found exception
			throw MetaTypeException("Invalid Generic Object");
		}
	}
}
template<class T>
inline
T ObjectType::getHelper<T,false>::operator()(const ObjectType* obj,const std::string& path) const
{
	auto pos = path.find(".");
	if( pos!=std::string::npos)
	{
		auto a_pos_start= path.substr(0,pos).find("[");
		if(a_pos_start == std::string::npos)
			a_pos_start = pos;
		const std::string& name=path.substr(0,a_pos_start);
		auto it=obj->_Index.find(name);
		if(it != obj->_Index.end()){
			auto a_pos_end = path.substr(a_pos_start,pos-a_pos_start).find("]");
			if(a_pos_start != pos && a_pos_end!= std::string::npos){
				try{
					int index = std::stoi(path.substr(a_pos_start+1,a_pos_end-(a_pos_start+1)));
					if(index >= 0){
						auto x = dynamic_cast<ArrayType*>( obj->_DataTypes[it->second].get() );
						auto objType = x->get<ObjectType&>(index);
						return objType.get<T>(path.substr(pos+1));
					}
					else
						throw MetaTypeException("Invalid Index");
				}catch(...){
					throw MetaTypeException("Invalid Index");
				}
			}
			else{
				int index = it->second;
				auto x = dynamic_cast<ObjectType*>( obj->_DataTypes[index].get() );
				if(x != nullptr)
					return x->get<T>(path.substr(pos+1));
			}
		}
		else
		{
			std::cout<<"Invalid Object"<<std::endl;
			throw MetaTypeException("Invalid Object");
		}
	}
	else
	{
		auto a_pos_start= path.find("[");
		const std::string& name = path.substr(0,a_pos_start);	
		auto it=obj->_Index.find(name);
		if(it != obj->_Index.end()){
			auto a_pos_end = path.find("]");
			if((a_pos_start != std::string::npos) && 
					(a_pos_end != std::string::npos)){
				try{
					int index = std::stoi(path.substr(a_pos_start+1,a_pos_end-(a_pos_start+1)));
					if(index >= 0){
						auto x = dynamic_cast<ArrayType*>( obj->_DataTypes[it->second].get() );
						return x->get<T>(index);
					}
					else
						throw MetaTypeException("Invalid Index");
				}catch(...){
					throw MetaTypeException("Invalid Index");
				}

			}
			else{
				int index = it->second;
				if((!std::is_same<T,ObjectType>::value) && 
					(obj->_DataTypes[index]->get_type_name() == ObjectType::get_type_name())){
					const MetaType& metaType = obj->_DataTypes[index];
					auto x = dynamic_cast<ObjectType*>( metaType.get() );
					if(x != nullptr && (x->_Default >= 0)){
						return x->get<T>(x->_Default).second;
					}
					throw MetaTypeException("Data Type not defined");
				}
				auto x = dynamic_cast<typename std::remove_reference_t<typename Type<T>::type>*>( obj->_DataTypes[index].get() );
				if(x != nullptr)
					return *x;
				else
					throw MetaTypeException("Invalid Type Conversion");
			}
		}
		else{
			//Data not found exception
			throw MetaTypeException("Data Type not defined");
		}
	}
}

template<class T>
inline
std::pair<std::string,T> ObjectType::getHelper<T,true>::operator()(const ObjectType* obj,int index) const
{
	if(index < obj->_DataTypes.size()){
		const MetaType& metaType = obj->_DataTypes[index];
#		define CHECK_DATA_TYPE(TYPE) \
		if(obj->_DataTypes[index]->get_type_name() == #TYPE){\
			auto x = dynamic_cast<GenericType<TYPE>*>( metaType.get() );\
			return std::make_pair(metaType.get_name(),*x);\
		}else 
#		include	"GenericType.def"
		if((!std::is_same<T,ObjectType>::value) && 
				(obj->_DataTypes[index]->get_type_name() == ObjectType::get_type_name())){
			auto x = dynamic_cast<ObjectType*>( metaType.get() );
			if(x != nullptr && (x->_Default >= 0)){
				return x->get<T>(x->_Default);
			}
			throw MetaTypeException("Data Type not defined");
		}
		else 
		{
			throw MetaTypeException("Data Type not defined");
			//: data type not found
		}
#		undef CHECK_DATA_TYPE
	}	
	else{
		//Data not found exception
		throw MetaTypeException("Invalid Generic Object");
	}
}
template<class T, bool = std::is_reference<typename Type<T>::type>::value>
struct make_pair_helper;
template<class T>
struct make_pair_helper<T,true>{
	std::pair<std::string,T> operator()(const std::string& name, T value){
		return std::make_pair(name,std::ref(value));
	}
};
template<class T>
struct make_pair_helper<T,false>{
	std::pair<std::string,T> operator()(const std::string& name, T value){
		return std::make_pair(name,value);
	}
};
template<class T>
inline
std::pair<std::string,T> ObjectType::getHelper<T,false>::operator()(const ObjectType* obj, int index) const
{
	if(index < obj->_DataTypes.size()){
		const MetaType& metaType = obj->_DataTypes[index];
		if((!std::is_same<T,ObjectType>::value) && 
				(metaType->get_type_name() == ObjectType::get_type_name())){
			auto x = dynamic_cast<ObjectType*>( metaType.get() );
			if(x != nullptr && (x->_Default >= 0)){
				std::cout<<"Default:"<<x->_Default<<std::endl;
				return x->get<T>(x->_Default);
			}
		}
		auto x = dynamic_cast<typename std::remove_reference_t<typename Type<T>::type>*>( metaType.get() );
		if(x != nullptr)
		{
			return make_pair_helper<T>{}(metaType.get_name(),*x);
		}
		else
			throw MetaTypeException("Invalid Type Conversion");
	}
	else{
		//Data not found exception
		throw MetaTypeException("Data Type not defined");
	}
}



template<class T>
inline
T ArrayType::getHelper<T,true>::operator()(const ArrayType* obj,int index) const
{
	if(index < obj->_DataTypes.size()){
		const MetaType& metaType = obj->_DataTypes[index];
#		define CHECK_DATA_TYPE(TYPE) \
		if(obj->_DataTypes[index]->get_type_name() == #TYPE){\
			auto x = dynamic_cast<GenericType<TYPE>*>( metaType.get() );\
			return std::make_pair(metaType.get_name(),*x);\
		}else 
#		include	"GenericType.def"
		//else 
		if((!std::is_same<T,ObjectType>::value) && 
				(metaType->get_type_name() == ObjectType::get_type_name())){
			auto x = dynamic_cast<ObjectType*>( metaType.get() );
			if(x != nullptr && (x->_Default >= 0)){
				std::cout<<"Default:"<<x->_Default<<std::endl;
				return x->get<T>(x->_Default);
			}
		}
		else
		{
			throw MetaTypeException("Data Type not defined");
			//: data type not found
		}
#		undef CHECK_DATA_TYPE
	}	
	else{
		//Data not found exception
		throw MetaTypeException("Invalid Generic Object");
	}
}

template<class T>
inline
T ArrayType::getHelper<T,false>::operator()(const ArrayType* obj, int index) const
{
	if(index < obj->_DataTypes.size()){
		const MetaType& metaType = obj->_DataTypes[index];
		if((!std::is_same<T,ObjectType>::value) && (metaType->get_type_name() == ObjectType::get_type_name())){
			auto x = dynamic_cast<ObjectType*>( metaType.get() );
			if(x != nullptr && (x->_Default >= 0)){
				std::cout<<"Default:"<<x->_Default<<std::endl;
				return x->get<T>(x->_Default).second;
			}
		}
		auto x = dynamic_cast<typename std::remove_reference_t<typename Type<T>::type>*>( metaType.get() );
		if(x != nullptr)
			return *x;
		else
			throw MetaTypeException("Invalid Type Conversion");
	}
	else{
		//Data not found exception
		throw MetaTypeException("Data Type not defined");
	}
}
