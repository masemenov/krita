/* This file is part of the KDE project
 * Made by Tomislav Lukman (tomislav.lukman@ck.tel.hr)
 * Copyright (C) 2012 Jean-Nicolas Artaud <jeannicolasartaud@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "KoFillConfigWidget.h"

#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QButtonGroup>
#include <QLabel>
#include <QSizePolicy>
#include <QBitmap>
#include <QAction>
#include <QSharedPointer>

#include <klocalizedstring.h>

#include <KoIcon.h>
#include <KoColor.h>
#include <KoColorPopupAction.h>
#include "KoResourceServerProvider.h"
#include "KoResourceServerAdapter.h"
#include "KoResourceSelector.h"
#include <KoSelection.h>
#include <KoToolManager.h>
#include <KoCanvasBase.h>
#include <KoCanvasController.h>
#include <KoCanvasResourceManager.h>
#include <KoDocumentResourceManager.h>
#include <KoShape.h>
#include <KoShapeManager.h>
#include <KoShapeController.h>
#include <KoShapeBackground.h>
#include <KoShapeBackgroundCommand.h>
#include <KoShapeStrokeCommand.h>
#include <KoShapeStroke.h>
#include <KoColorBackground.h>
#include <KoGradientBackground.h>
#include <KoPatternBackground.h>
#include <KoImageCollection.h>
#include <KoResourcePopupAction.h>
#include "KoZoomHandler.h"
#include "KoColorPopupButton.h"
#include "ui_KoFillConfigWidget.h"
#include <kis_signals_blocker.h>
#include <kis_signal_compressor.h>
#include <kis_acyclic_signal_connector.h>
#include <kis_assert.h>
#include <KoCanvasResourceManager.h>
#include "kis_canvas_resource_provider.h"
#include <KoStopGradient.h>
#include <QInputDialog>

#include "kis_global.h"
#include "kis_debug.h"

static const char* const buttonnone[]={
    "16 16 3 1",
    "# c #000000",
    "e c #ff0000",
    "- c #ffffff",
    "################",
    "#--------------#",
    "#-e----------e-#",
    "#--e--------e--#",
    "#---e------e---#",
    "#----e----e----#",
    "#-----e--e-----#",
    "#------ee------#",
    "#------ee------#",
    "#-----e--e-----#",
    "#----e----e----#",
    "#---e------e---#",
    "#--e--------e--#",
    "#-e----------e-#",
    "#--------------#",
    "################"};

static const char* const buttonsolid[]={
    "16 16 2 1",
    "# c #000000",
    ". c #969696",
    "################",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "################"};


// FIXME: Smoother gradient button.

static const char* const buttongradient[]={
    "16 16 15 1",
    "# c #000000",
    "n c #101010",
    "m c #202020",
    "l c #303030",
    "k c #404040",
    "j c #505050",
    "i c #606060",
    "h c #707070",
    "g c #808080",
    "f c #909090",
    "e c #a0a0a0",
    "d c #b0b0b0",
    "c c #c0c0c0",
    "b c #d0d0d0",
    "a c #e0e0e0",
    "################",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "#abcdefghijklmn#",
    "################"};

static const char* const buttonpattern[]={
    "16 16 4 1",
    ". c #0a0a0a",
    "# c #333333",
    "a c #a0a0a0",
    "b c #ffffffff",
    "################",
    "#aaaaabbbbaaaaa#",
    "#aaaaabbbbaaaaa#",
    "#aaaaabbbbaaaaa#",
    "#aaaaabbbbaaaaa#",
    "#aaaaabbbbaaaaa#",
    "#bbbbbaaaabbbbb#",
    "#bbbbbaaaabbbbb#",
    "#bbbbbaaaabbbbb#",
    "#bbbbbaaaabbbbb#",
    "#aaaaabbbbaaaaa#",
    "#aaaaabbbbaaaaa#",
    "#aaaaabbbbaaaaa#",
    "#aaaaabbbbaaaaa#",
    "#aaaaabbbbaaaaa#",
    "################"};

class Q_DECL_HIDDEN KoFillConfigWidget::Private
{
public:
    Private(FillType _fillType)
    : canvas(0),
      colorChangedCompressor(100, KisSignalCompressor::FIRST_ACTIVE),
      gradientChangedCompressor(100, KisSignalCompressor::FIRST_ACTIVE),
      fillType(_fillType)
    {
    }

    QSharedPointer<KoShapeBackground> applyFillGradientStops(KoShape *shape, const KoStopGradient *stopGradient)
    {
        QScopedPointer<QGradient> srcQGradient(stopGradient->toQGradient());
        QGradientStops stops = srcQGradient->stops();

        if (!shape || !stops.count()) {
            return QSharedPointer<KoShapeBackground>();
        }

        KoGradientBackground *newGradient = 0;
        QSharedPointer<KoGradientBackground> oldGradient = qSharedPointerDynamicCast<KoGradientBackground>(shape->background());
        if (oldGradient) {
            // just copy the gradient and set the new stops
            QGradient *g = KoFlake::mergeGradient(oldGradient->gradient(), srcQGradient.data());
            newGradient = new KoGradientBackground(g);
            newGradient->setTransform(oldGradient->transform());
        }
        else {
            // No gradient yet, so create a new one.
            QScopedPointer<QLinearGradient> fakeShapeGradient(new QLinearGradient(QPointF(0, 0), QPointF(1, 1)));
            fakeShapeGradient->setCoordinateMode(QGradient::ObjectBoundingMode);

            QGradient *g = KoFlake::mergeGradient(fakeShapeGradient.data(), srcQGradient.data());
            newGradient = new KoGradientBackground(g);
        }
        return QSharedPointer<KoGradientBackground>(newGradient);
    }

    KoShapeStrokeSP applyFillGradientStops(KoShapeStrokeSP shapeStroke, const KoStopGradient *stopGradient)
    {
        KoShapeStrokeSP newStroke;

        QScopedPointer<QGradient> srcQGradient(stopGradient->toQGradient());
        QGradientStops stops = srcQGradient->stops();

        if (!stops.count()) {
            return newStroke;
        }

        QLinearGradient fakeShapeGradient(QPointF(0, 0), QPointF(1, 1));
        fakeShapeGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
        QTransform gradientTransform;
        const QGradient *shapeGradient = 0;

        if (shapeStroke) {
            newStroke = toQShared(new KoShapeStroke(*shapeStroke));
            QBrush brush = newStroke->lineBrush();

            gradientTransform = brush.transform();
            shapeGradient = brush.gradient() ? brush.gradient() : &fakeShapeGradient;

        } else {
            newStroke = toQShared(new KoShapeStroke());
            shapeGradient = &fakeShapeGradient;
        }

        {
            QScopedPointer<QGradient> g(KoFlake::mergeGradient(shapeGradient, srcQGradient.data()));
            QBrush newBrush = *g;
            newBrush.setTransform(gradientTransform);
            newStroke->setLineBrush(newBrush);
        }

        return newStroke;
    }

    KoColorPopupAction *colorAction;
    KoResourcePopupAction *gradientAction;
    KoResourcePopupAction *patternAction;
    QButtonGroup *group;

    KoCanvasBase *canvas;

    KisSignalCompressor colorChangedCompressor;
    KisAcyclicSignalConnector shapeChangedAcyclicConnector;

    QSharedPointer<KoStopGradient> activeGradient;
    KisSignalCompressor gradientChangedCompressor;
    FillType fillType;

    Ui_KoFillConfigWidget *ui;
};

KoFillConfigWidget::KoFillConfigWidget(FillType type, QWidget *parent)
    :  QWidget(parent)
    , d(new Private(type))
{
    // connect to the canvas
    KoCanvasController *canvasController = KoToolManager::instance()->activeCanvasController();
    d->canvas = canvasController->canvas();

    d->shapeChangedAcyclicConnector.connectBackwardVoid(
                d->canvas->shapeManager()->selection(), SIGNAL(selectionChanged()),
                this, SLOT(shapeChanged()));

    d->shapeChangedAcyclicConnector.connectBackwardVoid(
                d->canvas->shapeManager(), SIGNAL(selectionContentChanged()),
                this, SLOT(shapeChanged()));

    // WARNING: not acyclic!
    connect(d->canvas->resourceManager(), SIGNAL(canvasResourceChanged(int,QVariant)),
            this, SLOT(slotCanvasResourceChanged(int,QVariant)));

    // confure GUI

    d->ui = new Ui_KoFillConfigWidget();
    d->ui->setupUi(this);

    d->group = new QButtonGroup(this);
    d->group->setExclusive(true);

    d->ui->btnNoFill->setIcon(QPixmap((const char **) buttonnone));
    d->group->addButton(d->ui->btnNoFill, None);

    d->ui->btnSolidFill->setIcon(QPixmap((const char **) buttonsolid));
    d->group->addButton(d->ui->btnSolidFill, Solid);

    d->ui->btnGradientFill->setIcon(QPixmap((const char **) buttongradient));
    d->group->addButton(d->ui->btnGradientFill, Gradient);

    d->ui->btnPatternFill->setIcon(QPixmap((const char **) buttonpattern));
    d->group->addButton(d->ui->btnPatternFill, Pattern);

    d->colorAction = new KoColorPopupAction(d->ui->btnChooseSolidColor);
    d->colorAction->setToolTip(i18n("Change the filling color"));
    d->colorAction->setCurrentColor(Qt::white);

    d->ui->btnChooseSolidColor->setDefaultAction(d->colorAction);
    d->ui->btnChooseSolidColor->setPopupMode(QToolButton::InstantPopup);
    d->ui->btnSolidColorPick->setIcon(KisIconUtils::loadIcon("krita_tool_color_picker"));

    // TODO: for now the color picking button is disabled!
    d->ui->btnSolidColorPick->setEnabled(false);

    connect(d->colorAction, SIGNAL(colorChanged(const KoColor &)), &d->colorChangedCompressor, SLOT(start()));
    connect(&d->colorChangedCompressor, SIGNAL(timeout()), SLOT(colorChanged()));

    connect(d->ui->btnChooseSolidColor, SIGNAL(iconSizeChanged()), d->colorAction, SLOT(updateIcon()));

    connect(d->group, SIGNAL(buttonClicked(int)), SLOT(styleButtonPressed(int)));
    connect(d->ui->stackWidget, SIGNAL(currentChanged(int)), SLOT(slotUpdateFillTitle()));
    slotUpdateFillTitle();
    styleButtonPressed(d->group->checkedId());

    // Gradient selector
    d->ui->wdgGradientEditor->setCompactMode(true);
    connect(d->ui->wdgGradientEditor, SIGNAL(sigGradientChanged()), &d->gradientChangedCompressor, SLOT(start()));
    connect(&d->gradientChangedCompressor, SIGNAL(timeout()), SLOT(activeGradientChanged()));

    KoResourceServerProvider *serverProvider = KoResourceServerProvider::instance();
    QSharedPointer<KoAbstractResourceServerAdapter> gradientResourceAdapter(
                new KoResourceServerAdapter<KoAbstractGradient>(serverProvider->gradientServer()));

    d->gradientAction = new KoResourcePopupAction(gradientResourceAdapter,
                                                  d->ui->btnChoosePredefinedGradient);

    d->gradientAction->setToolTip(i18n("Change filling gradient"));
    d->ui->btnChoosePredefinedGradient->setDefaultAction(d->gradientAction);
    d->ui->btnChoosePredefinedGradient->setPopupMode(QToolButton::InstantPopup);

    connect(d->gradientAction, SIGNAL(resourceSelected(QSharedPointer<KoShapeBackground> )),
            SLOT(gradientResourceChanged()));
    connect(d->ui->btnChoosePredefinedGradient, SIGNAL(iconSizeChanged()), d->gradientAction, SLOT(updateIcon()));

    d->ui->btnSaveGradient->setIcon(KisIconUtils::loadIcon("document-save"));
    connect(d->ui->btnSaveGradient, SIGNAL(clicked()), SLOT(slotSavePredefinedGradientClicked()));

    connect(d->ui->cmbGradientRepeat, SIGNAL(currentIndexChanged(int)), SLOT(slotGradientRepeatChanged()));
    connect(d->ui->cmbGradientType, SIGNAL(currentIndexChanged(int)), SLOT(slotGradientTypeChanged()));

#if 0

    // Pattern selector
    QSharedPointer<KoAbstractResourceServerAdapter>patternResourceAdapter(new KoResourceServerAdapter<KoPattern>(serverProvider->patternServer()));
    d->patternAction = new KoResourcePopupAction(patternResourceAdapter, d->colorButton);
    d->patternAction->setToolTip(i18n("Change the filling pattern"));
    connect(d->patternAction, SIGNAL(resourceSelected(QSharedPointer<KoShapeBackground> )), this, SLOT(patternChanged(QSharedPointer<KoShapeBackground> )));
    connect(d->colorButton, SIGNAL(iconSizeChanged()), d->patternAction, SLOT(updateIcon()));

#endif

}

KoFillConfigWidget::~KoFillConfigWidget()
{
    delete d;
}

void KoFillConfigWidget::slotUpdateFillTitle()
{
    QString text = d->group->checkedButton() ? d->group->checkedButton()->text() : QString();
    text.replace('&', QString());
    d->ui->lblFillTitle->setText(text);
}

void KoFillConfigWidget::slotCanvasResourceChanged(int key, const QVariant &value)
{
    if (!isVisible()) return;

    if (key == KoCanvasResourceManager::ForegroundColor) {
        KoColor color = value.value<KoColor>();

        const int checkedId = d->group->checkedId();

        if ((checkedId < 0 || checkedId == None || checkedId == Solid) &&
            !(checkedId == Solid && d->colorAction->currentKoColor() == color)) {

            d->ui->stackWidget->setCurrentIndex(Solid);
            d->colorAction->setCurrentColor(color);
            d->colorChangedCompressor.start();
        } else if (checkedId == Gradient) {
            d->ui->wdgGradientEditor->notifyGlobalColorChanged(color);
        }
    } else if (key == KisCanvasResourceProvider::CurrentGradient) {
        KoResource *gradient = value.value<KoAbstractGradient*>();
        const int checkedId = d->group->checkedId();

        if (gradient && (checkedId < 0 || checkedId == None || checkedId == Gradient)) {
            d->gradientAction->setCurrentResource(gradient);
        }
    }
}

QList<KoShape*> KoFillConfigWidget::currentShapes()
{

    return d->canvas->shapeManager()->selection()->selectedEditableShapes();
}

void KoFillConfigWidget::styleButtonPressed(int buttonId)
{
    switch (buttonId) {
        case KoFillConfigWidget::None:
            noColorSelected();
            break;
        case KoFillConfigWidget::Solid:
            colorChanged();
            break;
        case KoFillConfigWidget::Gradient:
            gradientResourceChanged();
            break;
        case KoFillConfigWidget::Pattern:
            // Only select mode in the widget, don't set actual pattern :/
            //d->colorButton->setDefaultAction(d->patternAction);
            //patternChanged(d->patternAction->currentBackground());
            break;
    }

    if (buttonId >= None && buttonId <= Pattern) {
        d->ui->stackWidget->setCurrentIndex(buttonId);
    }
}

void KoFillConfigWidget::noColorSelected()
{
    KisAcyclicSignalConnector::Blocker b(d->shapeChangedAcyclicConnector);

    QList<KoShape*> selectedShapes = currentShapes();
    if (selectedShapes.isEmpty()) {
        return;
    }

    KUndo2Command *command = 0;

    if (d->fillType == Fill) {
        command = new KoShapeBackgroundCommand(selectedShapes, QSharedPointer<KoShapeBackground>());
    } else {
        QList<KoShapeStrokeModelSP> strokes;

        Q_FOREACH(KoShape *shape, selectedShapes) {
            if (shape->stroke()) {
                KoShapeStrokeSP stroke = qSharedPointerDynamicCast<KoShapeStroke>(shape->stroke());
                KIS_SAFE_ASSERT_RECOVER_RETURN(stroke);

                KoShapeStrokeSP newStroke(new KoShapeStroke(*stroke));
                newStroke->setLineBrush(Qt::NoBrush);
                newStroke->setColor(Qt::transparent);
                strokes << newStroke;
            } else {
                strokes << KoShapeStrokeSP();
            }
        }

        command = new KoShapeStrokeCommand(selectedShapes, strokes);
    }

    KoCanvasController *canvasController = KoToolManager::instance()->activeCanvasController();
    canvasController->canvas()->addCommand(command);
}

void KoFillConfigWidget::colorChanged()
{
    KisAcyclicSignalConnector::Blocker b(d->shapeChangedAcyclicConnector);

    QList<KoShape*> selectedShapes = currentShapes();
    if (selectedShapes.isEmpty()) {
        return;
    }

    KUndo2Command *command = 0;

    if (d->fillType == Fill) {
        QSharedPointer<KoShapeBackground> fill(new KoColorBackground(d->colorAction->currentColor()));
        command = new KoShapeBackgroundCommand(selectedShapes, fill);
    } else {
        QList<KoShapeStrokeModelSP> strokes;

        Q_FOREACH(KoShape *shape, selectedShapes) {
            if (shape->stroke()) {
                KoShapeStrokeSP stroke = qSharedPointerDynamicCast<KoShapeStroke>(shape->stroke());
                KIS_SAFE_ASSERT_RECOVER_RETURN(stroke);

                KoShapeStrokeSP newStroke(new KoShapeStroke(*stroke));
                newStroke->setLineBrush(Qt::NoBrush);
                newStroke->setColor(d->colorAction->currentColor());
                strokes << newStroke;
            } else {
                KoShapeStrokeSP newStroke(new KoShapeStroke());
                newStroke->setColor(d->colorAction->currentColor());
                strokes << newStroke;
            }
        }

        command = new KoShapeStrokeCommand(selectedShapes, strokes);
    }


    KoCanvasController *canvasController = KoToolManager::instance()->activeCanvasController();
    canvasController->canvas()->addCommand(command);
}

template <class ResourceServer>
QString findFirstAvailableResourceName(const QString &baseName, ResourceServer *server)
{
    if (!server->resourceByName(baseName)) return baseName;

    int counter = 1;
    QString result;
    while ((result = QString("%1%2").arg(baseName).arg(counter)),
           server->resourceByName(result)) {

        counter++;
    }

    return result;
}


void KoFillConfigWidget::slotSavePredefinedGradientClicked()
{
    KoResourceServerProvider *serverProvider = KoResourceServerProvider::instance();
    auto server = serverProvider->gradientServer();

    const QString defaultGradientNamePrefix = i18nc("default prefix for the saved gradient", "gradient");

    QString name = d->activeGradient->name().isEmpty() ? defaultGradientNamePrefix : d->activeGradient->name();
    name = findFirstAvailableResourceName(name, server);
    name = QInputDialog::getText(this, i18nc("@title:window", "Save Gradient"), i18n("Enter gradient name:"), QLineEdit::Normal, name);

    // TODO: currently we do not allow the user to
    //       create two resources with the same name!
    //       Please add some feedback for it!
    name = findFirstAvailableResourceName(name, server);

    d->activeGradient->setName(name);

    const QString saveLocation = server->saveLocation();
    d->activeGradient->setFilename(saveLocation + d->activeGradient->name() + d->activeGradient->defaultFileExtension());

    KoAbstractGradient *newGradient = d->activeGradient->clone();
    server->addResource(newGradient);

    d->gradientAction->setCurrentResource(newGradient);
}

void KoFillConfigWidget::activeGradientChanged()
{
    setNewGradientBackgroundToShape();
    updateGradientSaveButtonAvailability();
}

void KoFillConfigWidget::gradientResourceChanged()
{
    QSharedPointer<KoGradientBackground> bg =
        qSharedPointerDynamicCast<KoGradientBackground>(
            d->gradientAction->currentBackground());

    uploadNewGradientBackground(bg->gradient());

    setNewGradientBackgroundToShape();
    updateGradientSaveButtonAvailability();
}

void KoFillConfigWidget::slotGradientTypeChanged()
{
    QGradient::Type type =
        d->ui->cmbGradientType->currentIndex() == 0 ?
            QGradient::LinearGradient : QGradient::RadialGradient;

    d->activeGradient->setType(type);
    activeGradientChanged();
}

void KoFillConfigWidget::slotGradientRepeatChanged()
{
    QGradient::Spread spread =
        QGradient::Spread(d->ui->cmbGradientRepeat->currentIndex());

    d->activeGradient->setSpread(spread);
    activeGradientChanged();
}

void KoFillConfigWidget::uploadNewGradientBackground(const QGradient *gradient)
{
    KisSignalsBlocker b1(d->ui->wdgGradientEditor,
                         d->ui->cmbGradientType,
                         d->ui->cmbGradientRepeat);

    d->ui->wdgGradientEditor->setGradient(0);

    d->activeGradient.reset(KoStopGradient::fromQGradient(gradient));

    d->ui->wdgGradientEditor->setGradient(d->activeGradient.data());
    d->ui->cmbGradientType->setCurrentIndex(d->activeGradient->type() != QGradient::LinearGradient);
    d->ui->cmbGradientRepeat->setCurrentIndex(int(d->activeGradient->spread()));
}

void KoFillConfigWidget::setNewGradientBackgroundToShape()
{
    QList<KoShape*> selectedShapes = currentShapes();
    if (selectedShapes.isEmpty()) {
        return;
    }

    KisAcyclicSignalConnector::Blocker b(d->shapeChangedAcyclicConnector);



    KUndo2Command *command = 0;

    if (d->fillType == Fill) {
        QList<QSharedPointer<KoShapeBackground>> newBackgrounds;

        foreach (KoShape *shape, selectedShapes) {
            newBackgrounds <<  d->applyFillGradientStops(shape, d->activeGradient.data());
        }

        command = new KoShapeBackgroundCommand(selectedShapes, newBackgrounds);

    } else {
        QList<KoShapeStrokeModelSP> strokes;

        Q_FOREACH(KoShape *shape, selectedShapes) {
            KoShapeStrokeSP stroke = shape->stroke() ? qSharedPointerDynamicCast<KoShapeStroke>(shape->stroke()) : KoShapeStrokeSP();
            strokes << d->applyFillGradientStops(stroke, d->activeGradient.data());
        }

        command = new KoShapeStrokeCommand(selectedShapes, strokes);
    }

    KoCanvasController *canvasController = KoToolManager::instance()->activeCanvasController();
    canvasController->canvas()->addCommand(command);
}

void KoFillConfigWidget::updateGradientSaveButtonAvailability()
{
    bool savingEnabled = false;

    QScopedPointer<QGradient> currentGradient(d->activeGradient->toQGradient());
    QSharedPointer<KoShapeBackground> bg = d->gradientAction->currentBackground();
    if (bg) {
        QSharedPointer<KoGradientBackground> resourceBackground =
            qSharedPointerDynamicCast<KoGradientBackground>(bg);

        savingEnabled = resourceBackground->gradient()->stops() != currentGradient->stops();
        savingEnabled |= resourceBackground->gradient()->type() != currentGradient->type();
        savingEnabled |= resourceBackground->gradient()->spread() != currentGradient->spread();
    }

    d->ui->btnSaveGradient->setEnabled(savingEnabled);
}

void KoFillConfigWidget::patternChanged(QSharedPointer<KoShapeBackground>  background)
{
    Q_UNUSED(background);

#if 0
    QSharedPointer<KoPatternBackground> patternBackground = qSharedPointerDynamicCast<KoPatternBackground>(background);
    if (! patternBackground) {
        return;
    }

    QList<KoShape*> selectedShapes = currentShapes();
    if (selectedShapes.isEmpty()) {
        return;
    }

    KoCanvasController *canvasController = KoToolManager::instance()->activeCanvasController();
    KoImageCollection *imageCollection = canvasController->canvas()->shapeController()->resourceManager()->imageCollection();
    if (imageCollection) {
        QSharedPointer<KoPatternBackground> fill(new KoPatternBackground(imageCollection));
        fill->setPattern(patternBackground->pattern());
        canvasController->canvas()->addCommand(new KoShapeBackgroundCommand(selectedShapes, fill));
    }
#endif
}

void KoFillConfigWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    shapeChanged();
}

struct ShapeFillFetchPolicyBase
{
    enum Type {
        None = 0,
        Solid,
        Gradient,
        Pattern
    };
};

struct ShapeBackgroundFetchPolicy : public ShapeFillFetchPolicyBase
{
    typedef QSharedPointer<KoShapeBackground> PointerType;
    static PointerType getBackground(KoShape *shape) {
        return shape->background();
    }
    static Type type(KoShape *shape) {
        QSharedPointer<KoShapeBackground> background = shape->background();
        QSharedPointer<KoColorBackground> colorBackground = qSharedPointerDynamicCast<KoColorBackground>(background);
        QSharedPointer<KoGradientBackground> gradientBackground = qSharedPointerDynamicCast<KoGradientBackground>(background);
        QSharedPointer<KoPatternBackground> patternBackground = qSharedPointerDynamicCast<KoPatternBackground>(background);

        return colorBackground ? Solid :
               gradientBackground ? Gradient :
               patternBackground ? Pattern : None;
    }

    static QColor color(KoShape *shape) {
        QSharedPointer<KoColorBackground> colorBackground = qSharedPointerDynamicCast<KoColorBackground>(shape->background());
        return colorBackground ? colorBackground->color() : QColor();
    }

    static const QGradient* gradient(KoShape *shape) {
        QSharedPointer<KoGradientBackground> gradientBackground = qSharedPointerDynamicCast<KoGradientBackground>(shape->background());
        return gradientBackground ? gradientBackground->gradient() : 0;
    }
};

struct ShapeStrokeFillFetchPolicy : public ShapeFillFetchPolicyBase
{
    typedef KoShapeStrokeModelSP PointerType;
    static PointerType getBackground(KoShape *shape) {
        return shape->stroke();
    }
    static Type type(KoShape *shape) {
        KoShapeStrokeSP stroke = qSharedPointerDynamicCast<KoShapeStroke>(shape->stroke());
        if (!stroke) return None;

        // TODO: patterns are not supported yet!
        return stroke->lineBrush().gradient() ? Gradient : Solid;
    }

    static QColor color(KoShape *shape) {
        KoShapeStrokeSP stroke = qSharedPointerDynamicCast<KoShapeStroke>(shape->stroke());
        return stroke ? stroke->color() : QColor();
    }

    static const QGradient* gradient(KoShape *shape) {
        KoShapeStrokeSP stroke = qSharedPointerDynamicCast<KoShapeStroke>(shape->stroke());
        return stroke ? stroke->lineBrush().gradient() : 0;
    }
};

template <class Policy>
bool compareBackgrounds(const QList<KoShape*> shapes)
{
    typename Policy::PointerType bg =
        Policy::getBackground(shapes.first());

    Q_FOREACH (KoShape *shape, shapes) {
        if (
            !(
              (!bg && !Policy::getBackground(shape)) ||
              (bg && bg->compareTo(Policy::getBackground(shape).data()))
             )) {

            return false;
        }
    }

    return true;
}

void KoFillConfigWidget::shapeChanged()
{
    if (d->fillType == Fill) {
        shapeChangedImpl<ShapeBackgroundFetchPolicy>();

    } else {
        shapeChangedImpl<ShapeStrokeFillFetchPolicy>();
    }
}

template <class Policy>
void KoFillConfigWidget::shapeChangedImpl()
{
    if (!isVisible()) return;

    QList<KoShape*> shapes = currentShapes();

    if (shapes.isEmpty() || (shapes.size() > 1 && !compareBackgrounds<Policy>(shapes))) {

        Q_FOREACH (QAbstractButton *button, d->group->buttons()) {
            button->setEnabled(!shapes.isEmpty());
        }

        d->group->button(None)->setChecked(true);
        d->ui->stackWidget->setCurrentIndex(None);

    } else {
        Q_FOREACH (QAbstractButton *button, d->group->buttons()) {
            button->setEnabled(true);
        }

        KoShape *shape = shapes.first();
        updateWidget<Policy>(shape);
    }
}

template <class Policy>
void KoFillConfigWidget::updateWidget(KoShape *shape)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(shape);

    StyleButton newActiveButton = None;

    switch (Policy::type(shape)) {
    case None:
        break;
    case Solid:
        newActiveButton = KoFillConfigWidget::Solid;
        d->colorAction->setCurrentColor(Policy::color(shape));
        break;
    case Gradient:
        newActiveButton = KoFillConfigWidget::Gradient;
        uploadNewGradientBackground(Policy::gradient(shape));
        updateGradientSaveButtonAvailability();
        break;
    case Pattern:
        newActiveButton = KoFillConfigWidget::Pattern;
        break;
    }

    d->group->button(newActiveButton)->setChecked(true);
    d->ui->stackWidget->setCurrentIndex(newActiveButton);
}
