#pragma once

namespace php {
	class property_entry {
	public:
		property_entry(const php::string& name, const php::value& v, int access = ZEND_ACC_PUBLIC)
		: key_(zend_string_init(name.c_str(), name.size(), 1))
		, acc_(access) {
			switch(Z_TYPE_P(static_cast<zval*>(v))) {
			case IS_NULL:
			case IS_TRUE:
			case IS_FALSE:
			case IS_LONG:
			case IS_DOUBLE:
				ZVAL_COPY_VALUE(val_, v);
				break;
			case IS_STRING: {
				php::string s = v;
				ZVAL_STR(val_, zend_string_dup(s, 1));
				break;
			}
			default:
				assert(0 && "属性类型受限");
			}
		}
		property_entry(property_entry&& entry)
		: key_(std::move(entry.key_))
		, val_(std::move(entry.val_))
		, acc_(entry.acc_) {
			// entry.acc_ = 0;
		}
		void declare(zend_class_entry* entry) {
			zend_declare_property_ex(entry, key_, val_, acc_, nullptr);
			// 属性声明要求使用 persistent , 这里需要重新创建
			
		}
	private:
		string key_;
		value  val_;
		int    acc_;
	};
}
