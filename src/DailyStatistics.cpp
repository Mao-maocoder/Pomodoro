#include "DailyStatistics.h"
void DailyStatistics::addFocus(qint64 durationMs, const QDateTime &completedAt)
{
    auto &entry = m_days[completedAt.date()];
    ++entry.count;
    entry.durationMs += durationMs;
}
DailyTotal DailyStatistics::total(const QDate &date) const { return m_days.value(date); }
QVariantList DailyStatistics::recent(const QDate &today, int days) const
{
    QVariantList result;
    for (int index = 0; index < days; ++index) {
        const QDate date = today.addDays(-index);
        const auto value = total(date);
        result.append(QVariantMap{{"date", date.toString("MM/dd")},
            {"label", index == 0 ? QStringLiteral("今天") : date.toString("MM/dd")},
            {"count", value.count}, {"minutes", value.durationMs / 60000}});
    }
    return result;
}
QJsonObject DailyStatistics::toJson() const
{
    QJsonObject json;
    for (auto it = m_days.cbegin(); it != m_days.cend(); ++it)
        json.insert(it.key().toString(Qt::ISODate),
            QJsonObject{{"count", it.value().count}, {"durationMs", it.value().durationMs}});
    return json;
}
bool DailyStatistics::fromJson(const QJsonObject &json, QString &error)
{
    QMap<QDate, DailyTotal> candidate;
    for (auto it = json.constBegin(); it != json.constEnd(); ++it) {
        const QDate date = QDate::fromString(it.key(), Qt::ISODate);
        const auto entry = it.value().toObject();
        const auto count = entry.value("count");
        const auto duration = entry.value("durationMs");
        if (!date.isValid() || date.toString(Qt::ISODate) != it.key() || !it.value().isObject()
            || !count.isDouble() || count.toDouble() < 0 || count.toDouble() > 100000000
            || count.toDouble() != count.toInt()
            || !duration.isDouble() || duration.toDouble() < 0 || duration.toDouble() > 1.0e15
            || duration.toDouble() != static_cast<qint64>(duration.toDouble())) {
            error = QStringLiteral("保存的统计格式无效，原文件已保留。");
            return false;
        }
        candidate.insert(date, {count.toInt(), static_cast<qint64>(duration.toDouble())});
    }
    m_days = candidate;
    return true;
}

