
/*
   Copyright (c) 2003-2007 Clarence Dang <dang@kde.org>
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


#define DEBUG_KP_PAINTER 0


#include <kpPainter.h>

#include <cstdio>

#include <QBitmap>
#include <QPainter>
#include <QPainterPath>
#include <QPolygon>

#include <kdebug.h>
#include <krandom.h>

#include <kpBug.h>
#include <kpImage.h>
#include <kpPixmapFX.h>
#include <kpTool.h>
#include <kpToolFlowBase.h>


// Returns a random integer from 0 to 99 inclusive.
static int RandomNumberFrom0to99 ()
{
    return (KRandom::random () % 100);
}

// public static
QList <QPoint> kpPainter::interpolatePoints (const QPoint &startPoint,
    const QPoint &endPoint,
    bool brushIsDiagonalLine,
    double probability)
{
    QList <QPoint> ret;

    Q_ASSERT (probability >= 0.0 && probability <= 1.0);
    const int probabilityTimes100 = int (probability * 100);
#define SHOULD_DRAW()  (probabilityTimes100 == 100/*avoid ::RandomNumberFrom0to99() call*/ ||  \
                        ::RandomNumberFrom0to99 () < probabilityTimes100)

#if 0
    kDebug () << "prob=" << probability
               << " *100=" << probabilityTimes100
               << endl;
#endif


    // Derived from the zSprite2 Graphics Engine.
    // "MODIFIED" comment shows deviation from zSprite2 and Bresenham's line
    // algorithm.

    const int x1 = startPoint.x (),
        y1 = startPoint.y (),
        x2 = endPoint.x (),
        y2 = endPoint.y ();

    // Difference of x and y values
    const int dx = x2 - x1;
    const int dy = y2 - y1;

    // Absolute values of differences
    const int ix = qAbs (dx);
    const int iy = qAbs (dy);

    // Larger of the x and y differences
    const int inc = ix > iy ? ix : iy;

    // Plot location
    int plotx = x1;
    int ploty = y1;

    int x = 0;
    int y = 0;

    if (SHOULD_DRAW ())
        ret.append (QPoint (plotx, ploty));


    for (int i = 0; i <= inc; i++)
    {
        // oldplotx is equally as valid but would look different
        // (but nobody will notice which one it is)
        const int oldploty = ploty;
        int plot = 0;

        x += ix;
        y += iy;

        if (x > inc)
        {
            plot++;
            x -= inc;

            if (dx < 0)
                plotx--;
            else
                plotx++;
        }

        if (y > inc)
        {
            plot++;
            y -= inc;

            if (dy < 0)
                ploty--;
            else
                ploty++;
        }

        if (plot)
        {
            if (brushIsDiagonalLine && plot == 2)
            {
                // MODIFIED: every point is
                // horizontally or vertically adjacent to another point (if there
                // is more than 1 point, of course).  This is in contrast to the
                // ordinary line algorithm which can create diagonal adjacencies.

                if (SHOULD_DRAW ())
                    ret.append (QPoint (plotx, oldploty));
            }

            if (SHOULD_DRAW ())
                ret.append (QPoint (plotx, ploty));
        }
    }

#undef SHOULD_DRAW

    return ret;
}


// public static
void kpPainter::drawLine (kpImage *image,
        int x1, int y1, int x2, int y2,
        const kpColor &color, int penWidth)
{
    kpPixmapFX::drawLine (image, x1, y1, x2, y2, color, penWidth);
}


// public static
void kpPainter::drawPolyline (kpImage *image,
        const QPolygon &points,
        const kpColor &color, int penWidth)
{
    kpPixmapFX::drawPolyline (image, points, color, penWidth);
}

// public static
void kpPainter::drawPolygon (kpImage *image,
        const QPolygon &points,
        const kpColor &fcolor, int penWidth,
        const kpColor &bcolor,
        bool isFinal)
{
    kpPixmapFX::drawPolygon (image, points, fcolor, penWidth, bcolor, isFinal);
}


// public static
void kpPainter::drawCurve (kpImage *image,
    const QPoint &startPoint,
    const QPoint &controlPointP, const QPoint &controlPointQ,
    const QPoint &endPoint,
    const kpColor &color, int penWidth)
{
    kpPixmapFX::drawCurve (image,
        startPoint, controlPointP, controlPointQ, endPoint, color, penWidth);
}


// public static
void kpPainter::fillRect (kpImage *image,
        int x, int y, int width, int height,
        const kpColor &color)
{
    kpPixmapFX::fillRect (image, x, y, width, height, color);
}


// public static
void kpPainter::drawRect (kpImage *image,
        int x, int y, int width, int height,
        const kpColor &fcolor, int penWidth,
        const kpColor &bcolor)
{
    kpPixmapFX::drawRect (image, x, y, width, height, fcolor, penWidth, bcolor);
}

// public static
void kpPainter::drawRoundedRect (kpImage *image,
        int x, int y, int width, int height,
        const kpColor &fcolor, int penWidth,
        const kpColor &bcolor)
{
    kpPixmapFX::drawRoundedRect (image, x, y, width, height, fcolor, penWidth, bcolor);
}

// public static
void kpPainter::drawEllipse (kpImage *image,
        int x, int y, int width, int height,
        const kpColor &fcolor, int penWidth,
        const kpColor &bcolor)
{
    kpPixmapFX::drawEllipse (image, x, y, width, height, fcolor, penWidth, bcolor);
}


// <rgbPainter> and <maskPainter> are operating on the original image
// (the original image is not passed to this function).
//
// <image> = subset of the original image containing all the pixels in
//           <imageRect>
// <drawRect> = the rectangle, relative to the painters, whose pixels we
//              want to change
static bool ReadableImageWashRect (QPainter *rgbPainter, QPainter *maskPainter,
        const QImage &image,
        const kpColor &colorToReplace,
        const QRect &imageRect, const QRect &drawRect,
        int processedColorSimilarity)
{
    bool didSomething = false;

#if DEBUG_KP_PAINTER && 0
    kDebug () << "kppixmapfx.cpp:WashRect(imageRect=" << imageRect
               << ",drawRect=" << drawRect
               << ")" << endl;
#endif

    // If you're going to pass painter pointers, those painters had better be
    // active (i.e. QPainter::begin() has been called).
    Q_ASSERT (!rgbPainter || rgbPainter->isActive ());
    Q_ASSERT (!maskPainter || maskPainter->isActive ());
    
// make use of scanline coherence
#define FLUSH_LINE()                                        \
{                                                           \
    if (rgbPainter)                                         \
        rgbPainter->drawLine (startDrawX + imageRect.x (),  \
            y + imageRect.y (),                             \
            x - 1 + imageRect.x (),                         \
            y + imageRect.y ());                            \
    if (maskPainter)                                        \
        maskPainter->drawLine (startDrawX + imageRect.x (), \
            y + imageRect.y (),                             \
            x - 1 + imageRect.x (),                         \
            y + imageRect.y ());                            \
    didSomething = true;                                    \
    startDrawX = -1;                                        \
}

    const int maxY = drawRect.bottom () - imageRect.top ();

    const int minX = drawRect.left () - imageRect.left ();
    const int maxX = drawRect.right () - imageRect.left ();

    for (int y = drawRect.top () - imageRect.top ();
         y <= maxY;
         y++)
    {
        int startDrawX = -1;

        int x;  // for FLUSH_LINE()
        for (x = minX; x <= maxX; x++)
        {
        #if DEBUG_KP_PAINTER && 0
            fprintf (stderr, "y=%i x=%i colorAtPixel=%08X colorToReplace=%08X ... ",
                     y, x,
                     kpPixmapFX::getColorAtPixel (image, QPoint (x, y)).toQRgb (),
                     colorToReplace.toQRgb ());
        #endif
            if (kpPixmapFX::getColorAtPixel (image, QPoint (x, y)).isSimilarTo (colorToReplace, processedColorSimilarity))
            {
            #if DEBUG_KP_PAINTER && 0
                fprintf (stderr, "similar\n");
            #endif
                if (startDrawX < 0)
                    startDrawX = x;
            }
            else
            {
            #if DEBUG_KP_PAINTER && 0
                fprintf (stderr, "different\n");
            #endif
                if (startDrawX >= 0)
                    FLUSH_LINE ();
            }
        }

        if (startDrawX >= 0)
            FLUSH_LINE ();
    }

#undef FLUSH_LINE

    return didSomething;
}


struct WashPack
{
    QPoint startPoint, endPoint;
    kpColor color;
    int penWidth, penHeight;
    kpColor colorToReplace;
    int processedColorSimilarity;
    
    QRect readableImageRect;
    QImage readableImage;
};


static QRect Wash (kpImage *image,
        const QPoint &startPoint, const QPoint &endPoint,
        const kpColor &color, int penWidth, int penHeight,
        const kpColor &colorToReplace,
        int processedColorSimilarity,
        QRect (*drawFunc) (QPainter * /*rgbPainter*/, QPainter * /*maskPainter*/,
            void * /*data*/))
{
    KP_PFX_CHECK_NO_ALPHA_CHANNEL (*image);
    
    WashPack pack;
    pack.startPoint = startPoint; pack.endPoint = endPoint;
    pack.color = color;
    pack.penWidth = penWidth; pack.penHeight = penHeight;
    pack.colorToReplace = colorToReplace;
    pack.processedColorSimilarity = processedColorSimilarity;


    // Get the rectangle that bounds the changes and the pixmap for that
    // rectangle.
    const QRect normalizedRect = kpBug::QRect_Normalized (
        QRect (pack.startPoint, pack.endPoint));
    pack.readableImageRect = kpTool::neededRect (normalizedRect,
        qMax (pack.penWidth, pack.penHeight));
#if DEBUG_KP_PAINTER
    kDebug () << "kppainter.cpp:Wash() startPoint=" << startPoint
              << " endPoint=" << endPoint
              << " --> normalizedRect=" << normalizedRect
              << " readableImageRect=" << pack.readableImageRect
              << endl;
#endif
    QPixmap pixmap = kpPixmapFX::getPixmapAt (*image, pack.readableImageRect);


    // Convert pixmap to QImage so that we can read off the pixels.
#if DEBUG_KP_PAINTER && 0
    timer.start ();
#endif
    pack.readableImage = kpPixmapFX::convertToImage (pixmap);
#if DEBUG_KP_PAINTER && 0
    convAndWashTime = timer.restart ();
    kDebug () << "\tconvert to image: " << convAndWashTime << " ms" << endl;
#endif


    const QRect ret = kpPixmapFX::draw (image, drawFunc,
        color.isOpaque (), color.isTransparent (),
        &pack);
    KP_PFX_CHECK_NO_ALPHA_CHANNEL (*image);
    return ret;
}

void WashHelperSetup (QPainter *rgbPainter, QPainter *maskPainter,
        const WashPack *pack)
{
    // Set the drawing colors for the painters.
    
    if (rgbPainter)
    {
        rgbPainter->setPen (
            kpPixmapFX::draw_ToQColor (pack->color,
                true/*drawing on RGB Layer*/));
    }

    if (maskPainter)
    {
        maskPainter->setPen (
            kpPixmapFX::draw_ToQColor (pack->color,
                false/*drawing on mask layer*/));
    }
}


static QRect WashLineHelper (QPainter *rgbPainter, QPainter *maskPainter,
        void *data)
{
#if DEBUG_KP_PAINTER && 0
    kDebug () << "Washing pixmap (w=" << rect.width ()
                << ",h=" << rect.height () << ")" << endl;
    QTime timer;
    int convAndWashTime;
#endif

    WashPack *pack = static_cast <WashPack *> (data);


    // Setup painters.
    ::WashHelperSetup (rgbPainter, maskPainter, pack);


    bool didSomething = false;

    QList <QPoint> points = kpPainter::interpolatePoints (pack->endPoint, pack->startPoint);
    for (QList <QPoint>::const_iterator pit = points.begin ();
            pit != points.end ();
            pit++)
    {
        if (::ReadableImageWashRect (rgbPainter, maskPainter,
                pack->readableImage,
                pack->colorToReplace,
                pack->readableImageRect,
                kpToolFlowBase::hotRectForMousePointAndBrushWidthHeight (
                    *pit, pack->penWidth, pack->penHeight),
                pack->processedColorSimilarity))
        {
            didSomething = true;
        }
    }


#if DEBUG_KP_PAINTER && 0
    int ms = timer.restart ();
    kDebug () << "\ttried to wash: " << ms << "ms"
                << " (" << (ms ? (rect.width () * rect.height () / ms) : -1234)
                << " pixels/ms)"
                << endl;
    convAndWashTime += ms;
#endif


    // TODO: Rectangle may be too big.  Use QRect::unite() incrementally?
    //       Efficiency?
    return didSomething ? pack->readableImageRect : QRect ();
}

// public static
QRect kpPainter::washLine (kpImage *image,
        int x1, int y1, int x2, int y2,
        const kpColor &color, int penWidth, int penHeight,
        const kpColor &colorToReplace,
        int processedColorSimilarity)
{
    return ::Wash (image,
        QPoint (x1, y1), QPoint (x2, y2),
        color, penWidth, penHeight,
        colorToReplace,
        processedColorSimilarity,
        &::WashLineHelper);
}


static QRect WashRectHelper (QPainter *rgbPainter, QPainter *maskPainter,
        void *data)
{
#if DEBUG_KP_PAINTER && 0
    kDebug () << "Washing pixmap (w=" << rect.width ()
                << ",h=" << rect.height () << ")" << endl;
    QTime timer;
    int convAndWashTime;
#endif

    WashPack *pack = static_cast <WashPack *> (data);


    // Setup painters.
    ::WashHelperSetup (rgbPainter, maskPainter, pack);


    const QRect drawRect (pack->startPoint, pack->endPoint);
    
    bool didSomething = false;

    if (::ReadableImageWashRect (rgbPainter, maskPainter,
            pack->readableImage,
            pack->colorToReplace,
            pack->readableImageRect,
            drawRect,
            pack->processedColorSimilarity))
    {
        didSomething = true;
    }


#if DEBUG_KP_PAINTER && 0
    int ms = timer.restart ();
    kDebug () << "\ttried to wash: " << ms << "ms"
                << " (" << (ms ? (rect.width () * rect.height () / ms) : -1234)
                << " pixels/ms)"
                << endl;
    convAndWashTime += ms;
#endif


    return didSomething ? drawRect : QRect ();
}

// public static
QRect kpPainter::washRect (kpImage *image,
        int x, int y, int width, int height,
        const kpColor &color,
        const kpColor &colorToReplace,
        int processedColorSimilarity)
{
    return ::Wash (image,
        QPoint (x, y), QPoint (x + width - 1, y + height - 1),
        color, 1/*pen width*/, 1/*pen height*/,
        colorToReplace,
        processedColorSimilarity,
        &::WashRectHelper);
}


struct SprayPointsPackage
{
    QList <QPoint> points;
    kpColor color;
    int spraycanSize;
};

static QRect SprayPointsHelper (QPainter *rgbPainter, QPainter *maskPainter,
        void *data)
{
    SprayPointsPackage *pack = static_cast <SprayPointsPackage *> (data);

#if DEBUG_KP_PAINTER
    kDebug () << "kppainter.cpp:SprayPointsHelper("
               << ") spraycanSize=" << pack->spraycanSize
               << endl;
#endif

    const int radius = pack->spraycanSize / 2;

    // Set the drawing colors for the painters.
    
    if (rgbPainter)
    {
        rgbPainter->setPen (
            kpPixmapFX::draw_ToQColor (pack->color,
                true/*drawing on RGB Layer*/));
    }

    if (maskPainter)
    {
        maskPainter->setPen (
            kpPixmapFX::draw_ToQColor (pack->color,
                false/*drawing on mask layer*/));
    }

    foreach (QPoint p, pack->points)
    {
        for (int i = 0; i < 10; i++)
        {
            const int dx = (KRandom::random () % pack->spraycanSize) - radius;
            const int dy = (KRandom::random () % pack->spraycanSize) - radius;
    
            // Make it look circular.
            // TODO: Can be done better by doing a random vector angle & length
            //       but would sin and cos be too slow?
            if ((dx * dx) + (dy * dy) > (radius * radius))
                continue;
    
            const QPoint p2 (p.x () + dx, p.y () + dy);
    
            if (rgbPainter)
                rgbPainter->drawPoint (p2);
    
            if (maskPainter)
                maskPainter->drawPoint (p2);
        }
    }

    // kpPainter::sprayPoints() ignores the return value of kpPixmapFX::draw(),
    // which is based on this return value.
    return QRect ();
}

// public static
void kpPainter::sprayPoints (kpImage *image,
        const QList <QPoint> &points,
        const kpColor &color,
        int spraycanSize)
{
#if DEBUG_KP_PAINTER
    kDebug () << "kpPainter::sprayPoints()" << endl;
#endif

    Q_ASSERT (spraycanSize > 0);
    
    SprayPointsPackage pack;
    pack.points = points;
    pack.color = color;
    pack.spraycanSize = spraycanSize;

    kpPixmapFX::draw (image, &::SprayPointsHelper,
        color.isOpaque (),
        color.isTransparent (),
        &pack);
}