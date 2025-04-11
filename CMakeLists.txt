cmake_minimum_required(VERSION 3.20)
project(TaskMngr LANGUAGES CXX)

# C++ standart
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Win options
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
endif()

# ==== PATHS ====
set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})
set(SRC_DIR      ${PROJECT_ROOT}/src)
set(HEADERS_DIR  ${PROJECT_ROOT}/headers)
set(FONT_DIR     ${PROJECT_ROOT}/fonts)
set(ICONS_DIR    ${PROJECT_ROOT}/icons)
set(GLFW_DIR     ${PROJECT_ROOT}/third-party/glfw)
set(IMGUI_DIR    ${PROJECT_ROOT}/third-party/imgui)
set(JSON_DIR     ${PROJECT_ROOT}/third-party/nlohmann)
set(OPENSSL_DIR  ${PROJECT_ROOT}/third-party/OpenSSL)
set(CURL_DIR     ${PROJECT_ROOT}/third-party/curl)
set(SODIUM_DIR   ${PROJECT_ROOT}/third-party/libsodium)
set(CMAKE_PREFIX_PATH "${OPENSSL_DIR}/lib/cmake" ${CMAKE_PREFIX_PATH})


# ==== HEADERS ====
include_directories(
    ${HEADERS_DIR}
    ${GLFW_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# ==== Seeking for Vulkan ====
find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
    message(STATUS "Found Vulkan: ${Vulkan_INCLUDE_DIRS}")
    include_directories(${Vulkan_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "Vulkan not found!")
endif()

# ==== Proj Files ====
file(GLOB HEADER_FILES "${HEADERS_DIR}/*.[hH]*")
set(SOURCE_FILES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/ProcListMngr.cpp
    ${SRC_DIR}/RequestMngr.cpp
)

file(GLOB IMGUI_SOURCES 
    ${IMGUI_DIR}/*.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

# ==== Libs ====

#glfw
link_directories(${GLFW_DIR}/lib)

#imgui
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${Vulkan_INCLUDE_DIRS}
)

# nlohmann/json
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE 
    ${JSON_DIR}
)

#libsodium
link_directories(${SODIUM_DIR}/lib)
add_library(libsodium STATIC IMPORTED)
set_target_properties(libsodium PROPERTIES
    IMPORTED_LOCATION "${SODIUM_DIR}/lib/libsodium.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${SODIUM_DIR}/include"
)

# openssl
link_directories(${OPENSSL_DIR}/lib)

add_library(OpenSSL::Crypto STATIC IMPORTED)
set_target_properties(OpenSSL::Crypto PROPERTIES
    IMPORTED_LOCATION "${OPENSSL_DIR}/lib/libcrypto_static.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_DIR}/include"
)

add_library(OpenSSL::SSL STATIC IMPORTED)
set_target_properties(OpenSSL::SSL PROPERTIES
    IMPORTED_LOCATION "${OPENSSL_DIR}/lib/libssl_static.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_DIR}/include"
)


# Curl
link_directories(${CURL_DIR}/lib)
add_library(curl STATIC IMPORTED)  
set_target_properties(curl PROPERTIES 
    IMPORTED_LOCATION "${CURL_DIR}/lib/libcurl_a.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CURL_DIR}/include"
)

# ==== Executable ====
add_executable(${PROJECT_NAME} WIN32
    ${SOURCE_FILES}
    ${HEADER_FILES}
)

# ==== Grouping headers and source files for vs studio ====
source_group("Header Files" FILES ${HEADER_FILES})
source_group("Source Files" FILES ${SOURCE_FILES})

# ==== Linking ====
target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui
    glfw3
    Vulkan::Vulkan
    Rpcrt4
    Crypt32
    psapi
    nlohmann_json
    libsodium
    OpenSSL::Crypto
    OpenSSL::SSL
    curl
)

# ==== Fonts copying ====
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${FONT_DIR}
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/fonts"
)

# ==== Icons copying ====
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${ICONS_DIR}
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/icons"
)

