#include "phpext.h"

namespace php {
	value class_base::get(const php::string& key, bool ptr) const {
		assert(Z_TYPE(obj_) == IS_OBJECT);
		zval rv, *op = property::get(const_cast<zval*>(&obj_), key, &rv);
		assert(!ptr || op != &rv);
		return value(op, ptr);
	}
	void class_base::set(const php::string& key, const php::value& val) const {
		assert(Z_TYPE(obj_) == IS_OBJECT);
		property::set(const_cast<zval*>(&obj_), key, val);
	}
	php::value class_base::call(const php::string& name) const {
		assert(Z_TYPE(obj_) == IS_OBJECT);
		return object::call(const_cast<zval*>(&obj_), name);
	}
	php::value class_base::call(const php::string& name, const std::vector<php::value>& argv) const {
		assert(Z_TYPE(obj_) == IS_OBJECT);
		return object::call(const_cast<zval*>(&obj_), name, argv);
	}
	property class_base::operator [](const php::string& name) const {
		assert(Z_TYPE(obj_) == IS_OBJECT);
		return php::property(const_cast<zval*>(&obj_), name);
	}
	property class_base::property(const php::string& name) const {
		assert(Z_TYPE(obj_) == IS_OBJECT);
		return php::property(const_cast<zval*>(&obj_), name);
	}
}
