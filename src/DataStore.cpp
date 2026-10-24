#include "DataStore.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSaveFile>
DataStore::DataStore(QString filePath) : m_filePath(std::move(filePath)) {}
bool DataStore::read(QJsonObject &document, bool &exists, QString &error) const
{
    exists = QFileInfo::exists(m_filePath);
    if (!exists) return true;
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("无法读取数据文件：%1\n%2").arg(m_filePath, file.errorString());
        return false;
    }
    const QByteArray contents = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        error = QStringLiteral("数据文件读取失败：%1").arg(file.errorString());
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument parsed = QJsonDocument::fromJson(contents, &parseError);
    if (parseError.error != QJsonParseError::NoError || !parsed.isObject()) {
        error = QStringLiteral("数据文件损坏：%1\n%2\n原文件已保留，程序不会覆盖它。")
            .arg(m_filePath, parseError.errorString());
        return false;
    }
    document = parsed.object();
    return true;
}
bool DataStore::write(const QJsonObject &document, QString &error) const
{
    const QString directory = QFileInfo(m_filePath).absolutePath();
    if (!QDir().mkpath(directory)) {
        error = QStringLiteral("无法创建数据目录：%1").arg(directory);
        return false;
    }
    QSaveFile file(m_filePath);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly)) {
        error = QStringLiteral("无法保存数据：%1\n%2").arg(m_filePath, file.errorString());
        return false;
    }
    const QByteArray bytes = QJsonDocument(document).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size()) {
        error = QStringLiteral("数据写入失败：%1").arg(file.errorString());
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        error = QStringLiteral("数据提交失败：%1").arg(file.errorString());
        return false;
    }
    return true;
}

