#pragma once
namespace php {
	void fail(const std::string& msg);
	void fail(const char *format, ...);
	void warn(const std::string& msg);
	void warn(const char *format, ...);
	void info(const std::string& msg);
	void info(const char *format, ...);
	char* error_string(size_t* perrlen = nullptr);
	char* exception_string(size_t* perrlen = nullptr);

	inline void strtoupper_inplace(char* str, size_t len) {
		php_strtoupper(str, len);
	}
	inline void strtolower_inplace(char* str, size_t len) {
		php_strtolower(str, len);
	}
	php::string strtoupper(char* str, size_t len);
	php::string strtolower(char* str, size_t len);
	// �� shutdown ִ��ʱ����� (module_shutdown ʱ php/zend ���Ĺ����Ѿ�ֹͣ)
	void register_shutdown_function(const php::callable& cb);
}
