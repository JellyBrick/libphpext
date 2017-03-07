#pragma once

namespace php {
	enum {
		PUBLIC    = ZEND_ACC_PUBLIC,
		PROTECTED = ZEND_ACC_PROTECTED,
		PRIVATE   = ZEND_ACC_PRIVATE,
	};
	class class_entry_base {
	public:
		virtual void declare() = 0;
	};
	template <class T>
	class class_entry: public class_entry_base {
	public:
		class_entry(const char* name, const char* parent_name = nullptr, int flags = 0)
		: name_(name)
		, parent_class_name_(parent_name) {
			init_handlers();
		}
		class_entry(class_entry&& entry)
		: name_(entry.name_)
		, parent_class_name_(entry.parent_class_name_)
		, constant_entries_(std::move(entry.constant_entries_))
		, property_entries_(std::move(entry.property_entries_))
		, method_entries_(std::move(entry.method_entries_))
		, arguments_(std::move(entry.arguments_))
		, interfaces_(std::move(entry.interfaces_))
		, traits_(std::move(entry.traits_))
		, flags_(entry.flags_) {
			// entry.name_ = nullptr;
			// entry.parent_class_entry_ = nullptr;
		}

		class_entry& add(const constant_entry& entry) {
			constant_entries_.push_back(entry);
			return *this;
		}

		class_entry& add(property_entry&& entry) {
			property_entries_.emplace_back(std::move(entry));
			return *this;
		}
		// 方法
		template <value (T::*FUNCTION)(parameters& params) >
		class_entry& add(const char* name, int access = ZEND_ACC_PUBLIC) {
			zend_function_entry entry;
			if(std::strncmp("__construct", name, sizeof("__construct")) == 0) {
				access |= ZEND_ACC_CTOR;
			}else if(std::strncmp("__destruct", name, sizeof("__destruct")) == 0) {
				access |= ZEND_ACC_DTOR;
			}
			method_entry<T,FUNCTION>::fill(&entry, name, access);
			method_entries_.push_back(entry);
			return *this;
		}
		// 方法
		template <value (T::*FUNCTION)(parameters& params) >
		class_entry& add(const char* name, arguments&& info, int access = ZEND_ACC_PUBLIC) {
			zend_function_entry entry;
			arguments_.emplace_back(std::move(info));
			if(std::strncmp("__construct", name, sizeof("__construct")) == 0) {
				access |= ZEND_ACC_CTOR;
			}else if(std::strncmp("__destruct", name, sizeof("__destruct")) == 0) {
				access |= ZEND_ACC_DTOR;
			}
			method_entry<T,FUNCTION>::fill(&entry, name, arguments_.back(), access);
			method_entries_.push_back(entry);
			return *this;
		}
		// 函数
		template<value FUNCTION(parameters& params)>
		class_entry& add(const char* name, int access = ZEND_ACC_PUBLIC) {
			zend_function_entry entry;
			function_entry<FUNCTION>::fill(&entry, name, access |= ZEND_ACC_STATIC);
			method_entries_.push_back(entry);
			return *this;
		}
		// 函数
		template<value FUNCTION(parameters& params)>
		class_entry& add(const char* name, arguments&& info, int access = ZEND_ACC_PUBLIC) {
			zend_function_entry entry;
			arguments_.emplace_back(std::move(info));
			function_entry<FUNCTION>::fill(&entry, name, arguments_.back(), access |= ZEND_ACC_STATIC);
			method_entries_.push_back(entry);
			return *this;
		}

		class_entry& implements(const std::string& interface_name) {
			interfaces_.push_back(interface_name);
			return *this;
		}

		class_entry& use(const std::string& trait_name) {
			traits_.push_back(trait_name);
			return *this;
		}

		virtual void declare() override {
			method_entries_.push_back(zend_function_entry{}); // 结束数组条件
			zend_class_entry ce, *pce = nullptr;
			INIT_OVERLOADED_CLASS_ENTRY_EX(ce, name_, std::strlen(name_), method_entries_.data(), nullptr, nullptr, nullptr, nullptr, nullptr);
			// create_object 会在继承动作发生后被重置为父类 create_object
			// ce.create_object = class_entry<T>::create_object; 
			ce.ce_flags = flags_;
			if(parent_class_name_) {
				zend_string*      name = zend_string_init(parent_class_name_, std::strlen(parent_class_name_), 0);
				pce = zend_lookup_class(name);
				zend_string_release(name);
			}
			zend_class_entry* ce_ = zend_register_internal_class_ex(&ce, pce);
			ce_->create_object = class_entry<T>::create_object;
			// implements
			for(auto i=interfaces_.begin();i!=interfaces_.end();++i) {
				zend_string*      name = zend_string_init(i->c_str(), i->length(), 0);
				zend_class_entry* intf = zend_lookup_class(name);
				zend_string_release(name);
				if(intf != nullptr && (intf->ce_flags & ZEND_ACC_TRAIT)) {
					zend_class_implements(ce_, 1, intf);
				}else{
					zend_throw_exception(zend_ce_exception, "interface not found", 0);
				}
			}
			interfaces_.clear();
			// traits
			for(auto i=traits_.begin();i!=traits_.end();++i) {
				zend_string*      name = zend_string_init(i->c_str(), i->length(), 0);
				zend_class_entry* trai = zend_lookup_class(name);
				zend_string_release(name);
				if(trai != nullptr && (trai->ce_flags & ZEND_ACC_TRAIT)) {
					zend_do_implement_trait(ce_, trai);
				}else{
					zend_throw_exception(zend_ce_exception, "traits not found", 0);
				}
			}
			traits_.clear();
			// 常量声明
			for(auto i=constant_entries_.begin();i!=constant_entries_.end();++i) {
				i->declare(ce_);
			}
			constant_entries_.clear();
			// 属性声明
			for(auto i=property_entries_.begin();i!=property_entries_.end();++i) {
				i->declare(ce_);
			}
			property_entries_.clear();
		}
	private:
		const char*                      name_;
		const char*                      parent_class_name_;
		std::vector<constant_entry>      constant_entries_;
		std::vector<property_entry>      property_entries_;
		std::vector<zend_function_entry> method_entries_;
		std::vector<arguments>           arguments_;
		std::vector<std::string> interfaces_;
		std::vector<std::string> traits_;
		int flags_;

		static zend_object_handlers handlers_;
		void init_handlers() {
			std::memcpy(&handlers_, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
			handlers_.offset    = XtOffsetOf(class_wrapper<T>, obj);
			handlers_.free_obj  = class_entry<T>::free_object;
			handlers_.clone_obj = class_entry<T>::clone_object;
		}
		static zend_object *create_object(zend_class_entry *entry) {
			auto wrapper = reinterpret_cast<class_wrapper<T>*>(ecalloc(1, sizeof(class_wrapper<T>) + zend_object_properties_size(entry)));
			wrapper->cpp = new T(); // 创建 C++ 类对象
			zend_object_std_init(&wrapper->obj, entry);
			object_properties_init(&wrapper->obj, entry);
			wrapper->obj.handlers = &handlers_;
			wrapper->cpp->_object_set(&wrapper->obj);
			return &wrapper->obj;
		}

		static void free_object(zend_object* object) {
			auto wrapper = reinterpret_cast<class_wrapper<T>*>((char*)object - XtOffsetOf(class_wrapper<T>, obj));
			zend_object_std_dtor(&wrapper->obj);
			delete wrapper->cpp;
			// XXX 不太明白，貌似这里不能做释放
			// efree(wrapper);
		}

		static zend_object* clone_object(zval* self) {
			return nullptr;
		}
	};


	template <class T>
	zend_object_handlers class_entry<T>::handlers_;
}