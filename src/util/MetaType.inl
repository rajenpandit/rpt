#define EXCEPTION_MSG(ss,msg)	ss << __FILE__<<":"<<__LINE__<<" "<<msg; 
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
const MetaType& MetaType::append(const MetaType& mt)
{
	if(mt.get_type_name() == utils::type<ObjectType>::name()){
		const ObjectType& obj = *(static_cast<ObjectType*>(mt._MetaTypePtr.get()));
		_MetaTypePtr->append(obj);
	}
	else if(mt.get_type_name() == utils::type<ArrayType>::name()){
		const ArrayType& arr = *(static_cast<ArrayType*>(mt._MetaTypePtr.get()));
		_MetaTypePtr->append(arr);
	}
	return *this;	
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
T MetaType::get(const std::string& name) const
{
	if(_MetaTypePtr == nullptr)
		throw MetaTypeException("null pointer exception");
	if(get_type_name() == utils::type<ObjectType>::name())
	{
		return (dynamic_cast<ObjectType*>(_MetaTypePtr.get()))->get<T>(name);
	}
	else
	{
		throw MetaTypeException("Operation MetaType::get(const std::string&) not supported for type:"+get_type_name());
	}
}

template<class T>
inline
T MetaType::get(int index) const
{
	if(_MetaTypePtr == nullptr)
		throw MetaTypeException("null pointer exception");
	if(get_type_name() == utils::type<ArrayType>::name())
	{
		return (dynamic_cast<ArrayType*>(_MetaTypePtr.get()))->get<T>(index);
	}
	else
	{
		throw MetaTypeException("Operation MetaType::get(int) not supported for type:"+get_type_name());
	}
}
inline
std::size_t MetaType::size() const{
return _MetaTypePtr->size();
}
inline
void MetaType::push_back(MetaType& val){
	_MetaTypePtr->push_back(val);
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
						if(x != nullptr){
							auto objType = x->get<ObjectType&>(index);
							return objType.get<T>(path.substr(pos+1));
						}
						else if(index==0 && obj->_CheckArray){
							obj->_CheckArray = false;
							return obj->get<T>(name + path.substr(pos)); //remove [0] sbuscript
						}
					}
					std::stringstream ss;
					EXCEPTION_MSG(ss,"Invalid Index");
					throw MetaTypeException(ss.str());
				}catch(MetaTypeException& e){
					throw MetaTypeException(e.what());
					//throw MetaTypeException("Invalid Index");
				}
			}
			else{
				int index = it->second;
				auto x = dynamic_cast<ObjectType*>( obj->_DataTypes[index].get() );
				if(x != nullptr)
					return x->get<T>(path.substr(pos+1));
				else
					return obj->get<T>(name + "[0]"+ path.substr(pos));		
			}
		}
		std::stringstream ss;
		EXCEPTION_MSG(ss,"Invalid Object");
		throw MetaTypeException(ss.str());
	}
#if 0
	{
		const std::string& name = path.substr(0,pos);
		std::cout<<"name:"<<name<<std::endl;
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
#endif
	else
	{
#if 1
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
					{
						std::stringstream ss;
						EXCEPTION_MSG(ss,"Invalid Index");
						throw MetaTypeException(ss.str());
					}
				}catch(MetaTypeException& e){
					throw MetaTypeException(e.what());
				}

			}
			else
			{
				int index = it->second;
#			define CHECK_DATA_TYPE(TYPE) \
				if(obj->_DataTypes[index]->get_type_name() == #TYPE){\
					auto x = dynamic_cast<GenericType<TYPE>*>( obj->_DataTypes[index].get() );\
					return *x;\
				}else 
#			include "GenericType.def"				//else 
				if(obj->_DataTypes[index]->get_type_name() == utils::type<StringType>::name())
				{
					auto x = dynamic_cast<StringType*>( obj->_DataTypes[index].get() );\
						 std::string s = *x;
					return StringToType<T>{}(s);
				}
				else if((!std::is_same<T,ObjectType>::value) && 
						(obj->_DataTypes[index]->get_type_name() == ObjectType::get_type_name())){
					const MetaType& metaType = obj->_DataTypes[index];
					auto x = dynamic_cast<ObjectType*>( metaType.get() );
					if(x != nullptr && (x->_Default >= 0)){
						return x->get<T>(x->_Default).second;
					}
					throw MetaTypeException("Data Type not defined1");
				}
				/**
				 * If path is refering to an ArrayType, but path doesnt contain array subscript [], 
				 * allow user to access 0th index i.e ArrayType[0].
				 */
				else if(obj->_DataTypes[index]->get_type_name() == ArrayType::get_type_name()){
					return obj->get<T>(path+"[0]");
				}
				else
				{
					throw MetaTypeException("Data Type not defined2");
					//: data type not found
				}
#			undef CHECK_DATA_TYPE
			}
		}	
#else

		auto it=obj->_Index.find(path);
		if(it != obj->_Index.end()){
			int index = it->second;
#                       define CHECK_DATA_TYPE(TYPE) \
			if(obj->_DataTypes[index]->get_type_name() == #TYPE){\
				auto x = dynamic_cast<GenericType<TYPE>*>( obj->_DataTypes[index].get() );\
				return *x;\
			}else 
#                       include "GenericType.def"                               //else 
			if(obj->_DataTypes[index]->get_type_name() == utils::type<StringType>::name())
			{
				auto x = dynamic_cast<StringType*>( obj->_DataTypes[index].get() );\
					 std::string s = *x;
				return StringToType<T>{}(s);\
			}
			/**
			 * Requested type is Generic Type, but path is refering to an object.
			 * Check for default field, if configured. If not, then throw error.
			 */
			else if((!std::is_same<T,ObjectType>::value) &&
					(obj->_DataTypes[index]->get_type_name() == ObjectType::get_type_name())){
				const MetaType& metaType = obj->_DataTypes[index];
				auto x = dynamic_cast<ObjectType*>( metaType.get() );
				if(x != nullptr && (x->_Default >= 0)){
					return x->get<T>(x->_Default).second;
				}
				throw MetaTypeException("Data Type not defined1");
			}
			/**
			 * If path is refering to an ArrayType, but path doesnt contain array subscript [], 
			 * allow user to access 0th index i.e ArrayType[0].
			 */
			else if(obj->_DataTypes[index]->get_type_name() == ArrayType::get_type_name()){
				obj->get<T>(path+"[0]");
			}
			else
			{
				throw MetaTypeException("Data Type not defined2");
				//: data type not found
			}
#                       undef CHECK_DATA_TYPE
		}
#endif
		else{
#if 0
			//This is not ObjectType, need to check if [0] is appended then provide access by removing [0]
			auto pos = path.find("[0]");
			if(pos != std::string::npos){
				return obj->get<T>(path.substr(0,pos));
			}
#endif
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
						if(x != nullptr){
							auto objType = x->get<ObjectType&>(index);
							return objType.get<T>(path.substr(pos+1));
						}
						else if(index==0 && obj->_CheckArray){
							obj->_CheckArray = false;
							return obj->get<T>(name + path.substr(pos)); //remove [0] subscript
						}
					}
					else
					{
						std::stringstream ss;
						EXCEPTION_MSG(ss,"Invalid Object");
						throw MetaTypeException(ss.str());
					}
				}catch(MetaTypeException& e)
				{
					throw MetaTypeException(e.what());
				}
			}
			else{
				int index = it->second;
				auto x = dynamic_cast<ObjectType*>( obj->_DataTypes[index].get() );
				if(x != nullptr)
					return x->get<T>(path.substr(pos+1));
				else
					return obj->get<T>(name + "[0]"+ path.substr(pos));		
			}
		}
		else
		{
			std::stringstream ss;
			EXCEPTION_MSG(ss,"Invalid Object");
			throw MetaTypeException(ss.str());
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
						if(x != nullptr)
							return x->get<T>(index);
						else if(index==0 && obj->_CheckArray){
							obj->_CheckArray = false;
							return obj->get<T>(name); //remove [0] subscript
						}
					}
					else
					{
						std::stringstream ss;
						EXCEPTION_MSG(ss,"Invalid Index");
						throw MetaTypeException(ss.str());
					}
				}catch(MetaTypeException& e){
					throw MetaTypeException(e.what());
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

					std::stringstream ss;
					EXCEPTION_MSG(ss,"Data Type not defined");
					throw MetaTypeException(ss.str());
				}
				auto x = dynamic_cast<typename std::remove_reference_t<typename Type<T>::type>*>( obj->_DataTypes[index].get() );
				if(x != nullptr)
					return *x;
				else
				{
					return obj->get<T>(name + "[0]");		
					//std::stringstream ss;
					//EXCEPTION_MSG(ss,"Invalid Type Conversion");
					//throw MetaTypeException(ss.str());
				}
			}
		}
		else{
			//Data not found exception
			std::stringstream ss;
			EXCEPTION_MSG(ss,"Data Type not defined");
			throw MetaTypeException(ss.str());
		}
	}
	std::stringstream ss;
	EXCEPTION_MSG(ss,"Invalid Object");
	throw MetaTypeException(ss.str());
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
		if(obj->_DataTypes[index]->get_type_name() == utils::type<StringType>::name())
		{
			auto x = dynamic_cast<StringType*>( obj->_DataTypes[index].get() );
			std::string s = *x;
			//T must be GenericType
			if(s.empty())
				s.assign("0");
			return std::make_pair(metaType.get_name(),static_cast<T>(std::stod(s)));
		}
		else if((!std::is_same<T,ObjectType>::value) && 
				(obj->_DataTypes[index]->get_type_name() == ObjectType::get_type_name())){
			auto x = dynamic_cast<ObjectType*>( metaType.get() );
			if(x != nullptr && (x->_Default >= 0)){
				return x->get<T>(x->_Default);
			}
			std::stringstream ss;
			EXCEPTION_MSG(ss,"Data Type not defined");
			throw MetaTypeException(ss.str());
		}
		else 
		{
			std::stringstream ss;
			EXCEPTION_MSG(ss,"Data Type not defined");
			throw MetaTypeException(ss.str());
			//: data type not found
		}
#		undef CHECK_DATA_TYPE
	}	
	else{
		//Data not found exception
		std::stringstream ss;
		EXCEPTION_MSG(ss,"Invalid Generic Object");
		throw MetaTypeException(ss.str());
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
				return x->get<T>(x->_Default);
			}
		}
		auto x = dynamic_cast<typename std::remove_reference_t<typename Type<T>::type>*>( metaType.get() );
		if(x != nullptr)
		{
			return make_pair_helper<T>{}(metaType.get_name(),*x);
		}
		else
		{
			std::stringstream ss;
			EXCEPTION_MSG(ss,"Invalid Type Conversion");
			throw MetaTypeException(ss.str());
		}
	}
	else{
		//Data not found exception
		std::stringstream ss;
		EXCEPTION_MSG(ss,"Data Type not defined");
		throw MetaTypeException(ss.str());
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
			return *x;\
		}else 
#		include	"GenericType.def"
		//else 
		if(obj->_DataTypes[index]->get_type_name() == utils::type<StringType>::name())
		{
			auto x = dynamic_cast<StringType*>( obj->_DataTypes[index].get() );\
				 std::string s = *x;
			return StringToType<T>{}(s);
		}
		else if((!std::is_same<T,ObjectType>::value) && 
				(metaType->get_type_name() == ObjectType::get_type_name())){
			auto x = dynamic_cast<ObjectType*>( metaType.get() );
			if(x != nullptr && (x->_Default >= 0)){
				return x->get<T>(x->_Default).second;
			}
			
		}
		std::stringstream ss;
		EXCEPTION_MSG(ss,"Data Type not defined");
		throw MetaTypeException(ss.str());
		//: data type not found
#		undef CHECK_DATA_TYPE
	}	
	//Data not found exception
	std::stringstream ss;
	EXCEPTION_MSG(ss,"Invalid Generic Object");
	throw MetaTypeException(ss.str());
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
				return x->get<T>(x->_Default).second;
			}
		}
		auto x = dynamic_cast<typename std::remove_reference_t<typename Type<T>::type>*>( metaType.get() );
		if(x != nullptr)
			return *x;
		else
		{
			std::stringstream ss;
			EXCEPTION_MSG(ss,"Invalid Type Conversion");
			throw MetaTypeException(ss.str());
		}
	}
	else{
		//Data not found exception
		std::stringstream ss;
		EXCEPTION_MSG(ss,"Data Type not defined");
		throw MetaTypeException(ss.str());
	}
}
