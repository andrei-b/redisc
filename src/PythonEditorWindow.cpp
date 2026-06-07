#include "PythonEditorWindow.h"

#include "PythonHighlighter.h"

#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextStream>
#include <QVBoxLayout>

PythonEditorWindow::PythonEditorWindow(const QString &path, QWidget *parent)
    : QWidget(parent)
    , m_path(path)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);

    auto *title = new QLabel(QFileInfo(path).fileName(), this);
    m_editor = new QPlainTextEdit(this);
    m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    QFont font("monospace");
    font.setStyleHint(QFont::Monospace);
    font.setPointSize(10);
    m_editor->setFont(font);
    new PythonHighlighter(m_editor->document());

    auto *buttons = new QHBoxLayout;
    auto *saveButton = new QPushButton("Save", this);
    buttons->addWidget(saveButton);
    buttons->addStretch(1);

    layout->addWidget(title);
    layout->addWidget(m_editor, 1);
    layout->addLayout(buttons);

    connect(saveButton, &QPushButton::clicked, this, [this]() {
        if (save()) {
            emit saved(m_path);
        }
    });

    load();
}

QString PythonEditorWindow::path() const
{
    return m_path;
}

bool PythonEditorWindow::load()
{
    QFile file(m_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Python editor", QString("Could not open %1").arg(m_path));
        return false;
    }

    QTextStream stream(&file);
    m_editor->setPlainText(stream.readAll());
    return true;
}

bool PythonEditorWindow::save()
{
    QFile file(m_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Python editor", QString("Could not save %1").arg(m_path));
        return false;
    }

    QTextStream stream(&file);
    stream << m_editor->toPlainText();
    return true;
}
