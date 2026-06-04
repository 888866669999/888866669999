cmake_minimum_required(VERSION 3.16)
project(OmniAssist VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

set(SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/core/WeChatDecoder.cpp
    src/adapters/WeChatAdapter.cpp
    src/ai/AIService.cpp
    src/ai/providers/OpenAIProvider.cpp
    src/automation/WindowFinder.cpp
    src/automation/MessageSender.cpp
)

set(HEADERS
    src/core/models/Platform.h
    src/core/models/Contact.h
    src/core/models/MediaFile.h
    src/core/models/ChatMessage.h
    src/adapters/IPlatformAdapter.h
    src/ui/MainWindow.h
    src/core/WeChatDecoder.h
    src/adapters/WeChatAdapter.h
    src/ai/AIService.h
    src/ai/Prompts.h
    src/ai/providers/IAIServiceProvider.h
    src/ai/providers/OpenAIProvider.h
    src/automation/WindowFinder.h
    src/automation/MessageSender.h
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
    Qt6::Widgets
    Qt6::Network
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        user32
        gdi32
        shell32
        ole32
    )
endif()
