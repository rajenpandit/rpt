#ifndef __META_TYPE_H__16012017_RAJEN__
#define __META_TYPE_H__16012017_RAJEN__
#include <map>
#include <sstream>
#include <iostream>
#include <memory>
#include <typeinfo>
#include <cxxabi.h>
#include <type_traits>
#include <vector>
#include <execinfo.h>
#include "utils.h"
namespace rpt{
///helper type for identifier, used in get call
template<class T>
using element_t = std::pair<std::string,T>;

template<class T>
struct StringToType;

template<class T, class=void>
struct Type;

template<class T, class=void>
struct Node;

class MetaTypeImpl; /// forward declaration
class ObjectType;
class ArrayType;



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
	MetaType():_MetaTypePtr(nullptr){}
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
	const std::unique_ptr<MetaTypeImpl>& operator * () const{
		return _MetaTypePtr;
	}

	const std::unique_ptr<MetaTypeImpl>& operator -> () const{
		return _MetaTypePtr;
	}

	const MetaType& operator [] (const std::string& name) const;
	const MetaType& operator [] (std::size_t index) const;
	MetaType& operator [] (const std::string& name);
	MetaType& operator [] (std::size_t index);
	
public:
		
	const MetaType& append(const MetaType& obj);
	const ObjectType& append(const ObjectType& obj);
	const ArrayType& append(const ArrayType& obj);
	void push_back(MetaType& val);
	std::size_t size() const;
	template<class T>
		void push_back(T val);
	template<class T>
		void push_back(const std::string&,T val);
	template<class T>
		T get(const std::string& name) const;
	template<class T>
		T get(int index) const;

	bool set_default(const std::string& name);
	MetaTypeImpl* get() const{
		return _MetaTypePtr.get();
	}
	std::string get_name() const{
		return _NodeName;
	}
	std::string get_type_name() const;
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
	virtual ~MetaTypeImpl(){
	}
public:
        virtual void clear(){
        }
	virtual bool set_default(const std::string& name) {
		throw MetaTypeException("Error: [Type mismatch] Destination is not an ObjectType: Accesing " + get_type_name());
	}
public://operators

	virtual const MetaType& operator [] (const std::string& name) const{
		throw MetaTypeException("Error: [Type mismatch] Destination is not an ObjectType: Accesing " + get_type_name());
	}
	virtual const MetaType& operator [] (std::size_t index) const{
		throw MetaTypeException("Error: [Type mismatch] Destination is not an ArrayType: Accesing " + get_type_name());
	}
	virtual MetaType& operator [] (const std::string& name){
		throw MetaTypeException("Error: [Type mismatch] Destination is not an ObjectType: Accesing " + get_type_name());
	}
	virtual MetaType& operator [] (std::size_t index){
		throw MetaTypeException("Error: [Type mismatch] Destination is not an ArrayType: Accesing " + get_type_name());
	}
	virtual size_t size() const{
		throw MetaTypeException("Error: [Type mismatch] Destination is not an ArrayType/ObjectType: Accesing " + get_type_name());
	}
	virtual void push_back(MetaType& val){
		throw MetaTypeException("Error: [Type mismatch] Destination is not an ArrayType: Accesing " + get_type_name());
	}
	virtual void push_back(std::unique_ptr<MetaTypeImpl>&& MetaTypePtr){
		throw MetaTypeException("Error: [Type mismatch] Destination is not an ArrayType: Accesing " + get_type_name());
	}

	virtual const ObjectType& append(const ObjectType& obj){
		throw MetaTypeException("Error: [Type mismatch] Destination is not an ObjectType: Accesing " + get_type_name());
	}
	virtual const ArrayType& append(const ArrayType& arr){
		throw MetaTypeException("Error: [Type mismatch] Destination is not an ArrayType: Accesing " + get_type_name());
	}

	virtual std::unique_ptr<MetaTypeImpl> clone() = 0;
	virtual std::ostream& write_to(std::ostream& os) const =0;
public:
	const std::string& get_type_name() const
	{
		return _typeName;
	}
	friend std::ostream& operator << (std::ostream& os, const MetaTypeImpl& Obj)
	{
		return Obj.write_to(os);
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
	std::ostream& write_to(std::ostream& os) const override;
	
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

	std::ostream& write_to(std::ostream& os) const  override{
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

	std::ostream& write_to(std::ostream& os) const override{
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
	static std::string get_type_name(){
		return utils::type<ObjectType>::name();
	}
	ObjectType() : MetaTypeImpl(this),_Default(-1), _CheckArray(true){
	}
	template<class T>
	ObjectType(const std::string& name, T val) : MetaTypeImpl(this), _Default(-1), _CheckArray(true){
		(*this)[name]=val;
	}
	ObjectType(const ObjectType& Obj) : MetaTypeImpl(this){
		(*this) = Obj;
	}
	template<class T>
	ObjectType(std::initializer_list<std::pair<std::string,T>> list) : MetaTypeImpl(this), _Default(-1), _CheckArray(true){
		for(auto l : list){
			(*this)[l.first]=l.second;
		}
	}
public:
	bool set_default(const std::string& name) override{
		auto it=_Index.find(name);
		if(it != _Index.end()){
			_Default = it->second;
			return true;
		}
		return false;
	}
        void clear() override{
                _Index.clear();
                _DataTypes.clear();     
        }
	std::unique_ptr<MetaTypeImpl> clone() override{
		return std::make_unique<ObjectType>(*this);	
	}
	std::ostream& write_to(std::ostream& os) const  override;
public: //operators
	const MetaType& operator [](const std::string& name) const override;
	const MetaType& operator [](std::size_t index) const override
	{
		return _DataTypes[index];
	}
	MetaType& operator [](const std::string& name) override;
	MetaType& operator [](std::size_t index) override
	{
		return _DataTypes[index];
	}
	std::size_t size() const override{
		return _DataTypes.size();
	}
	//MetaType& operator [](std::size_t index) override;
	/**
		Merge another object with this.	
	 */
	virtual const ObjectType& append(const ObjectType& obj) override;


private:
	//-------------- get Helper ---------------------
	template<class T,bool=std::is_arithmetic<std::remove_reference_t<T>>::value>
		struct getHelper;

	template<typename T>
		struct getHelper<T,true>{
			T operator()(const ObjectType* obj,const std::string& name) const;
			std::pair<std::string,T> operator()(const ObjectType* obj,int index) const;
		};

	template<typename T>
		struct getHelper<T,false>{
			T operator()(const ObjectType* obj,const std::string& name) const;
			std::pair<std::string,T> operator()(const ObjectType* obj,int index) const;
		};
	//-------------------------------------------------


public:
	template<class T>
	T get(const std::string& name) const{
		return getHelper<T>{}(this,name);		
	}
	template<class T>
	std::pair<std::string,T> get(int index) const{
		return getHelper<T>{}(this,index);		
	};
	friend std::ostream& operator << (std::ostream& os, const ObjectType& Obj)
	{
		return Obj.write_to(os);
	}
private:
	std::map<std::string,std::size_t> _Index;
	std::vector<MetaType>  _DataTypes;
	mutable bool _CheckArray;
	int _Default;
friend class ArrayType;
};
class ArrayType: public MetaTypeImpl{
public:
	static std::string get_type_name(){
		return utils::type<ArrayType>::name();
	}
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
			T operator()(const ArrayType* obj,int index) const;
		};

	template<typename T>
		struct getHelper<T,false>{
			T operator()(const ArrayType* obj,int index) const;
		};
	//-------------------------------------------------
public:
	template<class T>
	T get(int index) const{
		return getHelper<T>{}(this,index);		
	}
public:
	void clear() override{
                _DataTypes.clear();     
        }
	std::unique_ptr<MetaTypeImpl> clone() override{
		return std::make_unique<ArrayType>(*this);	
	}
public: //operators
	MetaType& operator [](std::size_t index) override
	{
		return _DataTypes[index];
	}
	void push_back(MetaType& val) override{
		_DataTypes.push_back(val);
	}
	void push_back(std::unique_ptr<MetaTypeImpl>&& MetaTypePtr) override{
		_DataTypes.emplace_back(std::move(MetaTypePtr));
	}
	const ArrayType& append(const ArrayType& arr) override{
		_DataTypes.clear();
		for( auto element : arr._DataTypes ){
			_DataTypes.push_back(element);	
		}
	}
	std::ostream& write_to(std::ostream& os) const override;
	std::size_t size() const override{
		return _DataTypes.size();
	}
private:
	std::vector<MetaType> _DataTypes;
};

#include "MetaType.inl"
}

#endif
