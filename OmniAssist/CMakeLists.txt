cmake_minimum_required(VERSION 3.16)
project(OmniAssist VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Sql)
find_package(OpenSSL REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

set(SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/core/WeChatDecoder.cpp
    src/core/QQDecoder.cpp
    src/adapters/WeChatAdapter.cpp
    src/adapters/QQAdapter.cpp
    src/ai/AIService.cpp
    src/ai/providers/OpenAIProvider.cpp
    src/automation/WindowFinder.cpp
    src/automation/MessageSender.cpp
    src/config/Settings.cpp
)

set(HEADERS
    src/core/models/Platform.h
    src/core/models/Contact.h
    src/core/models/MediaFile.h
    src/core/models/ChatMessage.h
    src/adapters/IPlatformAdapter.h
    src/ui/MainWindow.h
    src/core/WeChatDecoder.h
    src/core/QQDecoder.h
    src/adapters/WeChatAdapter.h
    src/adapters/QQAdapter.h
    src/ai/AIService.h
    src/ai/Prompts.h
    src/ai/providers/IAIServiceProvider.h
    src/ai/providers/OpenAIProvider.h
    src/automation/WindowFinder.h
    src/automation/MessageSender.h
    src/config/Settings.h
)

set(RESOURCES
    resources/resources.qrc
)

add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    OpenSSL::SSL
    OpenSSL::Crypto
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        user32
        gdi32
        shell32
        ole32
        psapi
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE ON
    )
endif()
