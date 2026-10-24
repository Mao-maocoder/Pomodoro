#pragma once
#include <QJsonObject>
#include <QString>
class DataStore {
public:
    explicit DataStore(QString filePath);
    const QString &filePath() const { return m_filePath; }
    bool read(QJsonObject &document, bool &exists, QString &error) const;
    bool write(const QJsonObject &document, QString &error) const;
private:
    QString m_filePath;
};

