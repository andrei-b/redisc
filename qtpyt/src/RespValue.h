#pragma once

#include <QList>
#include <QString>

struct RespValue {
    enum class Type {
        Null,
        SimpleString,
        Error,
        Integer,
        BulkString,
        Array
    };

    Type type = Type::Null;
    QString text;
    qint64 integer = 0;
    QList<RespValue> array;

    bool isError() const { return type == Type::Error; }
};
