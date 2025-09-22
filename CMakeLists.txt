cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
  set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
  set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

project(GDGuesser VERSION 1.0.0)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Github actions (Taken from EclipseMenu)
if(DEFINED ENV{GITHUB_ACTIONS})
  # Enable LTO for smaller binaries
  set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  target_compile_definitions(${PROJECT_NAME} PRIVATE GITHUB_ACTIONS)
  message(STATUS "GitHub Actions build detected")
endif()

target_include_directories(${PROJECT_NAME} PRIVATE src)

# Debug macro (Taken from EclipseMenu)
if (NOT DEFINED ENV{GITHUB_ACTIONS})
  target_compile_definitions(
    ${PROJECT_NAME} PRIVATE
      $<$<CONFIG:Debug>:DEBUG_BUILD>
      $<$<CONFIG:RelWithDebInfo>:DEBUG_BUILD>
  )
endif()

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

CPMAddPackage("gh:GlobedGD/argon@1.1.0")

CPMAddPackage(
  NAME websocketpp
  GITHUB_REPOSITORY zaphoyd/websocketpp
  GIT_TAG 0.8.2
  DOWNLOAD_ONLY YES
)

include(cmake/FindAsio.cmake)

# THANK YOU ALK
CPMAddPackage(
	NAME wolfssl
	GIT_REPOSITORY "https://github.com/wolfSSL/wolfssl"
	GIT_TAG "v5.8.0-stable"
	OPTIONS "WOLFSSL_QUIC ON"
			"WOLFSSL_HARDEN OFF"
			"WOLFSSL_SESSION_TICKET ON"
			"WOLFSSL_INSTALL OFF"
			"WOLFSSL_CRYPT_TESTS OFF"
			"WOLFSSL_EXAMPLES OFF"
			"BUILD_SHARED_LIBS OFF"
			"WOLFSSL_ASIO ON"
			"WOLFSSL_OPENSSLEXTRA ON"
)

get_target_property(WOLFSSL_DEFINITIONS wolfssl COMPILE_DEFINITIONS)
get_target_property(WOLFSSL_INCLUDE_DIRS wolfssl INCLUDE_DIRECTORIES)

target_compile_definitions(asio INTERFACE "${WOLFSSL_DEFINITIONS}")
target_include_directories(asio INTERFACE "${WOLFSSL_INCLUDE_DIRS}" "${wolfssl_SOURCE_DIR}/wolfssl")

target_compile_definitions(${PROJECT_NAME} PUBLIC ASIO_STANDALONE WIN32_LEAN_AND_MEAN=1)
# OpenSSL::SSL OpenSSL::Crypto
target_link_libraries(${PROJECT_NAME} argon wolfssl asio)
target_include_directories(${PROJECT_NAME} PRIVATE
  ${websocketpp_SOURCE_DIR}
  ${asio_SOURCE_DIR}/asio/include
  ${OPENSSL_INCLUDE_DIR}
  ${wolfssl_SOURCE_DIR}/wolfssl
)

setup_geode_mod(${PROJECT_NAME})
