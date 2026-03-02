// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Modified by llahteinen

#ifndef LSYNTAXHIGHLIGHTER_H
#define LSYNTAXHIGHLIGHTER_H

#include <QObject>
#include <QTextObject>

QT_BEGIN_NAMESPACE
class QTextDocument;
class LSyntaxHighlighterPrivate;
class QTextCharFormat;
class QFont;
class QColor;
class QTextBlockUserData;
QT_END_NAMESPACE


class LSyntaxHighlighter : public QObject
{
    Q_OBJECT
    friend class LSyntaxHighlighterPrivate;
public:
    explicit LSyntaxHighlighter(QObject *parent);
    explicit LSyntaxHighlighter(QTextDocument *parent);
    ~LSyntaxHighlighter();

    void setDocument(QTextDocument *doc);
    QTextDocument *document() const;

public Q_SLOTS:
    void rehighlight();
    void rehighlight(int topPos);
    void continueRehighlight();
    void rehighlightBlock(const QTextBlock &block);
    virtual void abort(bool abort = true);

signals:
    void rehighlightFinished();

protected:
    virtual void highlightBlock(const QString &text) = 0;

    void setFormat(int start, int count, const QTextCharFormat &format);
    void setFormat(int start, int count, const QColor &color);
    void setFormat(int start, int count, const QFont &font);
    QTextCharFormat format(int pos) const;

    int previousBlockState() const;
    int currentBlockState() const;
    void setCurrentBlockState(int newState);

    void setCurrentBlockUserData(QTextBlockUserData *data);
    QTextBlockUserData *currentBlockUserData() const;

    QTextBlock currentBlock() const;

private:
    Q_DISABLE_COPY(LSyntaxHighlighter)

    static constexpr int m_maxBatchSize = 1'000'000'000;
    static constexpr int m_minBatchSize = 10'000;
    static constexpr int m_startingBatchSize = 100'000; /// Start a bit low to ensure smooth GUI updates
    static constexpr int m_frameTimeTarget = 16;
    int m_batchSize = m_startingBatchSize;
    int m_rehighlightProgress{-1};

    std::unique_ptr<LSyntaxHighlighterPrivate> const d;
};


#endif // LSYNTAXHIGHLIGHTER_H
