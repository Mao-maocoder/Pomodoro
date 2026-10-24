#include "AppController.h"
#include "PowerMonitor.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setOrganizationName("Pomodoro");
    application.setApplicationName("Pomodoro");
    application.setApplicationDisplayName(QStringLiteral("番茄钟"));
    application.setApplicationVersion("1.0.0");
    application.setQuitOnLastWindowClosed(false);
    application.setWindowIcon(AppController::applicationIcon());
    QQuickStyle::setStyle("Basic");
    QString projectRoot = QCoreApplication::applicationDirPath();
    const QString directoryName = QFileInfo(projectRoot).fileName();
    if (directoryName.compare("dist", Qt::CaseInsensitive) == 0
        || directoryName.compare("build", Qt::CaseInsensitive) == 0)
        projectRoot = QDir(projectRoot).absoluteFilePath("..");
    projectRoot = QDir::cleanPath(projectRoot);
    const QByteArray identity = (projectRoot.toLower() + "|" + qEnvironmentVariable("USERNAME")).toUtf8();
    const QString serverName = "Pomodoro-" + QString::fromLatin1(
        QCryptographicHash::hash(identity, QCryptographicHash::Sha256).toHex().left(24));
    QLocalSocket existing;
    existing.connectToServer(serverName);
    if (existing.waitForConnected(500)) {
        existing.write("show");
        if (existing.bytesToWrite() > 0 && !existing.waitForBytesWritten(1000)) {
            QMessageBox::critical(nullptr, QStringLiteral("无法打开番茄钟"), existing.errorString());
            return 1;
        }
        return 0;
    }
    QLocalServer singleInstance;
    singleInstance.setSocketOptions(QLocalServer::UserAccessOption);
    if (!singleInstance.listen(serverName)) {
        QMessageBox::critical(nullptr, QStringLiteral("番茄钟启动失败"),
            QStringLiteral("无法建立单实例通信：%1").arg(singleInstance.errorString()));
        return 1;
    }
    AppController controller(projectRoot);
    QString error;
    if (!controller.initialize(error)) {
        QMessageBox::critical(nullptr, QStringLiteral("番茄钟数据读取失败"), error);
        return 1;
    }
    PowerMonitor powerMonitor(&controller);
    application.installNativeEventFilter(&powerMonitor);
    QQmlApplicationEngine qml;
    qml.rootContext()->setContextProperty("appController", &controller);
    QObject::connect(&qml, &QQmlApplicationEngine::objectCreationFailed, &application,
        [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    qml.loadFromModule("PomodoroApp", "Main");
    if (qml.rootObjects().isEmpty()) return 1;
    auto *window = qobject_cast<QQuickWindow *>(qml.rootObjects().first());
    if (!window) return 1;
    controller.attachWindow(window);
    QObject::connect(&singleInstance, &QLocalServer::newConnection, &controller, [&] {
        while (singleInstance.hasPendingConnections()) {
            auto *socket = singleInstance.nextPendingConnection();
            QObject::connect(socket, &QLocalSocket::readyRead, &controller, [&controller, socket] {
                socket->readAll();
                controller.showWindow();
                socket->disconnectFromServer();
            });
            QObject::connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
            if (socket->bytesAvailable() > 0) {
                socket->readAll();
                controller.showWindow();
                socket->disconnectFromServer();
            }
        }
    });
    QObject::connect(&application, &QCoreApplication::aboutToQuit, &controller, [&controller] {
        controller.engine()->pause();
        controller.save();
    });
    const int result = application.exec();
    application.removeNativeEventFilter(&powerMonitor);
    return result;
}
