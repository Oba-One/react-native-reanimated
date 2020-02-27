//
// Created by Szymon Kapala on 2020-02-13.
//

#include "WorkletModule.h"
#include "EventRegistry.h"
#include <android/log.h>
#define APPNAME "NATIVE_REANIMATED"

WorkletModule::WorkletModule(std::shared_ptr<SharedValueRegistry> sharedValueRegistry,
                                   std::shared_ptr<ApplierRegistry> applierRegistry,
                                   std::shared_ptr<WorkletRegistry> workletRegistry,
                                   std::shared_ptr<EventRegistry> eventRegistry,
                                   std::shared_ptr<Scheduler> scheduler,
                                   std::shared_ptr<jsi::Value> event) {
  this->sharedValueRegistry = sharedValueRegistry;
  this->applierRegistry = applierRegistry;
  this->workletRegistry = workletRegistry;
  this->eventRegistry = eventRegistry;
  this->scheduler = scheduler;
  this->event = event;
}

jsi::Value WorkletModule::get(jsi::Runtime &rt, const jsi::PropNameID &name) {
  __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "[worklet module] PRE-ENTER");
  auto propName = name.utf8(rt);
  __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "[worklet module] ENTER [%s]", propName.c_str());
  if (propName == "startWorklet") {
     auto callback = [this](
        jsi::Runtime &rt,
        const jsi::Value &thisValue,
        const jsi::Value *args,
        size_t count
        ) -> jsi::Value {

        int newApplierId = WorkletModule::applierId--;

        int workletId = args[0].getNumber();
        std::shared_ptr<jsi::Function> workletPtr = workletRegistry->getWorklet(workletId);

        std::vector<std::shared_ptr<SharedValue>> svs;
        std::string id = "id";
        for (int i = 1; i < count; ++i) {
          int svId = args[i].getObject(rt).getProperty(rt, id.c_str()).getNumber();
          std::shared_ptr<SharedValue> sv = sharedValueRegistry->getSharedValue(svId);
          svs.push_back(sv);
        }

        std::shared_ptr<Applier> applier(new Applier(workletPtr, svs));
        applierRegistry->registerApplierForRender(newApplierId, applier);

        return jsi::Value::undefined();
     };
    return jsi::Function::createFromHostFunction(rt, name, 1, callback);
  } else if (propName == "emit") {
    auto callback = [](
      jsi::Runtime &rt,
      const jsi::Value &thisValue,
      const jsi::Value *args,
      size_t count
      ) -> jsi::Value {
        std::string eventMessage = args[0].getString(rt).utf8(rt);
        __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "[worklet module] EMIT %s", eventMessage.c_str());
        return jsi::Value::undefined();
    };
    return jsi::Function::createFromHostFunction(rt, name, 1, callback);
  } else if (propName == "event") {

    return event->getObject(rt).getProperty(rt, "NativeMap");

  } else if (propName == "log") {
    auto callback = [](
        jsi::Runtime &rt,
        const jsi::Value &thisValue,
        const jsi::Value *args,
        size_t count
        ) -> jsi::Value {
          const jsi::Value *value = &args[0];
          if (value->isString()) {
            __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "[Worklet module logger] %s", value->getString(rt).utf8(rt).c_str());
          } else if (value->isNumber()) {
            __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "[Worklet module logger] %f", value->getNumber());
          } else {
            __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "[Worklet module logger] unhandled value type");
          }
      return jsi::Value::undefined();
    };
    return jsi::Function::createFromHostFunction(rt, name, 1, callback);
  } else if(propName == "message") {
    auto callback = [this](
        jsi::Runtime &rt,
        const jsi::Value &thisValue,
        const jsi::Value *args,
        size_t count
        ) -> jsi::Value {
      __android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "[worklet module] message: [%s], looking for listener", args[0].getString(rt).utf8(rt).c_str());
      std::string str = args[0].getString(rt).utf8(rt).c_str();

      this->eventRegistry->notify(str);
      return jsi::Value::undefined();
    };
    return jsi::Function::createFromHostFunction(rt, name, 1, callback);
  }

  return jsi::Value::undefined();
}

int WorkletModule::applierId = INT_MAX;
