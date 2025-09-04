#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml>

void printResources() {
    // Get the list of resources
    QStringList resourceList = QDir(":/").entryList(QDir::Files | QDir::NoDotAndDotDot);
    
    qDebug() << "Resources:";
    for (const QString &resource : resourceList) {
        qDebug() << resource;
    }
}

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    // Print the resources
    printResources();

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/UI/qml/main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}