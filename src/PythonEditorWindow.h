#pragma once

#include <QWidget>

class QPlainTextEdit;

class PythonEditorWindow : public QWidget {
    Q_OBJECT

public:
    explicit PythonEditorWindow(const QString &path, QWidget *parent = nullptr);

    QString path() const;
    bool load();
    bool save();

signals:
    void saved(const QString &path);

private:
    QString m_path;
    QPlainTextEdit *m_editor = nullptr;
};
