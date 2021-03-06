function(enable_sanitizers project_name)

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    option(ENABLE_COVERAGE "Enable coverage reporting for gcc/clang" OFF)

    if(ENABLE_COVERAGE)
      target_compile_options(${project_name} INTERFACE --coverage -O0 -g)
      target_link_libraries(${project_name} INTERFACE --coverage)
    endif()

    set(SANITIZERS "")

    option(ENABLE_SANITIZER_ADDRESS "Enable address sanitizer" OFF)
    if(ENABLE_SANITIZER_ADDRESS)
      message(STATUS "Using address sanitizer!")
      list(APPEND SANITIZERS "address")
    endif()

    option(ENABLE_SANITIZER_LEAK "Enable leak sanitizer" OFF)
    if(ENABLE_SANITIZER_LEAK)
      message(STATUS "Using leak sanitizer!")
      list(APPEND SANITIZERS "leak")
    endif()

    option(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR "Enable undefined behavior sanitizer" OFF)
    if(ENABLE_SANITIZER_UNDEFINED_BEHAVIOR)
      message(STATUS "Using undefined behavior sanitizer!")
      list(APPEND SANITIZERS "undefined")
    endif()

    option(ENABLE_SANITIZER_THREAD "Enable thread sanitizer" OFF)
    if(ENABLE_SANITIZER_THREAD)
      if("address" IN_LIST SANITIZERS OR "leak" IN_LIST SANITIZERS)
        message(WARNING "Thread sanitizer does not work with Address and Leak sanitizer enabled")
      else()
        message(STATUS "Using thread sanitizer!")
        list(APPEND SANITIZERS "thread")
      endif()
    endif()

    option(ENABLE_SANITIZER_MEMORY "Enable memory sanitizer" OFF)
    if(ENABLE_SANITIZER_MEMORY AND CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
      if("address" IN_LIST SANITIZERS
         OR "thread" IN_LIST SANITIZERS
         OR "leak" IN_LIST SANITIZERS)
        message(WARNING "Memory sanitizer does not work with Address, Thread and Leak sanitizer enabled")
      else()
        message(STATUS "Using memory sanitizer!")
        list(APPEND SANITIZERS "memory")
      endif()
    endif()

    list(
      JOIN
      SANITIZERS
      ","
      LIST_OF_SANITIZERS)

  endif()

  if(LIST_OF_SANITIZERS)
    if(NOT
       "${LIST_OF_SANITIZERS}"
       STREQUAL
       "")
      target_link_libraries(${project_name} INTERFACE ubsan)
      target_compile_options(${project_name} INTERFACE -fsanitize=${LIST_OF_SANITIZERS})
      target_link_options(${project_name} INTERFACE -fsanitize=${LIST_OF_SANITIZERS})
    endif()
  endif()

endfunction()
