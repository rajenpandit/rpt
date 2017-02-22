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
std::string MetaType::getTypeName() const{
	return _MetaTypePtr->getTypeName();
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
	os << (*Obj._MetaTypePtr);
	return os;
}

//--------------------- GenericType ---------------------
template<class T>
inline
std::ostream& GenericType<T>::writeTo(std::ostream& os)
{
	os << _value;
	return os;
}
template<>
inline
std::ostream& GenericType<char>::writeTo(std::ostream& os)
{
	os << "\""<< _value <<"\"";
	return os;
}

//--------------------- ObjectType ---------------------
template<class T>
inline
T ObjectType::getHelper<T,true>::operator()(ObjectType* obj,const std::string& name)
{
	auto pos = name.find(".");
	if( pos!=std::string::npos)
	{
		auto it=obj->_Index.find(name.substr(0,pos));
		if(it != obj->_Index.end()){
			int index = it->second;
			auto x = dynamic_cast<ObjectType*>( obj->_DataTypes[index].get() );
			if(x != nullptr)
				return x->get<T>(name.substr(pos+1));
		}
		else{
			std::cout<<"Invalide Generic Object"<<std::endl;
			throw MetaTypeException("Invalid Generic Object");
		}
	}
	else
	{
		auto it=obj->_Index.find(name);
		if(it != obj->_Index.end()){
			int index = it->second;
#			define CHECK_DATA_TYPE(TYPE) \
			if(obj->_DataTypes[index]->getTypeName() == #TYPE){\
				auto x = dynamic_cast<GenericType<TYPE>*>( obj->_DataTypes[index].get() );\
				return *x;\
			}else 

#			include "GenericType.def"				//else 
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
T ObjectType::getHelper<T,false>::operator()(ObjectType* obj,const std::string& name)
{
	auto pos = name.find(".");
	if( pos!=std::string::npos)
	{
		auto it=obj->_Index.find(name.substr(0,pos));
		if(it != obj->_Index.end()){
			int index = it->second;
			auto x = dynamic_cast<ObjectType*>( obj->_DataTypes[index].get() );
			if(x != nullptr)
				return x->get<T>(name.substr(pos+1));
		}
		else
		{
			std::cout<<"Invalid Object"<<std::endl;
			throw MetaTypeException("Invalid Object");
		}
	}
	else
	{
		auto it=obj->_Index.find(name);
		if(it != obj->_Index.end()){
			int index = it->second;
			auto x = dynamic_cast<typename std::remove_reference_t<typename Type<T>::type>*>( obj->_DataTypes[index].get() );
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
}

template<class T>
inline
std::pair<std::string,T> ObjectType::getHelper<T,true>::operator()(ObjectType* obj,int index)
{
	if(index < obj->_DataTypes.size()){
		MetaType& metaType = obj->_DataTypes[index];
#		define CHECK_DATA_TYPE(TYPE) \
		if(obj->_DataTypes[index]->getTypeName() == #TYPE){\
			auto x = dynamic_cast<GenericType<TYPE>*>( metaType.get() );\
			return std::make_pair(metaType.getName(),*x);\
		}else 
#		include	"GenericType.def"
		//else 
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
std::pair<std::string,T> ObjectType::getHelper<T,false>::operator()(ObjectType* obj, int index)
{
	if(index < obj->_DataTypes.size()){
		MetaType& metaType = obj->_DataTypes[index];
		auto x = dynamic_cast<typename std::remove_reference_t<typename Type<T>::type>*>( metaType.get() );
		if(x != nullptr)
		{
			return make_pair_helper<T>{}(metaType.getName(),*x);
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
T ArrayType::getHelper<T,true>::operator()(ArrayType* obj,int index)
{
	if(index < obj->_DataTypes.size()){
		MetaType& metaType = obj->_DataTypes[index];
#				define CHECK_DATA_TYPE(TYPE) \
		if(obj->_DataTypes[index]->getTypeName() == #TYPE){\
			auto x = dynamic_cast<GenericType<TYPE>*>( metaType.get() );\
			return *x;\
		}else 

		CHECK_DATA_TYPE(char)
			CHECK_DATA_TYPE(int)
			CHECK_DATA_TYPE(long)
			CHECK_DATA_TYPE(long int)
			CHECK_DATA_TYPE(float)
			CHECK_DATA_TYPE(double)
			CHECK_DATA_TYPE(long double)
			CHECK_DATA_TYPE(unsigned char)
			CHECK_DATA_TYPE(unsigned int)
			CHECK_DATA_TYPE(unsigned long)
			CHECK_DATA_TYPE(unsigned long int)
			//else 
			{
				throw MetaTypeException("Data Type not defined");
				//: data type not found
			}
#				undef CHECK_DATA_TYPE
	}	
	else{
		//Data not found exception
		throw MetaTypeException("Invalid Generic Object");
	}
}

template<class T>
inline
T ArrayType::getHelper<T,false>::operator()(ArrayType* obj, int index)
{
	if(index < obj->_DataTypes.size()){
		MetaType& metaType = obj->_DataTypes[index];
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
