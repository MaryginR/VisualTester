#include <QApplication>
#include "formsForTest.h"
#include "visualtester_includes.h"
#include "visualtester.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    VisualTester tester("baseline", "diffs");

    // Настройка цвета выделения и интенсивности (по умолчанию красный, интенсивность 0.5)
    //tester.setHighlightColor(Qt::green);      // Можно установить любой цвет
    //tester.setHighlightIntensity(0.5);       // Интенсивность от 0.0 до 1.0

    // Список форм для создания эталонов
    //tester.createBaseline("Form1", new Form1());
    //tester.createBaseline("Form2", new Form2());

    // Список форм для тестирования
    tester.compareWithBaseline("Form1", new Form1());

    Form2* formAlt = new Form2();
    tester.configure<Form2>(formAlt, [](Form2* form) {
        QLabel* label = form->findChild<QLabel*>("label");
        if (label) {
            label->setText("Changed text");
            label->move(10, 30);  // Изменяем координаты на x=10, y=30
        }
    });
    tester.compareWithBaseline("Form2", formAlt);

    return 0;
}
