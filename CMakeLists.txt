add_executable(VisualTester
    main.cpp
    visualtester.cpp
    visualtester.h
    formsForTest.h
    visualtester_includes.h
)

# Линкуем к библиотеке форм из TestApp2
target_link_libraries(VisualTester Qt6::Widgets TestApp2Lib)
