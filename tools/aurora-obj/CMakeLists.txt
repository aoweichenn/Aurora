add_executable(aurora-obj
    main.cpp
)

target_include_directories(aurora-obj PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(aurora-obj PRIVATE
    AuroraMC
)
