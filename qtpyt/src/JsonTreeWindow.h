#pragma once

#include <QJsonValue>
#include <QWidget>

class QTreeWidget;
class QTreeWidgetItem;

class JsonTreeWindow : public QWidget {
    Q_OBJECT

public:
    explicit JsonTreeWindow(const QString &channel, const QString &jsonText, QWidget *parent = nullptr);

private:
    void addValue(QTreeWidgetItem *parent, const QString &name, const QJsonValue &value);
    QString typeName(const QJsonValue &value) const;
    QString primitiveText(const QJsonValue &value) const;

    QTreeWidget *m_tree = nullptr;
};
