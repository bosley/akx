set(AK24_GIT_BRANCH "akx.dev.0" CACHE STRING "AK24 git branch to use")

function(ensure_ak24_installed)
    if(DEFINED ENV{AK24_HOME})
        set(AK24_HOME "$ENV{AK24_HOME}" PARENT_SCOPE)
        set(AK24_HOME_LOCAL "$ENV{AK24_HOME}")
    else()
        set(AK24_HOME "$ENV{HOME}/.ak24" PARENT_SCOPE)
        set(AK24_HOME_LOCAL "$ENV{HOME}/.ak24")
    endif()

    message(STATUS "AK24_HOME: ${AK24_HOME_LOCAL}")
    message(STATUS "AK24_GIT_BRANCH: ${AK24_GIT_BRANCH}")

    if(NOT EXISTS "${AK24_HOME_LOCAL}/include/ak24/kernel.h")
        message(STATUS "AK24 not found at ${AK24_HOME_LOCAL}")
        message(STATUS "Attempting to clone and install AK24 (branch: ${AK24_GIT_BRANCH})...")
        
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/external
            RESULT_VARIABLE MKDIR_RESULT
        )
        
        if(NOT EXISTS "${CMAKE_BINARY_DIR}/external/ak24")
            message(STATUS "Cloning AK24 repository (branch: ${AK24_GIT_BRANCH})...")
            execute_process(
                COMMAND git clone --depth 1 --branch ${AK24_GIT_BRANCH} git@github.com:bosley/ak24.git ${CMAKE_BINARY_DIR}/external/ak24
                RESULT_VARIABLE CLONE_RESULT
                OUTPUT_QUIET
                ERROR_QUIET
            )
            
            if(NOT CLONE_RESULT EQUAL 0)
                message(FATAL_ERROR "Failed to clone AK24 repository (branch: ${AK24_GIT_BRANCH}). Please install AK24 manually.")
            endif()
        endif()
        
        message(STATUS "Building and installing AK24 to ${AK24_HOME_LOCAL}...")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/external/ak24/build
        )
        
        execute_process(
            COMMAND ${CMAKE_COMMAND}
                -S ${CMAKE_BINARY_DIR}/external/ak24
                -B ${CMAKE_BINARY_DIR}/external/ak24/build
                -DCMAKE_INSTALL_PREFIX=${AK24_HOME_LOCAL}
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DAK24_GC_ENABLED=ON
                -DAK24_BUILD_ASAN=OFF
                -DAK24_BUILD_DEBUG_MEMORY=OFF
                -DAK24_BUILD_SHARED_KERNEL=OFF
                -DAK24_BUILD_DOCS=OFF
                -DAK24_BUILD_TESTS=OFF
            RESULT_VARIABLE CONFIG_RESULT
            OUTPUT_QUIET
        )
        
        if(NOT CONFIG_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to configure AK24")
        endif()
        
        execute_process(
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/external/ak24/build -j
            RESULT_VARIABLE BUILD_RESULT
            OUTPUT_QUIET
        )
        
        if(NOT BUILD_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to build AK24")
        endif()
        
        execute_process(
            COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR}/external/ak24/build
            RESULT_VARIABLE INSTALL_RESULT
            OUTPUT_QUIET
        )
        
        if(NOT INSTALL_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to install AK24")
        endif()
        
        message(STATUS "AK24 installed successfully to ${AK24_HOME_LOCAL}")
    endif()

    set(AK24_INCLUDE_DIR "${AK24_HOME_LOCAL}/include" PARENT_SCOPE)
    set(AK24_LIB_DIR "${AK24_HOME_LOCAL}/lib" PARENT_SCOPE)
    
    find_library(AK24_GC_LIBRARY NAMES gc PATHS ${AK24_HOME_LOCAL}/lib NO_DEFAULT_PATH)
    if(AK24_GC_LIBRARY)
        set(AK24_GC_LIBRARY "${AK24_GC_LIBRARY}" PARENT_SCOPE)
        message(STATUS "Found AK24 GC library: ${AK24_GC_LIBRARY}")
    endif()
    
    find_library(AK24_TCC_LIBRARY NAMES tcc PATHS /usr/local/lib)
    if(AK24_TCC_LIBRARY)
        set(AK24_TCC_LIBRARY "${AK24_TCC_LIBRARY}" PARENT_SCOPE)
        message(STATUS "Found TCC library: ${AK24_TCC_LIBRARY}")
    endif()
endfunction()

