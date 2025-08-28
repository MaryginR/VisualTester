#include "visualtester.h"
#include <QDir>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <QDebug>

VisualTester::VisualTester(const QString &baselineDir, const QString &diffDir)
    : m_baselineDir(baselineDir), m_diffDir(diffDir), m_highlightColor(Qt::red), m_highlightIntensity(0.5) {
    QDir().mkpath(baselineDir);
    QDir().mkpath(diffDir);
}

void VisualTester::setHighlightColor(const QColor &color) {
    m_highlightColor = color;
}

void VisualTester::setHighlightIntensity(double intensity) {
    m_highlightIntensity = qBound(0.0, intensity, 1.0);
}

QString VisualTester::baselinePath(const QString &name) const {
    return m_baselineDir + "/" + name + ".png";
}

QString VisualTester::diffPath(const QString &name) const {
    return m_diffDir + "/" + name + "_diff.png";
}

QPixmap VisualTester::captureWidget(QWidget *w) {
    return w->grab();
}

QColor VisualTester::blendColors(const QColor &base, const QColor &highlight, double alpha) {
    int r = static_cast<int>((1 - alpha) * base.red() + alpha * highlight.red());
    int g = static_cast<int>((1 - alpha) * base.green() + alpha * highlight.green());
    int b = static_cast<int>((1 - alpha) * base.blue() + alpha * highlight.blue());
    return QColor::fromRgb(r, g, b);
}

bool VisualTester::compareAndHighlight(const QPixmap &a, const QPixmap &b, QPixmap &diff) {
    QImage imgA = a.toImage();
    QImage imgB = b.toImage();

    // Рассчитаем общую область сравнения
    int commonWidth  = qMin(imgA.width(), imgB.width());
    int commonHeight = qMin(imgA.height(), imgB.height());

    // Начинаем с копии текущего (B), чтобы подсветить различия
    QImage imgDiff = imgB;
    bool identical = true;

    // Сравниваем только общую часть
    for (int y = 0; y < commonHeight; ++y) {
        for (int x = 0; x < commonWidth; ++x) {
            if (imgA.pixel(x, y) != imgB.pixel(x, y)) {
                QColor originalColor = imgB.pixelColor(x, y);
                QColor highlightColor = blendColors(originalColor, m_highlightColor, m_highlightIntensity);
                imgDiff.setPixelColor(x, y, highlightColor);
                identical = false;
            }
        }
    }

    // Если размеры разные — отметим края рамкой
    if (imgA.size() != imgB.size()) {
        QPainter painter(&imgDiff);
        painter.setPen(QPen(Qt::yellow, 2, Qt::DashLine));
        painter.drawRect(0, 0, imgDiff.width()-1, imgDiff.height()-1);
        painter.end();
        identical = false; // всё же считаем разными
    }

    diff = QPixmap::fromImage(imgDiff);
    return identical;
}

void VisualTester::createBaseline(const QString &formName, QWidget *formWidget, FormSetupFunc setup) {
    if (setup) setup(formWidget);
    formWidget->show();
    formWidget->repaint();
    QPixmap current = captureWidget(formWidget);
    formWidget->hide();

    QString baseFile = baselinePath(formName);
    current.save(baseFile);
    qDebug() << "[BASELINE] Saved baseline for" << formName << "at" << baseFile;
}

void VisualTester::compareWithBaseline(const QString &formName, QWidget *formWidget, FormSetupFunc setup) {
    if (setup) setup(formWidget);
    formWidget->show();
    formWidget->repaint();
    QPixmap current = captureWidget(formWidget);
    formWidget->hide();

    QString baseFile = baselinePath(formName);
    QString diffFile = diffPath(formName);

    if (!QFile::exists(baseFile)) {
        qDebug() << "[ERROR] Baseline not found for" << formName;
        return;
    }

    QPixmap baseline(baseFile);
    QPixmap diff;
    if (!compareAndHighlight(baseline, current, diff)) {
        diff.save(diffFile);
        qDebug() << "[FAIL] Differences in" << formName << "-> saved:" << diffFile;
    } else {
        qDebug() << "[PASS]" << formName << "matches baseline.";
    }
}

void VisualTester::createElementBaseline(const QString &formName,
                                         QWidget *formWidget,
                                         const QString &elementName,
                                         FormSetupFunc setup)
{
    if (setup) setup(formWidget);
    formWidget->show();
    formWidget->repaint();

    QPixmap formCapture = captureWidget(formWidget);

    QWidget *child = formWidget->findChild<QWidget*>(elementName);
    if (!child) {
        qDebug() << "[ERROR] Element not found:" << elementName;
        formWidget->hide();
        return;
    }

    QRect elemRect = child->geometry();
    QPixmap current = formCapture.copy(elemRect);

    formWidget->hide();

    QString baseFile = baselinePath(formName + "_" + elementName);
    current.save(baseFile);
    qDebug() << "[BASELINE] Saved baseline for element" << elementName << "in" << formName << "at" << baseFile;
}

void VisualTester::compareElementWithBaseline(const QString &formName,
                                              QWidget *formWidget,
                                              const QString &elementName,
                                              FormSetupFunc setup)
{
    if (setup) setup(formWidget);
    formWidget->show();
    formWidget->repaint();

    // Захватим форму целиком
    QPixmap formCapture = captureWidget(formWidget);

    // Найдём элемент
    QWidget *child = formWidget->findChild<QWidget*>(elementName);
    if (!child) {
        qDebug() << "[ERROR] Element not found:" << elementName;
        formWidget->hide();
        return;
    }

    // Геометрия элемента относительно формы
    QRect elemRect = child->geometry();

    // Вырежем область с элементом
    QPixmap current = formCapture.copy(elemRect);

    formWidget->hide();

    // Пути к baseline и diff
    QString baseFile = baselinePath(formName + "_" + elementName);
    QString diffFile = diffPath(formName + "_" + elementName);

    // Проверим, есть ли baseline
    if (!QFile::exists(baseFile)) {
        qDebug() << "[ERROR] Baseline not found for element" << elementName << "in form" << formName;
        return;
    }

    QPixmap baseline(baseFile);
    QPixmap diff;
    if (!compareAndHighlight(baseline, current, diff)) {
        diff.save(diffFile);
        qDebug() << "[FAIL] Differences in element" << elementName << "of" << formName << " -> saved:" << diffFile;
    } else {
        qDebug() << "[PASS] Element" << elementName << "in" << formName << "matches baseline.";
    }
}
