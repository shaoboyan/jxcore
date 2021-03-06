// Copyright Microsoft. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and / or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#include <functional>
#include <unordered_map>
#include <vector>

namespace jsrt {

class ContextShim {
 public:
  // This has the same layout as v8::Context::Scope
  class Scope {
   private:
    Scope * previous;
    ContextShim * contextShim;
   public:
    explicit Scope(ContextShim * context);
    ~Scope();

    friend class IsolateShim;
  };

  // These global constructors will be cached
  enum GlobalType {
#define DEFTYPE(T) T,
#include "jsrtcontextcachedobj.inc"
    _TypeCount
  };

  // These prototype functions will be cached/shimmed
  enum GlobalPrototypeFunction {
#define DEFMETHOD(T, M)  T##_##M,
#include "jsrtcontextcachedobj.inc"
    _FunctionCount
  };

  static ContextShim * New(IsolateShim * isolateShim, bool exposeGC,
                           JsValueRef globalObjectTemplateInstance);
  ~ContextShim();
  bool EnsureInitialized();

  IsolateShim * GetIsolateShim();
  JsContextRef GetContextRef();

  JsValueRef GetPromiseContinuationFunction();
  JsValueRef GetTrue();
  JsValueRef GetFalse();
  JsValueRef GetUndefined();
  JsValueRef GetNull();
  JsValueRef GetZero();
  JsValueRef GetObjectConstructor();
  JsValueRef GetBooleanObjectConstructor();
  JsValueRef GetNumberObjectConstructor();
  JsValueRef GetStringObjectConstructor();
  JsValueRef GetDateConstructor();
  JsValueRef GetRegExpConstructor();
  JsValueRef GetProxyConstructor();
  JsValueRef GetGlobalType(GlobalType index);
  JsValueRef GetGetOwnPropertyDescriptorFunction();
  JsValueRef GetStringConcatFunction();
  JsValueRef GetGlobalPrototypeFunction(GlobalPrototypeFunction index);
  JsValueRef GetProxyOfGlobal();
  JsValueRef GetReflectObject();
  JsValueRef GetReflectFunctionForTrap(ProxyTraps traps);

  JsValueRef GetInstanceOfFunction();
  JsValueRef GetCloneObjectFunction();
  JsValueRef GetForEachNonConfigurablePropertyFunction();
  JsValueRef GetGetPropertyNamesFunction();
  JsValueRef GetGetEnumerableNamedPropertiesFunction();
  JsValueRef GetGetEnumerableIndexedPropertiesFunction();
  JsValueRef GetCreateEnumerationIteratorFunction();
  JsValueRef GetCreatePropertyDescriptorsEnumerationIteratorFunction();
  JsValueRef GetGetNamedOwnKeysFunction();
  JsValueRef GetGetIndexedOwnKeysFunction();
  JsValueRef GetGetStackTraceFunction();

  void EnsureThrowAccessorErrorFunctions();
  bool FindThrowAccessorErrorFunction(JsValueRef func, int* index);
  JsValueRef GetThrowAccessorErrorFunction(int index);
  JsValueRef GetTestFunctionTypeFunction();
  JsValueRef GetCreateTargetFunction();

  void * GetAlignedPointerFromEmbedderData(int index);
  void SetAlignedPointerInEmbedderData(int index, void * value);

  static ContextShim * GetCurrent();

  template <class R>
  static R ExecuteInContextOf(JsValueRef object, const std::function<R()> & fn);

 private:
  ContextShim(IsolateShim * isolateShim, JsContextRef context, bool exposeGC,
              JsValueRef globalObjectTemplateInstance);
  bool InitializeBuiltIns();
  bool InitializeProxyOfGlobal();
  bool InitializeReflect();
  bool InitializeGlobalPrototypeFunctions();
  bool InitializeObjectPrototypeToStringShim();

  template <typename Fn>
  bool InitializeBuiltIn(JsValueRef * builtInValue, Fn getBuiltIn);
  bool InitializeBuiltIn(JsValueRef * builtInValue, const wchar_t* globalName);
  bool InitializeGlobalTypes();
  bool KeepAlive(JsValueRef value);
  JsValueRef GetCachedShimFunction(CachedPropertyIdRef id, JsValueRef* func);
  bool ExposeGc();
  bool CheckConfigGlobalObjectTemplate();
  bool ExecuteChakraShimJS();

  // Maintains a map of {(this fromContext) object -> (toContext) proxy} for
  // cross context objects. We can return the same proxy when an object was
  // previously marshalled to toContext.
  struct CrossContextMapInfo {
    ContextShim* fromContext;
    ContextShim* toContext;
    JsValueRef object;  // original object
    JsValueRef proxy;   // result proxy
  };
  bool RegisterCrossContextObject(JsValueRef fakeTarget,
                                  const CrossContextMapInfo& info);
  bool UnregisterCrossContextObject(const CrossContextMapInfo& info);
  bool TryGetCrossContextObject(JsValueRef object, ContextShim* toContext,
                                JsValueRef* proxy);
  static void CALLBACK CrossContextFakeTargetFinalizeCallback(
    void *callbackState);

  friend JsValueRef MarshalObjectToContext(JsValueType valueType,
                                           JsValueRef valueRef,
                                           ContextShim * contextShim,
                                           ContextShim * toContextShim);

  IsolateShim * isolateShim;
  JsContextRef context;
  bool initialized;
  bool exposeGC;
  JsValueRef keepAliveObject;
  int builtInCount;
  JsValueRef globalObjectTemplateInstance;

  JsValueRef trueRef;
  JsValueRef falseRef;
  JsValueRef undefinedRef;
  JsValueRef nullRef;
  JsValueRef zero;
  JsValueRef globalConstructor[GlobalType::_TypeCount];
  JsValueRef globalObject;
  JsValueRef proxyOfGlobal;
  JsValueRef reflectObject;
  JsValueRef reflectFunctions[ProxyTraps::TrapCount];

  JsValueRef globalPrototypeFunction[GlobalPrototypeFunction::_FunctionCount];
  JsValueRef getOwnPropertyDescriptorFunction;

  JsValueRef promiseContinuationFunction;

  JsValueRef instanceOfFunction;
  JsValueRef cloneObjectFunction;
  JsValueRef forEachNonConfigurablePropertyFunction;
  JsValueRef isUintFunction;
  JsValueRef getPropertyNamesFunction;

  JsValueRef getEnumerableNamedPropertiesFunction;
  JsValueRef getEnumerableIndexedPropertiesFunction;
  JsValueRef createEnumerationIteratorFunction;
  JsValueRef createPropertyDescriptorsEnumerationIteratorFunction;
  JsValueRef getNamedOwnKeysFunction;
  JsValueRef getIndexedOwnKeysFunction;
  JsValueRef getStackTraceFunction;

  static const int THROWACCESSORERRORFUNCTIONS = 4;
  JsValueRef throwAccessorErrorFunctions[THROWACCESSORERRORFUNCTIONS];
  JsValueRef testFunctionTypeFunction;
  JsValueRef createTargetFunction;

  std::vector<void*> embedderData;
  std::unordered_map<JsValueRef, std::vector<CrossContextMapInfo*>>
    crossContextObjects;
};

template <class R>
R ContextShim::ExecuteInContextOf(
    JsValueRef object, const std::function<R()> & fn) {
  IsolateShim * isolateShim = IsolateShim::GetCurrent();
  ContextShim * contextShim = isolateShim->GetJsValueRefContextShim(object);
  if (contextShim != nullptr &&
      contextShim != isolateShim->GetCurrentContextShim()) {
    ContextShim::Scope scope(contextShim);
    return fn();
  }
  return fn();
}

}  // namespace jsrt
