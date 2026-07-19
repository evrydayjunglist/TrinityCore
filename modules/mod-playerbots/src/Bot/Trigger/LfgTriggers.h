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

#ifndef TRINITY_PLAYERBOT_LFG_TRIGGERS_H
#define TRINITY_PLAYERBOT_LFG_TRIGGERS_H

#include "Trigger.h"

class BotPlayerbotAI;

// Enable=0 / lost-signal twin for SMSG_LFG_PROPOSAL_UPDATE.
// Packet path uses SignalTrigger("lfg proposal"); this covers PayloadParse off after a
// prior Layer-2 stash, or a lost pending signal while still in LFG_STATE_PROPOSAL.
// Accept still requires a stashed ProposalID (no public LFGMgr proposal-id lookup).
class LfgProposalActiveTrigger : public Trigger
{
public:
    explicit LfgProposalActiveTrigger(BotPlayerbotAI* botAI) : Trigger(botAI, "lfg proposal active") { }

    bool IsActive() override;
};

#endif
