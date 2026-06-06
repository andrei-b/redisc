#include "JsonTreeWindow.h"

#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

JsonTreeWindow::JsonTreeWindow(const QString &channel, const QString &jsonText, QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    auto *header = new QLabel(QString("Channel: %1").arg(channel), this);
    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(3);
    m_tree->setHeaderLabels({"Key", "Type", "Value"});
    m_tree->setAlternatingRowColors(false);

    auto *buttons = new QHBoxLayout;
    auto *expandAll = new QPushButton("Expand all", this);
    auto *collapseAll = new QPushButton("Collapse all", this);
    buttons->addWidget(expandAll);
    buttons->addWidget(collapseAll);
    buttons->addStretch(1);

    layout->addWidget(header);
    layout->addWidget(m_tree, 1);
    layout->addLayout(buttons);

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(jsonText.toUtf8(), &error);
    if (error.error == QJsonParseError::NoError && !document.isNull()) {
        if (document.isObject()) {
            addValue(nullptr, "$", document.object());
        } else if (document.isArray()) {
            addValue(nullptr, "$", document.array());
        }
        m_tree->expandToDepth(1);
        m_tree->resizeColumnToContents(0);
        m_tree->resizeColumnToContents(1);
    } else {
        auto *item = new QTreeWidgetItem({"Parse error", "error", error.errorString()});
        m_tree->addTopLevelItem(item);
    }

    connect(expandAll, &QPushButton::clicked, m_tree, &QTreeWidget::expandAll);
    connect(collapseAll, &QPushButton::clicked, m_tree, &QTreeWidget::collapseAll);
}

void JsonTreeWindow::addValue(QTreeWidgetItem *parent, const QString &name, const QJsonValue &value)
{
    auto *item = new QTreeWidgetItem({name, typeName(value), primitiveText(value)});
    if (parent) {
        parent->addChild(item);
    } else {
        m_tree->addTopLevelItem(item);
    }

    if (value.isObject()) {
        const QJsonObject object = value.toObject();
        for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
            addValue(item, it.key(), it.value());
        }
    } else if (value.isArray()) {
        const QJsonArray array = value.toArray();
        for (int i = 0; i < array.size(); ++i) {
            addValue(item, QString("[%1]").arg(i), array.at(i));
        }
    }
}

QString JsonTreeWindow::typeName(const QJsonValue &value) const
{
    if (value.isObject()) {
        return "object";
    }
    if (value.isArray()) {
        return "array";
    }
    if (value.isString()) {
        return "string";
    }
    if (value.isDouble()) {
        return "number";
    }
    if (value.isBool()) {
        return "boolean";
    }
    if (value.isNull()) {
        return "null";
    }
    return "undefined";
}

QString JsonTreeWindow::primitiveText(const QJsonValue &value) const
{
    if (value.isString()) {
        return value.toString();
    }
    if (value.isDouble()) {
        return QString::number(value.toDouble(), 'g', 15);
    }
    if (value.isBool()) {
        return value.toBool() ? "true" : "false";
    }
    if (value.isNull()) {
        return "null";
    }
    return QString();
}
