cmake_minimum_required (VERSION 2.8)
find_package(PkgConfig REQUIRED)
pkg_search_module(DRM REQUIRED libdrm)
IF (${GF_USERMODE_TRACE})
    pkg_search_module(UDEV REQUIRED libudev)
ENDIF()

set (LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
set (CURDIR "${CMAKE_CURRENT_SOURCE_DIR}")
set (ROOT_XSERVER "${CURDIR}/../../../Xserver")
set (SRCS
    ${CURDIR}/context_gf.c  
    ${CURDIR}/gf_keinterface.c  
    ${CURDIR}/gf_bufmgr.c 
    ${CURDIR}/gf_dump.c 
)

IF (${GF_USERMODE_TRACE})
    set (SRCS
        ${SRCS}
        ${CURDIR}/gf_trace.c)
    add_definitions(-Wno-unused-result)
ENDIF()

add_definitions (
  -DTRUE=1 -DFALSE=0 -fms-extensions -Werror -Wno-error=cpp -D_XSERVER64 -Wno-deprecated-declarations
  -D__INTERFACE_IMPLEMENT_DEFINED__
  "-DBYTE=int"
)

include_directories (
    .
    ../../shared 
   ../../core/include
   ../../core/e3k/include
   ../../core/e3k/include/Chip
   ../
)

add_compile_options (-fPIC -rdynamic -fvisibility=hidden)

add_library (keinterface_gf SHARED ${SRCS})
set_target_properties(keinterface_gf PROPERTIES OUTPUT_NAME "${DRIVER_NAME}keinterface_v2")
set_target_properties(keinterface_gf PROPERTIES NO_SONAME ON)
set_target_properties(keinterface_gf PROPERTIES LINK_FLAGS 
	"-Wl,--no-undefined -Wl,-soname -Wl,lib${DRIVER_NAME}keinterface_v2.so.0"
    )

set_target_properties(keinterface_gf PROPERTIES SUFFIX ".so.0.0.0")

target_include_directories(keinterface_gf PUBLIC 
    ${DRM_INCLUDE_DIRS}
)

target_link_libraries(keinterface_gf
    ${DRM_LDFLAGS}
)

IF (${GF_USERMODE_TRACE})
    target_link_libraries(keinterface_gf
        ${UDEV_LIBRARIES}
        pthread
    )
ENDIF()

target_compile_options(keinterface_gf PUBLIC 
    ${DRM_CFLAGS_OTHER}
)
