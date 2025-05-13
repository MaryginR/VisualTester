add_executable(VisualTester
    main.cpp
    visualtester.cpp
    visualtester.h
    formsForTest.h
    visualtester_includes.h
)

# После Qt6::Widgets необходимо добавить библиотеку с формами приложения для тестирования
target_link_libraries(VisualTester Qt6::Widgets)
