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

#include "TradeActions.h"
#include "BotPlayerbotAI.h"
#include "Log.h"
#include "Opcodes.h"
#include "Player.h"
#include "PlayerbotsConfig.h"
#include "SharedDefines.h"
#include "TradeData.h"
#include "TradePackets.h"
#include "WorldPacket.h"
#include "WorldSession.h"

namespace
{
bool ResolveMasterTrade(BotPlayerbotAI* botAI, Player*& bot, Player*& master, TradeData*& trade)
{
    if (!botAI)
        return false;

    bot = botAI->GetBot();
    master = botAI->GetMaster();
    if (!bot || !bot->GetSession() || !master)
        return false;

    trade = bot->GetTradeData();
    if (!trade)
        return false;

    Player* trader = trade->GetTrader();
    return trader && trader == master;
}
}

bool AcceptTradeAction::IsUseful()
{
    Player* bot = nullptr;
    Player* master = nullptr;
    TradeData* trade = nullptr;
    if (!ResolveMasterTrade(_botAI, bot, master, trade))
        return false;

    std::optional<::TradeStatus> const pending = _botAI->GetPendingTradeStatus();
    if (!pending)
        return false;

    return *pending == TRADE_STATUS_PROPOSED || *pending == TRADE_STATUS_ACCEPTED;
}

bool AcceptTradeAction::Execute(Event /*event*/)
{
    Player* bot = nullptr;
    Player* master = nullptr;
    TradeData* trade = nullptr;
    if (!ResolveMasterTrade(_botAI, bot, master, trade))
        return false;

    std::optional<::TradeStatus> const pending = _botAI->GetPendingTradeStatus();
    if (!pending)
        return false;

    ::TradeStatus const status = *pending;
    _botAI->ClearPendingTradeStatus();

    if (status == TRADE_STATUS_PROPOSED)
    {
        // Midnight: CMSG_BEGIN_TRADE → both sides TRADE_STATUS_INITIATED.
        WorldPacket packet(CMSG_BEGIN_TRADE);
        WorldPackets::Trade::BeginTrade begin(std::move(packet));
        bot->GetSession()->HandleBeginTradeOpcode(begin);

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots", "AcceptTradeAction bot={} master={} begin=yes",
                bot->GetName(), master->GetName());
        return true;
    }

    if (status == TRADE_STATUS_ACCEPTED)
    {
        // Re-resolve: begin/other ticks may have replaced TradeData.
        trade = bot->GetTradeData();
        if (!trade || trade->GetTrader() != master)
            return false;

        // HandleAcceptTradeOpcode compares StateIndex to the *trader's*
        // GetServerStateIndex() (his_trade), not the acceptor's. Money/item
        // changes call UpdateServerStateIndex() only on the side that changed,
        // so using the bot's own index (often still 1) yields STATE_CHANGED.
        TradeData* traderTrade = trade->GetTraderData();
        if (!traderTrade)
            return false;

        WorldPacket packet(CMSG_ACCEPT_TRADE);
        WorldPackets::Trade::AcceptTrade accept(std::move(packet));
        accept.StateIndex = traderTrade->GetServerStateIndex();
        bot->GetSession()->HandleAcceptTradeOpcode(accept);

        if (Playerbots::GetLogLevel() >= 1)
            TC_LOG_DEBUG("playerbots", "AcceptTradeAction bot={} master={} accept=yes stateIndex={}",
                bot->GetName(), master->GetName(), accept.StateIndex);
        return true;
    }

    return false;
}
