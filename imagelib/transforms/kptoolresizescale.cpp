
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


#define DEBUG_KP_TOOL_RESIZE_SCALE_COMMAND 0
#define DEBUG_KP_TOOL_RESIZE_SCALE_DIALOG 0


#include <kptoolresizescale.h>

#include <math.h>

#include <q3accel.h>
#include <qapplication.h>
#include <qboxlayout.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qgridlayout.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qpoint.h>
#include <qpolygon.h>
#include <qpushbutton.h>
#include <qrect.h>
#include <qsize.h>
#include <qtoolbutton.h>
#include <qmatrix.h>


#include <kapplication.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobal.h>
#include <khbox.h>
#include <kiconeffect.h>
#include <kiconloader.h>
#include <klocale.h>
#include <knuminput.h>

#include <kpdefs.h>
#include <kpdocument.h>
#include <kpmainwindow.h>
#include <kppixmapfx.h>
#include <kpselection.h>
#include <kptool.h>


/*
 * kpToolResizeScaleCommand
 */

kpToolResizeScaleCommand::kpToolResizeScaleCommand (bool actOnSelection,
                                                    int newWidth, int newHeight,
                                                    Type type,
                                                    kpMainWindow *mainWindow)
    : kpCommand (mainWindow),
      m_actOnSelection (actOnSelection),
      m_type (type),
      m_backgroundColor (mainWindow ? mainWindow->backgroundColor () : kpColor::Invalid),
      m_oldSelection (0)
{
    kpDocument *doc = document ();
    Q_ASSERT (doc);

    m_oldWidth = doc->width (m_actOnSelection);
    m_oldHeight = doc->height (m_actOnSelection);

    m_actOnTextSelection = (m_actOnSelection &&
                            doc->selection () &&
                            doc->selection ()->isText ());

    resize (newWidth, newHeight);

    // If we have a selection _border_ (but not a floating selection),
    // then scale the selection with the document
    m_scaleSelectionWithImage = (!m_actOnSelection &&
                                 (m_type == Scale || m_type == SmoothScale) &&
                                 document ()->selection () &&
                                 !document ()->selection ()->pixmap ());
}

kpToolResizeScaleCommand::~kpToolResizeScaleCommand ()
{
    delete m_oldSelection;
}


// public virtual [base kpCommand]
QString kpToolResizeScaleCommand::name () const
{
    if (m_actOnSelection)
    {
        if (m_actOnTextSelection)
        {
            if (m_type == Resize)
                return i18n ("Text: Resize Box");
        }
        else
        {
            if (m_type == Scale)
                return i18n ("Selection: Scale");
            else if (m_type == SmoothScale)
                return i18n ("Selection: Smooth Scale");
        }
    }
    else
    {
        switch (m_type)
        {
        case Resize:
            return i18n ("Resize");
        case Scale:
            return i18n ("Scale");
        case SmoothScale:
            return i18n ("Smooth Scale");
        }
    }

    return QString::null;
}

// public virtual [base kpCommand]
int kpToolResizeScaleCommand::size () const
{
    return kpPixmapFX::pixmapSize (m_oldPixmap) +
           kpPixmapFX::pixmapSize (m_oldRightPixmap) +
           kpPixmapFX::pixmapSize (m_oldBottomPixmap) +
           (m_oldSelection ? m_oldSelection->size () : 0);
}


// public
int kpToolResizeScaleCommand::newWidth () const
{
    return m_newWidth;
}

// public
void kpToolResizeScaleCommand::setNewWidth (int width)
{
    resize (width, newHeight ());
}


// public
int kpToolResizeScaleCommand::newHeight () const
{
    return m_newHeight;
}

// public
void kpToolResizeScaleCommand::setNewHeight (int height)
{
    resize (newWidth (), height);
}


// public
QSize kpToolResizeScaleCommand::newSize () const
{
    return QSize (newWidth (), newHeight ());
}

// public virtual
void kpToolResizeScaleCommand::resize (int width, int height)
{
    m_newWidth = width;
    m_newHeight = height;

    m_isLosslessScale = ((m_type == Scale) &&
                         (m_newWidth / m_oldWidth * m_oldWidth == m_newWidth) &&
                         (m_newHeight / m_oldHeight * m_oldHeight == m_newHeight));
}


// public
bool kpToolResizeScaleCommand::scaleSelectionWithImage () const
{
    return m_scaleSelectionWithImage;
}


// private
void kpToolResizeScaleCommand::scaleSelectionRegionWithDocument ()
{
#if DEBUG_KP_TOOL_RESIZE_SCALE_COMMAND
    kDebug () << "kpToolResizeScaleCommand::scaleSelectionRegionWithDocument"
               << endl;
#endif

    Q_ASSERT (m_oldSelection);
    Q_ASSERT (!m_oldSelection->pixmap ());


    const double horizScale = double (m_newWidth) / double (m_oldWidth);
    const double vertScale = double (m_newHeight) / double (m_oldHeight);

    const int newX = (int) (m_oldSelection->x () * horizScale);
    const int newY = (int) (m_oldSelection->y () * vertScale);


    QPolygon currentPoints = m_oldSelection->points ();
    currentPoints.translate (-currentPoints.boundingRect ().x (),
                             -currentPoints.boundingRect ().y ());

    // TODO: refactor into kpPixmapFX
    QMatrix scaleMatrix;
    scaleMatrix.scale (horizScale, vertScale);
    currentPoints = scaleMatrix.map (currentPoints);

    currentPoints.translate (
        -currentPoints.boundingRect ().x () + newX,
        -currentPoints.boundingRect ().y () + newY);

    document ()->setSelection (kpSelection (currentPoints, QPixmap (),
                                            m_oldSelection->transparency ()));


    if (m_mainWindow->tool ())
        m_mainWindow->tool ()->somethingBelowTheCursorChanged ();
}


// public virtual [base kpCommand]
void kpToolResizeScaleCommand::execute ()
{
#if DEBUG_KP_TOOL_RESIZE_SCALE_COMMAND
    kDebug () << "kpToolResizeScaleCommand::execute() type="
               << (int) m_type
               << " oldWidth=" << m_oldWidth
               << " oldHeight=" << m_oldHeight
               << " newWidth=" << m_newWidth
               << " newHeight=" << m_newHeight
               << endl;
#endif

    if (m_oldWidth == m_newWidth && m_oldHeight == m_newHeight)
        return;

    if (m_type == Resize)
    {
        if (m_actOnSelection)
        {
            if (!m_actOnTextSelection)
            {
                Q_ASSERT (!"kpToolResizeScaleCommand::execute() resizing sel doesn't make sense");
                return;
            }
            else
            {
                QApplication::setOverrideCursor (Qt::WaitCursor);
                document ()->selection ()->textResize (m_newWidth, m_newHeight);

                Q_ASSERT (m_mainWindow->tool ());
                m_mainWindow->tool ()->somethingBelowTheCursorChanged ();

                QApplication::restoreOverrideCursor ();
            }
        }
        else
        {
            QApplication::setOverrideCursor (Qt::WaitCursor);


            if (m_newWidth < m_oldWidth)
            {
                m_oldRightPixmap = kpPixmapFX::getPixmapAt (
                    *document ()->pixmap (),
                    QRect (m_newWidth, 0,
                        m_oldWidth - m_newWidth, m_oldHeight));
            }

            if (m_newHeight < m_oldHeight)
            {
                m_oldBottomPixmap = kpPixmapFX::getPixmapAt (
                    *document ()->pixmap (),
                    QRect (0, m_newHeight,
                        m_newWidth, m_oldHeight - m_newHeight));
            }

            document ()->resize (m_newWidth, m_newHeight, m_backgroundColor);


            QApplication::restoreOverrideCursor ();
        }
    }
    else
    {
        QApplication::setOverrideCursor (Qt::WaitCursor);


        QPixmap oldPixmap = *document ()->pixmap (m_actOnSelection);

        if (!m_isLosslessScale)
            m_oldPixmap = oldPixmap;

        QPixmap newPixmap = kpPixmapFX::scale (oldPixmap, m_newWidth, m_newHeight,
                                               m_type == SmoothScale);


        if (!m_oldSelection && document ()->selection ())
        {
            // Save sel border
            m_oldSelection = new kpSelection (*document ()->selection ());
            m_oldSelection->setPixmap (QPixmap ());
        }

        if (m_actOnSelection)
        {
            Q_ASSERT (m_oldSelection);
            QRect newRect = QRect (m_oldSelection->x (), m_oldSelection->y (),
                                   newPixmap.width (), newPixmap.height ());

            // Not possible to retain non-rectangular selection borders on scale
            // (think about e.g. a 45 deg line as part of the border & 2x scale)
            document ()->setSelection (
                kpSelection (kpSelection::Rectangle, newRect, newPixmap,
                             m_oldSelection->transparency ()));

            Q_ASSERT (m_mainWindow->tool ());
            m_mainWindow->tool ()->somethingBelowTheCursorChanged ();
        }
        else
        {
            document ()->setPixmap (newPixmap);

            if (m_scaleSelectionWithImage)
            {
                scaleSelectionRegionWithDocument ();
            }
        }


        QApplication::restoreOverrideCursor ();
    }
}

// public virtual [base kpCommand]
void kpToolResizeScaleCommand::unexecute ()
{
#if DEBUG_KP_TOOL_RESIZE_SCALE_COMMAND
    kDebug () << "kpToolResizeScaleCommand::unexecute() type="
               << m_type << endl;
#endif

    if (m_oldWidth == m_newWidth && m_oldHeight == m_newHeight)
        return;

    kpDocument *doc = document ();
    Q_ASSERT (doc);

    if (m_type == Resize)
    {
        if (m_actOnSelection)
        {
            if (!m_actOnTextSelection)
            {
                Q_ASSERT (!"kpToolResizeScaleCommand::unexecute() resizing sel doesn't make sense");
                return;
            }
            else
            {
                QApplication::setOverrideCursor (Qt::WaitCursor);
                doc->selection ()->textResize (m_oldWidth, m_oldHeight);

                Q_ASSERT (m_mainWindow->tool ());
                m_mainWindow->tool ()->somethingBelowTheCursorChanged ();

                QApplication::restoreOverrideCursor ();
            }
        }
        else
        {
            QApplication::setOverrideCursor (Qt::WaitCursor);


            QPixmap newPixmap (m_oldWidth, m_oldHeight);

            kpPixmapFX::setPixmapAt (&newPixmap, QPoint (0, 0),
                                    *doc->pixmap ());

            if (m_newWidth < m_oldWidth)
            {
                kpPixmapFX::setPixmapAt (&newPixmap,
                                        QPoint (m_newWidth, 0),
                                        m_oldRightPixmap);
            }

            if (m_newHeight < m_oldHeight)
            {
                kpPixmapFX::setPixmapAt (&newPixmap,
                                        QPoint (0, m_newHeight),
                                        m_oldBottomPixmap);
            }

            doc->setPixmap (newPixmap);


            QApplication::restoreOverrideCursor ();
        }
    }
    else
    {
        QApplication::setOverrideCursor (Qt::WaitCursor);


        QPixmap oldPixmap;

        if (!m_isLosslessScale)
            oldPixmap = m_oldPixmap;
        else
            oldPixmap = kpPixmapFX::scale (*doc->pixmap (m_actOnSelection),
                                           m_oldWidth, m_oldHeight);


        if (m_actOnSelection)
        {
            kpSelection oldSelection = *m_oldSelection;
            oldSelection.setPixmap (oldPixmap);
            doc->setSelection (oldSelection);

            Q_ASSERT (m_mainWindow->tool ());
            m_mainWindow->tool ()->somethingBelowTheCursorChanged ();
        }
        else
        {
            doc->setPixmap (oldPixmap);

            if (m_scaleSelectionWithImage)
            {
                doc->setSelection (*m_oldSelection);

                Q_ASSERT (m_mainWindow->tool ());
                m_mainWindow->tool ()->somethingBelowTheCursorChanged ();
            }
        }


        QApplication::restoreOverrideCursor ();
    }
}


/*
 * kpToolResizeScaleDialog
 */

#define SET_VALUE_WITHOUT_SIGNAL_EMISSION(knuminput_instance,value)    \
{                                                                      \
    knuminput_instance->blockSignals (true);                           \
    knuminput_instance->setValue (value);                              \
    knuminput_instance->blockSignals (false);                          \
}

#define IGNORE_KEEP_ASPECT_RATIO(cmd) \
{                                     \
    m_ignoreKeepAspectRatio++;        \
    cmd;                              \
    m_ignoreKeepAspectRatio--;        \
}


// private static
kpToolResizeScaleCommand::Type kpToolResizeScaleDialog::s_lastType =
    kpToolResizeScaleCommand::Resize;

// private static
double kpToolResizeScaleDialog::s_lastPercentWidth = 100,
       kpToolResizeScaleDialog::s_lastPercentHeight = 100;


kpToolResizeScaleDialog::kpToolResizeScaleDialog (kpMainWindow *mainWindow)
    : KDialog ((QWidget *) mainWindow),
      m_mainWindow (mainWindow),
      m_ignoreKeepAspectRatio (0)
{
    setCaption( i18n ("Resize / Scale") );
    setButtons( KDialog::Ok | KDialog::Cancel);
    // Using the percentage from last time become too confusing so disable for now
    s_lastPercentWidth = 100, s_lastPercentHeight = 100;


    QWidget *baseWidget = new QWidget (this);
    setMainWidget (baseWidget);


    createActOnBox (baseWidget);
    createOperationGroupBox (baseWidget);
    createDimensionsGroupBox (baseWidget);


    QVBoxLayout *baseLayout = new QVBoxLayout (baseWidget);
    baseLayout->setSpacing(spacingHint ());
    baseLayout->setMargin(0/*margin*/);
    baseLayout->addWidget (m_actOnBox);
    baseLayout->addWidget (m_operationGroupBox);
    baseLayout->addWidget (m_dimensionsGroupBox);


    slotActOnChanged ();

    m_operationGroupBox->setFocus ();

    //enableButtonOk (!isNoOp ());
}

kpToolResizeScaleDialog::~kpToolResizeScaleDialog ()
{
}


// private
kpDocument *kpToolResizeScaleDialog::document () const
{
    Q_ASSERT (m_mainWindow);
    return m_mainWindow->document ();
}

// private
kpSelection *kpToolResizeScaleDialog::selection () const
{
    Q_ASSERT (document ());
    return document ()->selection ();
}


// private
void kpToolResizeScaleDialog::createActOnBox (QWidget *baseWidget)
{
    m_actOnBox = new KHBox (baseWidget);
    m_actOnBox->setSpacing (spacingHint () * 2);


    m_actOnLabel = new QLabel (i18n ("Ac&t on:"), m_actOnBox);
    m_actOnCombo = new KComboBox (m_actOnBox);


    m_actOnLabel->setBuddy (m_actOnCombo);

    m_actOnCombo->insertItem (Image, i18n ("Entire Image"));
    if (selection ())
    {
        QString selName = i18n ("Selection");

        if (selection ()->isText ())
            selName = i18n ("Text Box");

        m_actOnCombo->insertItem (Selection, selName);
        m_actOnCombo->setCurrentIndex (Selection);
    }
    else
    {
        m_actOnLabel->setEnabled (false);
        m_actOnCombo->setEnabled (false);
    }


    m_actOnBox->setStretchFactor (m_actOnCombo, 1);


    connect (m_actOnCombo, SIGNAL (activated (int)),
             this, SLOT (slotActOnChanged ()));
}


static QIcon toolButtonIconSet (const QString &iconName)
{
    QIcon iconSet = UserIconSet (iconName);


    // No "disabled" pixmap is generated by UserIconSet() so generate it
    // ourselves:

    QPixmap disabledIcon = KGlobal::iconLoader ()->iconEffect ()->apply (
        UserIcon (iconName),
        K3Icon::Toolbar, K3Icon::DisabledState);

    const QPixmap iconSetNormalIcon = iconSet.pixmap (QIcon::Small,
                                                      QIcon::Normal);

    // I bet past or future versions of KIconEffect::apply() resize the
    // disabled icon if we claim it's in group K3Icon::Toolbar.  So resize
    // it to match the QIcon::Normal icon, just in case.
    disabledIcon = kpPixmapFX::scale (disabledIcon,
                                      iconSetNormalIcon.width (),
                                      iconSetNormalIcon.height (),
                                      true/*smooth scale*/);


    iconSet.setPixmap (disabledIcon,
                       QIcon::Small, QIcon::Disabled);

    return iconSet;
}

static void toolButtonSetLook (QToolButton *button,
                               const QString &iconName,
                               const QString &name)
{
    button->setIconSet (toolButtonIconSet (iconName));
    button->setUsesTextLabel (true);
    button->setTextLabel (name, false/*no tooltip*/);
    button->setAccel (Q3Accel::shortcutKey (name));
    button->setFocusPolicy (Qt::StrongFocus);
    button->setToggleButton (true);
}


// private
void kpToolResizeScaleDialog::createOperationGroupBox (QWidget *baseWidget)
{
    m_operationGroupBox = new QGroupBox (i18n ("Operation"), baseWidget);
    m_operationGroupBox->setWhatsThis(
        i18n ("<qt>"
              "<ul>"
                  "<li><b>Resize</b>: The size of the picture will be"
                  " increased"
                  " by creating new areas to the right and/or bottom"
                  " (filled in with the background color) or"
                  " decreased by cutting"
                  " it at the right and/or bottom.</li>"

                  "<li><b>Scale</b>: The picture will be expanded"
                  " by duplicating pixels or squashed by dropping pixels.</li>"

                  "<li><b>Smooth Scale</b>: This is the same as"
                  " <i>Scale</i> except that it blends neighboring"
                  " pixels to produce a smoother looking picture.</li>"
              "</ul>"
              "</qt>"));

    m_resizeButton = new QToolButton (m_operationGroupBox);
    toolButtonSetLook (m_resizeButton,
                       QLatin1String ("resize"),
                       i18n ("&Resize"));

    m_scaleButton = new QToolButton (m_operationGroupBox);
    toolButtonSetLook (m_scaleButton,
                       QLatin1String ("scale"),
                       i18n ("&Scale"));

    m_smoothScaleButton = new QToolButton (m_operationGroupBox);
    toolButtonSetLook (m_smoothScaleButton,
                       QLatin1String ("smooth_scale"),
                       i18n ("S&mooth Scale"));


    //m_resizeLabel = new QLabel (i18n ("&Resize"), m_operationGroupBox);
    //m_scaleLabel = new QLabel (i18n ("&Scale"), m_operationGroupBox);
    //m_smoothScaleLabel = new QLabel (i18n ("S&mooth scale"), m_operationGroupBox);


    //m_resizeLabel->setAlignment (m_resizeLabel->alignment () | Qt::TextShowMnemonic);
    //m_scaleLabel->setAlignment (m_scaleLabel->alignment () | Qt::TextShowMnemonic);
    //m_smoothScaleLabel->setAlignment (m_smoothScaleLabel->alignment () | Qt::TextShowMnemonic);


    QButtonGroup *resizeScaleButtonGroup = new QButtonGroup (baseWidget);
    resizeScaleButtonGroup->addButton (m_resizeButton);
    resizeScaleButtonGroup->addButton (m_scaleButton);
    resizeScaleButtonGroup->addButton (m_smoothScaleButton);


    QGridLayout *operationLayout = new QGridLayout (m_operationGroupBox );
    operationLayout->setMargin( marginHint () * 2/*don't overlap groupbox title*/ );
    operationLayout->setSpacing( spacingHint ());

    operationLayout->addWidget (m_resizeButton, 0, 0, Qt::AlignCenter);
    //operationLayout->addWidget (m_resizeLabel, 1, 0, Qt::AlignCenter);

    operationLayout->addWidget (m_scaleButton, 0, 1, Qt::AlignCenter);
    //operationLayout->addWidget (m_scaleLabel, 1, 1, Qt::AlignCenter);

    operationLayout->addWidget (m_smoothScaleButton, 0, 2, Qt::AlignCenter);
    //operationLayout->addWidget (m_smoothScaleLabel, 1, 2, Qt::AlignCenter);


    connect (m_resizeButton, SIGNAL (toggled (bool)),
             this, SLOT (slotTypeChanged ()));
    connect (m_scaleButton, SIGNAL (toggled (bool)),
             this, SLOT (slotTypeChanged ()));
    connect (m_smoothScaleButton, SIGNAL (toggled (bool)),
             this, SLOT (slotTypeChanged ()));
}

// private
void kpToolResizeScaleDialog::createDimensionsGroupBox (QWidget *baseWidget)
{
    m_dimensionsGroupBox = new QGroupBox (i18n ("Dimensions"), baseWidget);

    QLabel *widthLabel = new QLabel (i18n ("Width:"), m_dimensionsGroupBox);
    widthLabel->setAlignment (widthLabel->alignment () | Qt::AlignHCenter);
    QLabel *heightLabel = new QLabel (i18n ("Height:"), m_dimensionsGroupBox);
    heightLabel->setAlignment (heightLabel->alignment () | Qt::AlignHCenter);

    QLabel *originalLabel = new QLabel (i18n ("Original:"), m_dimensionsGroupBox);
    m_originalWidthInput = new KIntNumInput (
        document ()->width ((bool) selection ()),
        m_dimensionsGroupBox);
    QLabel *xLabel0 = new QLabel (i18n ("x"), m_dimensionsGroupBox);
    m_originalHeightInput = new KIntNumInput (
        document ()->height ((bool) selection ()),
        m_dimensionsGroupBox);

    QLabel *newLabel = new QLabel (i18n ("&New:"), m_dimensionsGroupBox);
    m_newWidthInput = new KIntNumInput (m_dimensionsGroupBox);
    QLabel *xLabel1 = new QLabel (i18n ("x"), m_dimensionsGroupBox);
    m_newHeightInput = new KIntNumInput (m_dimensionsGroupBox);

    QLabel *percentLabel = new QLabel (i18n ("&Percent:"), m_dimensionsGroupBox);
    m_percentWidthInput = new KDoubleNumInput (0.01/*lower*/, 1000000/*upper*/,
                                               100/*value*/,
                                               m_dimensionsGroupBox,
                                               1/*step*/,
                                               2/*precision*/);
    m_percentWidthInput->setSuffix (i18n ("%"));
    QLabel *xLabel2 = new QLabel (i18n ("x"), m_dimensionsGroupBox);
    m_percentHeightInput = new KDoubleNumInput (0.01/*lower*/, 1000000/*upper*/,
                                                100/*value*/,
                                                m_dimensionsGroupBox,
                                                1/*step*/,
                                                2/*precision*/);
    m_percentHeightInput->setSuffix (i18n ("%"));

    m_keepAspectRatioCheckBox = new QCheckBox (i18n ("Keep &aspect ratio"),
                                               m_dimensionsGroupBox);


    m_originalWidthInput->setEnabled (false);
    m_originalHeightInput->setEnabled (false);
    originalLabel->setBuddy (m_originalWidthInput);
    newLabel->setBuddy (m_newWidthInput);
    m_percentWidthInput->setValue (s_lastPercentWidth);
    m_percentHeightInput->setValue (s_lastPercentHeight);
    percentLabel->setBuddy (m_percentWidthInput);


    QGridLayout *dimensionsLayout = new QGridLayout (m_dimensionsGroupBox );
    dimensionsLayout->setMargin( marginHint () * 2 );
    dimensionsLayout->setSpacing( spacingHint ());
    dimensionsLayout->setColumnStretch (1/*column*/, 1);
    dimensionsLayout->setColumnStretch (3/*column*/, 1);


    dimensionsLayout->addWidget (widthLabel, 0, 1);
    dimensionsLayout->addWidget (heightLabel, 0, 3);

    dimensionsLayout->addWidget (originalLabel, 1, 0);
    dimensionsLayout->addWidget (m_originalWidthInput, 1, 1);
    dimensionsLayout->addWidget (xLabel0, 1, 2);
    dimensionsLayout->addWidget (m_originalHeightInput, 1, 3);

    dimensionsLayout->addWidget (newLabel, 2, 0);
    dimensionsLayout->addWidget (m_newWidthInput, 2, 1);
    dimensionsLayout->addWidget (xLabel1, 2, 2);
    dimensionsLayout->addWidget (m_newHeightInput, 2, 3);

    dimensionsLayout->addWidget (percentLabel, 3, 0);
    dimensionsLayout->addWidget (m_percentWidthInput, 3, 1);
    dimensionsLayout->addWidget (xLabel2, 3, 2);
    dimensionsLayout->addWidget (m_percentHeightInput, 3, 3);

    dimensionsLayout->addWidget (m_keepAspectRatioCheckBox, 4, 0, 1, 4);
    dimensionsLayout->setRowStretch (4/*row*/, 1);
    dimensionsLayout->setRowSpacing (4/*row*/, dimensionsLayout->rowSpacing (4) * 2);


    connect (m_newWidthInput, SIGNAL (valueChanged (int)),
             this, SLOT (slotWidthChanged (int)));
    connect (m_newHeightInput, SIGNAL (valueChanged (int)),
             this, SLOT (slotHeightChanged (int)));

    connect (m_percentWidthInput, SIGNAL (valueChanged (double)),
             this, SLOT (slotPercentWidthChanged (double)));
    connect (m_percentHeightInput, SIGNAL (valueChanged (double)),
             this, SLOT (slotPercentHeightChanged (double)));

    connect (m_keepAspectRatioCheckBox, SIGNAL (toggled (bool)),
             this, SLOT (setKeepAspectRatio (bool)));
}


// private
void kpToolResizeScaleDialog::widthFitHeightToAspectRatio ()
{
    if (m_keepAspectRatioCheckBox->isChecked () && !m_ignoreKeepAspectRatio)
    {
        // width / height = oldWidth / oldHeight
        // height = width * oldHeight / oldWidth
        const int newHeight = qRound (double (imageWidth ()) * double (originalHeight ())
                                      / double (originalWidth ()));
        IGNORE_KEEP_ASPECT_RATIO (m_newHeightInput->setValue (newHeight));
    }
}

// private
void kpToolResizeScaleDialog::heightFitWidthToAspectRatio ()
{
    if (m_keepAspectRatioCheckBox->isChecked () && !m_ignoreKeepAspectRatio)
    {
        // width / height = oldWidth / oldHeight
        // width = height * oldWidth / oldHeight
        const int newWidth = qRound (double (imageHeight ()) * double (originalWidth ())
                                     / double (originalHeight ()));
        IGNORE_KEEP_ASPECT_RATIO (m_newWidthInput->setValue (newWidth));
    }
}


// private
bool kpToolResizeScaleDialog::resizeEnabled () const
{
    return (!actOnSelection () ||
            (actOnSelection () && selection ()->isText ()));
}

// private
bool kpToolResizeScaleDialog::scaleEnabled () const
{
    return (!(actOnSelection () && selection ()->isText ()));
}

// private
bool kpToolResizeScaleDialog::smoothScaleEnabled () const
{
    return scaleEnabled ();
}


// public slot
void kpToolResizeScaleDialog::slotActOnChanged ()
{
#if DEBUG_KP_TOOL_RESIZE_SCALE_DIALOG && 1
    kDebug () << "kpToolResizeScaleDialog::slotActOnChanged()" << endl;
#endif

    m_resizeButton->setEnabled (resizeEnabled ());
    //m_resizeLabel->setEnabled (resizeEnabled ());

    m_scaleButton->setEnabled (scaleEnabled ());
    //m_scaleLabel->setEnabled (scaleEnabled ());

    m_smoothScaleButton->setEnabled (smoothScaleEnabled ());
    //m_smoothScaleLabel->setEnabled (smoothScaleEnabled ());


    // TODO: somehow share logic with (resize|*scale)Enabled()
    if (actOnSelection ())
    {
        if (selection ()->isText ())
        {
            m_resizeButton->setChecked (true);
        }
        else
        {
            if (s_lastType == kpToolResizeScaleCommand::Scale)
                m_scaleButton->setChecked (true);
            else
                m_smoothScaleButton->setChecked (true);
        }
    }
    else
    {
        if (s_lastType == kpToolResizeScaleCommand::Resize)
            m_resizeButton->setChecked (true);
        else if (s_lastType == kpToolResizeScaleCommand::Scale)
            m_scaleButton->setChecked (true);
        else
            m_smoothScaleButton->setChecked (true);
    }


    m_originalWidthInput->setValue (originalWidth ());
    m_originalHeightInput->setValue (originalHeight ());


    m_newWidthInput->blockSignals (true);
    m_newHeightInput->blockSignals (true);

    m_newWidthInput->setMinimum (actOnSelection () ?
                                      selection ()->minimumWidth () :
                                      1);
    m_newHeightInput->setMinimum (actOnSelection () ?
                                       selection ()->minimumHeight () :
                                       1);

    m_newWidthInput->blockSignals (false);
    m_newHeightInput->blockSignals (false);


    IGNORE_KEEP_ASPECT_RATIO (slotPercentWidthChanged (m_percentWidthInput->value ()));
    IGNORE_KEEP_ASPECT_RATIO (slotPercentHeightChanged (m_percentHeightInput->value ()));

    setKeepAspectRatio (m_keepAspectRatioCheckBox->isChecked ());
}


// public slot
void kpToolResizeScaleDialog::slotTypeChanged ()
{
    s_lastType = type ();
}

// public slot
void kpToolResizeScaleDialog::slotWidthChanged (int width)
{
#if DEBUG_KP_TOOL_RESIZE_SCALE_DIALOG && 1
    kDebug () << "kpToolResizeScaleDialog::slotWidthChanged("
               << width << ")" << endl;
#endif
    const double newPercentWidth = double (width) * 100 / double (originalWidth ());

    SET_VALUE_WITHOUT_SIGNAL_EMISSION (m_percentWidthInput, newPercentWidth);

    widthFitHeightToAspectRatio ();

    //enableButtonOk (!isNoOp ());
    s_lastPercentWidth = newPercentWidth;
}

// public slot
void kpToolResizeScaleDialog::slotHeightChanged (int height)
{
#if DEBUG_KP_TOOL_RESIZE_SCALE_DIALOG && 1
    kDebug () << "kpToolResizeScaleDialog::slotHeightChanged("
               << height << ")" << endl;
#endif
    const double newPercentHeight = double (height) * 100 / double (originalHeight ());

    SET_VALUE_WITHOUT_SIGNAL_EMISSION (m_percentHeightInput, newPercentHeight);

    heightFitWidthToAspectRatio ();

    //enableButtonOk (!isNoOp ());
    s_lastPercentHeight = newPercentHeight;
}

// public slot
void kpToolResizeScaleDialog::slotPercentWidthChanged (double percentWidth)
{
#if DEBUG_KP_TOOL_RESIZE_SCALE_DIALOG && 1
    kDebug () << "kpToolResizeScaleDialog::slotPercentWidthChanged("
               << percentWidth << ")" << endl;
#endif

    SET_VALUE_WITHOUT_SIGNAL_EMISSION (m_newWidthInput,
                                       qRound (percentWidth * originalWidth () / 100.0));

    widthFitHeightToAspectRatio ();

    //enableButtonOk (!isNoOp ());
    s_lastPercentWidth = percentWidth;
}

// public slot
void kpToolResizeScaleDialog::slotPercentHeightChanged (double percentHeight)
{
#if DEBUG_KP_TOOL_RESIZE_SCALE_DIALOG && 1
    kDebug () << "kpToolResizeScaleDialog::slotPercentHeightChanged("
               << percentHeight << ")" << endl;
#endif

    SET_VALUE_WITHOUT_SIGNAL_EMISSION (m_newHeightInput,
                                       qRound (percentHeight * originalHeight () / 100.0));

    heightFitWidthToAspectRatio ();

    //enableButtonOk (!isNoOp ());
    s_lastPercentHeight = percentHeight;
}

// public
bool kpToolResizeScaleDialog::keepAspectRatio () const
{
    return m_keepAspectRatioCheckBox->isChecked ();
}

// public slot
void kpToolResizeScaleDialog::setKeepAspectRatio (bool on)
{
#if DEBUG_KP_TOOL_RESIZE_SCALE_DIALOG && 1
    kDebug () << "kpToolResizeScaleDialog::setKeepAspectRatio("
               << on << ")" << endl;
#endif
    if (on != m_keepAspectRatioCheckBox->isChecked ())
        m_keepAspectRatioCheckBox->setChecked (on);

    if (on)
        widthFitHeightToAspectRatio ();
}

#undef IGNORE_KEEP_ASPECT_RATIO
#undef SET_VALUE_WITHOUT_SIGNAL_EMISSION


// private
int kpToolResizeScaleDialog::originalWidth () const
{
    return document ()->width (actOnSelection ());
}

// private
int kpToolResizeScaleDialog::originalHeight () const
{
    return document ()->height (actOnSelection ());
}


// public
int kpToolResizeScaleDialog::imageWidth () const
{
    return m_newWidthInput->value ();
}

// public
int kpToolResizeScaleDialog::imageHeight () const
{
    return m_newHeightInput->value ();
}

// public
bool kpToolResizeScaleDialog::actOnSelection () const
{
    return (m_actOnCombo->currentIndex () == Selection);
}

// public
kpToolResizeScaleCommand::Type kpToolResizeScaleDialog::type () const
{
    if (m_resizeButton->isChecked ())
        return kpToolResizeScaleCommand::Resize;
    else if (m_scaleButton->isChecked ())
        return kpToolResizeScaleCommand::Scale;
    else
        return kpToolResizeScaleCommand::SmoothScale;
}

// public
bool kpToolResizeScaleDialog::isNoOp () const
{
    return (imageWidth () == originalWidth () &&
            imageHeight () == originalHeight ());
}


// private slot virtual [base QDialog]
void kpToolResizeScaleDialog::accept ()
{
    enum { eText, eSelection, eImage } actionTarget = eText;

    if (actOnSelection ())
    {
        if (selection ()->isText ())
        {
            actionTarget = eText;
        }
        else
        {
            actionTarget = eSelection;
        }
    }
    else
    {
        actionTarget = eImage;
    }


    KLocalizedString message;
    QString caption, continueButtonText;

    // Note: If eText, can't Scale nor SmoothScale.
    //       If eSelection, can't Resize.

    switch (type ())
    {
    default:
    case kpToolResizeScaleCommand::Resize:
        if (actionTarget == eText)
        {
            message =
                ki18n ("<qt><p>Resizing the text box to %1x%2"
                      " may take a substantial amount of memory."
                      " This can reduce system"
                      " responsiveness and cause other application resource"
                      " problems.</p>"

                      "<p>Are you sure you want to resize the text box?</p></qt>");

            caption = i18n ("Resize Text Box?");
            continueButtonText = i18n ("R&esize Text Box");
        }
        else if (actionTarget == eImage)
        {
            message =
                ki18n ("<qt><p>Resizing the image to %1x%2"
                      " may take a substantial amount of memory."
                      " This can reduce system"
                      " responsiveness and cause other application resource"
                      " problems.</p>"

                      "<p>Are you sure you want to resize the image?</p></qt>");

            caption = i18n ("Resize Image?");
            continueButtonText = i18n ("R&esize Image");
        }

        break;

    case kpToolResizeScaleCommand::Scale:
        if (actionTarget == eImage)
        {
            message =
                ki18n ("<qt><p>Scaling the image to %1x%2"
                      " may take a substantial amount of memory."
                      " This can reduce system"
                      " responsiveness and cause other application resource"
                      " problems.</p>"

                      "<p>Are you sure you want to scale the image?</p></qt>");

            caption = i18n ("Scale Image?");
            continueButtonText = i18n ("Scal&e Image");
        }
        else if (actionTarget == eSelection)
        {
            message =
                ki18n ("<qt><p>Scaling the selection to %1x%2"
                      " may take a substantial amount of memory."
                      " This can reduce system"
                      " responsiveness and cause other application resource"
                      " problems.</p>"

                      "<p>Are you sure you want to scale the selection?</p></qt>");

            caption = i18n ("Scale Selection?");
            continueButtonText = i18n ("Scal&e Selection");
        }

        break;

    case kpToolResizeScaleCommand::SmoothScale:
        if (actionTarget == eImage)
        {
            message =
                ki18n ("<qt><p>Smooth Scaling the image to %1x%2"
                      " may take a substantial amount of memory."
                      " This can reduce system"
                      " responsiveness and cause other application resource"
                      " problems.</p>"

                      "<p>Are you sure you want to smooth scale the image?</p></qt>");

            caption = i18n ("Smooth Scale Image?");
            continueButtonText = i18n ("Smooth Scal&e Image");
        }
        else if (actionTarget == eSelection)
        {
            message =
                ki18n ("<qt><p>Smooth Scaling the selection to %1x%2"
                      " may take a substantial amount of memory."
                      " This can reduce system"
                      " responsiveness and cause other application resource"
                      " problems.</p>"

                      "<p>Are you sure you want to smooth scale the selection?</p></qt>");

            caption = i18n ("Smooth Scale Selection?");
            continueButtonText = i18n ("Smooth Scal&e Selection");
        }

        break;
    }


    if (kpTool::warnIfBigImageSize (originalWidth (),
            originalHeight (),
            imageWidth (), imageHeight (),
            message.subs (imageWidth ()).subs (imageHeight ()).toString (),
            caption,
            continueButtonText,
            this))
    {
        KDialog::accept ();
    }
}


#include <kptoolresizescale.moc>