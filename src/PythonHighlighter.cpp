#include "PythonHighlighter.h"

#include <QTextDocument>

PythonHighlighter::PythonHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    QTextCharFormat keyword;
    keyword.setForeground(QColor("#c586c0"));
    keyword.setFontWeight(QFont::Bold);

    const QStringList keywords = {
        "False", "None", "True", "and", "as", "assert", "async", "await", "break", "class",
        "continue", "def", "del", "elif", "else", "except", "finally", "for", "from",
        "global", "if", "import", "in", "is", "lambda", "nonlocal", "not", "or", "pass",
        "raise", "return", "try", "while", "with", "yield"
    };
    for (const QString &word : keywords) {
        m_rules.append({QRegularExpression(QString("\\b%1\\b").arg(word)), keyword});
    }

    QTextCharFormat builtin;
    builtin.setForeground(QColor("#4fc1ff"));
    const QStringList builtins = {
        "bool", "dict", "enumerate", "float", "int", "len", "list", "print", "range", "set",
        "str", "tuple", "publish", "subscribe", "unsubscribe", "log"
    };
    for (const QString &word : builtins) {
        m_rules.append({QRegularExpression(QString("\\b%1\\b").arg(word)), builtin});
    }

    QTextCharFormat function;
    function.setForeground(QColor("#dcdcaa"));
    m_rules.append({QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\s*\\()"), function});

    QTextCharFormat number;
    number.setForeground(QColor("#b5cea8"));
    m_rules.append({QRegularExpression("\\b[0-9]+(?:\\.[0-9]+)?\\b"), number});

    QTextCharFormat string;
    string.setForeground(QColor("#ce9178"));
    m_rules.append({QRegularExpression("\"[^\"\\\\]*(?:\\\\.[^\"\\\\]*)*\""), string});
    m_rules.append({QRegularExpression("'[^'\\\\]*(?:\\\\.[^'\\\\]*)*'"), string});

    QTextCharFormat comment;
    comment.setForeground(QColor("#6a9955"));
    comment.setFontItalic(true);
    m_rules.append({QRegularExpression("#[^\\n]*"), comment});

    m_tripleQuoteFormat = string;
}

void PythonHighlighter::highlightBlock(const QString &text)
{
    for (const Rule &rule : m_rules) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    setCurrentBlockState(0);
    const QRegularExpression triple("('''|\"\"\")");
    int start = 0;
    if (previousBlockState() != 1) {
        const QRegularExpressionMatch match = triple.match(text);
        start = match.hasMatch() ? match.capturedStart() : -1;
    }

    while (start >= 0) {
        const QString delimiter = text.mid(start, 3);
        const int end = text.indexOf(delimiter, start + 3);
        int length = 0;
        if (end == -1) {
            setCurrentBlockState(1);
            length = text.length() - start;
        } else {
            length = end - start + 3;
        }
        setFormat(start, length, m_tripleQuoteFormat);
        if (end == -1) {
            break;
        }
        const QRegularExpressionMatch next = triple.match(text, start + length);
        start = next.hasMatch() ? next.capturedStart() : -1;
    }
}
