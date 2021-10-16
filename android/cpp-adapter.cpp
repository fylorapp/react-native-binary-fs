#include <jni.h>
#include <sys/types.h>
#include "lz4.h"
#include "TypedArrayApi.h"
#include "pthread.h"

using namespace facebook::jsi;
using namespace expo::gl_cpp;
using namespace std;

JavaVM *java_vm;
jclass java_class;
jobject java_object;

/**
* A simple callback function that allows us to detach current JNI Environment
* when the thread
* See https://stackoverflow.com/a/30026231 for detailed explanation
*/

void DeferThreadDetach(JNIEnv *env) {
  static pthread_key_t thread_key;

  // Set up a Thread Specific Data key, and a callback that
  // will be executed when a thread is destroyed.
  // This is only done once, across all threads, and the value
  // associated with the key for any given thread will initially
  // be NULL.
  static auto run_once = [] {
      const auto err = pthread_key_create(&thread_key, [](void *ts_env) {
          if (ts_env) {
            java_vm->DetachCurrentThread();
          }
      });
      if (err) {
        // Failed to create TSD key. Throw an exception if you want to.
      }
      return 0;
  }();

  // For the callback to actually be executed when a thread exits
  // we need to associate a non-NULL value with the key on that thread.
  // We can use the JNIEnv* as that value.
  const auto ts_env = pthread_getspecific(thread_key);
  if (!ts_env) {
    if (pthread_setspecific(thread_key, env)) {
      // Failed to set thread-specific value for key. Throw an exception if you
      // want to.
    }
  }
}

/**
* Get a JNIEnv* valid for this thread, regardless of whether
* we're on a native thread or a Java thread.
* If the calling thread is not currently attached to the JVM
* it will be attached, and then automatically detached when the
* thread is destroyed.
*
* See https://stackoverflow.com/a/30026231 for detailed explanation
*/
JNIEnv *GetJniEnv() {
  JNIEnv *env = nullptr;
  // We still call GetEnv first to detect if the thread already
  // is attached. This is done to avoid setting up a DetachCurrentThread
  // call on a Java thread.

  // g_vm is a global.
  auto get_env_result = java_vm->GetEnv((void **)&env, JNI_VERSION_1_6);
  if (get_env_result == JNI_EDETACHED) {
    if (java_vm->AttachCurrentThread(&env, NULL) == JNI_OK) {
      DeferThreadDetach(env);
    } else {
      // Failed to attach thread. Throw an exception if you want to.
    }
  } else if (get_env_result == JNI_EVERSION) {
    // Unsupported JNI version. Throw an exception if you want to.
  }
  return env;
}

string jstring2string(JNIEnv *env, jstring str) {
    if (str) {
        const char *kstr = env->GetStringUTFChars(str, nullptr);
        if (kstr) {
            string result(kstr);
            env->ReleaseStringUTFChars(str, kstr);
            return result;
        }
    }
    return "";
}

jstring string2jstring(JNIEnv *env, const string &str) {
    return env->NewStringUTF(str.c_str());
}

void install(Runtime &jsiRuntime) {
    auto isFileAvailable = Function::createFromHostFunction(
            jsiRuntime, PropNameID::forAscii(jsiRuntime, "getDeviceName"), 0,
            [](Runtime &runtime, const Value &thisValue, const Value *arguments,
               size_t count) -> Value {
                JNIEnv *jniEnv = GetJniEnv();
                java_class = jniEnv->GetObjectClass(java_object);
                string fileUrl = arguments[0].getString(runtime).utf8(runtime);
                jstring jFileUrl = string2jstring(jniEnv, fileUrl);
                jvalue params[1];
                params[0].l = jFileUrl;
                jmethodID isFileAvailable = jniEnv->GetMethodID(
                        java_class, "isFileAvailable", "(Ljava/lang/String;)Z");
                bool checkFile = (bool) jniEnv->CallBooleanMethodA(java_object, isFileAvailable, params);
                return Value(runtime, checkFile);
    });
    auto readFile = Function::createFromHostFunction(
            jsiRuntime, PropNameID::forAscii(jsiRuntime, "getDeviceName"), 0,
            [](Runtime &runtime, const Value &thisValue, const Value *arguments,
               size_t count) -> Value {
                JNIEnv *jniEnv = GetJniEnv();
                if (count < 1) {
                    throw JSError(runtime, "readFile need at least the file URL to read from, check the docs");
                }
                java_class = jniEnv->GetObjectClass(java_object);
                string fileUrl = arguments[0].getString(runtime).utf8(runtime);
                if (fileUrl.substr(0, 10) != "content://") {
                    throw JSError(runtime, "URL must be a local file URL starting with content://");
                }
                jstring jFileUrl = string2jstring(jniEnv, fileUrl);
                jvalue isFileAvailableParams[1];
                isFileAvailableParams[0].l = jFileUrl;
                jmethodID isFileAvailable = jniEnv->GetMethodID(
                        java_class, "isFileAvailable", "(Ljava/lang/String;)Z");
                bool checkFile = (bool) jniEnv->CallBooleanMethodA(java_object, isFileAvailable, isFileAvailableParams);
                if (!checkFile) {
                    throw JSError(runtime, "Could not open file at the given URL");
                }
                int chunkSize;
                bool readAllBytes = false;
                if (count == 1) {
                    // Case 1 = one argument only = Read everything from file URL
                    chunkSize = 0;
                    readAllBytes = true;
                } else {
                    // Case 2 = chunkSize provided, read the chunk
                    chunkSize = (int) arguments[1].getNumber();
                }
                int offset;
                // At least three arguments given = Read from offset
                if (count > 2) {
                    offset = (int) arguments[2].getNumber();
                } else {
                    offset = 0;
                }
                bool shouldCompress = false;
                // At least three arguments given = Check if shouldCompress
                if (count > 3) {
                    shouldCompress = arguments[3].getBool();
                }
                jvalue readFileParams[4];
                readFileParams[0].l = jFileUrl;
                readFileParams[1].i = chunkSize;
                readFileParams[2].i = offset;
                readFileParams[3].z = readAllBytes;
                jmethodID readFile = jniEnv->GetMethodID(
                        java_class, "readFile", "(Ljava/lang/String;IIZ)[B");
                jbyteArray jByteArray = (jbyteArray) jniEnv->CallObjectMethodA(java_object, readFile, readFileParams);
                uint8_t *bytes = (uint8_t*) jniEnv->GetByteArrayElements(jByteArray, NULL);
                bool isCompressed = false;
                TypedArray<TypedArrayKind::Uint8Array> *ta;
                if (shouldCompress) {
                    size_t size = LZ4_compressBound((int)chunkSize);
                    uint8_t *dstBuffer = (uint8_t*) malloc(size);
                    size_t compressedSize = (size_t) LZ4_compress_default((const char *)bytes, (char *)dstBuffer, (int)chunkSize, (int)size);
                    // Only use compressed data if its smaller than original
                    if (compressedSize < chunkSize) {
                        isCompressed = true;
                        uint8_t *compressedBytes = (uint8_t*) malloc(compressedSize);
                        copy(dstBuffer, dstBuffer + compressedSize, compressedBytes);
                        bytes = compressedBytes;
                        ta = new TypedArray<TypedArrayKind::Uint8Array>(runtime, compressedSize);
                    } else {
                        ta = new TypedArray<TypedArrayKind::Uint8Array>(runtime, chunkSize);
                    }
                    free(dstBuffer);
                } else {
                    ta = new TypedArray<TypedArrayKind::Uint8Array>(runtime, chunkSize);
                }
                ta->update(runtime, bytes);
                jniEnv->DeleteLocalRef(jFileUrl);
                jniEnv->DeleteLocalRef(jByteArray);
                Object *returnObject = new Object(runtime);
                returnObject->setProperty(runtime, "isCompressed", isCompressed);
                returnObject->setProperty(runtime, "data", *ta);
                return Value(runtime, *returnObject);
    });
    auto writeFile = Function::createFromHostFunction(
            jsiRuntime, PropNameID::forAscii(jsiRuntime, "getDeviceName"), 0,
            [](Runtime &runtime, const Value &thisValue, const Value *arguments,
               size_t count) -> Value {
                JNIEnv *jniEnv = GetJniEnv();
                if (count < 3) {
                    throw JSError(runtime, "write need at least the file name to write to, the buffer of data to write on disk and the buffer size, check the docs");
                }
                java_class = jniEnv->GetObjectClass(java_object);
                jstring jFileName = string2jstring(jniEnv, arguments[0].getString(runtime).utf8(runtime));
                uint8_t *bytes = arguments[1].getObject(runtime).getArrayBuffer(runtime).data(runtime);
                size_t originalSize = (size_t) arguments[2].getNumber();
                bool append = true;
                if (count > 3) {
                    append = arguments[3].getBool();
                }
                bool isCompressed = false;
                size_t compressedSize = 0;
                if (count > 3) {
                    isCompressed = arguments[4].getBool();
                    if (isCompressed) {
                        compressedSize = (size_t) arguments[5].getNumber();
                    }
                }
                if (isCompressed) {
                    uint8_t *dstBuffer = (uint8_t*) malloc(originalSize);
                    LZ4_decompress_safe((const char *)bytes, (char *)dstBuffer, (int)compressedSize, (int)originalSize);
                    bytes = dstBuffer;
                }
                jbyteArray byteArray = jniEnv->NewByteArray(originalSize);
                jniEnv->SetByteArrayRegion(byteArray, 0, originalSize, (const jbyte*) bytes);
                if (isCompressed) {
                    free(bytes);
                }
                jvalue writeFileParams[3];
                writeFileParams[0].l = jFileName;
                writeFileParams[1].l = byteArray;
                writeFileParams[2].z = append;
                jmethodID writeFile = jniEnv->GetMethodID(
                        java_class, "writeFile", "(Ljava/lang/String;[BZ)Ljava/lang/String;");
                jstring jFileUrl = (jstring) jniEnv->CallObjectMethodA(java_object, writeFile, writeFileParams);
                string fileUrl = jstring2string(jniEnv, jFileUrl);
                return Value(runtime, String::createFromUtf8(runtime, fileUrl));
    });
    Object *RNBinaryFs = new Object(jsiRuntime);
    RNBinaryFs->setProperty(jsiRuntime, "isFileAvailable", move(isFileAvailable));
    RNBinaryFs->setProperty(jsiRuntime, "readFile", move(readFile));
    RNBinaryFs->setProperty(jsiRuntime, "writeFile", move(writeFile));
    jsiRuntime.global().setProperty(jsiRuntime, "RNBinaryFs", *RNBinaryFs);
}

extern "C" JNIEXPORT void JNICALL
Java_com_reactnativebinaryfs_BinaryFsModule_nativeInstall(JNIEnv *env,
                                                          jobject thiz,
                                                          jlong jsi) {
    auto runtime = reinterpret_cast<Runtime *>(jsi);
    if (runtime) {
        install(*runtime);
    }
    env->GetJavaVM(&java_vm);
    java_object = env->NewGlobalRef(thiz);
}
