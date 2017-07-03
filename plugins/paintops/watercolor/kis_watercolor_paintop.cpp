/* This file is part of the KDE project
 *
 * Copyright (C) 2017 Grigory Tantsevov <tantsevov@gmail.com>
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

#include "kis_watercolor_paintop.h"
#include "kis_watercolor_base_items.h"

#include <krita_utils.h>

KisWatercolorPaintOp::KisWatercolorPaintOp(const KisPaintOpSettingsSP settings, KisPainter *painter, KisNodeSP node, KisImageSP image)
    : KisPaintOp(painter)
{
    Q_UNUSED(image);
    Q_UNUSED(node);

    m_watercolorOption.readOptionSetting(settings);
}

KisSpacingInformation KisWatercolorPaintOp::paintAt(const KisPaintInformation &info)
{
    KoColor clr;
    clr.fromQColor(QColor(Qt::red));
    KisWatercolorBaseItems::instance()->paint(info.pos(), 25, clr);

    return KisSpacingInformation(1.0);
}