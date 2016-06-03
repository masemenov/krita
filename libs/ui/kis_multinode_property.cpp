/*
 *  Copyright (c) 2016 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_multinode_property.h"

/******************************************************************/
/*               MultinodePropertyConnectorInterface              */
/******************************************************************/

MultinodePropertyConnectorInterface::~MultinodePropertyConnectorInterface()
{
}

void MultinodePropertyConnectorInterface::connectValueChangedSignal(const QObject *receiver, const char *method, Qt::ConnectionType type) {
    connect(this, SIGNAL(sigValueChanged()), receiver, method, type);
    notifyValueChanged();
}

void MultinodePropertyConnectorInterface::notifyValueChanged() {
    emit sigValueChanged();
}

void MultinodePropertyConnectorInterface::connectAutoEnableWidget(QWidget *widget)
{
    Q_UNUSED(widget);
}

/******************************************************************/
/*               MultinodePropertyBaseConnector                   */
/******************************************************************/

MultinodePropertyBaseConnector::MultinodePropertyBaseConnector(KisMultinodePropertyInterface *parent)
    : m_parent(parent)
{
}

void MultinodePropertyBaseConnector::connectIgnoreCheckBox(QCheckBox *ignoreBox) {
    m_ignoreBox = ignoreBox;

    if (!m_parent->isIgnored() && !m_parent->savedValuesDiffer()) {
        m_ignoreBox->setEnabled(false);
        m_ignoreBox->setChecked(true);

        if (m_parent->haveTheOnlyNode()) {
            m_ignoreBox->setVisible(false);
        }
    } else {
        connect(m_ignoreBox, SIGNAL(stateChanged(int)), SLOT(slotIgnoreCheckBoxChanged(int)));
        m_ignoreBox->setEnabled(true);
        m_ignoreBox->setChecked(!m_parent->isIgnored());
    }
}

#include <QEvent>

struct AutoEnabler : public QObject {
    Q_OBJECT
public:
    AutoEnabler(QObject *watched, KisMultinodePropertyInterface *property, QObject *parent)
        : QObject(parent), m_watched(watched), m_property(property)
    {
        watched->installEventFilter(this);
    }

    bool eventFilter(QObject *watched, QEvent * event) {
        if (watched != m_watched) return false;
        if (!m_property->isIgnored()) return false;

        if (event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::TabletPress) {

            emit enableWidget(true);
        }

        return false;
    }
Q_SIGNALS:
    void enableWidget(bool value);

private:
    QObject *m_watched;
    KisMultinodePropertyInterface *m_property;
};

void MultinodePropertyBaseConnector::connectAutoEnableWidget(QWidget *widget)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(m_ignoreBox);

    AutoEnabler *enabler = new AutoEnabler(widget, m_parent, this);
    connect(enabler, SIGNAL(enableWidget(bool)), m_ignoreBox, SLOT(setChecked(bool)));
}

void MultinodePropertyBaseConnector::slotIgnoreCheckBoxChanged(int state) {
    m_parent->setIgnored(state != Qt::Checked);
}

void MultinodePropertyBaseConnector::notifyIgnoreChanged() {
    if (!m_ignoreBox) return;

    if (m_ignoreBox->isChecked() != !m_parent->isIgnored()) {
        m_ignoreBox->setChecked(!m_parent->isIgnored());
    }
}

/******************************************************************/
/*               KisMultinodePropertyInterface                    */
/******************************************************************/

KisMultinodePropertyInterface::KisMultinodePropertyInterface()
{
}

KisMultinodePropertyInterface::~KisMultinodePropertyInterface()
{
}

#include "kis_multinode_property.moc"
