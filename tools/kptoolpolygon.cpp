
/*
   Copyright (c) 2003-2004 Clarence Dang <dang@kde.org>
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


#define DEBUG_KP_TOOL_POLYGON 1

#include <float.h>
#include <math.h>

#include <qbitmap.h>
#include <qcursor.h>
#include <qlayout.h>
#include <qpainter.h>
#include <qpoint.h>
#include <qpushbutton.h>
#include <qrect.h>
#include <qtooltip.h>
#include <qvbuttongroup.h>

#include <kdebug.h>
#include <klocale.h>

#include <kpdocument.h>
#include <kpdefs.h>
#include <kpmainwindow.h>
#include <kppixmapfx.h>
#include <kptoolpolygon.h>
#include <kptooltoolbar.h>
#include <kptoolwidgetlinewidth.h>
#include <kpviewmanager.h>


#if DEBUG_KP_TOOL_POLYGON
static const char *pointArrayToString (const QPointArray &pointArray)
{
    static char string [1000];
    string [0] = '\0';
    
    for (QPointArray::ConstIterator it = pointArray.begin ();
         it != pointArray.end ();
         it++)
    {
        QString ps = QString (" (%1, %2)").arg ((*it).x ()).arg ((*it).y ());
        const char *pss = ps.latin1 ();
        if (strlen (string) + strlen (pss) + 1 > sizeof (string) / sizeof (string [0]))
            break;
        strcat (string, pss);
    }
    
    return string;
}
#endif


static QPen makeMaskPen (const QColor &color, int lineWidth, Qt::PenStyle lineStyle)
{
    QColor maskPenColor;

    if (kpTool::isColorOpaque (color))
        maskPenColor = Qt::color1/*opaque*/;
    else
        maskPenColor = Qt::color0/*transparent*/;

    return QPen (maskPenColor,
                 lineWidth == 1 ? 0/*closer to looking width 1*/ : lineWidth, lineStyle,
                 Qt::RoundCap, Qt::RoundJoin);
}

static QPen makePen (const QColor &color, int lineWidth, Qt::PenStyle lineStyle)
{
    return QPen (color,
                 lineWidth == 1 ? 0/*closer to looking width 1*/ : lineWidth, lineStyle,
                 Qt::RoundCap, Qt::RoundJoin);
}

static QBrush makeMaskBrush (const QColor &foregroundColor,
                            const QColor &backgroundColor,
                            kpToolWidgetFillStyle *toolWidgetFillStyle)
{
    if (toolWidgetFillStyle)
        return toolWidgetFillStyle->maskBrush (foregroundColor, backgroundColor);
    else
        return Qt::NoBrush;
}

static QBrush makeBrush (const QColor &foregroundColor,
                         const QColor &backgroundColor,
                         kpToolWidgetFillStyle *toolWidgetFillStyle)
{
    if (toolWidgetFillStyle)
        return toolWidgetFillStyle->brush (foregroundColor, backgroundColor);
    else
        return QBrush (backgroundColor);
}

static QPixmap pixmap (const QPixmap &oldPixmap,
                       const QPointArray &points, const QRect &rect,
                       const QColor &foregroundColor, QColor backgroundColor,
                       int lineWidth, Qt::PenStyle lineStyle,
                       kpToolWidgetFillStyle *toolWidgetFillStyle,
                       enum kpToolPolygon::Mode mode, bool final = true)
{
    //
    // figure out points to draw relative to topLeft of oldPixmap

    QPointArray pointsInRect = points;
    pointsInRect.detach ();
    pointsInRect.translate (-rect.x (), -rect.y ());
    
#if DEBUG_KP_TOOL_POLYGON
    kdDebug () << "kptoolpolygon.cpp: pixmap(): points=" << pointArrayToString (points) << endl;
#endif


    //
    // draw

    QPen pen = makePen (foregroundColor, lineWidth, lineStyle),
         maskPen = makeMaskPen (foregroundColor, lineWidth, lineStyle);
    QBrush brush = makeBrush (foregroundColor, backgroundColor, toolWidgetFillStyle),
           maskBrush = makeMaskBrush (foregroundColor, backgroundColor, toolWidgetFillStyle);
    
    QPixmap pixmap = oldPixmap;
    QBitmap maskBitmap;
    
    QPainter painter, maskPainter;

    if (pixmap.mask () ||
        (maskPen.style () != Qt::NoPen &&
         kpTool::colorEq (maskPen.color (), Qt::color0/*transparent*/)) ||
        (maskBrush.style () != Qt::NoBrush &&
         kpTool::colorEq (maskBrush.color (), Qt::color0/*transparent*/)))
    {
        maskBitmap = kpPixmapFX::getNonNullMask (pixmap);
        maskPainter.begin (&maskBitmap);
        maskPainter.setPen (maskPen);
        maskPainter.setBrush (maskBrush);
    }
    
    if (pen.style () != Qt::NoPen ||
        brush.style () != Qt::NoBrush)
    {
        painter.begin (&pixmap);
        painter.setPen (pen);
        painter.setBrush (brush);
    }

#define PAINTER_CALL(cmd)         \
{                                 \
    if (painter.isActive ())      \
        painter . cmd ;           \
                                  \
    if (maskPainter.isActive ())  \
        maskPainter . cmd ;       \
}

    switch (mode)
    {
    case kpToolPolygon::Line:
    case kpToolPolygon::Polyline:
        PAINTER_CALL (drawPolyline (pointsInRect));
        break;
    case kpToolPolygon::Polygon:
        // TODO: why aren't the ends rounded?
        PAINTER_CALL (drawPolygon (pointsInRect));

        if (!final && 0/*HACK for TODO*/)
        {
            int count = pointsInRect.count ();
            
            if (count > 2)
            {
                if (painter.isActive ())
                {
                    QPen XORpen = painter.pen ();
                    XORpen.setColor (Qt::white);
                
                    painter.setPen (XORpen);
                    painter.setRasterOp (Qt::XorROP);
                }
                
                if (maskPainter.isActive ())
                {
                    QPen XORpen = maskPainter.pen ();
                    
                    // TODO???
                    #if 0
                    if (kpTool::isColorTransparent (foregroundColor))
                        XORpen.setColor (Qt::color1/*opaque*/);
                    else
                        XORpen.setColor (Qt::color0/*transparent*/);
                    #endif
                    
                    maskPainter.setPen (XORpen);
                }
                
                PAINTER_CALL (drawLine (pointsInRect [0], pointsInRect [count - 1]));
            }
        }
        break;
    case kpToolPolygon::Curve:
        int numPoints = pointsInRect.count ();
        QPointArray pa (4);
        
        pa [0] = pointsInRect [0];
        pa [3] = pointsInRect [1];
        
        switch (numPoints)
        {
        case 2:
            pa [1] = pointsInRect [0];
            pa [2] = pointsInRect [1];
            break;
        case 3:
            pa [1] = pa [2] = pointsInRect [2];
            break;
        case 4:
            pa [1] = pointsInRect [2];
            pa [2] = pointsInRect [3];
        }
        
        PAINTER_CALL (drawCubicBezier (pa));
    }
#undef PAINTER_CALL

    if (painter.isActive ())
        painter.end ();
    
    if (maskPainter.isActive ())
        maskPainter.end ();
    
    if (!maskBitmap.isNull ())
        pixmap.setMask (maskBitmap);

    return pixmap;
}


/*
 * kpToolPolygon
 */

kpToolPolygon::kpToolPolygon (kpMainWindow *mainWindow)
    : kpTool (i18n ("Polygon"), i18n ("Draws polygons"), mainWindow, "tool_polygon"),
      m_mode (Polygon),
      m_toolWidgetFillStyle (0),
      m_toolWidgetLineWidth (0)
{
}

kpToolPolygon::~kpToolPolygon ()
{
}

void kpToolPolygon::setMode (Mode m)
{
    m_mode = m;
}


// virtual
void kpToolPolygon::begin ()
{
    kpToolToolBar *tb = toolToolBar ();

    kdDebug () << "kpToolPolygon::begin() tb=" << tb << endl;

    if (tb)
    {
        if (m_mode == Polygon)
            m_toolWidgetFillStyle = tb->toolWidgetFillStyle ();
        else
            m_toolWidgetFillStyle = 0;

        m_toolWidgetLineWidth = tb->toolWidgetLineWidth ();

        if (m_toolWidgetFillStyle)
        {
            connect (m_toolWidgetFillStyle, SIGNAL (fillStyleChanged (kpToolWidgetFillStyle::FillStyle)),
                     this, SLOT (slotFillStyleChanged (kpToolWidgetFillStyle::FillStyle)));
        }
        connect (m_toolWidgetLineWidth, SIGNAL (lineWidthChanged (int)),
                 this, SLOT (slotLineWidthChanged (int)));

        if (m_toolWidgetFillStyle)
            m_toolWidgetFillStyle->show ();
        m_toolWidgetLineWidth->show ();

        m_lineWidth = m_toolWidgetLineWidth->lineWidth ();
    }
    else
    {
        m_toolWidgetFillStyle = 0;
        m_toolWidgetLineWidth = 0;

        m_lineWidth = 1;
    }

    viewManager ()->setCursor (QCursor (CrossCursor));
    
    m_originatingMouseButton = -1;
}

// virtual
void kpToolPolygon::end ()
{
    endShape ();

    if (m_toolWidgetFillStyle)
    {
        disconnect (m_toolWidgetFillStyle, SIGNAL (fillStyleChanged (kpToolWidgetFillStyle::FillStyle)),
                    this, SLOT (slotFillStyleChanged (kpToolWidgetFillStyle::FillStyle)));
        m_toolWidgetFillStyle = 0;
    }
    
    if (m_toolWidgetLineWidth)
    {
        disconnect (m_toolWidgetLineWidth, SIGNAL (lineWidthChanged (int)),
                    this, SLOT (slotLineWidthChanged (int)));
        m_toolWidgetLineWidth = 0;
    }

    viewManager ()->unsetCursor ();
}


void kpToolPolygon::beginDraw ()
{
#if DEBUG_KP_TOOL_POLYGON
    kdDebug () << "kpToolPolygon::beginDraw()  m_points=" << pointArrayToString (m_points)
               << ", startPoint=" << m_startPoint << endl;
#endif

    // starting with a line...    
    if (m_points.count () == 0)
    {
        m_originatingMouseButton = m_mouseButton;
        m_points.putPoints (m_points.count (), 2,
                            m_startPoint.x (), m_startPoint.y (),
                            m_startPoint.x (), m_startPoint.y ());
    }
    // continuing poly*
    else
    {
        if (m_mouseButton != m_originatingMouseButton)
        {
            m_mouseButton = m_originatingMouseButton;
            endShape ();
        }
        else
        {
            int count = m_points.count ();
            m_points.putPoints (count, 1,
                                m_startPoint.x (), m_startPoint.y ());

            // start point = last end point;
            // _not_ the new/current start point
            // (which is disregarded in a poly* as only the end points count
            //  after the initial line)
            //
            // Curve Tool ignores m_startPoint (doesn't call applyModifiers())
            // after the initial has been defined.
            m_startPoint = m_points [count - 1];
        }
    }
                        
#if DEBUG_KP_TOOL_POLYGON
    kdDebug () << "\tafterwards, m_points=" << pointArrayToString (m_points) << endl;
#endif
}

// private
void kpToolPolygon::applyModifiers ()
{
    int count = m_points.count ();

    m_toolLineStartPoint = m_startPoint;  /* also correct for poly* tool (see beginDraw()) */
    m_toolLineEndPoint = m_currentPoint;

    // angles
    if (m_shiftPressed || m_altPressed)
    {
        int diffx = m_toolLineEndPoint.x () - m_toolLineStartPoint.x ();
        int diffy = m_toolLineEndPoint.y () - m_toolLineStartPoint.y ();

        double ratio;
        if (fabs (diffx - 0) < KP_EPSILON)
            ratio = DBL_MAX;
        else
            ratio = fabs (double (diffy) / double (diffx));

        // Shift        = 0, 45, 90
        // Alt          = 0, 30, 60, 90
        // Shift + Alt  = 0, 30, 45, 60, 90
        double angles [10];  // "ought to be enough for anybody"
        int numAngles = 0;
        angles [numAngles++] = 0;
        if (m_altPressed)
            angles [numAngles++] = KP_PI / 6;
        if (m_shiftPressed)
            angles [numAngles++] = KP_PI / 4;
        if (m_altPressed)
            angles [numAngles++] = KP_PI / 3;
        angles [numAngles++] = KP_PI / 2;

        double angle = angles [numAngles - 1];
        for (int i = 0; i < numAngles - 1; i++)
        {
            double acceptingRatio = tan ((angles [i] + angles [i + 1]) / 2.0);
            if (ratio < acceptingRatio)
            {
                angle = angles [i];
                break;
            }
        }

        // horizontal (dist from start !maintained)
        if (fabs (angle - 0) < KP_EPSILON)
            m_toolLineEndPoint = QPoint (m_toolLineEndPoint.x (), m_toolLineStartPoint.y ());
        // vertical (dist from start !maintained)
        else if (fabs (angle - KP_PI / 2) < KP_EPSILON)
            m_toolLineEndPoint = QPoint (m_toolLineStartPoint.x (), m_toolLineEndPoint.y ());
        // diagonal (dist from start maintained)
        else
        {
            double dist = sqrt (diffx * diffx + diffy * diffy);

            #define sgn(a) ((a)<0?-1:1)
            int dx = int (m_toolLineStartPoint.x () + dist * cos (angle) * sgn (diffx));
            int dy = int (m_toolLineStartPoint.y () + dist * sin (angle) * sgn (diffy));
            #undef sgn

            m_toolLineEndPoint = QPoint (dx, dy);
        }
    }    // if (m_shiftPressed || m_altPressed) {

    // centring
    if (m_controlPressed)
    {
        // start = start - diff
        //       = start - (end - start)
        //       = start - end + start
        //       = 2 * start - end
        if (count == 2)
            m_toolLineStartPoint += (m_toolLineStartPoint - m_toolLineEndPoint);
        else
            m_toolLineEndPoint += (m_toolLineEndPoint - m_toolLineStartPoint);
    }    // if (m_controlPressed) {

    m_points [count - 2] = m_toolLineStartPoint;
    m_points [count - 1] = m_toolLineEndPoint;
    
    m_toolLineRect = kpTool::neededRect (QRect (m_toolLineStartPoint, m_toolLineEndPoint).normalize (),
                                         m_lineWidth);
}

// virtual
void kpToolPolygon::draw (const QPoint &, const QPoint &, const QRect &)
{
    if (m_points.count () == 0)
        return;
        
#if DEBUG_KP_TOOL_POLYGON
    kdDebug () << "kpToolPolygon::draw()  m_points=" << pointArrayToString (m_points)
               << ", endPoint=" << m_currentPoint << endl;
#endif    

    bool drawingALine = (m_mode != Curve) ||
                        (m_mode == Curve && m_points.count () == 2);
    
    if (drawingALine)
        applyModifiers ();
    else
        m_points [m_points.count () - 1] = m_currentPoint;

#if DEBUG_KP_TOOL_POLYGON
    kdDebug () << "\tafterwards, m_points=" << pointArrayToString (m_points) << endl;
#endif

    updateShape ();
    
    if (drawingALine)
        emit mouseDragged (QRect (m_toolLineStartPoint, m_toolLineEndPoint));
}

// private slot
void kpToolPolygon::updateShape ()
{
    if (m_points.count () == 0)
        return;

    QRect boundingRect = kpTool::neededRect (m_points.boundingRect (), m_lineWidth);
    
    QPixmap oldPixmap = document ()->getPixmapAt (boundingRect);
    QPixmap newPixmap = pixmap (oldPixmap,
                                m_points, boundingRect,
                                color (m_mouseButton), color (1 - m_mouseButton),
                                m_lineWidth, Qt::SolidLine,
                                m_toolWidgetFillStyle,
                                m_mode, false/*not final*/);

    viewManager ()->setFastUpdates ();
    viewManager ()->setTempPixmapAt (newPixmap, boundingRect.topLeft ());
    viewManager ()->restoreFastUpdates ();
}

// virtual
void kpToolPolygon::cancelShape ()
{
#if 0
    endDraw (QPoint (), QRect ());
    commandHistory ()->undo ();
#else
    viewManager ()->invalidateTempPixmap ();
#endif
    m_points.resize (0);
}

// virtual
void kpToolPolygon::endDraw (const QPoint &, const QRect &)
{
#if DEBUG_KP_TOOL_POLYGON
    kdDebug () << "kpToolPolygon::endDraw()  m_points=" << pointArrayToString (m_points) << endl;
#endif

    if (m_points.count () == 0)
        return;

    if (m_mode == Line ||
        (m_mode == Curve && m_points.count () >= 4) ||
        m_points.count () >= 50)
    {
        endShape ();
    }
}

// public virtual
void kpToolPolygon::endShape (const QPoint &, const QRect &)
{
#if DEBUG_KP_TOOL_POLYGON
    kdDebug () << "kpToolPolygon::endShape()  m_points=" << pointArrayToString (m_points) << endl;
#endif

    if (!hasBegunShape ())
        return;
        
    viewManager ()->invalidateTempPixmap (true);

    QRect boundingRect = kpTool::neededRect (m_points.boundingRect (), m_lineWidth);
    
    kpToolPolygonCommand *lineCommand = new kpToolPolygonCommand
        (viewManager (), document (),
            text (),
            m_points, boundingRect,
            color (m_mouseButton), color (1 - m_mouseButton),
            m_lineWidth, Qt::SolidLine,
            m_toolWidgetFillStyle,
            document ()->getPixmapAt (boundingRect),
            m_mode);

    commandHistory ()->addCommand (lineCommand);
    
    m_points.resize (0);
}

// public virtual
bool kpToolPolygon::hasBegunShape () const
{
    return (m_points.count () > 0);
}


// public slot
void kpToolPolygon::slotLineWidthChanged (int width)
{
    m_lineWidth = width;
    updateShape ();
}

// public slot
void kpToolPolygon::slotFillStyleChanged (kpToolWidgetFillStyle::FillStyle /*fillStyle*/)
{
    updateShape ();
}

// virtual protected slot
void kpToolPolygon::slotForegroundColorChanged (const QColor &)
{
    updateShape ();
}

// virtual protected slot
void kpToolPolygon::slotBackgroundColorChanged (const QColor &)
{
    updateShape ();
}


/*
 * kpToolPolygonCommand
 */

kpToolPolygonCommand::kpToolPolygonCommand (kpViewManager *viewManager, kpDocument *document,
                                            const QString &toolText,
                                            const QPointArray &points,
                                            const QRect &normalizedRect,
                                            const QColor &foregroundColor, const QColor &backgroundColor,
                                            int lineWidth, Qt::PenStyle lineStyle,
                                            kpToolWidgetFillStyle *toolWidgetFillStyle,
                                            const QPixmap &originalArea,
                                            enum kpToolPolygon::Mode mode)
    : m_viewManager (viewManager), m_document (document),
      m_name (toolText),
      m_points (points),
      m_normalizedRect (normalizedRect),
      m_foregroundColor (foregroundColor), m_backgroundColor (backgroundColor),
      m_lineWidth (lineWidth), m_lineStyle (lineStyle),
      m_toolWidgetFillStyle (toolWidgetFillStyle),
      m_originalArea (originalArea),
      m_mode (mode)
{
    m_points.detach ();
}

kpToolPolygonCommand::~kpToolPolygonCommand ()
{
}

void kpToolPolygonCommand::execute ()
{
    QPixmap p = pixmap (m_originalArea,
                        m_points, m_normalizedRect,
                        m_foregroundColor, m_backgroundColor,
                        m_lineWidth, m_lineStyle,
                        m_toolWidgetFillStyle,
                        m_mode);
    m_document->setPixmapAt (p, m_normalizedRect.topLeft ());
}

void kpToolPolygonCommand::unexecute ()
{
    m_document->setPixmapAt (m_originalArea, m_normalizedRect.topLeft ());
}

QString kpToolPolygonCommand::name () const
{
    return m_name;
}

#include <kptoolpolygon.moc>
