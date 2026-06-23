add_executable(tests
    tests.c
    mocks.c
    ../src/midi.c
    ../src/voice.c
    ../src/envelope.c 
    ../src/filter.c
    ../src/tables.c
    ../src/oscillators.c
)

target_link_libraries(tests
    m
)


