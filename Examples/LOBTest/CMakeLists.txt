if(NOT TARGET simUtil)
    return()
endif()

project(EXAMPLE_LOB_TEST)

set(PROJECT_FILES
    ExampleLOBTest.cpp
)

add_executable(example_lobtest ${PROJECT_FILES})
target_link_libraries(example_lobtest PRIVATE simVis simUtil)
set_target_properties(example_lobtest PROPERTIES
    FOLDER "Examples"
    PROJECT_LABEL "Line of Bearing Test"
)
vsi_install_target(example_lobtest SDK_Examples)
