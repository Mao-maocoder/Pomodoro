#pragma once
#include <QDate>
#include <QDateTime>
#include <QJsonObject>
#include <QMap>
#include <QVariantList>
struct DailyTotal { int count = 0; qint64 durationMs = 0; };
class DailyStatistics {
public:
    void addFocus(qint64 durationMs, const QDateTime &completedAt);
    DailyTotal total(const QDate &date) const;
    QVariantList recent(const QDate &today, int days = 7) const;
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject &json, QString &error);
private:
    QMap<QDate, DailyTotal> m_days;
};

