#include <QApplication>
#include "visualtester_includes.h"
#include "visualtester.h"
#include "appconnection.h"

#include "formsForTest.h"
#include "formregistry.h"

// REGISTER_FORM

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Пример подключения приложения для тестирования
    //ConnectApplication("C:/TestApp");

    VisualTester tester("baseline", "diffs");

    // Настройка цвета выделения визуальных багов на формах, можно не указывать, по умолчанию места различий между эталоном и тестируемой формой будут помечаться красным
    tester.setHighlightColor(Qt::blue);      // Можно установить любой цвет
    tester.setHighlightIntensity(0.5);       // Интенсивность от 0.0 до 1.0

    // // Пример автоматического создания эталона формы, первый параметр - имя сохраняемого эталона, второй параметр - объект класса формы
    //tester.createBaseline("Form1", new Form1());

    // // Пример тестирования формы, первый параметр - эталон с которым сравнивается форма, второй параметр - объект класса формы
    // tester.compareWithBaseline("Form1", new Form1());

    // // Пример тестирования формы с дополнительными параметрами
    // Form2* formAlt = new Form2();
    // tester.configure<Form2>(formAlt, [](Form2* form) {
    //     // Пример с комплексным изменением различных параметров элемента формы
    //     QLabel* label = form->findChild<QLabel*>("label");
    //     if (label) {
    //         label->setText("Changed text");
    //         label->move(10, 30);  // Изменяем координаты на x=10, y=30
    //     }
    // });
    // tester.compareWithBaseline("Form2", formAlt);

    // Пример создания эталона для отдельного элемента управления
    //tester.createElementBaseline("Form1", new Form1(), "label");

    // Пример сравнения с эталоном отдельного элемента управления
    //tester.compareElementWithBaseline("Form1", new Form1(), "label");

    return a.exec();
}
