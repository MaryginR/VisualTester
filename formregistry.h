#ifndef FORMREGISTRY_H
#define FORMREGISTRY_H

#include <QString>
#include <QMap>
#include <QWidget>
#include <functional>
#include <QStringList>

class FormRegistry
{
public:
    static FormRegistry& instance();
    void registerForm(const QString& name, std::function<QWidget*()> creator);
    QStringList availableForms() const;
    QWidget* createForm(const QString& name) const;

private:
    FormRegistry() = default;
    QMap<QString, std::function<QWidget*()>> creators;
};

#define REGISTER_FORM(NAME, TYPE) \
static bool _reg_##TYPE = []() { \
        FormRegistry::instance().registerForm(NAME, []() -> QWidget* { return new TYPE; }); \
        return true; \
}()
#endif // FORMREGISTRY_H
