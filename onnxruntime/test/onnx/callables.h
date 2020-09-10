// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <type_traits>
#include <tuple>

namespace onnxruntime {
namespace test {

// Main callback class
template <typename    Result,
          typename... Args>
class Callable {
public:

  using
  ContextType = void;

  using
  ResultType = Result;

  using
  FunctionType = ResultType(*)(ContextType*, Args...);

  // You can use this directly if you have a simple
  // function like C-function or a stateless lambda
  // For methods we want to make use of AsyncMethodCallback
  // And covert it to a Callable
  Callable(ContextType*  context,
    FunctionType func) :
    context_(context),
    func_(func) {
  }

  Callable() : Callable(nullptr, nullptr) {}

  Callable(const Callable&) = default;
  Callable& operator=(const Callable&) = default;

  Callable(Callable&&) = default;
  Callable& operator=(Callable&&) = default;

  void Clear() {
    context_ = nullptr;
    func_ = nullptr;
  }

  operator bool() const { return func_ != nullptr; }

  bool Valid() const { return func_ != nullptr; }

  ContextType* GetContext() const { return context_; }

  FunctionType GetFunctionPtr() const { return func_; }

  // Differentiate instantiation on the presence
  // of return value. The below two overloads
  // must be mutually exclusive
  template<typename T = ResultType>
  typename std::enable_if<!std::is_same<void, T>::value, T>::type
  Invoke(Args... args) const {
    return func_(context_, std::forward<Args>(args)...);
  }

  template <typename T = ResultType>
  typename std::enable_if<std::is_same<void, T>::value, void>::type
  Invoke(Args... args) const {
    func_(context_, std::forward<Args>(args)...);
  }

private:
  ContextType*  context_;
  FunctionType  func_;
};

template<typename ObjectType,
          typename ResultType,
          typename... Args>
class CallableFactory {
public:

  using
  CallableType = Callable<ResultType,Args...>;

  explicit
  CallableFactory(ObjectType* obj) :
    obj_(obj) {
  }

  template<typename ResultType2,
          ResultType2 (ObjectType::*MethodPtr)(Args...)>
  struct Binder {
    static
    ResultType2 InvokeMethod(void* obj, Args... args) {
      return (reinterpret_cast<ObjectType*>(obj)->*MethodPtr)(
        std::forward<Args>(args)...);
    }
  };

  // UnaVOIDable void specialization
  template<ResultType (ObjectType::*MethodPtr)(Args...)>
  struct Binder<void,MethodPtr> {
    static
    void InvokeMethod(void* obj, Args... args) {
      (reinterpret_cast<ObjectType*>(obj)->*MethodPtr)(
        std::forward<Args>(args)...);
    }
  };

  template<ResultType (ObjectType::*MethodPtr)(Args...)>
  CallableType GetCallable() const {
    CallableType result(obj_, &Binder<ResultType,MethodPtr>::InvokeMethod);
      return result;
  }

private:
  ObjectType* obj_;
};

} // test
} // onnxruntime