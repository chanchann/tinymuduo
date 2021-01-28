add_subdirectory(tests)

set(base_SRCS
  Logging.cc
  LogStream.cc
  Thread.cc
  Exception.cc
  Timestamp.cc
  )

add_library(muduo_base ${base_SRCS})
target_link_libraries(muduo_base pthread)

install(TARGETS muduo_base DESTINATION lib)
file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/muduo/base)

