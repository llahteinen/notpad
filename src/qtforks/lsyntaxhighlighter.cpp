// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Modified by llahteinen

#include "lsyntaxhighlighter.h"

#include <qtextdocument.h>
#include <qtextlayout.h>
#include <qpointer.h>
#include <qscopedvaluerollback.h>
#include <qtextobject.h>
#include <qtextcursor.h>
#include <qdebug.h>
#include <qtimer.h>
#include <QElapsedTimer>



class LSyntaxHighlighterPrivate
{
public:
    inline explicit LSyntaxHighlighterPrivate(LSyntaxHighlighter* q_)
        : doc{}
        , formatChanges{}
        , currentBlock{}
        , rehighlightPending{false}, inReformatBlocks{false}
        , _q_reformatBlocks_conn{}
        , m_abort{false}
        , q{q_}
    {}

    QPointer<QTextDocument> doc;

    void _q_reformatBlocks(int from, int charsRemoved, int charsAdded);
    void reformatBlocks(int from, int charsRemoved, int charsAdded);
    void reformatBlock(const QTextBlock &block);

    inline void rehighlight(QTextCursor &cursor, QTextCursor::MoveOperation operation)
    {
        QScopedValueRollback<bool> bg(inReformatBlocks, true);
        cursor.beginEditBlock();
        int from = cursor.position();
        cursor.movePosition(operation);
        reformatBlocks(from, 0, cursor.position() - from);
        cursor.endEditBlock();
    }

    inline void _q_delayedRehighlight() {
        if (!rehighlightPending)
            return;
        rehighlightPending = false;
        q->rehighlight();
    }

    void applyFormatChanges();
    QList<QTextCharFormat> formatChanges;
    QTextBlock currentBlock;
    bool rehighlightPending;
    bool inReformatBlocks;
    QMetaObject::Connection _q_reformatBlocks_conn;

    std::atomic_bool m_abort;

    LSyntaxHighlighter* const q;

    Q_DISABLE_COPY(LSyntaxHighlighterPrivate)
};

void LSyntaxHighlighterPrivate::applyFormatChanges()
{
    bool formatsChanged = false;

    QTextLayout *layout = currentBlock.layout();

    QList<QTextLayout::FormatRange> ranges = layout->formats();

    const int preeditAreaStart = layout->preeditAreaPosition();
    const int preeditAreaLength = layout->preeditAreaText().size();

    if (preeditAreaLength != 0) {
        auto isOutsidePreeditArea = [=](const QTextLayout::FormatRange &range) {
            return range.start < preeditAreaStart
                    || range.start + range.length > preeditAreaStart + preeditAreaLength;
        };
        if (ranges.removeIf(isOutsidePreeditArea) > 0)
            formatsChanged = true;
    } else if (!ranges.isEmpty()) {
        ranges.clear();
        formatsChanged = true;
    }

    int i = 0;
    while (i < formatChanges.size()) {
        QTextLayout::FormatRange r;

        while (i < formatChanges.size() && formatChanges.at(i) == r.format)
            ++i;

        if (i == formatChanges.size())
            break;

        r.start = i;
        r.format = formatChanges.at(i);

        while (i < formatChanges.size() && formatChanges.at(i) == r.format)
            ++i;

        Q_ASSERT(i <= formatChanges.size());
        r.length = i - r.start;

        if (preeditAreaLength != 0) {
            if (r.start >= preeditAreaStart)
                r.start += preeditAreaLength;
            else if (r.start + r.length >= preeditAreaStart)
                r.length += preeditAreaLength;
        }

        ranges << r;
        formatsChanged = true;
    }

    if (formatsChanged) {
        layout->setFormats(ranges);
        doc->markContentsDirty(currentBlock.position(), currentBlock.length());
    }
}

void LSyntaxHighlighterPrivate::_q_reformatBlocks(int from, int charsRemoved, int charsAdded)
{
    if (!inReformatBlocks && !rehighlightPending)
        reformatBlocks(from, charsRemoved, charsAdded);
}

void LSyntaxHighlighterPrivate::reformatBlocks(int from, int charsRemoved, int charsAdded)
{
    QTextBlock block = doc->findBlock(from);
    if (!block.isValid())
        return;

    int endPosition;
    QTextBlock lastBlock = doc->findBlock(from + charsAdded + (charsRemoved > 0 ? 1 : 0));
    if (lastBlock.isValid())
        endPosition = lastBlock.position() + lastBlock.length();
    else
        endPosition = doc->lastBlock().position() + doc->lastBlock().length();

    bool forceHighlightOfNextBlock = false;

    while(!m_abort && block.isValid() && (block.position() < endPosition || forceHighlightOfNextBlock))
    {
        const int stateBeforeHighlight = block.userState();

        reformatBlock(block);

        forceHighlightOfNextBlock = (block.userState() != stateBeforeHighlight);

        block = block.next();
    }

    formatChanges.clear();
    q->m_rehighlightProgress = block.position();
}

void LSyntaxHighlighterPrivate::reformatBlock(const QTextBlock &block)
{
    Q_ASSERT_X(!currentBlock.isValid(), "LSyntaxHighlighter::reformatBlock()", "reFormatBlock() called recursively");

    currentBlock = block;

    formatChanges.fill(QTextCharFormat(), block.length() - 1);
    q->highlightBlock(block.text());
    applyFormatChanges();

    currentBlock = QTextBlock();
}

/*!
    \class LSyntaxHighlighter
    \reentrant
    \inmodule QtGui

    \brief The LSyntaxHighlighter class allows you to define syntax
    highlighting rules, and in addition you can use the class to query
    a document's current formatting or user data.

    \since 4.1

    \ingroup richtext-processing

    The LSyntaxHighlighter class is a base class for implementing
    QTextDocument syntax highlighters.  A syntax highligher automatically
    highlights parts of the text in a QTextDocument. Syntax highlighters are
    often used when the user is entering text in a specific format (for example source code)
    and help the user to read the text and identify syntax errors.

    To provide your own syntax highlighting, you must subclass
    LSyntaxHighlighter and reimplement highlightBlock().

    When you create an instance of your LSyntaxHighlighter subclass,
    pass it the QTextDocument that you want the syntax
    highlighting to be applied to. For example:

    \snippet code/src_gui_text_LSyntaxHighlighter.cpp 0

    After this your highlightBlock() function will be called
    automatically whenever necessary. Use your highlightBlock()
    function to apply formatting (e.g. setting the font and color) to
    the text that is passed to it. LSyntaxHighlighter provides the
    setFormat() function which applies a given QTextCharFormat on
    the current text block. For example:

    \snippet code/src_gui_text_LSyntaxHighlighter.cpp 1

    \target LSyntaxHighlighter multiblock

    Some syntaxes can have constructs that span several text
    blocks. For example, a C++ syntax highlighter should be able to
    cope with \c{/}\c{*...*}\c{/} multiline comments. To deal with
    these cases it is necessary to know the end state of the previous
    text block (e.g. "in comment").

    Inside your highlightBlock() implementation you can query the end
    state of the previous text block using the previousBlockState()
    function. After parsing the block you can save the last state
    using setCurrentBlockState().

    The currentBlockState() and previousBlockState() functions return
    an int value. If no state is set, the returned value is -1. You
    can designate any other value to identify any given state using
    the setCurrentBlockState() function. Once the state is set the
    QTextBlock keeps that value until it is set again or until the
    corresponding paragraph of text is deleted.

    For example, if you're writing a simple C++ syntax highlighter,
    you might designate 1 to signify "in comment":

    \snippet code/src_gui_text_LSyntaxHighlighter.cpp 2

    In the example above, we first set the current block state to
    0. Then, if the previous block ended within a comment, we highlight
    from the beginning of the current block (\c {startIndex =
    0}). Otherwise, we search for the given start expression. If the
    specified end expression cannot be found in the text block, we
    change the current block state by calling setCurrentBlockState(),
    and make sure that the rest of the block is highlighted.

    In addition you can query the current formatting and user data
    using the format() and currentBlockUserData() functions
    respectively. You can also attach user data to the current text
    block using the setCurrentBlockUserData() function.
    QTextBlockUserData can be used to store custom settings. In the
    case of syntax highlighting, it is in particular interesting as
    cache storage for information that you may figure out while
    parsing the paragraph's text. For an example, see the
    setCurrentBlockUserData() documentation.

    \sa QTextDocument, {Syntax Highlighter Example}
*/

/*!
    Constructs a LSyntaxHighlighter with the given \a parent.

    If the parent is a QTextEdit, it installs the syntax highlighter on the
    parents document. The specified QTextEdit also becomes the owner of
    the LSyntaxHighlighter.
*/
LSyntaxHighlighter::LSyntaxHighlighter(QObject *parent)
    : QObject(parent), d{new LSyntaxHighlighterPrivate{this}}
{
    if (parent && parent->inherits("QTextEdit")) {
        QTextDocument *doc = qvariant_cast<QTextDocument *>(parent->property("document"));
        if (doc)
            setDocument(doc);
    }
}

/*!
    Constructs a LSyntaxHighlighter and installs it on \a parent.
    The specified QTextDocument also becomes the owner of the
    LSyntaxHighlighter.
*/
LSyntaxHighlighter::LSyntaxHighlighter(QTextDocument *parent)
    : QObject(parent), d{new LSyntaxHighlighterPrivate{this}}
{
    setDocument(parent);
}

/*!
    Destructor. Uninstalls this syntax highlighter from the text document.
*/
LSyntaxHighlighter::~LSyntaxHighlighter()
{
    setDocument(nullptr);
}

/*!
    Installs the syntax highlighter on the given QTextDocument \a doc.
    A LSyntaxHighlighter can only be used with one document at a time.
*/
void LSyntaxHighlighter::setDocument(QTextDocument *doc)
{
    if (d->doc) {
        disconnect(d->_q_reformatBlocks_conn);

        QTextCursor cursor(d->doc);
        cursor.beginEditBlock();
        for (QTextBlock blk = d->doc->begin(); blk.isValid(); blk = blk.next())
            blk.layout()->clearFormats();
        cursor.endEditBlock();
    }
    d->doc = doc;
    if (d->doc) {
        d->_q_reformatBlocks_conn = connect(d->doc, &QTextDocument::contentsChange,
            this, [this](int from, int charsRemoved, int charsAdded){ d->_q_reformatBlocks(from, charsRemoved, charsAdded); });
        if (!d->doc->isEmpty()) {
            d->rehighlightPending = true;
            QMetaObject::invokeMethod(this, [this]{ d->_q_delayedRehighlight(); });
        }
    }
}

/*!
    Returns the QTextDocument on which this syntax highlighter is
    installed.
*/
QTextDocument *LSyntaxHighlighter::document() const
{
    return d->doc;
}

/*!
    \since 4.2

    Reapplies the highlighting to the whole document.

    \sa rehighlightBlock()
*/
void LSyntaxHighlighter::rehighlight()
{
    if (!d->doc)
        return;

    /// Qt implementation:
//    QTextCursor cursor(d->doc);
//    d->rehighlight(cursor, QTextCursor::End);
//    d->rehighlightPending = false; // user manually did a full rehighlight
    /// NOTE the Qt method emits contentsChanged after the rehighlight, but this method does not

    /// Start rehighlighting process in chunks
    m_rehighlightProgress = 0;

    continueRehighlight();
}

void LSyntaxHighlighter::continueRehighlight()
{
    if (!d->doc)
        return;

    /// Last argument of reformatBlocks should be length of the job
    const auto total_length = d->doc->lastBlock().position() + d->doc->lastBlock().length() - 1;
    const auto length = total_length - m_rehighlightProgress;
    const auto todo = qMin(length, m_batchSize);

    QElapsedTimer timer;
    timer.start();

    d->reformatBlocks(m_rehighlightProgress, 0, todo);

    const int elapsed = timer.elapsed();
    if(elapsed < m_frameTimeTarget)
    {
        /// Too small value here slightly slows down. Too big value does not give any benefit
        const int batch_size = m_batchSize * 1.3f;
        m_batchSize = qMin(m_maxBatchSize, batch_size);
    }
    else if(elapsed > m_frameTimeTarget)
    {
        const int batch_size = m_batchSize * 0.9f;
        m_batchSize = qMax(m_minBatchSize, batch_size);
    }

    if(!d->m_abort && m_rehighlightProgress > 0 && m_rehighlightProgress < total_length)
    {
        QMetaObject::invokeMethod(this, &LSyntaxHighlighter::continueRehighlight, Qt::QueuedConnection);
    }
    else if(d->m_abort)
    {
        qDebug() << "rehighlight aborted";
        emit rehighlightFinished();
    }
    else
    {
        qDebug() << "rehighlight ended";
        d->rehighlightPending = false;
        emit rehighlightFinished();
    }
}

/*!
    \since 4.6

    Reapplies the highlighting to the given QTextBlock \a block.

    \sa rehighlight()
*/
void LSyntaxHighlighter::rehighlightBlock(const QTextBlock &block)
{
    if (!d->doc || !block.isValid() || block.document() != d->doc)
        return;

    const bool rehighlightPending = d->rehighlightPending;

    QTextCursor cursor(block);
    d->rehighlight(cursor, QTextCursor::EndOfBlock);

    if (rehighlightPending)
        d->rehighlightPending = rehighlightPending;
}

void LSyntaxHighlighter::abort(bool abort)
{
    d->m_abort = abort;
}

/*!
    \fn void LSyntaxHighlighter::highlightBlock(const QString &text)

    Highlights the given text block. This function is called when
    necessary by the rich text engine, i.e. on text blocks which have
    changed.

    To provide your own syntax highlighting, you must subclass
    LSyntaxHighlighter and reimplement highlightBlock(). In your
    reimplementation you should parse the block's \a text and call
    setFormat() as often as necessary to apply any font and color
    changes that you require. For example:

    \snippet code/src_gui_text_LSyntaxHighlighter.cpp 1

    See the \l{LSyntaxHighlighter multiblock}{Detailed Description} for
    examples of using setCurrentBlockState(), currentBlockState()
    and previousBlockState() to handle syntaxes with constructs that
    span several text blocks

    \sa previousBlockState(), setFormat(), setCurrentBlockState()
*/

/*!
    This function is applied to the syntax highlighter's current text
    block (i.e. the text that is passed to the highlightBlock()
    function).

    The specified \a format is applied to the text from the \a start
    position for a length of \a count characters (if \a count is 0,
    nothing is done). The formatting properties set in \a format are
    merged at display time with the formatting information stored
    directly in the document, for example as previously set with
    QTextCursor's functions. Note that the document itself remains
    unmodified by the format set through this function.

    \sa format(), highlightBlock()
*/
void LSyntaxHighlighter::setFormat(int start, int count, const QTextCharFormat &format)
{
    if (start < 0 || start >= d->formatChanges.size())
        return;

    const int end = qMin(start + count, d->formatChanges.size());
    for (int i = start; i < end; ++i)
        d->formatChanges[i] = format;
}

/*!
    \overload

    The specified \a color is applied to the current text block from
    the \a start position for a length of \a count characters.

    The other attributes of the current text block, e.g. the font and
    background color, are reset to default values.

    \sa format(), highlightBlock()
*/
void LSyntaxHighlighter::setFormat(int start, int count, const QColor &color)
{
    QTextCharFormat format;
    format.setForeground(color);
    setFormat(start, count, format);
}

/*!
    \overload

    The specified \a font is applied to the current text block from
    the \a start position for a length of \a count characters.

    The other attributes of the current text block, e.g. the font and
    background color, are reset to default values.

    \sa format(), highlightBlock()
*/
void LSyntaxHighlighter::setFormat(int start, int count, const QFont &font)
{
    QTextCharFormat format;
    format.setFont(font);
    setFormat(start, count, format);
}

/*!
    \fn QTextCharFormat LSyntaxHighlighter::format(int position) const

    Returns the format at \a position inside the syntax highlighter's
    current text block.
*/
QTextCharFormat LSyntaxHighlighter::format(int pos) const
{
    if (pos < 0 || pos >= d->formatChanges.size())
        return QTextCharFormat();
    return d->formatChanges.at(pos);
}

/*!
    Returns the end state of the text block previous to the
    syntax highlighter's current block. If no value was
    previously set, the returned value is -1.

    \sa highlightBlock(), setCurrentBlockState()
*/
int LSyntaxHighlighter::previousBlockState() const
{
    if (!d->currentBlock.isValid())
        return -1;

    const QTextBlock previous = d->currentBlock.previous();
    if (!previous.isValid())
        return -1;

    return previous.userState();
}

/*!
    Returns the state of the current text block. If no value is set,
    the returned value is -1.
*/
int LSyntaxHighlighter::currentBlockState() const
{
    if (!d->currentBlock.isValid())
        return -1;

    return d->currentBlock.userState();
}

/*!
    Sets the state of the current text block to \a newState.

    \sa highlightBlock()
*/
void LSyntaxHighlighter::setCurrentBlockState(int newState)
{
    if (!d->currentBlock.isValid())
        return;

    d->currentBlock.setUserState(newState);
}

/*!
    Attaches the given \a data to the current text block.  The
    ownership is passed to the underlying text document, i.e. the
    provided QTextBlockUserData object will be deleted if the
    corresponding text block gets deleted.

    QTextBlockUserData can be used to store custom settings. In the
    case of syntax highlighting, it is in particular interesting as
    cache storage for information that you may figure out while
    parsing the paragraph's text.

    For example while parsing the text, you can keep track of
    parenthesis characters that you encounter ('{[(' and the like),
    and store their relative position and the actual QChar in a simple
    class derived from QTextBlockUserData:

    \snippet code/src_gui_text_LSyntaxHighlighter.cpp 3

    During cursor navigation in the associated editor, you can ask the
    current QTextBlock (retrieved using the QTextCursor::block()
    function) if it has a user data object set and cast it to your \c
    BlockData object. Then you can check if the current cursor
    position matches with a previously recorded parenthesis position,
    and, depending on the type of parenthesis (opening or closing),
    find the next opening or closing parenthesis on the same level.

    In this way you can do a visual parenthesis matching and highlight
    from the current cursor position to the matching parenthesis. That
    makes it easier to spot a missing parenthesis in your code and to
    find where a corresponding opening/closing parenthesis is when
    editing parenthesis intensive code.

    \sa QTextBlock::setUserData()
*/
void LSyntaxHighlighter::setCurrentBlockUserData(QTextBlockUserData *data)
{
    if (!d->currentBlock.isValid())
        return;

    d->currentBlock.setUserData(data);
}

/*!
    Returns the QTextBlockUserData object previously attached to the
    current text block.

    \sa QTextBlock::userData(), setCurrentBlockUserData()
*/
QTextBlockUserData *LSyntaxHighlighter::currentBlockUserData() const
{
    if (!d->currentBlock.isValid())
        return nullptr;

    return d->currentBlock.userData();
}

/*!
    \since 4.4

    Returns the current text block.
*/
QTextBlock LSyntaxHighlighter::currentBlock() const
{
    return d->currentBlock;
}
