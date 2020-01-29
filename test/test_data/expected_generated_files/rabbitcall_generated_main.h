// This file was auto-generated by RabbitCall - do not modify manually.
#pragma once

#include <atomic>
#include <string>
#include <unordered_map>
#if defined (_MSC_VER)
#include <windows.h>
#endif

// NOLINTNEXTLINE
#define _RC_FUNC_EXC(_rc_declaration, _rc_call) extern "C" RC_EXPORT void _rc_declaration noexcept{try{_rc_call}catch(std::exception &_rc_ex){*_rc_e = _rc_createString(std::string(_rc_ex.what()));}}
// NOLINTNEXTLINE
#define _RC_FUNC_NOEXC(_rc_declaration, _rc_call) extern "C" RC_EXPORT void _rc_declaration noexcept{_rc_call}

// NOLINTNEXTLINE
#define _RC_CALLBACK(_rc_wrapperClass, _rc_ptrTypedef, _rc_callOperator) \
	struct _rc_wrapperClass {\
		_rc_ptrTypedef;\
		_rc_CbH<FunctionPtrType> *cb;\
		explicit _rc_wrapperClass(_rc_CbH<FunctionPtrType> *cb) noexcept : cb(cb) {}\
		_rc_wrapperClass(_rc_wrapperClass const &o) noexcept { cb = NULL; *this = o; }\
		_rc_wrapperClass(_rc_wrapperClass &&o) noexcept { cb = NULL; *this = std::move(o); }/* NOLINT */\
		~_rc_wrapperClass() noexcept { if (cb) { cb->releaseRef(); cb = NULL; } }\
		_rc_wrapperClass & operator=(_rc_wrapperClass const &o) noexcept { if (cb) cb->releaseRef(); cb = o.cb; cb->addRef(); return *this; }/* NOLINT */\
		_rc_wrapperClass & operator=(_rc_wrapperClass &&o) noexcept { if (cb) cb->releaseRef(); cb = o.cb; o.cb = NULL; return *this; }/* NOLINT */\
		_rc_callOperator/* NOLINT */\
	};

class RabbitCallType {
	std::string name;
	size_t size;
public:
	RabbitCallType(const std::string &name, size_t size);
	size_t getSize();
	const std::string & getName();
};

namespace RabbitCallInternalNamespace {
	
	struct _rc_PtrAndSize {
		void *ptr;
		int64_t size;
	};
	
	void * _rc_alloc(int64_t size);
	void _rc_dealloc(void *ptr) noexcept;
	void * _rc_allocTaskMem(int64_t size);
	void _rc_deallocTaskMem(void *ptr) noexcept;
	
	template<typename T>
	_rc_PtrAndSize _rc_createString(const T &s) {
		int64_t charSize = sizeof(s[0]);
		int64_t dataSize = (s.size() + 1) * charSize;
		void *data = _rc_alloc(dataSize);
		memcpy(data, s.c_str(), dataSize);
		return _rc_PtrAndSize{data, dataSize};
	}
	
	class RabbitCallEnum {
		std::unordered_map<int64_t, std::string> namesById;
		std::unordered_map<std::string, int64_t> idsByName;
	public:
		void setMapping(int64_t id, const std::string &name);
		int64_t parse(const std::string &name);
		std::string toString(int64_t id);
	};
	
	struct RabbitCallInternal {
		bool initialized = false;
		void(*releaseCallbackCallback)(void *) = NULL;
		std::unordered_map<std::string, RabbitCallType *> typesByName;
		
		RabbitCallType * getTypeByName(const std::string &name);
	};
	extern RabbitCallInternal rabbitCallInternal;
	
	template<typename T>
	struct _rc_CbH {
		T callbackFunction;
		void *callbackId;
		std::atomic_int refCount;
		
		_rc_CbH(T callbackFunction, void *callbackId) : callbackFunction(callbackFunction), callbackId(callbackId), refCount(1) {
		}
		
		void addRef() noexcept {
			refCount.fetch_add(1);
		}
		
		void releaseRef() noexcept {
			int oldValue = refCount.fetch_add(-1);
			if (oldValue <= 1) {
				rabbitCallInternal.releaseCallbackCallback(callbackId);
				delete this;
			}
		}
	};
	
	void initPartition_main();
	void initPartition_partition1();
	void initPartition_partition2();
}