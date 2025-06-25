#include <QApplication>
#include <QStyleFactory>
#include <QtCore/QDir>
#include <QtCore/QLibraryInfo>
#include <QtCore/QTranslator>
#include <QtCore/QLocale>
#include <QSplashScreen>
#include <QPixmap>
#include <QTimer>

#include "MainWindow.h"

void setApplicationStyle()
{
    // Set a modern dark theme
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    
    QApplication::setPalette(darkPalette);
}

void setupTranslation(QApplication& app)
{
    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "HybridCAD_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            app.installTranslator(&translator);
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application information
    app.setApplicationName("HybridCAD");
    app.setApplicationVersion("1.0.0");
    app.setApplicationDisplayName("HybridCAD - Advanced CAD & Mesh Editor");
    app.setOrganizationName("HybridCAD Team");
    app.setOrganizationDomain("hybridcad.org");
    
    // High DPI support is automatically enabled in Qt6
    
    // Set application style
    setApplicationStyle();
    
    // Setup translations
    setupTranslation(app);
    
    // Create splash screen
    QPixmap splashPixmap(400, 300);
    splashPixmap.fill(QColor(53, 53, 53));
    
    QSplashScreen splash(splashPixmap);
    splash.showMessage("Loading HybridCAD...", Qt::AlignHCenter | Qt::AlignBottom, Qt::white);
    splash.show();
    
    app.processEvents();
    
    // Create main window
    HybridCAD::MainWindow* window = new HybridCAD::MainWindow();
    
    // Simulate loading time and then show window
    QTimer::singleShot(2000, [&splash, window]() {
        window->show();
        splash.finish(window);
    });
    
    return app.exec();
} 