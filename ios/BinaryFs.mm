#import <React/RCTUtils.h>
#import "RCTBridge+Private.h"
#import "YeetJSIUtils.h"
#import "BinaryFs.h"

@implementation BinaryFs

@synthesize bridge = _bridge;
@synthesize methodQueue = _methodQueue;

RCT_EXPORT_MODULE()

+ (BOOL)requiresMainQueueSetup {
    return YES;
}

+ (BOOL)isFileAvailable:(NSString *)uri {
    NSURL *url = [NSURL URLWithString:uri];
    return [url checkResourceIsReachableAndReturnError:nil];
}

- (void)setBridge:(RCTBridge *)bridge {
    _bridge = bridge;
    _setBridgeOnMainQueue = RCTIsMainQueue();
    [self installLibrary];
}

using namespace std;
using namespace facebook::jsi;
using namespace expo::gl_cpp;

- (void)installLibrary {
    RCTCxxBridge *cxxBridge = (RCTCxxBridge *)self.bridge;
    if (!cxxBridge.runtime) {
        /**
         * This is a workaround to install library
         * as soon as runtime becomes available and is
         * not recommended. If you see random crashes in iOS
         * global.xxx not found etc. use this.
         */

        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 0.001 * NSEC_PER_SEC),
                       dispatch_get_main_queue(), ^{
            /**
             When refreshing the app while debugging, the setBridge
             method is called too soon. The runtime is not ready yet
             quite often. We need to install library as soon as runtime
             becomes available.
             */
            [self installLibrary];
        });
        return;
    }
    install(*(Runtime *)cxxBridge.runtime, self);
}

static void install(Runtime &jsiRuntime, BinaryFs *binaryFs) {
    auto isFileAvailable = Function::createFromHostFunction(
        jsiRuntime, PropNameID::forAscii(jsiRuntime, "isFileAvailable"), 0,
        [binaryFs](Runtime& runtime, const Value& thisValue, const Value* arguments,
           size_t count) -> Value {
               NSString *uri = convertJSIStringToNSString(runtime, arguments[0].getString(runtime));
               bool isAvailable = [BinaryFs isFileAvailable:uri];
               return Value(runtime, isAvailable);
    });
    auto readFile = Function::createFromHostFunction(
         jsiRuntime, PropNameID::forAscii(jsiRuntime, "readFile"), 0,
         [binaryFs](Runtime& runtime, const Value& thisValue, const Value* arguments,
            size_t count) -> Value {
                if (count < 1) {
                    throw JSError(runtime, "readFile need at least the file URL to read from, check the docs");
                }
                NSString *uri;
                try {
                    uri = convertJSIStringToNSString(runtime, arguments[0].getString(runtime));
                } catch (...) {
                    throw JSError(runtime, "URL of the file is invalid, check if it's a valid string");
                }
                if (![[uri substringToIndex:7] isEqual:@"file://"]) {
                    throw JSError(runtime, "URL must be a local file URL starting with file://");
                }
                if (![BinaryFs isFileAvailable:uri]) {
                    throw JSError(runtime, "Could not open file at the given URL");
                }
                NSURL *url = [NSURL URLWithString:uri];
                NSFileHandle *fileHandle = [NSFileHandle fileHandleForReadingFromURL:url error:nil];
                size_t chunkSize;
                NSData *fileData;
                long offset;
                // At least three arguments given = Read from offset
                if (count > 2) {
                    offset = (long) arguments[2].getNumber();
                } else {
                    offset = 0;
                }
                [fileHandle seekToFileOffset:offset];
                if (count == 1) {
                    // Case 1 = one argument only = Read everything from file URL
                    fileData = [fileHandle availableData];
                    chunkSize = (size_t) [fileData length];
                } else {
                    // Case 2 = chunkSize provided, read the chunk
                    chunkSize = (size_t) arguments[1].getNumber();
                    fileData = [fileHandle readDataOfLength:chunkSize];
                }
                bool shouldCompress = false;
                // At least three arguments given = Check if shouldCompress
                if (count > 3) {
                    shouldCompress = arguments[3].getBool();
                }
                uint8_t *bytes = (uint8_t*)[fileData bytes];
                [fileHandle closeFile];
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
                Object *returnObject = new Object(runtime);
                returnObject->setProperty(runtime, "isCompressed", isCompressed);
                returnObject->setProperty(runtime, "data", *ta);
                return Value(runtime, *returnObject);
    });
    auto writeFile = Function::createFromHostFunction(
         jsiRuntime, PropNameID::forAscii(jsiRuntime, "writeFile"), 0,
         [binaryFs](Runtime& runtime, const Value& thisValue, const Value* arguments,
            size_t count) -> Value {
                if (count < 3) {
                    throw JSError(runtime, "write need at least the file name to write to, the buffer of data to write on disk and the buffer size, check the docs");
                }
                NSString *fileName;
                try {
                    fileName = convertJSIStringToNSString(runtime, arguments[0].getString(runtime));
                } catch (...) {
                    throw JSError(runtime, "Name of the file is invalid, check if it's a valid string");
                }
                NSArray *paths = [[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask];
                NSURL *documentsURL = [paths lastObject];
                NSURL *fileUrl = [documentsURL URLByAppendingPathComponent:fileName];
                uint8_t *bytes = arguments[1].getObject(runtime).getArrayBuffer(runtime).data(runtime);
                size_t originalSize = (size_t) arguments[2].getNumber();
                NSFileManager *fileManager = [NSFileManager defaultManager];
                NSFileHandle *fileHandle;
                bool append = true;
                if (count > 3) {
                    append = arguments[3].getBool();
                }
                if ([fileUrl checkResourceIsReachableAndReturnError:nil]) {
                    if (!append) {
                        [fileManager removeItemAtURL:fileUrl error:nil];
                        [fileManager createFileAtPath:[fileUrl path] contents:nil attributes:nil];
                    }
                } else {
                    [fileManager createFileAtPath:[fileUrl path] contents:nil attributes:nil];
                }
                fileHandle = [NSFileHandle fileHandleForWritingToURL:fileUrl error:nil];
                if (append) {
                    [fileHandle seekToEndOfFile];
                } else {
                    [fileHandle seekToFileOffset:0];
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
                NSData *data = [NSData dataWithBytes:bytes length:originalSize];
                [fileHandle writeData:data];
                [fileHandle closeFile];
                if (isCompressed) {
                    free(bytes);
                }
                NSString *fileUrlStr = [fileUrl absoluteString];
                String savedUrl = convertNSStringToJSIString(runtime, fileUrlStr);
                return Value(runtime, savedUrl);
    });
    Object *RNBinaryFs = new Object(jsiRuntime);
    RNBinaryFs->setProperty(jsiRuntime, "isFileAvailable", move(isFileAvailable));
    RNBinaryFs->setProperty(jsiRuntime, "readFile", move(readFile));
    RNBinaryFs->setProperty(jsiRuntime, "writeFile", move(writeFile));
    jsiRuntime.global().setProperty(jsiRuntime, "RNBinaryFs", *RNBinaryFs);
}

@end
