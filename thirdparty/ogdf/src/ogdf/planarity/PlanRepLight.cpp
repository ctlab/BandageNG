/** \file
 * \brief Implementation of class PlanRepLight.
 *
 * \author Carsten Gutwenger
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.md in the OGDF root directory for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, see
 * http://www.gnu.org/copyleft/gpl.html
 */


#include <ogdf/planarity/PlanRepLight.h>

namespace ogdf {


PlanRepLight::PlanRepLight(const PlanRep& pr)
	: m_ccInfo(pr.ccInfo()), m_pr(pr), m_currentCC(-1), m_eAuxCopy(pr.original()) {
	GraphCopy::createEmpty(pr.original());
}

void PlanRepLight::initCC(int cc) {
	if (m_currentCC >= 0) {
		for (int i = m_ccInfo.startNode(m_currentCC); i < m_ccInfo.stopNode(m_currentCC); ++i) {
			m_vCopy[m_ccInfo.v(i)] = nullptr;
		}

		for (int i = m_ccInfo.startEdge(m_currentCC); i < m_ccInfo.stopEdge(m_currentCC); ++i) {
			m_eCopy[m_ccInfo.e(i)].clear();
		}
	}

	m_currentCC = cc;
	initByCC(m_ccInfo, cc, m_eAuxCopy);
}

}
