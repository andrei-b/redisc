#pragma once

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class PythonHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit PythonHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QVector<Rule> m_rules;
    QTextCharFormat m_tripleQuoteFormat;
};
