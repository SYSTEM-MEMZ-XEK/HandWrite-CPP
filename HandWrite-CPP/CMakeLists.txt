cmake_minimum_required(VERSION 3.16)
project(HandWrite VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Static linking to avoid runtime library issues
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
set(CMAKE_SHARED_LINKER_FLAGS "-static-libgcc -static-libstdc++")

# Set AUTOUIC search paths
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)

# Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Source files
set(SOURCES
    src/main.cpp
    src/config.cpp
    src/tools.cpp
    src/core.cpp
    src/mainwindow.cpp
)

# Header files
set(HEADERS
    src/config.hpp
    src/tools.hpp
    src/core.hpp
    src/mainwindow.hpp
)

# UI files
set(UI_FILES
    ui/mainwindow.ui
)

# Resource files (if any)
set(RESOURCE_FILES
)

# Windows resource file for application icon
if(WIN32)
    # Configure the rc file with the correct icon path
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/app.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/app.rc
        @ONLY
    )
    set(WINDOWS_RESOURCE
        ${CMAKE_CURRENT_BINARY_DIR}/app.rc
    )
else()
    set(WINDOWS_RESOURCE
    )
endif()

# Create executable
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
    ${RESOURCE_FILES}
    ${WINDOWS_RESOURCE}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
)

# Copy ttf_library to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/../ttf_library"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/ttf_library"
)

# Windows specific settings
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Install rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
