#ifndef __META_TYPE_H__16012017_RAJEN__
#define __META_TYPE_H__16012017_RAJEN__
#include <map>
#include <iostream>
#include <memory>
#include <typeinfo>
#include <cxxabi.h>
#include <type_traits>
#include <vector>
#include <execinfo.h>
namespace rpt{
///helper type for identifier, used in get call
using pair_t = std::pair<std::string,int>;
template<class T, class=void>
struct Type;

template<class T, class=void>
struct Node;

class MetaTypeImpl; /// forward declaration



class MetaTypeException : public std::exception{
public:
	explicit MetaTypeException(const std::string& what_arg) : _reason(what_arg){
	}
	explicit MetaTypeException(const char* what_arg) : _reason(what_arg){
	}
	const char* what() const noexcept override{
		return _reason.c_str();
	}
private:
	private:
	std::string _reason;
};
class MetaType
{
public:
	MetaType(){}
	MetaType(const std::string& node_name, std::unique_ptr<MetaTypeImpl> ptr) : 
		_NodeName(node_name),_MetaTypePtr(std::move(ptr)){
	}
	MetaType(std::unique_ptr<MetaTypeImpl> ptr) : 
		_MetaTypePtr(std::move(ptr)){
	}
	MetaType(const MetaType& Obj){
		(*this) = Obj;
	}
public:
	void operator = (const MetaType& Obj);

	template<typename T,bool = std::is_base_of<MetaTypeImpl,std::remove_reference_t<T> >::value>
	struct Assign;


	template<typename T>
	struct Assign<T,true>
	{
		std::unique_ptr<T> makeMetaTypePtr(T val){
			return std::make_unique<T>(val);
		}
	};

	template<typename T>
	struct Assign<T,false>
	{
		static_assert(std::is_base_of<MetaTypeImpl,T>::value,"Invalid assignment type");
	};
	template<class T>
	MetaType& operator = (T val){
		_MetaTypePtr=Assign<typename Node<T>::type>().makeMetaTypePtr(val);
		return *this;
	}
	std::unique_ptr<MetaTypeImpl>& operator * (){
		return _MetaTypePtr;
	}

	std::unique_ptr<MetaTypeImpl>& operator -> (){
		return _MetaTypePtr;
	}

	MetaType& operator [] (const std::string& name);
	MetaType& operator [] (std::size_t index);
	template<class T>
	void push_back(T val);
	template<class T>
	void push_back(const std::string&,T val);
	MetaTypeImpl* get(){
		return _MetaTypePtr.get();
	}
	std::string getName() const{
		return _NodeName;
	}
	std::string getTypeName() const;
	friend std::ostream& operator << (std::ostream& os, const MetaType& Obj);
private:
	std::string _NodeName;
	std::unique_ptr<MetaTypeImpl> _MetaTypePtr;
};
class MetaTypeImpl
{
public:
	template<class T>
	MetaTypeImpl(T*) 
	{
		int status = -1;
		std::unique_ptr<char,void(*)(void*)> res{
			abi::__cxa_demangle(typeid(T).name(), NULL, NULL, &status),
				std::free
		};
		if(status == 0)
			_typeName = res.get();
	}
public://operators
	virtual MetaType& operator [] (const std::string& name){
		throw MetaTypeException("Error: [Type mismatch] Destination is not an ObjectType");
	}
	virtual MetaType& operator [] (std::size_t index){
		throw MetaTypeException("Error: [Type mismatch] Destination is not an ArrayType");
	}
	virtual void push_back(std::unique_ptr<MetaTypeImpl>&& MetaTypePtr){
		throw MetaTypeException("Error: [Type mismatch] Destination is not an ArrayType");
	}
	virtual std::unique_ptr<MetaTypeImpl> clone() = 0;
	virtual std::ostream& writeTo(std::ostream& os)=0;;
public:
	const std::string& getTypeName() const
	{
		return _typeName;
	}
	friend std::ostream& operator << (std::ostream& os, MetaTypeImpl& Obj)
	{
		return Obj.writeTo(os);
	}
protected:
	std::string _typeName;
};


template<class T>
class GenericType : public MetaTypeImpl{
static_assert(std::is_arithmetic<T>::value,
		"Required Integral or Floating Point data type");
public:
	GenericType(T v=0):MetaTypeImpl(&v), _value(v){
	}
	
public:
	std::unique_ptr<MetaTypeImpl> clone() override{
		return std::make_unique<GenericType>(_value);
	}
	std::ostream& writeTo(std::ostream& os);
	
public://operators
	operator T(){
		return _value;
	}
	template<class U>
	void operator = (U val){
		_value = val;
	}
private:
	T _value;
};

class StringType : public MetaTypeImpl
{
public:
	StringType(const std::string& s="") : MetaTypeImpl(this),
		_value(s){
	}
	StringType(const char* s="") : MetaTypeImpl(this),
		_value(s){
	}
public:
	std::unique_ptr<MetaTypeImpl> clone() override{
		return std::make_unique<StringType>(_value);
	}

	std::ostream& writeTo(std::ostream& os) override{
		os <<"\""<< _value<<"\"";
		return os;
	}
public://operator
	operator std::string(){
		return _value;
	}
	void operator = (const std::string&  val){
		_value = val;
	}
	friend std::ostream& operator << (std::ostream& os, const StringType& st)
	{
		os <<"\""<<st._value<<"\"";
		return os;
	}
private:
	std::string _value;
};

template<class T>
class ReferenceType;
template<class T>
class ReferenceType<std::reference_wrapper<T>> : public MetaTypeImpl
{
public:
	ReferenceType(const std::reference_wrapper<T>& s) : MetaTypeImpl(&s),
		_value(s){
	}
public:
	std::unique_ptr<MetaTypeImpl> clone() override{
		return std::make_unique<ReferenceType<std::reference_wrapper<T>>>(_value);
	}

	std::ostream& writeTo(std::ostream& os) override{
		os <<"\""<< (T)_value<<"\"";
		return os;
	}
public://operator
	operator std::reference_wrapper<T>(){
		return _value;
	}
	void operator = (const std::reference_wrapper<T>& val){
		_value = val;
	}
	friend std::ostream& operator << (std::ostream& os, const ReferenceType& st)
	{
		os <<"\""<<st._value<<"\"";
		return os;
	}
private:
	std::reference_wrapper<T> _value;
};


class ObjectType : public MetaTypeImpl
{
public:
	ObjectType() : MetaTypeImpl(this){
	}
	ObjectType(const ObjectType& Obj) : MetaTypeImpl(this){
		(*this) = Obj;
	}
	template<class T>
	ObjectType(const std::string& name, T val) : MetaTypeImpl(this){
		(*this)[name]=val;
	}
	template<class T>
	ObjectType(std::initializer_list<std::pair<std::string,T>> list) : MetaTypeImpl(this){
		for(auto l : list){
			(*this)[l.first]=l.second;
		}
	}
public:
	std::unique_ptr<MetaTypeImpl> clone() override{
		return std::make_unique<ObjectType>(*this);	
	}
	std::ostream& writeTo(std::ostream& os) override;
public: //operators
	MetaType& operator [](const std::string& name) override;
	MetaType& operator [](std::size_t index) override
	{
		return _DataTypes[index];
	}
	//MetaType& operator [](std::size_t index) override;

private:
	//-------------- get Helper ---------------------
	template<class T,bool=std::is_arithmetic<std::remove_reference_t<T>>::value>
		struct getHelper;

	template<typename T>
		struct getHelper<T,true>{
			T operator()(ObjectType* obj,const std::string& name);
			std::pair<std::string,T> operator()(ObjectType* obj,int index);
		};

	template<typename T>
		struct getHelper<T,false>{
			T operator()(ObjectType* obj,const std::string& name);
			std::pair<std::string,T> operator()(ObjectType* obj,int index);
		};
	//-------------------------------------------------


public:
	template<class T>
	T get(const std::string& name){
		return getHelper<T>{}(this,name);		
	}
	template<class T>
	std::pair<std::string,T> get(int index){
		return getHelper<T>{}(this,index);		
	};
	friend std::ostream& operator << (std::ostream& os, ObjectType& Obj)
	{
		return Obj.writeTo(os);
	}
	std::size_t size() const{
		return _DataTypes.size();
	}
private:
	std::map<std::string,std::size_t> _Index;
	std::vector<MetaType>  _DataTypes;
};
class ArrayType: public MetaTypeImpl{
public:
	ArrayType() : MetaTypeImpl(this){
	}
	ArrayType(std::size_t size) : MetaTypeImpl(this),_DataTypes(size){
	}
	ArrayType(const ArrayType& Obj) : MetaTypeImpl(this){
		(*this) = Obj;
	}
private:
	//-------------- get Helper ---------------------
	template<class T,bool=std::is_arithmetic<std::remove_reference_t<T>>::value>
		struct getHelper;

	template<typename T>
		struct getHelper<T,true>{
			T operator()(ArrayType* obj,int index);
		};

	template<typename T>
		struct getHelper<T,false>{
			T operator()(ArrayType* obj,int index);
		};
	//-------------------------------------------------
public:
	template<class T>
	T get(int index){
		return getHelper<T>{}(this,index);		
	}
public:
	std::unique_ptr<MetaTypeImpl> clone() override{
		return std::make_unique<ArrayType>(*this);	
	}
public: //operators
	MetaType& operator [](std::size_t index) override
	{
		return _DataTypes[index];
	}
	void push_back(std::unique_ptr<MetaTypeImpl>&& MetaTypePtr) override{
		_DataTypes.emplace_back(std::move(MetaTypePtr));
	}
	std::ostream& writeTo(std::ostream& os) override;
	std::size_t size() const{
		return _DataTypes.size();
	}
private:
	std::vector<MetaType> _DataTypes;
};

#include "MetaType.inl"

}
#endif
