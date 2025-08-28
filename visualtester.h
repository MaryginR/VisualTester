#pragma once
#include <QString>
#include <QWidget>
#include <QPixmap>
#include <QColor>
#include <functional>

class VisualTester {
public:
    using FormSetupFunc = std::function<void(QWidget*)>;

    VisualTester(const QString &baselineDir, const QString &diffDir);

    void setHighlightColor(const QColor &color = Qt::red);
    void setHighlightIntensity(double intensity = 0.5);

    void createBaseline(const QString &formName, QWidget *formWidget, FormSetupFunc setup = nullptr);
    void compareWithBaseline(const QString &formName, QWidget *formWidget, FormSetupFunc setup = nullptr);
    void createElementBaseline(const QString &formName, QWidget *formWidget, const QString &elementName, FormSetupFunc setup = nullptr);
    void compareElementWithBaseline(const QString &formName, QWidget *formWidget, const QString &elementName, FormSetupFunc setup = nullptr);

    template<typename T>
    void configure(QWidget* widget, std::function<void(T*)> setup);

private:
    QString baselinePath(const QString &name) const;
    QString diffPath(const QString &name) const;
    QPixmap captureWidget(QWidget *w);
    bool compareAndHighlight(const QPixmap &a, const QPixmap &b, QPixmap &diff);

    QColor blendColors(const QColor &base, const QColor &highlight, double alpha);

    QString m_baselineDir;
    QString m_diffDir;
    QColor m_highlightColor;
    double m_highlightIntensity;
};

template<typename T>
void VisualTester::configure(QWidget* widget, std::function<void(T*)> setup) {
    if (auto form = qobject_cast<T*>(widget); form) {
        setup(form);
    } else {
        qDebug() << "[ERROR] Widget type mismatch!";
    }
}
