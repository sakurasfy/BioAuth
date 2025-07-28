# experiments/secure-com/CMakeLists.txt

message(STATUS "Processing secure-com directory")

# 检查本地必要的源文件是否存在
set(REQUIRED_LOCAL_SOURCES
    com_party_0.cpp
    com_party_1.cpp
    com_config.h
)

foreach(SOURCE ${REQUIRED_LOCAL_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE}")
        message(FATAL_ERROR "Required source file ${SOURCE} not found in ${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
endforeach()

message(STATUS "All required local secure communication source files found")

# 检查src/networking下的FSS源文件是否存在
set(FSS_LIB_SOURCES
    ${PROJECT_SOURCE_DIR}/src/networking/fss-client.cpp
    ${PROJECT_SOURCE_DIR}/src/networking/fss-server.cpp
    ${PROJECT_SOURCE_DIR}/src/networking/fss-common.cpp
)

set(FSS_HEADERS
    ${PROJECT_SOURCE_DIR}/src/networking/fss-client.h
    ${PROJECT_SOURCE_DIR}/src/networking/fss-server.h
    ${PROJECT_SOURCE_DIR}/src/networking/fss-common.h
)

# 验证FSS库文件存在
foreach(SOURCE ${FSS_LIB_SOURCES})
    if(NOT EXISTS "${SOURCE}")
        message(FATAL_ERROR "Required FSS source file ${SOURCE} not found")
    endif()
endforeach()

foreach(HEADER ${FSS_HEADERS})
    if(NOT EXISTS "${HEADER}")
        message(FATAL_ERROR "Required FSS header file ${HEADER} not found")
    endif()
endforeach()

message(STATUS "All required FSS library files found in src/networking")

# 创建FSS静态库（使用src/networking下的源文件）
add_library(secure_com_fss_lib STATIC ${FSS_LIB_SOURCES})

# 添加头文件包含路径
target_include_directories(secure_com_fss_lib PUBLIC 
    ${PROJECT_SOURCE_DIR}/src/networking
    ${PROJECT_SOURCE_DIR}/src
)

# 设置库的属性
set_target_properties(secure_com_fss_lib PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED TRUE
)

# 链接必要的库
target_link_libraries(secure_com_fss_lib PUBLIC ${ONLINE_LIB})

# 如果定义了FSS编译标志，应用它们
if(DEFINED FSS_COMPILE_FLAGS)
    target_compile_options(secure_com_fss_lib PRIVATE ${FSS_COMPILE_FLAGS})
endif()

# 创建Party 0可执行文件
add_executable(com_party_0 com_party_0.cpp)
target_include_directories(com_party_0 PRIVATE 
    ${PROJECT_SOURCE_DIR}/src/networking
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}  # 添加当前目录以包含com_config.h
)
target_link_libraries(com_party_0 secure_com_fss_lib ${ONLINE_LIB})
set_target_properties(com_party_0 PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED TRUE
)

# 创建Party 1可执行文件  
add_executable(com_party_1 com_party_1.cpp)
target_include_directories(com_party_1 PRIVATE 
    ${PROJECT_SOURCE_DIR}/src/networking
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}  # 添加当前目录以包含com_config.h
)
target_link_libraries(com_party_1 secure_com_fss_lib ${ONLINE_LIB})
set_target_properties(com_party_1 PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED TRUE
)

# 应用编译标志
if(DEFINED FSS_COMPILE_FLAGS)
    target_compile_options(com_party_0 PRIVATE ${FSS_COMPILE_FLAGS})
    target_compile_options(com_party_1 PRIVATE ${FSS_COMPILE_FLAGS})
endif()

# 创建keys目录的构建后命令
add_custom_command(TARGET com_party_0 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/keys
    COMMENT "Creating keys directory for secure communication party_0"
)

add_custom_command(TARGET com_party_1 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/keys
    COMMENT "Creating keys directory for secure communication party_1"
)

# 添加运行目标
add_custom_target(run_secure_com_party_0
    COMMAND ${CMAKE_COMMAND} -E make_directory keys
    COMMAND ./com_party_0
    DEPENDS com_party_0
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running secure communication Party 0"
)

add_custom_target(run_secure_com_party_1
    COMMAND ./com_party_1
    DEPENDS com_party_1
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running secure communication Party 1"
)

add_custom_target(test_secure_com_complete
    COMMAND ${CMAKE_COMMAND} -E make_directory keys
    COMMAND echo "Step 1: Running Party 0"
    COMMAND ./com_party_0
    COMMAND echo "Step 2: Running Party 1"  
    COMMAND ./com_party_1
    DEPENDS com_party_0 com_party_1
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running complete secure communication test"
)

# 输出构建信息
message(STATUS "Secure communication executables will be built:")
message(STATUS "  - com_party_0 (using FSS library from src/networking)")
message(STATUS "  - com_party_1 (using FSS library from src/networking)")
message(STATUS "  - com_config.h (shared configuration header)")
message(STATUS "Build directory: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "FSS library source: ${PROJECT_SOURCE_DIR}/src/networking")