cmake_minimum_required(VERSION 2.6)

#################################################		
### Déclaration du projet
project(mjpegserver)

SET(CMAKE_MODULE_PATH ".")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find Threads.
find_package(Threads)

set(CMAKE_CXX_FLAGS "-Wall")
set(CMAKE_C_FLAGS "-Wall")

add_definitions(-DLOG_FIFO="/var/log/${PROJECT_NAME}")

##################################################
### Option de compilation

include_directories (".")
include_directories ("src")

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/lib/)

install(
    DIRECTORY ${CMAKE_BINARY_DIR}/bin/
    DESTINATION usr/local/bin
    USE_SOURCE_PERMISSIONS
)

##################################################
### Génération de la liste des fichiers sources
file(
	GLOB_RECURSE
	source_files
	src/*
)

##################################################
### Déclaration de l'exécutable
add_executable(
    ${PROJECT_NAME}
    ${source_files}
)

##################################################
### Configuration de l'édition de liens
target_link_libraries(
    ${PROJECT_NAME}
    pthread
    ${LIBS}
)
