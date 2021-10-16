#import <React/RCTBridgeModule.h>

#ifdef __cplusplus

#import "lz4.h"
#import "TypedArrayApi.h"

#endif

@interface BinaryFs : NSObject <RCTBridgeModule>

@property(nonatomic, assign) BOOL setBridgeOnMainQueue;

@end
