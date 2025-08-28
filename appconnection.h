#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>

struct FormClass {
    QString className;      // e.g. Form1
    QString headerRel;      // path relative to project root
    QString cppRel;         // may be empty if not found
    QString uiRel;          // may be empty if not found
};

static QList<FormClass> findForms(const QString& projectRoot);

static bool copyRecursively(const QString &src, const QString &dst);

void ConnectApplication(const QString sourceProjectPath);
