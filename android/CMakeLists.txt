cmake_minimum_required(VERSION 3.9.0)

add_library(cpp
            SHARED
            ../cpp/TypedArrayApi.cpp
            ../cpp/lz4.c
            ./cpp-adapter.cpp
            ../node_modules/react-native/ReactCommon/jsi/jsi/jsi.cpp
)

include_directories(
            ../node_modules/react-native/React
            ../node_modules/react-native/React/Base
            ../node_modules/react-native/ReactCommon/jsi
            ../cpp
)

set_target_properties(
        cpp PROPERTIES
        CXX_STANDARD 17
        CXX_EXTENSIONS OFF
        POSITION_INDEPENDENT_CODE ON
)

target_link_libraries(
        cpp
        android
)
