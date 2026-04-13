/// Interactive Trading Simulator
/// Uses the ACTUAL book code (PositionInfo, FeatureEngine logic, etc.)
/// Type commands to change market and watch MM/LT react.
///
/// Build:  cmake .. && make demo_interactive
/// Run:    ./demo_interactive

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <cmath>
#include <array>

#include "common/types.h"
#include "common/logging.h"
#include "trading/strategy/position_keeper.h"
#include "trading/strategy/market_order.h"
#include "exchange/order_server/client_response.h"

using namespace Common;
using namespace Trading;
using namespace Exchange;

// ── ANSI Colors ──
const char* RST  = "\033[0m";
const char* BOLD = "\033[1m";
const char* DIM  = "\033[2m";
const char* GRN  = "\033[32m";
const char* RED  = "\033[31m";
const char* BLU  = "\033[34m";
const char* CYN  = "\033[36m";
const char* YEL  = "\033[33m";
const char* MAG  = "\033[35m";
const char* BGRN = "\033[1;32m";
const char* BRED = "\033[1;31m";
const char* BCYN = "\033[1;36m";
const char* BBLU = "\033[1;34m";
const char* BYEL = "\033[1;33m";
const char* BMAG = "\033[1;35m";

// ── Market State ──
struct Market {
    Price bid = 100, ask = 102;
    Qty bqty = 200, aqty = 200;

    double fairPrice() const {
        return (bid * aqty + ask * bqty) / static_cast<double>(bqty + aqty);
    }
    double mid() const { return (bid + ask) * 0.5; }
};

// ── MM Order State ──
struct SimpleOrder {
    bool active = false;
    Side side = Side::INVALID;
    Price price = 0;
    Qty qty = 0;
};

// ── Global State ──
Market mkt;
Common::Logger* logger;

// MM state - uses REAL PositionInfo from book
PositionInfo mmPosition{};
SimpleOrder mmBid, mmAsk;
double mmThreshold = 0.6;
Qty mmClip = 50;

// LT state - uses REAL PositionInfo from book
PositionInfo ltPosition{};
SimpleOrder ltBid, ltAsk;
double ltThreshold = 0.5;
Qty ltClip = 50;

int stepNum = 0;

// ── Helpers ──
const char* pnlColor(double v) { return v > 0 ? BGRN : v < 0 ? BRED : DIM; }
const char* posColor(int v) { return v > 0 ? GRN : v < 0 ? RED : DIM; }

void printDivider(const char* c = "─") {
    for(int i = 0; i < 78; i++) std::cout << c;
    std::cout << "\n";
}

// ── Display Functions ──
void showBBO() {
    std::cout << "\n" << BCYN << "  ┌─ MARKET (BBO) ";
    printDivider("─");
    std::cout << "  │  " << BGRN << mkt.bqty << " × " << mkt.bid << RST
              << DIM << "  ◄── bid   ask ──►  " << RST
              << BRED << mkt.ask << " × " << mkt.aqty << RST
              << DIM << "   spread=" << (mkt.ask - mkt.bid) << RST << "\n";
    std::cout << BCYN << "  └─ " << RST
              << DIM << "fair_price = " << RST << BYEL
              << std::fixed << std::setprecision(2) << mkt.fairPrice() << RST
              << DIM << "  mid = " << mkt.mid() << RST << "\n";
}

void showStrategy(const char* name, const char* color,
                  const PositionInfo& pos,
                  const SimpleOrder& bid, const SimpleOrder& ask) {
    auto buyIdx = sideToIndex(Side::BUY);
    auto sellIdx = sideToIndex(Side::SELL);
    double vwap = 0;
    if(pos.position_ > 0) vwap = pos.open_vwap_.at(buyIdx) / std::abs(pos.position_);
    else if(pos.position_ < 0) vwap = pos.open_vwap_.at(sellIdx) / std::abs(pos.position_);

    std::cout << "\n" << color << BOLD << "  ┌─ " << name << " " << RST;
    printDivider("─");

    // Orders
    std::cout << "  │  Orders: ";
    if(bid.active)
        std::cout << BGRN << "BID " << bid.qty << "@" << bid.price << RST << "  ";
    else
        std::cout << DIM << "BID ---" << RST << "  ";
    if(ask.active)
        std::cout << BRED << "ASK " << ask.qty << "@" << ask.price << RST;
    else
        std::cout << DIM << "ASK ---" << RST;
    std::cout << "\n";

    // Position
    std::cout << "  │  Position: " << posColor(pos.position_) << BOLD << pos.position_ << RST;
    if(pos.position_) std::cout << DIM << "  VWAP=" << std::fixed << std::setprecision(2) << vwap << RST;
    std::cout << "\n";

    // PnL
    std::cout << "  │  PnL: "
              << DIM << "real=" << RST << pnlColor(pos.real_pnl_) << std::setprecision(1) << pos.real_pnl_ << RST
              << DIM << "  unreal=" << RST << pnlColor(pos.unreal_pnl_) << pos.unreal_pnl_ << RST
              << DIM << "  total=" << RST << pnlColor(pos.total_pnl_) << BOLD << pos.total_pnl_ << RST
              << "\n";
    std::cout << color << "  └" << RST << "\n";
}

// ── MarketMaker Logic (uses book's FeatureEngine formulas) ──
void mmUpdate() {
    double fair = mkt.fairPrice();
    double bidGap = fair - mkt.bid;
    double askGap = mkt.ask - fair;

    // Exact same logic as book's MarketMaker::onOrderBookUpdate()
    Price bidPrice = mkt.bid - (bidGap >= mmThreshold ? 0 : 1);
    Price askPrice = mkt.ask + (askGap >= mmThreshold ? 0 : 1);

    mmBid = {true, Side::BUY, bidPrice, mmClip};
    mmAsk = {true, Side::SELL, askPrice, mmClip};

    std::cout << BBLU << "  [MM] " << RST
              << "fair=" << std::setprecision(2) << fair
              << "  bidGap=" << bidGap << (bidGap >= mmThreshold ? " ≥ " : " < ") << mmThreshold
              << " → " << (bidGap >= mmThreshold ? "JOIN" : "BACK")
              << "  askGap=" << askGap << (askGap >= mmThreshold ? " ≥ " : " < ") << mmThreshold
              << " → " << (askGap >= mmThreshold ? "JOIN" : "BACK")
              << "  ⇒ bid=" << bidPrice << " ask=" << askPrice << "\n";

    // Update unrealized PnL using REAL updateBBO()
    BBO bbo;
    bbo.bid_price_ = mkt.bid; bbo.bid_qty_ = mkt.bqty;
    bbo.ask_price_ = mkt.ask; bbo.ask_qty_ = mkt.aqty;
    mmPosition.updateBBO(&bbo, logger);
}

// ── LiquidityTaker Logic ──
void ltOnTrade(Side aggSide, Qty tradeQty) {
    // Exact same logic as book's FeatureEngine::onTradeUpdate()
    double ratio = static_cast<double>(tradeQty) /
        (aggSide == Side::BUY ? mkt.aqty : mkt.bqty);

    std::cout << BYEL << "  [LT] " << RST
              << "agg_ratio=" << std::setprecision(2) << ratio
              << (ratio >= ltThreshold ? " ≥ " : " < ") << ltThreshold;

    // Exact same logic as book's LiquidityTaker::onTradeUpdate()
    if(ratio >= ltThreshold) {
        if(aggSide == Side::BUY) {
            // Follow buy: cross spread
            std::cout << " → " << BGRN << "FOLLOW BUY " << ltClip << "@" << mkt.ask << RST << "\n";
            ltBid = {true, Side::BUY, mkt.ask, ltClip};
            ltAsk = {false, Side::SELL, 0, 0};

            // Use REAL PositionInfo::addFill()
            MEClientResponse fill{};
            fill.type_ = ClientResponseType::FILLED;
            fill.side_ = Side::BUY;
            fill.price_ = mkt.ask;
            fill.exec_qty_ = ltClip;
            fill.leaves_qty_ = 0;
            ltPosition.addFill(&fill, logger);
        } else {
            std::cout << " → " << BRED << "FOLLOW SELL " << ltClip << "@" << mkt.bid << RST << "\n";
            ltBid = {false, Side::BUY, 0, 0};
            ltAsk = {true, Side::SELL, mkt.bid, ltClip};

            MEClientResponse fill{};
            fill.type_ = ClientResponseType::FILLED;
            fill.side_ = Side::SELL;
            fill.price_ = mkt.bid;
            fill.exec_qty_ = ltClip;
            fill.leaves_qty_ = 0;
            ltPosition.addFill(&fill, logger);
        }
    } else {
        std::cout << " → " << DIM << "SKIP (not aggressive enough)" << RST << "\n";
    }

    // Also check if MM got filled (passive fill)
    if(aggSide == Side::BUY && mmAsk.active) {
        Qty fillQty = std::min(tradeQty, mmAsk.qty);
        std::cout << BBLU << "  [MM] " << RST << "PASSIVE FILL: SELL " << fillQty << "@" << mmAsk.price << "\n";

        MEClientResponse fill{};
        fill.type_ = ClientResponseType::FILLED;
        fill.side_ = Side::SELL;
        fill.price_ = mmAsk.price;
        fill.exec_qty_ = fillQty;
        fill.leaves_qty_ = 0;
        mmPosition.addFill(&fill, logger);
    } else if(aggSide == Side::SELL && mmBid.active) {
        Qty fillQty = std::min(tradeQty, mmBid.qty);
        std::cout << BBLU << "  [MM] " << RST << "PASSIVE FILL: BUY " << fillQty << "@" << mmBid.price << "\n";

        MEClientResponse fill{};
        fill.type_ = ClientResponseType::FILLED;
        fill.side_ = Side::BUY;
        fill.price_ = mmBid.price;
        fill.exec_qty_ = fillQty;
        fill.leaves_qty_ = 0;
        mmPosition.addFill(&fill, logger);
    }
}

void showAll() {
    showBBO();
    showStrategy("MarketMaker (passive, both sides)", BBLU, mmPosition, mmBid, mmAsk);
    showStrategy("LiquidityTaker (aggressive, one side)", BYEL, ltPosition, ltBid, ltAsk);
}

void showHelp() {
    std::cout << "\n" << BOLD << "Commands:" << RST << "\n";
    std::cout << DIM << "  Market controls:" << RST << "\n";
    std::cout << "    bid +1 / bid -1       Change bid price\n";
    std::cout << "    ask +1 / ask -1       Change ask price\n";
    std::cout << "    bqty +100 / bqty -100 Change bid quantity\n";
    std::cout << "    aqty +100 / aqty -100 Change ask quantity\n";
    std::cout << DIM << "  Trade generation:" << RST << "\n";
    std::cout << "    buy 80                Someone buys 80 (triggers LT if big enough)\n";
    std::cout << "    sell 80               Someone sells 80\n";
    std::cout << DIM << "  Presets:" << RST << "\n";
    std::cout << "    buypressure           bid=100@500, ask=102@50\n";
    std::cout << "    sellpressure          bid=100@50,  ask=102@500\n";
    std::cout << "    balanced              bid=100@200, ask=102@200\n";
    std::cout << "    reset                 Reset everything\n";
    std::cout << DIM << "  Config:" << RST << "\n";
    std::cout << "    mmthresh 0.8          Set MM threshold\n";
    std::cout << "    ltthresh 0.3          Set LT threshold\n";
    std::cout << DIM << "  Other:" << RST << "\n";
    std::cout << "    show                  Refresh display\n";
    std::cout << "    help                  Show this help\n";
    std::cout << "    quit                  Exit\n";
}

int main() {
    logger = new Common::Logger("demo_interactive.log");

    // Initialize open_vwap arrays (not zero-initialized by default!)
    mmPosition.open_vwap_.fill(0);
    ltPosition.open_vwap_.fill(0);

    std::cout << BOLD << "\n";
    std::cout << "  ╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "  ║   HFT Interactive Simulator (using real C++ book code)  ║\n";
    std::cout << "  ║   MarketMaker + LiquidityTaker side by side             ║\n";
    std::cout << "  ╚══════════════════════════════════════════════════════════╝\n";
    std::cout << RST;
    showHelp();

    mmUpdate();
    showAll();

    std::string line;
    while(true) {
        std::cout << "\n" << BCYN << ">> " << RST;
        if(!std::getline(std::cin, line)) break;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if(cmd.empty()) continue;

        stepNum++;
        std::cout << DIM << "─── Step " << stepNum << " ───" << RST << "\n";

        if(cmd == "quit" || cmd == "q" || cmd == "exit") break;
        else if(cmd == "help" || cmd == "h") { showHelp(); continue; }
        else if(cmd == "show" || cmd == "s") { showAll(); continue; }
        else if(cmd == "bid") {
            int d; iss >> d;
            mkt.bid += d;
            if(mkt.bid >= mkt.ask) mkt.bid = mkt.ask - 1;
            std::cout << BCYN << "  [MKT] " << RST << "Bid price → " << mkt.bid << "\n";
            mmUpdate();
        }
        else if(cmd == "ask") {
            int d; iss >> d;
            mkt.ask += d;
            if(mkt.ask <= mkt.bid) mkt.ask = mkt.bid + 1;
            std::cout << BCYN << "  [MKT] " << RST << "Ask price → " << mkt.ask << "\n";
            mmUpdate();
        }
        else if(cmd == "bqty") {
            int d; iss >> d;
            mkt.bqty = std::max(Qty(10), Qty(int(mkt.bqty) + d));
            std::cout << BCYN << "  [MKT] " << RST << "Bid qty → " << mkt.bqty << "\n";
            mmUpdate();
        }
        else if(cmd == "aqty") {
            int d; iss >> d;
            mkt.aqty = std::max(Qty(10), Qty(int(mkt.aqty) + d));
            std::cout << BCYN << "  [MKT] " << RST << "Ask qty → " << mkt.aqty << "\n";
            mmUpdate();
        }
        else if(cmd == "buy") {
            Qty qty; iss >> qty;
            std::cout << BMAG << "  [TRADE] " << RST << "BUY " << qty << " @ " << mkt.ask << "\n";
            ltOnTrade(Side::BUY, qty);
            mmUpdate();
        }
        else if(cmd == "sell") {
            Qty qty; iss >> qty;
            std::cout << BMAG << "  [TRADE] " << RST << "SELL " << qty << " @ " << mkt.bid << "\n";
            ltOnTrade(Side::SELL, qty);
            mmUpdate();
        }
        else if(cmd == "buypressure") {
            mkt = {100, 102, 500, 50};
            std::cout << BCYN << "  [MKT] " << RST << "Preset: buy pressure (bid@500, ask@50)\n";
            mmUpdate();
        }
        else if(cmd == "sellpressure") {
            mkt = {100, 102, 50, 500};
            std::cout << BCYN << "  [MKT] " << RST << "Preset: sell pressure (bid@50, ask@500)\n";
            mmUpdate();
        }
        else if(cmd == "balanced") {
            mkt = {100, 102, 200, 200};
            std::cout << BCYN << "  [MKT] " << RST << "Preset: balanced\n";
            mmUpdate();
        }
        else if(cmd == "reset") {
            mkt = {100, 102, 200, 200};
            mmPosition = PositionInfo{}; mmPosition.open_vwap_.fill(0);
            ltPosition = PositionInfo{}; ltPosition.open_vwap_.fill(0);
            mmBid = mmAsk = {}; ltBid = ltAsk = {};
            stepNum = 0;
            std::cout << BCYN << "  [MKT] " << RST << "Everything reset\n";
            mmUpdate();
        }
        else if(cmd == "mmthresh") { iss >> mmThreshold; std::cout << "  MM threshold → " << mmThreshold << "\n"; mmUpdate(); }
        else if(cmd == "ltthresh") { iss >> ltThreshold; std::cout << "  LT threshold → " << ltThreshold << "\n"; }
        else {
            std::cout << RED << "  Unknown command: " << cmd << ". Type 'help' for commands." << RST << "\n";
            continue;
        }

        showAll();
    }

    std::cout << "\nBye!\n";
    delete logger;
    return 0;
}
