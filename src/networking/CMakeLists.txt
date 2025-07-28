set(SRC_NETWORKING
        Party.cpp
        Party.h
)

add_library(bioauth-networking ${SRC_NETWORKING})

# For Windows, we need to link against ws2_32 and wsock32 for sockets
if (WIN32)
    target_link_libraries(bioauth-networking ws2_32 wsock32)
endif ()

set(ONLINE_LIB ${ONLINE_LIB} bioauth-networking PARENT_SCOPE)