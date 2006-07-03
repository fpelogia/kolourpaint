
/*
   Copyright (c) 2003-2006 Clarence Dang <dang@kde.org>
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
   IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
   NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
   THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#define DEBUG_KP_COLOR_TOOL_BAR 0


#include <kpcolortoolbar.h>

#include <qbitmap.h>
#include <qboxlayout.h>
#include <qdrawutil.h>
#include <qevent.h>
#include <qlayout.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qsize.h>
#include <qtooltip.h>
#include <qwidget.h>

#include <kapplication.h>
#include <kcolordialog.h>
#include <k3colordrag.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>

#include <kpcolorsimilaritydialog.h>
#include <kpdefs.h>
#include <kpmainwindow.h>
#include <kppixmapfx.h>
#include <kptool.h>
#include <kpview.h>


/*
 * kpDualColorButton
 */

kpDualColorButton::kpDualColorButton (kpMainWindow *mainWindow,
                                      QWidget *parent)
    : QFrame (parent),
      m_mainWindow (mainWindow)
{
    setFrameStyle (QFrame::Panel | QFrame::Sunken);

    m_color [0] = kpColor (0, 0, 0);  // black
    m_color [1] = kpColor (255, 255, 255);  // white

    setAcceptDrops (true);
}

kpDualColorButton::~kpDualColorButton ()
{
}


kpColor kpDualColorButton::color (int which) const
{
    if (which < 0 || which > 1)
    {
        kWarning () << "kpDualColorButton::color (" << which
                     << ") - out of range" << endl;
        which = 0;
    }

    return m_color [which];
}

kpColor kpDualColorButton::foregroundColor () const
{
    return color (0);
}

kpColor kpDualColorButton::backgroundColor () const
{
    return color (1);
}


void kpDualColorButton::setColor (int which, const kpColor &color)
{
    if (which < 0 || which > 1)
    {
        kWarning () << "kpDualColorButton::setColor (" << which
                     << ") - out of range" << endl;
        which = 0;
    }

    if (m_color [which] == color)
        return;

    m_oldColor [which] = m_color [which];
    m_color [which] = color;
    update ();

    if (which == 0)
        emit foregroundColorChanged (color);
    else
        emit backgroundColorChanged (color);
}

void kpDualColorButton::setForegroundColor (const kpColor &color)
{
    setColor (0, color);
}

void kpDualColorButton::setBackgroundColor (const kpColor &color)
{
    setColor (1, color);
}


// public
kpColor kpDualColorButton::oldForegroundColor () const
{
    return m_oldColor [0];
}

// public
kpColor kpDualColorButton::oldBackgroundColor () const
{
    return m_oldColor [1];
}


// public virtual [base QWidget]
QSize kpDualColorButton::sizeHint () const
{
    return QSize (52, 52);
}


// protected
QRect kpDualColorButton::swapPixmapRect () const
{
    QPixmap swapPixmap = UserIcon ("colorbutton_swap_16x16");

    return QRect (contentsRect ().width () - swapPixmap.width (),
                  0,
                  swapPixmap.width (),
                  swapPixmap.height ());
}

// protected
QRect kpDualColorButton::foregroundBackgroundRect () const
{
    QRect cr (contentsRect ());
    return QRect (cr.width () / 8,
                  cr.height () / 8,
                  cr.width () * 6 / 8,
                  cr.height () * 6 / 8);
}

// protected
QRect kpDualColorButton::foregroundRect () const
{
    QRect fbr (foregroundBackgroundRect ());
    return QRect (fbr.x (),
                  fbr.y (),
                  fbr.width () * 3 / 4,
                  fbr.height () * 3 / 4);
}

// protected
QRect kpDualColorButton::backgroundRect () const
{
    QRect fbr (foregroundBackgroundRect ());
    return QRect (fbr.x () + fbr.width () / 4,
                  fbr.y () + fbr.height () / 4,
                  fbr.width () * 3 / 4,
                  fbr.height () * 3 / 4);
}


// TODO: drag a colour from this widget

// protected virtual [base QWidget]
void kpDualColorButton::dragMoveEvent (QDragMoveEvent *e)
{
    (void) e;
// COMPAT: linker errors???
#if 0
    e->accept ((foregroundRect ().contains (e->pos ()) ||
                backgroundRect ().contains (e->pos ())) &&
               K3ColorDrag::canDecode (e));
#endif
}

// protected virtual [base QWidget]
void kpDualColorButton::dropEvent (QDropEvent *e)
{
    (void) e;
// COMPAT: linker errors???
#if 0
    QColor col;
    K3ColorDrag::decode (e, col/*ref*/);

    if (col.isValid ())
    {
        if (foregroundRect ().contains (e->pos ()))
            setForegroundColor (kpColor (col.rgb ()));
        else if (backgroundRect ().contains (e->pos ()))
            setBackgroundColor (kpColor (col.rgb ()));
    }
#endif
}


// protected virtual [base QWidget]
void kpDualColorButton::mouseDoubleClickEvent (QMouseEvent *e)
{
    int whichColor = -1;

    if (foregroundRect ().contains (e->pos ()))
        whichColor = 0;
    else if (backgroundRect ().contains (e->pos ()))
        whichColor = 1;

    if (whichColor == 0 || whichColor == 1)
    {
        QColor col = Qt::black;
        if (color (whichColor).isOpaque ())
            col = color (whichColor).toQColor ();

        // TODO: parent
        if (KColorDialog::getColor (col/*ref*/))
            setColor (whichColor, kpColor (col.rgb ()));
    }
}

// protected virtual [base QWidget]
void kpDualColorButton::mouseReleaseEvent (QMouseEvent *e)
{
    if (swapPixmapRect ().contains (e->pos ()) &&
        m_color [0] != m_color [1])
    {
    #if DEBUG_KP_COLOR_TOOL_BAR && 1
        kDebug () << "kpDualColorButton::mouseReleaseEvent() swap colors:" << endl;
    #endif
        m_oldColor [0] = m_color [0];
        m_oldColor [1] = m_color [1];

        kpColor temp = m_color [0];
        m_color [0] = m_color [1];
        m_color [1] = temp;

        update ();

        emit colorsSwapped (m_color [0], m_color [1]);
        emit foregroundColorChanged (m_color [0]);
        emit backgroundColorChanged (m_color [1]);
    }
}


// protected virtual [base QWidget]
void kpDualColorButton::paintEvent (QPaintEvent *e)
{
#if DEBUG_KP_COLOR_TOOL_BAR && 1
    kDebug () << "kpDualColorButton::draw() rect=" << rect ()
               << " contentsRect=" << contentsRect ()
               << endl;
#endif

    // Draw frame first.
    QFrame::paintEvent (e);


    QPainter painter (this);

    // kpView::drawTransparentBackground() leaks outside contentsRect()
    // (and probably some other functions below too).
    painter.setClipRect (contentsRect ());

    // sync: painter translated to top-left of contentsRect().
    painter.translate (contentsRect ().x (), contentsRect ().y ());


    const QRect contentsRectMovedTo00 (0, 0,
        contentsRect ().width (), contentsRect ().height ());

    // Fill with background.
    if (isEnabled () && m_mainWindow)
    {
        // sync: ASSUMPTION: painter translated to top-left of contentsRect().
        kpView::drawTransparentBackground (&painter,
            contentsRect ().width (), contentsRect ().height (),
            contentsRectMovedTo00,
            true/*preview*/);
    }
    else
    {
        // TODO: Given we are no longer double buffering, is this even required?
        //       Remember to check everywhere else in KolourPaint.
        painter.fillRect (contentsRectMovedTo00,
            palette ().color (QPalette::Background));
    }


    // Draw "Swap Colours" button (top-right).
    QPixmap swapPixmap = UserIcon ("colorbutton_swap_16x16");
    if (!isEnabled ())
    {
        // Don't let the fill() touch the mask.
        QBitmap swapBitmapMask = swapPixmap.mask ();
        swapPixmap.setMask (QBitmap ());

        // Grey out the opaque parts of "swapPixmap".
        swapPixmap.fill (palette ().color (QPalette::Dark));

        swapPixmap.setMask (swapBitmapMask);
    }
    painter.drawPixmap (swapPixmapRect ().topLeft (), swapPixmap);


    // Draw background colour patch.
    QRect bgRect = backgroundRect ();
    QRect bgRectInside = QRect (bgRect.x () + 2, bgRect.y () + 2,
                                bgRect.width () - 4, bgRect.height () - 4);
    if (isEnabled ())
    {
    #if DEBUG_KP_COLOR_TOOL_BAR && 1
        kDebug () << "\tbackgroundColor=" << (int *) m_color [1].toQRgb ()
                   << endl;
    #endif
        if (m_color [1].isOpaque ())
            painter.fillRect (bgRectInside, m_color [1].toQColor ());
        else
            painter.drawPixmap (bgRectInside, UserIcon ("color_transparent_26x26"));
    }
    else
        painter.fillRect (bgRectInside, palette().color (QPalette::Button));
    qDrawShadePanel (&painter, bgRect, palette(),
                     false/*not sunken*/, 2/*lineWidth*/,
                     0/*never fill*/);


    // Draw foreground colour patch.
    // Must be drawn after background patch since we're on top.
    QRect fgRect = foregroundRect ();
    QRect fgRectInside = QRect (fgRect.x () + 2, fgRect.y () + 2,
                                fgRect.width () - 4, fgRect.height () - 4);
    if (isEnabled ())
    {
    #if DEBUG_KP_COLOR_TOOL_BAR && 1
        kDebug () << "\tforegroundColor=" << (int *) m_color [0].toQRgb ()
                   << endl;
    #endif
        if (m_color [0].isOpaque ())
            painter.fillRect (fgRectInside, m_color [0].toQColor ());
        else
            painter.drawPixmap (fgRectInside, UserIcon ("color_transparent_26x26"));
    }
    else
        painter.fillRect (fgRectInside, palette ().color (QPalette::Button));
    qDrawShadePanel (&painter, fgRect, palette (),
                     false/*not sunken*/, 2/*lineWidth*/,
                     0/*never fill*/);
}


/*
 * kpColorCells
 */

static inline int roundUp2 (int val)
{
    return val % 2 ? val + 1 : val;
}

static inline int btwn0_255 (int val)
{
    if (val < 0)
        return 0;
    else if (val > 255)
        return 255;
    else
        return val;
}

enum
{
    blendDark = 25,
    blendNormal = 50,
    blendLight = 75,
    blendAdd = 100
};

static QColor blend (const QColor &a, const QColor &b, int percent = blendNormal)
{
    return QColor (btwn0_255 (roundUp2 (a.red () + b.red ()) * percent / 100),
                   btwn0_255 (roundUp2 (a.green () + b.green ()) * percent / 100),
                   btwn0_255 (roundUp2 (a.blue () + b.blue ()) * percent / 100));
}

static QColor add (const QColor &a, const QColor &b)
{
    return blend (a, b, blendAdd);
}





//
// make our own colors in case weird ones like "Qt::cyan"
// (turquoise) get changed by Qt
//

// primary colors + B&W
static QColor kpRed;
static QColor kpGreen;
static QColor kpBlue;
static QColor kpBlack;
static QColor kpWhite;

// intentionally _not_ an HSV darkener
static QColor dark (const QColor &color)
{
    return blend (color, kpBlack);
}

// full-brightness colors
static QColor kpYellow;
static QColor kpPurple;
static QColor kpAqua;

// mixed colors
static QColor kpGrey;
static QColor kpLightGrey;
static QColor kpOrange;

// pastel colors
static QColor kpPink;
static QColor kpLightGreen;
static QColor kpLightBlue;
static QColor kpTan;

static bool ownColorsInitialised = false;

/* TODO: clean up this code!!!
 *       (probably when adding palette load/save)
 */
#define rows 2
#define cols 11
kpColorCells::kpColorCells (QWidget *parent,
                            Qt::Orientation o)
    : KColorCells (parent, rows, cols),
      m_mouseButton (-1)
{
    setShading (false);  // no 3D look

    // Trap KColorDrag so that kpMainWindow does not trap it.
    // See our impl of dropEvent().
    setAcceptDrops (true);
    setAcceptDrags (true);

    connect (this, SIGNAL (colorDoubleClicked (int)),
             SLOT (slotColorDoubleClicked (int)));

    if (!ownColorsInitialised)
    {
        // Don't initialise globally when we probably don't have a colour
        // allocation context.  This way, the colours aren't sometimes
        // invalid (e.g. at 8-bit).

        kpRed = QColor (255, 0, 0);
        kpGreen = QColor (0, 255, 0);
        kpBlue = QColor (0, 0, 255);
        kpBlack = QColor (0, 0, 0);
        kpWhite = QColor (255, 255, 255);

        kpYellow = add (kpRed, kpGreen);
        kpPurple = add (kpRed, kpBlue);
        kpAqua = add (kpGreen, kpBlue);

        kpGrey = blend (kpBlack, kpWhite);
        kpLightGrey = blend (kpGrey, kpWhite);
        kpOrange = blend (kpRed, kpYellow);

        kpPink = blend (kpRed, kpWhite);
        kpLightGreen = blend (kpGreen, kpWhite);
        kpLightBlue = blend (kpBlue, kpWhite);
        kpTan = blend (kpYellow, kpWhite);

        ownColorsInitialised = true;
    }

    setOrientation (o);
}

kpColorCells::~kpColorCells ()
{
}

Qt::Orientation kpColorCells::orientation () const
{
    return m_orientation;
}

void kpColorCells::setOrientation (Qt::Orientation o)
{
    int c, r;

    if (o == Qt::Horizontal)
    {
        c = cols;
        r = rows;
    }
    else
    {
        c = rows;
        r = cols;
    }

#if DEBUG_KP_COLOR_TOOL_BAR
    kDebug () << "kpColorCells::setOrientation(): r=" << r << " c=" << c << endl;
#endif

    setNumRows (r);
    setNumCols (c);

    setCellWidth (26);
    setCellHeight (26);

    setFixedSize (numCols () * cellWidth () + frameWidth () * 2,
                  numRows () * cellHeight () + frameWidth () * 2);

/*
    kDebug () << "\tlimits: array=" << sizeof (colors) / sizeof (colors [0])
               << " r*c=" << r * c << endl;
    kDebug () << "\tsizeof (colors)=" << sizeof (colors)
               << " sizeof (colors [0])=" << sizeof (colors [0])
               << endl;*/
    QColor colors [] =
    {
        kpBlack,
        kpGrey,
        kpRed,
        kpOrange,
        kpYellow,
        kpGreen,
        kpAqua,
        kpBlue,
        kpPurple,
        kpPink,
        kpLightGreen,

        kpWhite,
        kpLightGrey,
        dark (kpRed),
        dark (kpOrange)/*brown*/,
        dark (kpYellow),
        dark (kpGreen),
        dark (kpAqua),
        dark (kpBlue),
        dark (kpPurple),
        kpLightBlue,
        kpTan
    };

    for (int i = 0;
         /*i < int (sizeof (colors) / sizeof (colors [0])) &&*/
         i < r * c;
         i++)
    {
        int y, x;
        int pos;

        if (o == Qt::Horizontal)
        {
            y = i / cols;
            x = i % cols;
            pos = i;
        }
        else
        {
            y = i % cols;
            x = i / cols;
            // int x = rows - 1 - i / cols;
            pos = y * rows + x;
        }

        KColorCells::setColor (pos, colors [i]);
        //this->setToolTip( cellGeometry (y, x), colors [i].name ());
    }

    m_orientation = o;
}

// virtual protected [base KColorCells]
void kpColorCells::dropEvent (QDropEvent *e)
{
    // Eat event so that:
    //
    // 1. User doesn't clobber the palette (until we support reconfigurable
    //    palettes)
    // 2. kpMainWindow::dropEvent() doesn't try to paste colour code as text
    //    (when the user slips and drags colour cell a little instead of clicking)
    e->accept ();
}

// virtual protected
void kpColorCells::paintCell (QPainter *painter, int row, int col)
{
    QColor oldColor;
    int cellNo = -1;

    if (!isEnabled ())
    {
        cellNo = row * numCols () + col;

        // make all cells 3D (so that disabled palette doesn't look flat)
        setShading (true);

        oldColor = KColorCells::color (cellNo);
        KColorCells::colors [cellNo] = backgroundColor ();
    }


    // no focus rect as it doesn't make sense
    // since 2 colors (foreground & background) can be selected
    KColorCells::selected = -1;
    KColorCells::paintCell (painter, row, col);


    if (!isEnabled ())
    {
        KColorCells::colors [cellNo] = oldColor;
        setShading (false);
    }
}

// protected virtual [base QWidget]
void kpColorCells::contextMenuEvent (QContextMenuEvent *e)
{
    // Eat right-mouse press to prevent it from getting to the toolbar.
    e->accept ();
}

// virtual protected
void kpColorCells::mouseReleaseEvent (QMouseEvent *e)
{
    m_mouseButton = -1;

    Qt::ButtonState button = e->button ();
#if DEBUG_KP_COLOR_TOOL_BAR
    kDebug () << "kpColorCells::mouseReleaseEvent(left="
               << (button & Qt::LeftButton)
               << ",right="
               << (button & Qt::RightButton)
               << ")"
               << endl;
#endif
    if (!((button & Qt::LeftButton) && (button & Qt::RightButton)))
    {
        if (button & Qt::LeftButton)
            m_mouseButton = 0;
        else if (button & Qt::RightButton)
            m_mouseButton = 1;
    }

    connect (this, SIGNAL (colorSelected (int)), this, SLOT (slotColorSelected (int)));
    KColorCells::mouseReleaseEvent (e);
    disconnect (this, SIGNAL (colorSelected (int)), this, SLOT (slotColorSelected (int)));

#if DEBUG_KP_COLOR_TOOL_BAR
    kDebug () << "kpColorCells::mouseReleaseEvent() setting m_mouseButton back to -1" << endl;
#endif
    m_mouseButton = -1;
}

// protected virtual [base KColorCells]
void kpColorCells::resizeEvent (QResizeEvent *e)
{
    // KColorCells::resizeEvent() tries to adjust the cellWidth and cellHeight
    // to the current dimensions but doesn't take into account
    // frame{Width,Height}().
    //
    // In any case, we already set the cell{Width,Height} and a fixed
    // widget size and don't want any of it changed.  Eat the resize event.
    (void) e;
}

// protected slot
void kpColorCells::slotColorSelected (int cell)
{
#if DEBUG_KP_COLOR_TOOL_BAR
    kDebug () << "kpColorCells::slotColorSelected(cell=" << cell
               << ") mouseButton = " << m_mouseButton << endl;
#endif
    QColor c = KColorCells::color (cell);

    if (m_mouseButton == 0)
    {
        emit foregroundColorChanged (c);
        emit foregroundColorChanged (kpColor (c.rgb ()));
    }
    else if (m_mouseButton == 1)
    {
        emit backgroundColorChanged (c);
        emit backgroundColorChanged (kpColor (c.rgb ()));
    }

    m_mouseButton = -1;  // just in case
}

// protected slot
void kpColorCells::slotColorDoubleClicked (int cell)
{
#if DEBUG_KP_COLOR_TOOL_BAR
    kDebug () << "kpColorCells::slotColorDoubleClicked(cell="
               << cell << ")" << endl;
#endif

    QColor color = KColorCells::color (cell);

    // TODO: parent
    if (KColorDialog::getColor (color/*ref*/))
        KColorCells::setColor (cell, color);
}


/*
 * kpTransparentColorCell
 */

kpTransparentColorCell::kpTransparentColorCell (QWidget *parent)
    : QFrame (parent)
{
#if DEBUG_KP_COLOR_TOOL_BAR
    kDebug () << "kpTransparentColorCell::kpTransparentColorCell()" << endl;
#endif

    setFrameStyle (QFrame::Panel | QFrame::Sunken);
#if DEBUG_KP_COLOR_TOOL_BAR && 0
    kDebug () << "\tdefault line width=" << lineWidth ()
               << " frame width=" << frameWidth () << endl;
#endif
    //setLineWidth (2);
#if DEBUG_KP_COLOR_TOOL_BAR && 0
    kDebug () << "\tline width=" << lineWidth ()
               << " frame width=" << frameWidth () << endl;
#endif

    m_pixmap = UserIcon ("color_transparent_26x26");

    this->setToolTip( i18n ("Transparent"));
}

kpTransparentColorCell::~kpTransparentColorCell ()
{
}


// public virtual [base QWidget]
QSize kpTransparentColorCell::sizeHint () const
{
    return QSize (m_pixmap.width () + frameWidth () * 2,
                  m_pixmap.height () + frameWidth () * 2);
}

// protected virtual [base QWidget]
void kpTransparentColorCell::mousePressEvent (QMouseEvent * /*e*/)
{
    // Eat press so that we own the mouseReleaseEvent().
    // [http://blogs.qtdeveloper.net/archives/2006/05/27/mouse-event-propagation/]
    //
    // However, contrary to that blog, it doesn't seem to be needed?
}

// protected virtual [base QWidget]
void kpTransparentColorCell::contextMenuEvent (QContextMenuEvent *e)
{
    // Eat right-mouse press to prevent it from getting to the toolbar.
    e->accept ();
}

// protected virtual [base QWidget]
void kpTransparentColorCell::mouseReleaseEvent (QMouseEvent *e)
{
    if (rect ().contains (e->pos ()))
    {
        if (e->button () == Qt::LeftButton)
        {
            emit transparentColorSelected (0);
            emit foregroundColorChanged (kpColor::transparent);
        }
        else if (e->button () == Qt::RightButton)
        {
            emit transparentColorSelected (1);
            emit backgroundColorChanged (kpColor::transparent);
        }
    }
}

// protected virtual [base QWidget]
void kpTransparentColorCell::paintEvent (QPaintEvent *e)
{
    // Draw frame first.
    QFrame::paintEvent (e);

    if (isEnabled ())
    {
    #if DEBUG_KP_COLOR_TOOL_BAR
        kDebug () << "kpTransparentColorCell::paintEvent() contentsRect="
                   << contentsRect ()
                   << endl;
    #endif
        QPainter p (this);
        p.drawPixmap (contentsRect (), m_pixmap);
    }
}


/*
 * kpColorPalette
 */

kpColorPalette::kpColorPalette (QWidget *parent,
                                Qt::Orientation o)
    : QWidget (parent),
      m_boxLayout (0)
{
#if DEBUG_KP_COLOR_TOOL_BAR
    kDebug () << "kpColorPalette::kpColorPalette()" << endl;
#endif

    m_transparentColorCell = new kpTransparentColorCell (this);
    m_transparentColorCell->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect (m_transparentColorCell, SIGNAL (foregroundColorChanged (const kpColor &)),
             this, SIGNAL (foregroundColorChanged (const kpColor &)));
    connect (m_transparentColorCell, SIGNAL (backgroundColorChanged (const kpColor &)),
             this, SIGNAL (backgroundColorChanged (const kpColor &)));

    m_colorCells = new kpColorCells (this);
    connect (m_colorCells, SIGNAL (foregroundColorChanged (const kpColor &)),
             this, SIGNAL (foregroundColorChanged (const kpColor &)));
    connect (m_colorCells, SIGNAL (backgroundColorChanged (const kpColor &)),
             this, SIGNAL (backgroundColorChanged (const kpColor &)));

    setOrientation (o);
}

kpColorPalette::~kpColorPalette ()
{
}

// public
Qt::Orientation kpColorPalette::orientation () const
{
    return m_orientation;
}

void kpColorPalette::setOrientation (Qt::Orientation o)
{
    m_colorCells->setOrientation (o);

    delete m_boxLayout;

    if (o == Qt::Horizontal)
    {
        m_boxLayout = new QBoxLayout (QBoxLayout::LeftToRight, this );
        m_boxLayout->addWidget (m_transparentColorCell, 0/*stretch*/, Qt::AlignVCenter);
        m_boxLayout->addWidget (m_colorCells);
    }
    else
    {
        m_boxLayout = new QBoxLayout (QBoxLayout::TopToBottom, this);
        m_boxLayout->addWidget (m_transparentColorCell, 0/*stretch*/, Qt::AlignHCenter);
        m_boxLayout->addWidget (m_colorCells);
    }
    m_boxLayout->setSpacing( 5 );

    m_orientation = o;
}


/*
 * kpColorSimilarityToolBarItem
 */

kpColorSimilarityToolBarItem::kpColorSimilarityToolBarItem (kpMainWindow *mainWindow,
                                                            QWidget *parent)
    : kpColorSimilarityCube (kpColorSimilarityCube::Depressed |
                             kpColorSimilarityCube::DoubleClickInstructions,
                             mainWindow, parent),
      m_mainWindow (mainWindow),
      m_processedColorSimilarity (kpColor::Exact)
{
    setColorSimilarity (mainWindow->configColorSimilarity ());
}

kpColorSimilarityToolBarItem::~kpColorSimilarityToolBarItem ()
{
}


// public
int kpColorSimilarityToolBarItem::processedColorSimilarity () const
{
    return m_processedColorSimilarity;
}


// public slot
void kpColorSimilarityToolBarItem::setColorSimilarity (double similarity)
{
    m_oldColorSimilarity = colorSimilarity ();

    kpColorSimilarityCube::setColorSimilarity (similarity);
    if (similarity > 0)
        this->setToolTip( i18n ("Color similarity: %1%", qRound (similarity * 100)));
    else
        this->setToolTip( i18n ("Color similarity: Exact"));

    m_processedColorSimilarity = kpColor::processSimilarity (colorSimilarity ());

    m_mainWindow->configSetColorSimilarity (colorSimilarity ());

    emit colorSimilarityChanged (colorSimilarity (), m_processedColorSimilarity);
}

// public
double kpColorSimilarityToolBarItem::oldColorSimilarity () const
{
    return m_oldColorSimilarity;
}


// private virtual [base QWidget]
void kpColorSimilarityToolBarItem::mouseDoubleClickEvent (QMouseEvent * /*e*/)
{
    kpColorSimilarityDialog dialog (m_mainWindow, this);
    dialog.setColorSimilarity (colorSimilarity ());
    if (dialog.exec ())
    {
        setColorSimilarity (dialog.colorSimilarity ());
    }
}


/*
 * kpColorToolBar
 */

kpColorToolBar::kpColorToolBar (const QString &label, kpMainWindow *mainWindow)
    : KToolBar (mainWindow),
      m_mainWindow (mainWindow)
{
    setWindowTitle (label);


    QWidget *base = new QWidget (this);
    m_boxLayout = new QBoxLayout (QBoxLayout::LeftToRight, base );
    m_boxLayout->setMargin( 5 );
    m_boxLayout->setSpacing( 10 * 4 );

    m_dualColorButton = new kpDualColorButton (mainWindow, base);
    m_dualColorButton->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect (m_dualColorButton, SIGNAL (colorsSwapped (const kpColor &, const kpColor &)),
             this, SIGNAL (colorsSwapped (const kpColor &, const kpColor &)));
    connect (m_dualColorButton, SIGNAL (foregroundColorChanged (const kpColor &)),
             this, SIGNAL (foregroundColorChanged (const kpColor &)));
    connect (m_dualColorButton, SIGNAL (backgroundColorChanged (const kpColor &)),
             this, SIGNAL (backgroundColorChanged (const kpColor &)));
    m_boxLayout->addWidget (m_dualColorButton, 0/*stretch*/);

    m_colorPalette = new kpColorPalette (base);
    connect (m_colorPalette, SIGNAL (foregroundColorChanged (const kpColor &)),
             m_dualColorButton, SLOT (setForegroundColor (const kpColor &)));
    connect (m_colorPalette, SIGNAL (backgroundColorChanged (const kpColor &)),
             m_dualColorButton, SLOT (setBackgroundColor (const kpColor &)));
    m_boxLayout->addWidget (m_colorPalette, 0/*stretch*/);

    m_colorSimilarityToolBarItem = new kpColorSimilarityToolBarItem (mainWindow, base);
    m_colorSimilarityToolBarItem->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect (m_colorSimilarityToolBarItem, SIGNAL (colorSimilarityChanged (double, int)),
             this, SIGNAL (colorSimilarityChanged (double, int)));
    m_boxLayout->addWidget (m_colorSimilarityToolBarItem, 0/*stretch*/);

    // HACK: couldn't get QSpacerItem to work
    QWidget *fakeSpacer = new QWidget (base);
    m_boxLayout->addWidget (fakeSpacer, 1/*stretch*/);

    m_lastDockedOrientationSet = false;
    setOrientation (orientation ());

    addWidget (base);
}

// virtual
void kpColorToolBar::setOrientation (Qt::Orientation o)
{
    // (QDockWindow::undock() calls us)
    bool isOutsideDock =  false; //(place () == Q3DockWindow::OutsideDock);

    if (!m_lastDockedOrientationSet || !isOutsideDock)
    {
        m_lastDockedOrientation = o;
        m_lastDockedOrientationSet = true;
    }

    if (isOutsideDock)
    {
        //kDebug () << "\toutside dock, forcing orientation to last" << endl;
        o = m_lastDockedOrientation;
    }

    if (o == Qt::Horizontal)
    {
        m_boxLayout->setDirection (QBoxLayout::LeftToRight);
    }
    else
    {
        m_boxLayout->setDirection (QBoxLayout::TopToBottom);
    }

    m_colorPalette->setOrientation (o);

    KToolBar::setOrientation (o);
}

kpColorToolBar::~kpColorToolBar ()
{
}

kpColor kpColorToolBar::color (int which) const
{
    if (which < 0 || which > 1)
    {
        kWarning () << "kpColorToolBar::color (" << which
                     << ") - out of range" << endl;
        which = 0;
    }

    return m_dualColorButton->color (which);
}

void kpColorToolBar::setColor (int which, const kpColor &color)
{
    if (which < 0 || which > 1)
    {
        kWarning () << "kpColorToolBar::setColor (" << which
                     << ") - out of range" << endl;
        which = 0;
    }

    m_dualColorButton->setColor (which, color);
}

kpColor kpColorToolBar::foregroundColor () const
{
    return m_dualColorButton->foregroundColor ();
}

void kpColorToolBar::setForegroundColor (const kpColor &color)
{
    m_dualColorButton->setForegroundColor (color);
}

kpColor kpColorToolBar::backgroundColor () const
{
    return m_dualColorButton->backgroundColor ();
}

void kpColorToolBar::setBackgroundColor (const kpColor &color)
{
    m_dualColorButton->setBackgroundColor (color);
}


kpColor kpColorToolBar::oldForegroundColor () const
{
    return m_dualColorButton->oldForegroundColor ();
}

kpColor kpColorToolBar::oldBackgroundColor () const
{
    return m_dualColorButton->oldBackgroundColor ();
}

double kpColorToolBar::oldColorSimilarity () const
{
    return m_colorSimilarityToolBarItem->oldColorSimilarity ();
}


double kpColorToolBar::colorSimilarity () const
{
    return m_colorSimilarityToolBarItem->colorSimilarity ();
}

void kpColorToolBar::setColorSimilarity (double similarity)
{
    m_colorSimilarityToolBarItem->setColorSimilarity (similarity);
}

int kpColorToolBar::processedColorSimilarity () const
{
    return m_colorSimilarityToolBarItem->processedColorSimilarity ();
}


#include <kpcolortoolbar.moc>
