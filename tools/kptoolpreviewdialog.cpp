
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

#define DEBUG_KP_TOOL_PREVIEW_DIALOG 1

#include <qlayout.h>
#include <qgroupbox.h>
#include <qlabel.h>

#include <kdebug.h>
#include <klocale.h>

#include <kpcolor.h>
#include <kpdocument.h>
#include <kpmainwindow.h>
#include <kppixmapfx.h>
#include <kpselection.h>
#include <kptoolpreviewdialog.h>


kpToolPreviewDialog::kpToolPreviewDialog (const QString &actionName,
                                          bool actOnSelection,
                                          kpMainWindow *parent,
                                          const char *name)
    : KDialogBase (parent, name, true/*modal*/,
                   QString::null,
                   KDialogBase::Ok | KDialogBase::Cancel),
      m_actionName (actionName),
      m_actOnSelection (actOnSelection),
      m_mainWindow (parent)

{
    setCaption (captionFromActionName ());


    QWidget *baseWidget = new QWidget (this);
    setMainWidget (baseWidget);


    if (document ())
    {
        m_oldWidth = document ()->width (actOnSelection);
        m_oldHeight = document ()->height (actOnSelection);
    }
    else
    {
        m_oldWidth = m_oldHeight = 1;
    }


    createDimensionsGroupBox ();
    createPreviewGroupBox ();


    m_gridLayout = new QGridLayout (baseWidget, 4, 2,
                                    marginHint (), spacingHint ());
    m_gridLayout->addWidget (m_dimensionsGroupBox, 0, 0);
    m_gridLayout->addWidget (m_previewGroupBox, 0, 1);

    m_gridLayout->setRowStretch (0, 1);
    m_gridLayout->setColStretch (1, 1);

    m_gridNumRows = 1;
}

kpToolPreviewDialog::~kpToolPreviewDialog ()
{
}


// private
QString kpToolPreviewDialog::captionFromActionName () const
{
    if (m_actOnSelection)
        return i18n ("%1 Selection").arg (m_actionName);
    else
        return i18n ("%1 Image").arg (m_actionName);
}


// private
void kpToolPreviewDialog::createDimensionsGroupBox ()
{
    m_dimensionsGroupBox = new QGroupBox (i18n ("Dimensions"), mainWidget ());

    QLabel *originalLabel = new QLabel (i18n ("Original:"), m_dimensionsGroupBox);
    QString originalDimensions;
    if (document ())
    {
         originalDimensions = i18n ("%1 x %2")
                                  .arg (m_oldWidth)
                                  .arg (m_oldHeight);

         // Stop the Dimensions Group Box from resizing so often
         const QString minimumLengthString ("100000 x 100000");
         const int padLength = minimumLengthString.length ();
         for (int i = originalDimensions.length (); i < padLength; i++)
             originalDimensions += " ";
    }
    QLabel *originalDimensionsLabel = new QLabel (originalDimensions, m_dimensionsGroupBox);

    QLabel *afterTransformLabel = new QLabel (i18n ("After %1:").arg (m_actionName),
                                              m_dimensionsGroupBox);
    m_afterTransformDimensionsLabel = new QLabel (m_dimensionsGroupBox);


    QGridLayout *dimensionsLayout = new QGridLayout (m_dimensionsGroupBox,
                                                     2, 2,
                                                     marginHint () * 2, spacingHint ());

    dimensionsLayout->addWidget (originalLabel, 0, 0, Qt::AlignBottom);
    dimensionsLayout->addWidget (originalDimensionsLabel, 0, 1, Qt::AlignBottom);
    dimensionsLayout->addWidget (afterTransformLabel, 1, 0, Qt::AlignTop);
    dimensionsLayout->addWidget (m_afterTransformDimensionsLabel, 1, 1, Qt::AlignTop);
}

// private
void kpToolPreviewDialog::createPreviewGroupBox ()
{
    m_previewGroupBox = new QGroupBox (i18n ("Preview"), mainWidget ());

    m_previewPixmapLabel = new QLabel (m_previewGroupBox);
    m_previewPixmapLabel->setMinimumSize (150, 110);


    QVBoxLayout *previewLayout = new QVBoxLayout (m_previewGroupBox,
                                                  marginHint () * 2, spacingHint ());

    previewLayout->addWidget (m_previewPixmapLabel, Qt::AlignCenter);
}


// protected
kpDocument *kpToolPreviewDialog::document () const
{
    return m_mainWindow ? m_mainWindow->document () : 0;
}

// protected
void kpToolPreviewDialog::addCustomWidget (QWidget *w)
{
    m_gridLayout->addMultiCellWidget (w, m_gridNumRows, m_gridNumRows, 0, 1);
    m_gridNumRows++;
}


// private
void kpToolPreviewDialog::updateDimensions ()
{
    kpDocument *doc = document ();
    if (!doc)
        return;

    QSize newDim = newDimensions ();
#if DEBUG_KP_TOOL_PREVIEW_DIALOG
    kdDebug () << "kpToolPreviewDialog::updateDimensions(): newDim=" << newDim << endl;
#endif

    QString newDimString = i18n ("%1 x %2")
                               .arg (newDim.width ())
                               .arg (newDim.height ());
    m_afterTransformDimensionsLabel->setText (newDimString);
}


// private
double kpToolPreviewDialog::aspectScale (int newWidth, int newHeight,
                                         int oldWidth, int oldHeight) const
{
    double widthScale = double (newWidth) / double (oldWidth);
    double heightScale = double (newHeight) / double (oldHeight);

    // Keeps aspect ratio
    return QMIN (widthScale, heightScale);
}

// private
int kpToolPreviewDialog::scaleDimension (int dimension, double scale, int min, int max) const
{
    return QMAX (min,
                 QMIN (max,
                       qRound (dimension * scale)));
}


// private
void kpToolPreviewDialog::updateShrukenDocumentPixmap ()
{
#if DEBUG_KP_TOOL_PREVIEW_DIALOG
    kdDebug () << "kpToolPreviewDialog::updateShrukenDocumentPixmap()"
               << " shrunkenDocPixmap.size="
               << m_shrunkenDocumentPixmap.size ()
               << " previewPixmapLabelSizeWhenUpdatedPixmap="
               << m_previewPixmapLabelSizeWhenUpdatedPixmap
               << " previewPixmapLabel.size="
               << m_previewPixmapLabel->size ()
               << endl;
#endif

    kpDocument *doc = document ();
    if (!doc || !doc->pixmap ())
    {
        kdError () << "kpToolPreviewDialog::updateShrunkenDocumentPixmap() doc="
                   << doc << endl;
        return;
    }

    if (m_shrunkenDocumentPixmap.isNull () ||
        m_previewPixmapLabel->size () != m_previewPixmapLabelSizeWhenUpdatedPixmap)
    {
    #if DEBUG_KP_TOOL_PREVIEW_DIALOG
        kdDebug () << "\tupdating shrunkenDocPixmap" << endl;
    #endif

        // TODO: Why the need to keep aspect ratio here?
        //       Isn't scaling the skewed result maintaining aspect enough?
        double keepsAspectScale = aspectScale (m_previewPixmapLabel->width (),
                                               m_previewPixmapLabel->height (),
                                               m_oldWidth,
                                               m_oldHeight);

        QPixmap pixmap;

        if (m_actOnSelection)
        {
            kpSelection sel = *doc->selection ();
            if (!sel.pixmap ())
                sel.setPixmap (doc->getSelectedPixmap ());

            pixmap = sel.transparentPixmap ();
        }
        else
        {
            pixmap = *doc->pixmap ();
        }

        m_shrunkenDocumentPixmap = kpPixmapFX::scale (
            pixmap,
            scaleDimension (m_oldWidth,
                            keepsAspectScale,
                            1, m_previewPixmapLabel->width ()),
            scaleDimension (m_oldHeight,
                            keepsAspectScale,
                            1, m_previewPixmapLabel->height ()));
    #if 0
        m_shrunkenDocumentPixmap = kpPixmapFX::scale (
            m_actOnSelection ? doc->getSelectedPixmap () : *doc->pixmap (),
            m_previewPixmapLabel->width (),
            m_previewPixmapLabel->height ());
    #endif

        m_previewPixmapLabelSizeWhenUpdatedPixmap = m_previewPixmapLabel->size ();
    }
}


// private
void kpToolPreviewDialog::updatePreview ()
{
#if DEBUG_KP_TOOL_PREVIEW_DIALOG
    kdDebug () << "kpToolPreviewDialog::updatePreview()" << endl;
#endif

    kpDocument *doc = document ();
    if (!doc)
        return;

    updateShrukenDocumentPixmap ();

    if (!m_shrunkenDocumentPixmap.isNull ())
    {
        QSize newDim = newDimensions ();
        double keepsAspectScale = aspectScale (m_previewPixmapLabel->width (),
                                               m_previewPixmapLabel->height (),
                                               newDim.width (),
                                               newDim.height ());

        int targetWidth = scaleDimension (newDim.width (),
                                          keepsAspectScale,
                                          1,  // min
                                          m_previewPixmapLabel->width ());  // max
        int targetHeight = scaleDimension (newDim.height (),
                                           keepsAspectScale,
                                           1,  // min
                                           m_previewPixmapLabel->height ());  // max

        QPixmap transformedShrunkenDocumentPixmap =
            transformPixmap (m_shrunkenDocumentPixmap, targetWidth, targetHeight);

        QPixmap previewPixmap (m_previewPixmapLabel->width (),
                               m_previewPixmapLabel->height ());
        kpPixmapFX::fill (&previewPixmap, kpColor::transparent);
        kpPixmapFX::setPixmapAt (&previewPixmap,
                                 (previewPixmap.width () - transformedShrunkenDocumentPixmap.width ()) / 2,
                                 (previewPixmap.height () - transformedShrunkenDocumentPixmap.height ()) / 2,
                                 transformedShrunkenDocumentPixmap);

#if DEBUG_KP_TOOL_PREVIEW_DIALOG
    kdDebug () << "kpToolPreviewDialog::updatePreview ():"
               << "   shrunkenDocumentPixmap: w="
               << m_shrunkenDocumentPixmap.width ()
               << " h="
               << m_shrunkenDocumentPixmap.height ()
               << "   previewPixmapLabel: w="
               << m_previewPixmapLabel->width ()
               << " h="
               << m_previewPixmapLabel->height ()
               << "   transformedShrunkenDocumentPixmap: w="
               << transformedShrunkenDocumentPixmap.width ()
               << " h="
               << transformedShrunkenDocumentPixmap.height ()
               << "   previewPixmap: w="
               << previewPixmap.width ()
               << " h="
               << previewPixmap.height ()
               << endl;
#endif

        m_previewPixmapLabel->setPixmap (previewPixmap);

#if DEBUG_KP_TOOL_PREVIEW_DIALOG
    kdDebug () << "\tafter QLabel::setPixmap() previewPixmapLabel: w="
               << m_previewPixmapLabel->width ()
               << " h="
               << m_previewPixmapLabel->height ()
               << endl;
#endif
    }
}


// protected slot virtual [base this]
void kpToolPreviewDialog::slotUpdate ()
{
    updateDimensions ();
    updatePreview ();
}


// protected virtual [base QWidget]
void kpToolPreviewDialog::resizeEvent (QResizeEvent * /*e*/)
{
    updatePreview ();
}


#include <kptoolpreviewdialog.moc>
