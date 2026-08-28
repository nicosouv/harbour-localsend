# Builds everything in src/ except the Sailfish entry point, then one binary
# per test file. Kept separate from the qmake project on purpose: the device
# build needs the SDK, and this lane has to run anywhere.
cmake_minimum_required(VERSION 3.16)
project(harbour-localsend-tests CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 REQUIRED COMPONENTS Core Network Test)
find_package(OpenSSL REQUIRED)

# The request path is hand-written C++ reading whatever a stranger on the
# network sends. The unit tests cover the cases somebody thought of; the
# sanitizers cover the ones nobody did, which on a parser is the interesting
# half. Off by default because they slow the suite down several times over.
option(SANITIZE "Build the tests with AddressSanitizer and UBSan" OFF)
if(SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined)
endif()

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src)

add_library(localsend STATIC
    ${SRC_DIR}/appsettings.cpp
    ${SRC_DIR}/certificate.cpp
    ${SRC_DIR}/crypto.cpp
    ${SRC_DIR}/deviceinfo.cpp
    ${SRC_DIR}/devicemodel.cpp
    ${SRC_DIR}/discovery.cpp
    ${SRC_DIR}/hashingfile.cpp
    ${SRC_DIR}/historymodel.cpp
    ${SRC_DIR}/httpserver.cpp
    ${SRC_DIR}/knowndevices.cpp
    ${SRC_DIR}/protocol.cpp
    ${SRC_DIR}/ratelimiter.cpp
    ${SRC_DIR}/securestore.cpp
    ${SRC_DIR}/receiveservice.cpp
    ${SRC_DIR}/selectionmodel.cpp
    ${SRC_DIR}/sendservice.cpp
    ${SRC_DIR}/tlsclient.cpp
    ${SRC_DIR}/transfermodel.cpp
)

target_include_directories(localsend PUBLIC ${SRC_DIR})
target_link_libraries(localsend PUBLIC Qt5::Core Qt5::Network OpenSSL::Crypto)
target_compile_definitions(localsend PUBLIC APP_VERSION="0.0.0-test")

enable_testing()

foreach(name tst_protocol tst_httpserver tst_models tst_transfer tst_security)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE localsend Qt5::Test)
    add_test(NAME ${name} COMMAND ${name})
endforeach()
