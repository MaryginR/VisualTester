#include <QApplication>
#include "formsForTest.h"
#include "visualtester_includes.h"
#include "visualtester.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    VisualTester tester("baseline", "diffs");

    // Настройка цвета выделения визуальных багов на формах, можно не указывать, по умолчанию места различий между эталоном и тестируемой формой будут помечаться красным
    tester.setHighlightColor(Qt::blue);      // Можно установить любой цвет
    tester.setHighlightIntensity(0.5);       // Интенсивность от 0.0 до 1.0

    // Пример автоматического создания эталона формы, первый параметр - имя сохраняемого эталона, второй параметр - объект класса формы
    tester.createBaseline("Form1", new Form1());

    // Пример тестирования формы, первый параметр - эталон с которым сравнивается форма, второй параметр - объект класса формы
    tester.compareWithBaseline("Form1", new Form1());

    // Пример тестирования формы с дополнительными параметрами
    Form2* formAlt = new Form2();
    tester.configure<Form2>(formAlt, [](Form2* form) {
        // Пример с комплексным изменением различных параметров элемента формы
        QLabel* label = form->findChild<QLabel*>("label");
        if (label) {
            label->setText("Changed text");
            label->move(10, 30);  // Изменяем координаты на x=10, y=30
        }
    });
    tester.compareWithBaseline("Form2", formAlt);

    tester.configure<Form2>(formAlt, [](Form2* form) {
        // Пример с изменением различных свойств элементов по отдельности
        form->setWindowTitle("Form for test");
        form->findChild<QLineEdit*>("lineEdit")->setText("Alternative Text");
        form->findChild<QComboBox*>("comboBox")->setCurrentIndex(2);
    });

    return 0;
}
