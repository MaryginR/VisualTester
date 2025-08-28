#include "formregistry.h"

FormRegistry& FormRegistry::instance() {
    static FormRegistry registry;
    return registry;
}

void FormRegistry::registerForm(const QString& name, std::function<QWidget*()> creator) {
    creators[name] = creator;
}

QStringList FormRegistry::availableForms() const {
    return creators.keys();
}

QWidget* FormRegistry::createForm(const QString& name) const {
    auto it = creators.find(name);
    if (it != creators.end())
        return it.value()();
    return nullptr;
}
