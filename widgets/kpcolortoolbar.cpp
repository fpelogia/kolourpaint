
/* This file is part of the KolourPaint project
   Copyright (c) 2003 Clarence Dang <dang@kde.org>
   All rights reserved.
   
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   3. Neither the names of the copyright holders nor the names of
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <qlayout.h>

#include <kcolorbutton.h>
#include <kdebug.h>

#include <kpcolortoolbar.h>
#include <kpdefs.h>

/*
 * kpDualColorButton
 */

kpDualColorButton::kpDualColorButton (QWidget *parent, const char *name)
    : QWidget (parent, name)
{
    QVBoxLayout *lay = new QVBoxLayout (this);

    for (int i = 0; i < 2; i++)
    {
        m_colorButton [i] = new KColorButton (this);
        lay->addWidget (m_colorButton [i]);
    }

    setForegroundColor (Qt::black);
    setBackgroundColor (Qt::white);
}

kpDualColorButton::~kpDualColorButton ()
{
}

QColor kpDualColorButton::color (int which) const
{
    if (which < 0 || which > 1)
    {
        kdWarning () << "kpDualColorButton::color (" << which
                            << ") - out of range" << endl;
        which = 0;
    }

    return m_colorButton [which]->color ();
}

void kpDualColorButton::setColor (int which, const QColor &color)
{
    if (which < 0 || which > 1)
    {
        kdWarning () << "kpDualColorButton::setColor (" << which
                            << ") - out of range" << endl;
        which = 0;
    }

    m_colorButton [which]->setColor (color);
}

QColor kpDualColorButton::foregroundColor () const
{
    return color (0);
}

void kpDualColorButton::setForegroundColor (const QColor &color)
{
    setColor (0, color);
}

QColor kpDualColorButton::backgroundColor () const
{
    return color (1);
}

void kpDualColorButton::setBackgroundColor (const QColor &color)
{
    setColor (1, color);
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
// (turquoise) get changed by QT
//

// primary colors + B&W
static const QColor kpRed (255, 0, 0);
static const QColor kpGreen (0, 255, 0);
static const QColor kpBlue (0, 0, 255);
static const QColor kpBlack (0, 0, 0);
static const QColor kpWhite (255, 255, 255);

// intentionally _not_ an HSV darkener
static QColor dark (const QColor &color)
{
    return blend (color, kpBlack);
}

// full-brightness colors
static const QColor kpYellow = add (kpRed, kpGreen);
static const QColor kpPurple = add (kpRed, kpBlue);
static const QColor kpAqua = add (kpGreen, kpBlue);

// mixed colors
static const QColor kpGrey = blend (kpBlack, kpWhite);
static const QColor kpLightGrey = blend (kpGrey, kpWhite);
static const QColor kpOrange = blend (kpRed, kpYellow);

// pastel colors
static const QColor kpPink = blend (kpRed, kpWhite);
static const QColor kpLightGreen = blend (kpGreen, kpWhite);
static const QColor kpLightBlue = blend (kpBlue, kpWhite);
static const QColor kpTan = blend (kpYellow, kpWhite);

#define rows 2
#define cols 11
kpColorCells::kpColorCells (QWidget *parent)
    : KColorCells (parent, rows, cols),
      m_mouseButton (-1)
{
    setFixedSize (cols * 26, rows * 26);
    KColorCells::setShading (false);  // no 3D look

    // don't let the user clobber the palette too easily
    KColorCells::setAcceptDrops (false);

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
         i < int (sizeof (colors) / sizeof (colors [0])) &&
         i < rows * cols;
         i++)
    {
        KColorCells::setColor (i, colors [i]);
    }

    connect (this, SIGNAL (colorDoubleClicked (int)),
             SLOT (slotColorDoubleClicked (int)));
}

kpColorCells::~kpColorCells ()
{
}

// virtual protected
void kpColorCells::paintCell (QPainter *painter, int row, int col)
{
    // no focus rect as it doesn't make sense
    // since 2 colors (foreground & background) can be selected
    KColorCells::selected = -1;
    KColorCells::paintCell (painter, row, col);
}

// virtual protected
void kpColorCells::mouseReleaseEvent (QMouseEvent *e)
{
    m_mouseButton = -1;

    Qt::ButtonState button = e->button ();
    kdDebug () << "kpColorCells::mouseReleaseEvent(left="
                      << (button & Qt::LeftButton)
                      << ",right="
                      << (button & Qt::RightButton)
                      << ")"
                      << endl;
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

    kdDebug () << "kpColorCells::mouseReleaseEvent() setting m_mouseButton back to -1" << endl;
    m_mouseButton = -1;
}

// protected slot
void kpColorCells::slotColorSelected (int cell)
{
    kdDebug () << "kpColorCells::slotColorSelected(cell=" << cell
                      << ") mouseButton = " << m_mouseButton << endl;
    QColor c = KColorCells::color (cell);

    if (m_mouseButton == 0)
        emit foregroundColorChanged (c);
    else if (m_mouseButton == 1)
        emit backgroundColorChanged (c);

    m_mouseButton = -1;  // just in case
}

// protected slot
void kpColorCells::slotColorDoubleClicked (int cell)
{
    kdDebug () << "kpColorCells::slotColorDoubleClicked(cell="
                      << cell << ")" << endl;

    QColor color = KColorCells::color (cell);

    if (KColorDialog::getColor (color/*ref*/))
        KColorCells::setColor (cell, color);
}


/*
 * kpColorToolBar
 */

kpColorToolBar::kpColorToolBar (QWidget *parent, const char *name)
    : KToolBar (parent, name)
{
    QWidget *base = new QWidget (this);
    QHBoxLayout *lay = new QHBoxLayout (base);

    m_dualColorButton = new kpDualColorButton (base);
    lay->addWidget (m_dualColorButton);

    m_colorCells = new kpColorCells (base);
    connect (m_colorCells, SIGNAL (foregroundColorChanged (const QColor &)),
             SLOT (setForegroundColor (const QColor &)));
    connect (m_colorCells, SIGNAL (backgroundColorChanged (const QColor &)),
             SLOT (setBackgroundColor (const QColor &)));
    lay->addWidget (m_colorCells);

    KToolBar::insertWidget (0, base->width (), base);
}

kpColorToolBar::~kpColorToolBar ()
{
}

QColor kpColorToolBar::color (int which) const
{
    if (which < 0 || which > 1)
    {
        kdWarning () << "kpColorToolBar::color (" << which
                            << ") - out of range" << endl;
        which = 0;
    }

    return m_dualColorButton->color (which);
}

void kpColorToolBar::setColor (int which, const QColor &color)
{
    if (which < 0 || which > 1)
    {
        kdWarning () << "kpColorToolBar::setColor (" << which
                            << ") - out of range" << endl;
        which = 0;
    }

    m_dualColorButton->setColor (which, color);
}

QColor kpColorToolBar::foregroundColor () const
{
    return m_dualColorButton->foregroundColor ();
}

void kpColorToolBar::setForegroundColor (const QColor &color)
{
    m_dualColorButton->setForegroundColor (color);
    emit foregroundColorChanged (color);
}

QColor kpColorToolBar::backgroundColor () const
{
    return m_dualColorButton->backgroundColor ();
}

void kpColorToolBar::setBackgroundColor (const QColor &color)
{
    m_dualColorButton->setBackgroundColor (color);
    emit backgroundColorChanged (color);
}

#include <kpcolortoolbar.moc>
