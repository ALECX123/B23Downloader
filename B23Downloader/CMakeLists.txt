cmake_minimum_required(VERSION 3.16)

project(B23Downloader VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(WIN32)
    set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

add_executable(${PROJECT_NAME}
    main.cpp
    AboutWidget.cpp
    AboutWidget.h
    DownloadDialog.cpp
    DownloadDialog.h
    DownloadTask.cpp
    DownloadTask.h
    Extractor.cpp
    Extractor.h
    Flv.cpp
    Flv.h
    LoginDialog.cpp
    LoginDialog.h
    MainWindow.cpp
    MainWindow.h
    MyTabWidget.cpp
    MyTabWidget.h
    Network.cpp
    Network.h
    QrCode.cpp
    QrCode.h
    Settings.cpp
    Settings.h
    TaskTable.cpp
    TaskTable.h
    utils.cpp
    utils.h
    icons.qrc
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    QT_DISABLE_DEPRECATED_BEFORE=0x060000
)



if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/B23Downloader_resource.rc
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        APP_VERSION="${PROJECT_VERSION}"
    )
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
    endif()
endif()
