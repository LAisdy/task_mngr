cmake_minimum_required(VERSION 3.20)
project(TaskMngr)

# Настройка стандарта C++
set(CMAKE_CXX_STANDARD 17)

if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
endif()

# Пути к зависимостям
set(GLFW_DIR "${CMAKE_SOURCE_DIR}/third-party/glfw")
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/third-party/imgui")
set(FONT_DIR "${CMAKE_SOURCE_DIR}/fonts")

# Подключение заголовков GLFW и библиотек
include_directories("${GLFW_DIR}/include")
link_directories("${GLFW_DIR}/lib")

# Поиск Vulkan SDK
find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
    message(STATUS "Found Vulkan: ${Vulkan_INCLUDE_DIRS}")
else()
    message(FATAL_ERROR "Vulkan not found! Make sure Vulkan SDK is installed and VULKAN_SDK is set correctly.")
endif()

# Подключение заголовков Vulkan
include_directories(${Vulkan_INCLUDE_DIRS})

file(GLOB IMGUI_SOURCES 
    "${IMGUI_DIR}/*.cpp"
    "${IMGUI_DIR}/*.h"
)

# Сборка ImGui из исходников
add_library(imgui STATIC
    ${IMGUI_SOURCES}
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

# Подключение заголовков ImGui и Vulkan для библиотеки
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${Vulkan_INCLUDE_DIRS}
)

# Создание исполняемого файла
add_executable(${PROJECT_NAME} WIN32 
    src/main.cpp
)

# Линковка библиотек
target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui
    glfw3  
    Vulkan::Vulkan
)


add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/fonts"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/fonts"
)