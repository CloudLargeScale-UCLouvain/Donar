cmake_minimum_required(VERSION 2.6)
project(donar)

list(APPEND CSOURCES
	src/net_tools.h
	src/net_tools.c
	src/socks5.h
	src/socks5.c
	src/tor_os.h
	src/tor_os.c
	src/tor_ctl.h
	src/tor_ctl.c
	src/donar_client.h
	src/donar_client.c
	src/donar_server.h
	src/donar_server.c
	src/evt_core.h
	src/evt_core.c
	src/algo_naive.c
	src/utils.h
	src/utils.c
	src/packet.h
	src/packet.c
	src/url.h
	src/url.c
	src/donar_init.h
	src/donar_init.c
	src/algo_dup2.c
	src/algo_thunder.c
	src/algo_utils.h
	src/algo_utils.c
	src/proxy.h
	src/proxy.c
	src/timer.h
	src/timer.c
	src/capture_traffic.h
	src/capture_traffic.c
	src/cap_utils.h
	src/cap_utils.c
	src/measure.h
	src/measure.c
	src/stopwatch.h
	src/stopwatch.c
	src/algo_lightning.c
)

add_executable(donar ${CSOURCES} src/donar.c)
add_executable(measlat ${CSOURCES} src/meas_lat.c)
add_executable(udpecho ${CSOURCES} src/udp_echo.c)
add_executable(torecho ${CSOURCES} src/tor_echo.c)
add_executable(capdiff ${CSOURCES} src/capdiff.c)
add_executable(capreplay ${CSOURCES} src/capreplay.c)
add_executable(donar_unit_test ${CSOURCES} src/test.c)
add_executable(faketor ${CSOURCES} src/faketor.c)
add_executable(dcall src/dcall.c)

find_package(PkgConfig REQUIRED)
pkg_search_module(UUID REQUIRED uuid)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.14
                               gstreamer-rtp-1.0>=1.14)

target_include_directories(donar PRIVATE ${GLIB_INCLUDE_DIRS} ${UUID_INCLUDE_DIRS})
target_link_libraries(donar ${GLIB_LDFLAGS}  ${UUID_LDFLAGS})

target_include_directories(measlat PRIVATE ${GLIB_INCLUDE_DIRS} ${UUID_INCLUDE_DIRS})
target_link_libraries(measlat ${GLIB_LDFLAGS} ${UUID_LDFLAGS})

target_include_directories(udpecho PRIVATE ${GLIB_INCLUDE_DIRS} ${UUID_INCLUDE_DIRS})
target_link_libraries(udpecho ${GLIB_LDFLAGS} ${UUID_LDFLAGS})

target_include_directories(torecho PRIVATE ${GLIB_INCLUDE_DIRS} ${UUID_INCLUDE_DIRS})
target_link_libraries(torecho ${GLIB_LDFLAGS} ${UUID_LDFLAGS})

target_include_directories(capdiff PRIVATE ${GLIB_INCLUDE_DIRS} ${UUID_INCLUDE_DIRS})
target_link_libraries(capdiff ${GLIB_LDFLAGS} ${UUID_LDFLAGS})

target_include_directories(capreplay PRIVATE ${GLIB_INCLUDE_DIRS}  ${UUID_INCLUDE_DIRS})
target_link_libraries(capreplay ${GLIB_LDFLAGS} ${UUID_LDFLAGS})

target_include_directories(donar_unit_test PRIVATE ${GLIB_INCLUDE_DIRS} ${UUID_INCLUDE_DIRS})
target_link_libraries(donar_unit_test ${GLIB_LDFLAGS} ${UUID_LDFLAGS})

target_include_directories(faketor PRIVATE ${GLIB_INCLUDE_DIRS} ${UUID_INCLUDE_DIRS})
target_link_libraries(faketor ${GLIB_LDFLAGS} ${UUID_LDFLAGS})

target_include_directories(dcall PRIVATE ${GST_INCLUDE_DIRS})
target_link_libraries(dcall ${GST_LDFLAGS})

install(TARGETS donar measlat udpecho torecho capdiff capreplay donar_unit_test dcall faketor
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
