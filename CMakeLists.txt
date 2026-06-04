# 1 - INICJALIZACJA PROJEKTU
cmake_minimum_required(VERSION 3.20)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(lc_manager)
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

# 2 - POBIERANIE GLFW
FetchContent_Declare(
        glfw
        URL https://github.com/glfw/glfw/archive/refs/tags/3.3.8.zip
        DOWNLOAD_EXTRACT_TIMESTAMP true
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# 3 - POBIERANIE JSONA
FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(json)

# 4 - PODPIĘCIE IMGUI ORAZ ZASOBÓW
file(GLOB IMGUI_SOURCES
        "imgui/*.cpp"
        "imgui/backends/imgui_impl_glfw.cpp"
        "imgui/backends/imgui_impl_opengl3.cpp"
)
set(RESOURCE_FILE
        resources.rc
        lc_manager_database/database.h
        lc_manager_database/database.cpp
        lc_manager_database/database.h
        lc_manager_database/database.cpp
        exp_calculator.h
)

# 5 - KOMPILACJA
add_executable(lc_manager WIN32 main.cpp ${IMGUI_SOURCES} ${RESOURCE_FILE})
target_include_directories(lc_manager PRIVATE imgui imgui/backends)
target_link_directories(lc_manager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(lc_manager PRIVATE glfw opengl32 dwmapi nlohmann_json::nlohmann_json)