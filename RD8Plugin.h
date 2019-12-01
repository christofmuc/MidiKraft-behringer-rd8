#pragma once

#include "RD8.h"

#if WIN32
#define DllExport   __declspec( dllexport )
#else
#define DllExport /* */
#endif

namespace midikraft {

	template <typename T>
	struct PluginTraits {
		static const char *filename;
	};

	template <typename T>
	class DynamicPlugin : public DynamicLibrary {
		typedef  T *(*createFunctionType)();
		typedef  void(*deleteFunctionType)(T *);
	
		template <typename T>
		T getTypedFunction(const String& functionName) {
			return (T) this->getFunction(functionName);
		}

	public:
		DynamicPlugin() : DynamicLibrary(PluginTraits<T>::filename) {
		}

		~DynamicPlugin() {
			close();
		}

		T *create() {
			auto createFn = getTypedFunction<createFunctionType>("CreateObjectInstance");
			if (createFn) {
				return (*createFn)();
			}
			else {
				jassert(false);
				return nullptr;
			}
		}

		void destroy(T *object) {
			auto deleteFn = getTypedFunction<deleteFunctionType>("ReleaseObject");
			if (deleteFn) {
				(*deleteFn)(object);
			}
			else {
				jassert(false);
			}
		}
	};
}

extern "C" {

	DllExport midikraft::BehringerRD8 * CreateObjectInstance();

	DllExport void  ReleaseObject(midikraft::BehringerRD8 *rd8);

}

template<> struct midikraft::PluginTraits<midikraft::BehringerRD8> {
	static const char *filename;
};
const char *midikraft::PluginTraits < midikraft::BehringerRD8 >::filename = "midikraft-behringer-rd8.dll";
typedef midikraft::DynamicPlugin<midikraft::BehringerRD8> RD8Plugin;
