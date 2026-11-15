// SCAPMS - Smart Clinic Appointment & Patient Management System
// COS202 Programming II - Lab Assessment Project
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QStyleFactory>

#include "gui/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("SCAPMS");
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    scapms::gui::MainWindow window;

    // Load the sample data file if it is available next to the program or in the working folder.
    const QStringList candidates = {QDir(QCoreApplication::applicationDirPath()).filePath("data/clinic_data.csv"),
                                    QDir::current().filePath("data/clinic_data.csv")};
    for (const QString& path : candidates) {
        if (QFileInfo::exists(path)) {
            window.loadFile(path, true);
            break;
        }
    }

    window.show();
    return app.exec();
}
