project(protocol)

# 查找Qt组件
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core SerialPort)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core SerialPort)

# 添加源文件
set(PROTOCOL_SOURCES
    Ymodem.cpp
    YmodemFileReceive.cpp
    YmodemFileTransmit.cpp
)

# 创建库目标
add_library(${PROJECT_NAME} ${PROTOCOL_SOURCES})

# 设置包含目录
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接Qt库
target_link_libraries(${PROJECT_NAME} PUBLIC 
    Qt${QT_VERSION_MAJOR}::Core 
    Qt${QT_VERSION_MAJOR}::SerialPort
)
