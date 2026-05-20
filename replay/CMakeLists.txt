add_executable(Replay
    ReplayMain.cpp
)

add_subdirectory(utils)
add_subdirectory(engine)

target_link_libraries(Replay
    PUBLIC
        ReplayEngine
)