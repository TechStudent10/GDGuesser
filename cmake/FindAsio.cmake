# huge shoutout to alk for this file

find_package(Threads REQUIRED)

# To save bandwidth, you can simply download the source zip
CPMAddPackage(
  NAME asio
  VERSION 1.24.0
  URL https://github.com/chriskohlhoff/asio/archive/asio-1-24-0.zip
)

# ASIO doesn't use CMake, we have to configure it manually.
# Extra notes for using on Windows:
# 1) If _WIN32_WINNT is not set, ASIO assumes _WIN32_WINNT=0x0501, i.e. Windows XP target,
# which is definitely not the platform which most users target.
# 2) WIN32_LEAN_AND_MEAN is defined to make Winsock2 work.
if(asio_ADDED)
  add_library(asio INTERFACE)

  target_include_directories(asio
    INTERFACE ${asio_SOURCE_DIR}/asio/include
  )

  target_compile_definitions(asio
    INTERFACE
      ASIO_STANDALONE
  )

  target_link_libraries(asio
    INTERFACE
      Threads::Threads
  )

  if(WIN32)
    # macro see @ https://stackoverflow.com/a/40217291/1746503
    macro(get_win32_winnt version)
      if (CMAKE_SYSTEM_VERSION)
        set(ver ${CMAKE_SYSTEM_VERSION})
        string(REGEX MATCH "^([0-9]+).([0-9])" ver ${ver})
        string(REGEX MATCH "^([0-9]+)" verMajor ${ver})
        if ("${verMajor}" MATCHES "10")
          set(verMajor "A")
          string(REGEX REPLACE "^([0-9]+)" ${verMajor} ver ${ver})
        endif ("${verMajor}" MATCHES "10")
        string(REPLACE "." "" ver ${ver})
        string(REGEX REPLACE "([0-9A-Z])" "0\\1" ver ${ver})
        set(${version} "0x${ver}")
      endif()
    endmacro()

    if(NOT DEFINED _WIN32_WINNT)
      get_win32_winnt(ver)
      set(_WIN32_WINNT ${ver})
    endif()

    message(STATUS "Set _WIN32_WINNET=${_WIN32_WINNT}")

    target_compile_definitions(asio
      INTERFACE
        _WIN32_WINNT=${_WIN32_WINNT}
        WIN32_LEAN_AND_MEAN
    )
  endif()
endif()