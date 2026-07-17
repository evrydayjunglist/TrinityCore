/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "PassiveStrategy.h"

// Gate 6 idle baseline (no movement/combat). Lifecycle accepts for AC botOutgoing parity
// so GM `.playerbot login` / non-RPG random bots still react (signal + poll).

void PassiveStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode("resurrect request signal", { NextAction("accept resurrect", 100.0f) }));
    triggers.push_back(new TriggerNode("resurrect request", { NextAction("accept resurrect", 100.0f) }));

    // Guild charter offer — same-account master-alts cannot use client Request Signature;
    // reserved-account login bots are the practical offer/sign playtest path.
    triggers.push_back(new TriggerNode("petition offer signal", { NextAction("petition sign", 100.0f) }));
    triggers.push_back(new TriggerNode("petition offer", { NextAction("petition sign", 100.0f) }));
}
