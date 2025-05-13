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
    if (a.size() != b.size()) {
        diff = b;
        return false;
    }

    QImage imgA = a.toImage();
    QImage imgB = b.toImage();
    QImage imgDiff = imgB;
    bool identical = true;

    for (int y = 0; y < imgA.height(); ++y) {
        for (int x = 0; x < imgA.width(); ++x) {
            if (imgA.pixel(x, y) != imgB.pixel(x, y)) {
                QColor originalColor = imgB.pixelColor(x, y);
                QColor highlightColor = blendColors(originalColor, m_highlightColor, m_highlightIntensity);
                imgDiff.setPixelColor(x, y, highlightColor);
                identical = false;
            }
        }
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
