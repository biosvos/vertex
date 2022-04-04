add_library(${PROJECT_NAME} vertex.c)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}inc ring pthread)