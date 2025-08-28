#include "appconnection.h"

static bool copyRecursively(const QString &src, const QString &dst) {
    QFileInfo srcInfo(src);
    if (srcInfo.isDir()) {
        QDir dstDir(QFileInfo(dst).absolutePath());
        if (!dstDir.exists() && !dstDir.mkpath(".")) return false;
        QDir srcDir(src);
        if (!QDir(dst).exists() && !QDir().mkpath(dst)) return false;
        const auto items = srcDir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
        for (const QFileInfo &fi : items) {
            if (!copyRecursively(fi.absoluteFilePath(), dst + "/" + fi.fileName()))
                return false;
        }
        return true;
    } else {
        QDir d(QFileInfo(dst).absolutePath());
        if (!d.exists() && !d.mkpath(".")) return false;
        if (QFile::exists(dst)) QFile::remove(dst);
        return QFile::copy(src, dst);
    }
}

// Рекурсивный поиск форм в проекте
static QList<FormClass> findForms(const QString& projectRoot) {
    QList<FormClass> result;
    QDir root(projectRoot);

    // Собираем все .h (рекурсивно)
    QDirIterator it(projectRoot, QStringList() << "*.h", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString headerPath = it.next();
        QFile f(headerPath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        const QString text = QString::fromUtf8(f.readAll());
        f.close();

        // Должны совпасть оба условия: Q_OBJECT и наследование от QWidget/QDialog
        if (!text.contains("Q_OBJECT")) continue;
        QRegularExpression reClass(R"(class\s+([A-Za-z_]\w*)\s*:\s*public\s+(QWidget|QDialog))");
        auto m = reClass.match(text);
        if (!m.hasMatch()) continue;

        const QString cls = m.captured(1);
        // относительные пути
        const QString headerRel = root.relativeFilePath(headerPath);
        QString base = QFileInfo(headerPath).completeBaseName(); // без .h

        // Ищем .cpp и .ui рядом/в проекте (первая находка по имени)
        QString cppRel, uiRel;
        {
            // .cpp с тем же basename
            QDirIterator it2(projectRoot, QStringList() << (base + ".cpp"), QDir::Files, QDirIterator::Subdirectories);
            if (it2.hasNext()) cppRel = root.relativeFilePath(it2.next());
        }
        {
            // .ui с тем же basename
            QDirIterator it3(projectRoot, QStringList() << (base + ".ui"), QDir::Files, QDirIterator::Subdirectories);
            if (it3.hasNext()) uiRel = root.relativeFilePath(it3.next());
        }

        // Уберём возможные дубликаты по имени класса
        bool dup = false;
        for (const auto& r : result) if (r.className == cls) { dup = true; break; }
        if (!dup) result.push_back(FormClass{cls, headerRel, cppRel, uiRel});
    }
    return result;
}

void ConnectApplication(QString sourceProjectPath)
{
    sourceProjectPath.remove(QRegularExpression("[\\x00-\\x1F\\x7F]"));
    sourceProjectPath.replace(QChar(0x00A0), QChar(' '));
    sourceProjectPath = QDir::fromNativeSeparators(sourceProjectPath);
    sourceProjectPath = QDir::cleanPath(sourceProjectPath);

    QDir srcDir(sourceProjectPath.trimmed());
    if (!srcDir.exists()) {
        qDebug() << "Project folder does not exist.";
        return;
    }

    const QString visualTesterSrc = QString::fromUtf8(VISUALTESTER_SOURCE_DIR);

    QDir vtSrcDir(visualTesterSrc);
    if (!vtSrcDir.exists()) {
        qDebug() << "VisualTester path (VISUALTESTER_SOURCE_DIR) incorrect: " << visualTesterSrc;
        return;
    }

    const QString aggregateRoot = QFileInfo(visualTesterSrc).dir().absolutePath(); // корень, где лежат VisualTester и будут проекты
    const QString projectName = srcDir.dirName();
    const QString projectDstPath = aggregateRoot + QDir::separator() + projectName;
    const QString projectLibName = projectName + "Lib";

    // 1) Копируем проект целиком в aggregateRoot (удаляем старый, если есть)
    if (QDir(projectDstPath).exists()) QDir(projectDstPath).removeRecursively();
    if (!copyRecursively(sourceProjectPath, projectDstPath)) {
        qDebug() << "Failed to copy project in " << projectDstPath;
        return;
    }

    // 2) Находим формы в скопированном проекте
    const QList<FormClass> forms = findForms(projectDstPath);

    // 3) Обновляем CMakeLists.txt подключаемого проекта: добавляем/заменяем add_library(<ProjectLib> ...)
    {
        const QString appCmakePath = projectDstPath + QDir::separator() + "CMakeLists.txt";
        QString existing;
        if (QFile::exists(appCmakePath)) {
            QFile f(appCmakePath);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                existing = QString::fromUtf8(f.readAll());
                f.close();
            }
        }

        // Удаляем старую секцию add_library(<projectLibName> ... )
        QString escLib = QRegularExpression::escape(projectLibName);
        QRegularExpression reLibBlock(QStringLiteral("(?s)add_library\\(\\s*%1.*?\\)\\s*").arg(escLib));
        existing.remove(reLibBlock);

        // Формируем новый блок библиотеки
        QString libBlock;
        libBlock += "add_library(" + projectLibName + "\n";
        for (const auto &f : forms) {
            libBlock += "    " + f.headerRel + "\n";
            if (!f.cppRel.isEmpty()) libBlock += "    " + f.cppRel + "\n";
            if (!f.uiRel.isEmpty())  libBlock += "    " + f.uiRel + "\n";
        }
        libBlock += ")\n";
        libBlock += "target_link_libraries(" + projectLibName + " PUBLIC Qt${QT_VERSION_MAJOR}::Widgets)\n";
        libBlock += "target_include_directories(" + projectLibName + " PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})\n";

        // Записываем файл (сохраняем существующий контент + добавляем libBlock в конец)
        QFile out(appCmakePath);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "Failed to write in " << appCmakePath;
            return;
        }
        QTextStream ts(&out);
        ts << existing.trimmed() << "\n\n" << libBlock;
        out.close();
    }

    // 4) Обновляем VisualTester/CMakeLists.txt: заменяем (если есть) target_link_libraries(...) для цели VisualTester на новую строку
    {
        const QString vtCmakePath = visualTesterSrc + QDir::separator() + "CMakeLists.txt";
        QFile f(vtCmakePath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Failed to open " << vtCmakePath;
            return;
        }
        QString content = QString::fromUtf8(f.readAll());
        f.close();

        // Определяем имя цели (ищем add_executable( <Name> ... ) - предпочитаем VisualTesterApp или VisualTester)
        QRegularExpression reExe(R"(add_executable\(\s*([A-Za-z_]\w*))");
        QRegularExpressionMatch m = reExe.match(content);
        QString vtTarget = "VisualTester";
        if (m.hasMatch()) {
            // Если первый найденный таргет имеет имя VisualTesterApp или VisualTester, используем его. Иначе берем первое.
            QString found = m.captured(1);
            if (!found.isEmpty()) vtTarget = found;
        }

        // Убираем все старые target_link_libraries(vtTarget ...) (строки, которые ссылаются на vtTarget)
        QRegularExpression reOldLink(QStringLiteral(".*target_link_libraries.*").arg(QRegularExpression::escape(vtTarget)));
        content.remove(reOldLink);

        // Добавляем нашу новую строку (заменяем старую) — ставим после content (или лучше — после первой occurrence add_executable)
        QString newLink = QString("\ntarget_link_libraries(%1 Qt6::Widgets %2)\n").arg(vtTarget, projectLibName);

        // Попробуем вставить newLink непосредственно после строки add_executable(<vtTarget ...>)
        QRegularExpression reAddExeLine(QStringLiteral("(?m)^\\s*add_executable\\(\\s*%1[\\s\\S]*?\\)\\s*$").arg(QRegularExpression::escape(vtTarget)));
        QRegularExpressionMatch addExeMatch = reAddExeLine.match(content);
        if (addExeMatch.hasMatch()) {
            int insertPos = addExeMatch.capturedEnd();
            content.insert(insertPos, "\n" + newLink);
        } else {
            content.append("\n" + newLink);
        }

        // Запись обратно
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "Failed to write " << vtCmakePath;
            return;
        }
        QTextStream(&f) << content;
        f.close();
    }

    // 5) Обновляем VisualTester/formsForTest.h: пишем инклюды к найденным заголовкам
    {
        const QString formsHeaderPath = visualTesterSrc + QDir::separator() + "formsForTest.h";
        QFile fh(formsHeaderPath);
        if (!fh.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "Failed to write " << formsHeaderPath;
            return;
        }
        QTextStream out(&fh);
        out << "#pragma once\n\n";
        for (const auto &f : forms) {
            // include относительный: "../<ProjectName>/<headerRel>"
            QString incl = QString("#include \"../%1/%2\"").arg(projectName, f.headerRel);
            out << incl << "\n";
        }
        fh.close();
    }

    // 6) Обновляем VisualTester/main.cpp: заменяем маркер "// REGISTER_FORM" на строки REGISTER_FORM("Name", Name);
    {
        const QString mainCppPath = visualTesterSrc + QDir::separator() + "main.cpp";
        QFile mf(mainCppPath);
        if (!mf.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Failed to open " << mainCppPath;
            return;
        }
        QString content = QString::fromUtf8(mf.readAll());
        mf.close();

        QString regs;
        for (const auto &f : forms) {
            regs += QString("    REGISTER_FORM(\"%1\", %2);\n").arg(f.className, f.className);
        }
        if (regs.isEmpty()) regs = "    // (формы не найдены)\n";

        QRegularExpression marker(R"(//\s*REGISTER_FORM\b)");
        if (marker.match(content).hasMatch()) {
            content.replace(marker, regs.trimmed());
        } else {
            // вставляем перед main() если маркера нет
            QRegularExpression reMain(R"(\bint\s+main\s*\()");
            QRegularExpressionMatch mm = reMain.match(content);
            if (mm.hasMatch()) {
                int bracePos = content.indexOf('{', mm.capturedStart());
                if (bracePos != -1) content.insert(bracePos + 1, "\n" + regs + "\n");
                else content.append("\n" + regs + "\n");
            } else {
                content.append("\n" + regs + "\n");
            }
        }

        if (!mf.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "Failed to write " << mainCppPath;
            return;
        }
        QTextStream(&mf) << content;
        mf.close();
    }

    // 7) Перезаписываем корневой CMakeLists.txt по заданному шаблону (в котором меняется лишь имя добавляемого проекта)
    {
        const QString rootCMakePath = aggregateRoot + QDir::separator() + "CMakeLists.txt";
        QFile rf(rootCMakePath);
        if (!rf.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << "Failed to write " << rootCMakePath;
            return;
        }
        QTextStream ts(&rf);
        ts << "cmake_minimum_required(VERSION 3.16)\n"
              "project(VisualTesterAggregate)\n\n"
              "set(CMAKE_CXX_STANDARD 17)\n\n"
              "# Qt setup\n"
              "set(CMAKE_AUTOMOC ON)\n"
              "set(CMAKE_AUTORCC ON)\n"
              "set(CMAKE_AUTOUIC ON)\n"
              "find_package(Qt6 REQUIRED COMPONENTS Widgets)\n\n"
           << "add_subdirectory(" << projectName << ")\n"
           << "add_subdirectory(" << QFileInfo(visualTesterSrc).fileName() << ")\n";
        rf.close();
    }

    qDebug() <<  "Succesful!\nProject " << projectName << "connected.\nRebuild project from root CMakeLists.txt: " << aggregateRoot;
    QCoreApplication::quit();
}

