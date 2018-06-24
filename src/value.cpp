#include "vendor.h"
#include "value.h"

#include "class_base.h"
#include "object.h"
#include "stream_buffer.h"
#include "class_entry.h"
#include "closure.h"
#include "class_wrapper.h"

namespace php {
	// ---------------------------------------------------------------------
	value::~value() {
		if(ptr_ == &val_) zval_ptr_dtor(ptr_);
	}
	// ---------------------------------------------------------------------
	value::value()
	: ptr_(&val_) {
		ZVAL_UNDEF(&val_);
	}
	value::value(std::nullptr_t v)
	: ptr_(&val_) {
		ZVAL_NULL(&val_);
	}
	value::value(class_base* v)
	: value(&v->obj_) {
		assert(Z_TYPE(val_) == IS_OBJECT);
	}
	value::value(zval* v, bool ptr) {
		if(v == nullptr) {
			ZVAL_UNDEF(&val_);
			ptr_ = &val_;
		}else if(ptr) {
			ZVAL_UNDEF(&val_);
			ptr_ = v;
		}else{
			ZVAL_COPY(&val_, v);
			ptr_ = &val_;
		}
	}
	value::value(const zend_string* v)
	: ptr_(&val_) {
		ZVAL_STR(&val_, const_cast<zend_string*>(v));
		addref();
	}
	value::value(smart_str* str)
	: ptr_(&val_) {
		smart_str_0(str);
		ZVAL_STR(&val_, str->s);
		str->s = nullptr;
		str->a = 0;
	}
	value::value(const zend_object* v)
	: ptr_(&val_) {
		ZVAL_OBJ(&val_, const_cast<zend_object*>(v));
		addref();
	}
	value::value(zend_class_entry* e)
	: ptr_(&val_) {
		int r = object_init_ex(&val_, e);
		assert(r == SUCCESS && "无法创建实例");
		// object::call(&val_, "__construct");
	}
	value::value(const CLASS& c)
	: value(static_cast<zend_class_entry*>(c)) {
		
	}
	value::value(zend_class_entry* e, std::vector<value> argv)
	: ptr_(&val_) {
		int r = object_init_ex(&val_, e);
		assert(r == SUCCESS && "无法创建实例");
		object::call(&val_, "__construct", std::move(argv));
	}
	value::value(const CLASS& c, std::vector<value> argv)
	: value(static_cast<zend_class_entry*>(c), std::move(argv)) {

	}
	value::value(const zend_array* v)
	: ptr_(&val_) {
		ZVAL_ARR(&val_, const_cast<zend_array*>(v));
		addref();
	}
	value::value(const void* data)
	: ptr_(&val_) {
		ZVAL_PTR(&val_, const_cast<void*>(data));
	}
	value::value(const value& v)
	: ptr_(&val_) {
		ZVAL_COPY(&val_, v.ptr_);
	}
	value::value(value&& v) {
		if(v.ptr_ == &v.val_) {
			ZVAL_COPY_VALUE(&val_, &v.val_);
			ZVAL_UNDEF(&v.val_);
			ptr_ = &val_;
		}else{
			ZVAL_UNDEF(&val_);
			ptr_ = v.ptr_;
			v.ptr_ = &v.val_;
			assert(Z_TYPE(v.val_) == IS_UNDEF);
		}
	}
	// 基础类型
	// ---------------------------------------------------------------------
	value::value(bool v)
	: ptr_(&val_) {
		ZVAL_BOOL(&val_, v);
	}
	value::value(int v)
	: ptr_(&val_)  {
		ZVAL_LONG(&val_, v);
	}
	value::value(std::uint32_t v)
	: ptr_(&val_)  {
		ZVAL_LONG(&val_, v);
	}
	value::value(std::int64_t v)
	: ptr_(&val_)  {
		ZVAL_LONG(&val_, v);
	}
	value::value(std::size_t v)
	: ptr_(&val_)  {
		ZVAL_LONG(&val_, v);
	}
	value::value(double v)
	: ptr_(&val_)  {
		ZVAL_DOUBLE(&val_, v);
	}
	value::value(const char* str)
	: ptr_(&val_)  {
		ZVAL_STRINGL(&val_, str, std::strlen(str));
	}
	value::value(const std::string& str)
	: ptr_(&val_)  {
		ZVAL_STRINGL(&val_, str.c_str(), str.length());
	}
	value::value(stream_buffer&& buf)
	: ptr_(&val_) {
		assert(buf.gptr() == buf.eback() && "缓冲已被读取");
		buf.str_.s->len = buf.size();
		smart_str_0(&buf.str_);
		ZVAL_STR(&val_, buf.str_.s);
		// 此状态继续使用时会重新分配内存
		buf.str_ = {nullptr, 0};
		buf.setg(nullptr, nullptr, nullptr);
		buf.setp(nullptr, nullptr);
	}
	value::value(std::function<value (parameters& params)> fn)
	: ptr_(&val_)  {
		int r = object_init_ex(&val_, class_entry<closure>::entry());
		assert(r == SUCCESS && "无法创建实例");
		static_cast<closure*>(native(Z_OBJ(val_)))->fn_ = fn;
	}
	// ---------------------------------------------------------------------
	bool value::empty() const {
		switch(Z_TYPE_P(ptr_)) {
		case IS_UNDEF:
		case IS_NULL:
		case IS_FALSE:
			return true;
		case IS_LONG:
		case IS_DOUBLE:
			return Z_LVAL_P(ptr_) == 0;
		case IS_STRING:
			return Z_STRLEN_P(ptr_) == 0;
		case IS_ARRAY:
			return Z_ARRVAL_P(ptr_)->nNumOfElements == 0;
		default: // TODO how to determine 'empty' for other types?
			return false;
		}
	}
	std::size_t value::length() const {
		switch(Z_TYPE_P(ptr_)) {
			case IS_UNDEF:
			case IS_NULL:
				return 0l;
			case IS_FALSE:
			case IS_TRUE:
				return sizeof(zend_bool);
			case IS_LONG:
				return sizeof(zend_long);
			case IS_DOUBLE:
				return sizeof(double);
			case IS_STRING:
				return Z_STRLEN_P(ptr_);
			case IS_ARRAY:
				return zend_array_count(Z_ARRVAL_P(ptr_));
			default: // TODO 其它类型？
				return 0;
		}
	}
	std::size_t value::size() const {
		return length();
	}
	// --------------------------------------------------------------------
	
	TYPE value::typeof() const {
		return TYPE(ptr_);
	}
	bool value::typeof(const TYPE& t) const {
		zend_uchar t_ = Z_TYPE_P(ptr_);
		return t == t_ // 类型相同
			|| (t == TYPE::BOOLEAN && (t_ == IS_TRUE || t_ == IS_FALSE))
			|| (t == TYPE::CALLABLE && zend_is_callable(ptr_, IS_CALLABLE_CHECK_SYNTAX_ONLY, nullptr));
	}
	CLASS value::classof() const {
		assert(typeof(TYPE::OBJECT));
		return CLASS(Z_OBJCE_P(ptr_));
	}
	bool value::instanceof(const CLASS& c) const {
		return typeof(TYPE::OBJECT) && instanceof_function(Z_OBJCE_P(ptr_), c);
	}
	// 转换
	// ---------------------------------------------------------------------
	value::operator bool() const {
		return !empty();
	}
	value::operator int() const {
		assert(typeof(TYPE::INTEGER));
		return Z_LVAL_P(ptr_);
	}
	value::operator std::int64_t() const {
		assert(typeof(TYPE::INTEGER));
		return Z_LVAL_P(ptr_);
	}
	value::operator std::size_t() const {
		assert(typeof(TYPE::INTEGER));
		return Z_LVAL_P(ptr_);
	}
	value::operator float() const {
		assert(typeof(TYPE::FLOAT));
		return Z_DVAL_P(ptr_);
	}
	value::operator double() const {
		assert(typeof(TYPE::FLOAT));
		return Z_DVAL_P(ptr_);
	}
	value::operator std::string() const {
		assert(typeof(TYPE::STRING));
		return std::string(Z_STRVAL_P(ptr_), Z_STRLEN_P(ptr_));
	}
	value::operator zval*() const {
		return ptr_;
	}
	value::operator zend_string*() const {
		assert(typeof(TYPE::STRING));
		return Z_STR_P(ptr_);
	}
	value::operator zend_object*() const {
		assert(typeof(TYPE::OBJECT));
		return Z_OBJ_P(ptr_);
	}
	value::operator zend_array*() const {
		assert(typeof(TYPE::ARRAY));
		return Z_ARR_P(ptr_);
	}
	value::operator zend_class_entry*() const {
		assert(typeof(TYPE::OBJECT));
		return Z_OBJCE_P(ptr_);
	}
	// (无类型检查)转换
	// ---------------------------------------------------------------------
	bool value::to_boolean() {
		convert_to_boolean(ptr_);
		return typeof() == TYPE::TRUE;
	}
	std::int64_t value::to_integer(int base) {
		convert_to_long(ptr_);
		return Z_LVAL_P(ptr_);
	}
	double value::to_float() {
		convert_to_double(ptr_);
		return Z_DVAL_P(ptr_);
	}
	std::string value::to_string() {
		convert_to_string(ptr_);
		return std::string(Z_STRVAL_P(ptr_), Z_STRLEN_P(ptr_));
	}
	// 赋值
	// -------------------------------------------------------------------
	value& value::operator =(const value& v) {
		zval_ptr_dtor(ptr_);
		ZVAL_COPY(ptr_, v.ptr_);
		return *this;
	}
	value& value::operator =(value&& v) {
		zval_ptr_dtor(ptr_);
		ZVAL_COPY_VALUE(ptr_, v.ptr_);
		ZVAL_UNDEF(v.ptr_ = &v.val_);
		return *this;
	}
	// ---------------------------------------------------------------------
	bool value::operator ==(const value& v) const {
		return Z_PTR_P(ptr_) == Z_PTR_P(v.ptr_);
	}
	bool value::operator !=(const value& v) const {
		return Z_PTR_P(ptr_) != Z_PTR_P(v.ptr_);
	}
	// 引用
	// ---------------------------------------------------------------------
	std::uint32_t value::addref() const {
		if(Z_REFCOUNTED_P(ptr_)) {
			return ++GC_REFCOUNT(Z_COUNTED_P(ptr_));
		}
		return 1;
	}
	std::uint32_t value::delref() {
		if(Z_REFCOUNTED_P(ptr_)) {
			return --GC_REFCOUNT(Z_COUNTED_P(ptr_));
		}
		return 1;
	}
	value value::ref() const {
		value v;
		ZVAL_NEW_REF(&v.val_, ptr_);
		return std::move(v);
	}
}