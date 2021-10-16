package com.reactnativebinaryfs;

import com.facebook.react.bridge.JSIModulePackage;
import com.facebook.react.bridge.JSIModuleSpec;
import com.facebook.react.bridge.JavaScriptContextHolder;
import com.facebook.react.bridge.ReactApplicationContext;
import java.util.Collections;
import java.util.List;

public class BinaryFsModulePackage implements JSIModulePackage {
  @Override
  public List<JSIModuleSpec> getJSIModules(ReactApplicationContext reactApplicationContext,
      JavaScriptContextHolder jsContext) {

    reactApplicationContext.getNativeModule(BinaryFsModule.class).installLib(jsContext);
    return Collections.emptyList();
  }
}
