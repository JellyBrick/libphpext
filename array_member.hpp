#pragma once

namespace php {
	class array_member {
	private:
		value&       arr_;
		zend_ulong   idx_;
		HashPosition pos_;
		string       key_;
	public:
		array_member(value& arr, const string& key)
		: arr_(arr)
		, idx_(-1)
		, pos_(HT_INVALID_IDX)
		, key_(key) {
			
		}
		array_member(value& arr, zend_ulong idx)
		: arr_(arr)
		, idx_(idx)
		, pos_(HT_INVALID_IDX)
		, key_() {
			
		}
		array_member(value& arr, HashPosition pos)
		: arr_(arr)
		, idx_(-1)
		, pos_(pos)
		, key_() {

		}
		array_member& operator =(const value& val) {
			SEPARATE_ARRAY(static_cast<zval*>(arr_));
			if(idx_ != -1) {
				zend_hash_index_update(arr_, idx_, val);
				val.addref();
			}else if(pos_ != HT_INVALID_IDX) {
				zval* cur = zend_hash_get_current_data_ex(arr_, const_cast<HashPosition*>(&pos_)), tmp;
				zval_ptr_dtor(cur);
				ZVAL_COPY(cur, static_cast<zval*>(val));
			}else{
				zend_symtable_update(arr_, key_, val);
				val.addref();
			}
			// 直接将 value_ins 的引用, 移动到数组中, 此处无需 addref() 引用调整
			return *this;
		}
		bool exists() const {
			if(idx_ != -1) {
				return zend_hash_index_exists(arr_, idx_);
			}else if(pos_ != HT_INVALID_IDX) {
				return true;
			}else{
				return zend_hash_exists(arr_, key_);
			}
		}
		// ---------------------------------------------------------
		operator value() const {
			if(idx_ != -1) {
				return value(zend_hash_index_find(arr_, idx_));
			}else if(pos_ != HT_INVALID_IDX) {
				return value(zend_hash_get_current_data_ex(arr_, const_cast<HashPosition*>(&pos_)));
			}else{
				return value(zend_symtable_find_ind(arr_, key_));
			}
		}
	};
}
