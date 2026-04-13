/// Full Market Simulator: 2 MarketMakers + 2 LiquidityTakers + 1 Random
/// All using REAL book code (PositionInfo::addFill, updateBBO)
///
/// Build:  cmake .. && make demo_full_market
/// Run:    ./demo_full_market

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>

#include "common/types.h"
#include "common/logging.h"
#include "trading/strategy/position_keeper.h"
#include "trading/strategy/market_order.h"
#include "exchange/order_server/client_response.h"

using namespace Common;
using namespace Trading;
using namespace Exchange;

// ── ANSI Colors ──
const char* RST="\033[0m"; const char* BOLD="\033[1m"; const char* DIM="\033[2m";
const char* GRN="\033[32m"; const char* RED="\033[31m"; const char* BLU="\033[34m";
const char* CYN="\033[36m"; const char* YEL="\033[33m"; const char* MAG="\033[35m"; const char* WHT="\033[37m";
const char* BGRN="\033[1;32m"; const char* BRED="\033[1;31m"; const char* BCYN="\033[1;36m";
const char* BBLU="\033[1;34m"; const char* BYEL="\033[1;33m"; const char* BMAG="\033[1;35m";
const char* BWHT="\033[1;37m";

// ── Market ──
struct Market {
    Price bid=100, ask=102;
    Qty bqty=300, aqty=300;
    double fairPrice() const { return (bid*aqty+ask*bqty)/static_cast<double>(bqty+aqty); }
    double mid() const { return (bid+ask)*0.5; }
};

// ── Order ──
struct SimpleOrder { bool active=false; Price price=0; Qty qty=0; };

// ── Participant ──
struct Participant {
    std::string name;
    const char* color;
    const char* tag;  // MM-A, MM-B, LT-1, LT-2, RND
    PositionInfo pos{};
    SimpleOrder bid_order{}, ask_order{};
    double threshold=0;
    Qty clip=0;
    enum Type { MAKER, TAKER, RANDOM_T } type;

    void init() { pos.open_vwap_.fill(0); }

    void fill(Side side, Price price, Qty qty, Common::Logger* logger) {
        MEClientResponse r{};
        r.type_ = ClientResponseType::FILLED;
        r.side_ = side; r.price_ = price;
        r.exec_qty_ = qty; r.leaves_qty_ = 0;
        pos.addFill(&r, logger);
    }

    void markToMarket(const BBO& bbo, Common::Logger* logger) {
        BBO b = bbo;
        pos.updateBBO(&b, logger);
    }

    double vwap() const {
        if(!pos.position_) return 0;
        if(pos.position_>0) return pos.open_vwap_.at(sideToIndex(Side::BUY))/std::abs(pos.position_);
        return pos.open_vwap_.at(sideToIndex(Side::SELL))/std::abs(pos.position_);
    }
};

// ── Globals ──
Market mkt;
Common::Logger* logger;
int stepNum = 0;

Participant participants[5] = {
    {"MarketMaker-A",  BBLU, "MM-A", {}, {}, {}, 0.6,  50, Participant::MAKER},
    {"MarketMaker-B",  BCYN, "MM-B", {}, {}, {}, 0.8,  40, Participant::MAKER},
    {"LiquidityTaker-1", BYEL, "LT-1", {}, {}, {}, 0.5, 50, Participant::TAKER},
    {"LiquidityTaker-2", BMAG, "LT-2", {}, {}, {}, 0.7, 30, Participant::TAKER},
    {"Random",         BWHT, "RND",  {}, {}, {}, 0,    0,  Participant::RANDOM_T},
};

const char* pnlC(double v){return v>0.5?BGRN:v<-0.5?BRED:DIM;}
const char* posC(int v){return v>0?GRN:v<0?RED:DIM;}

// ── MarketMaker pricing ──
void mmUpdate(Participant& mm) {
    double fair = mkt.fairPrice();
    double bidGap = fair - mkt.bid;
    double askGap = mkt.ask - fair;
    Price bp = mkt.bid - (bidGap >= mm.threshold ? 0 : 1);
    Price ap = mkt.ask + (askGap >= mm.threshold ? 0 : 1);
    mm.bid_order = {true, bp, mm.clip};
    mm.ask_order = {true, ap, mm.clip};

    std::cout << mm.color << "  [" << mm.tag << "] " << RST
              << "fair=" << std::fixed << std::setprecision(2) << fair
              << " bid=" << bp << (bidGap>=mm.threshold?" (JOIN)":" (BACK)")
              << " ask=" << ap << (askGap>=mm.threshold?" (JOIN)":" (BACK)")
              << DIM << " [thresh=" << mm.threshold << " clip=" << mm.clip << "]" << RST << "\n";
}

void updateAllMMs() {
    for(auto& p : participants)
        if(p.type == Participant::MAKER) mmUpdate(p);
}

void markAllToMarket() {
    BBO bbo; bbo.bid_price_=mkt.bid; bbo.bid_qty_=mkt.bqty;
    bbo.ask_price_=mkt.ask; bbo.ask_qty_=mkt.aqty;
    for(auto& p : participants) p.markToMarket(bbo, logger);
}

// ── Trade execution: match against best MM ──
void executeTrade(Side aggSide, Qty tradeQty, Participant* aggressor) {
    // ── Gather MM liquidity ──
    std::vector<Participant*> mms;
    for(auto& p : participants) {
        if(p.type != Participant::MAKER) continue;
        if(aggSide==Side::BUY && p.ask_order.active) mms.push_back(&p);
        if(aggSide==Side::SELL && p.bid_order.active) mms.push_back(&p);
    }
    // Sort: buy -> lowest ask first; sell -> highest bid first
    if(aggSide==Side::BUY)
        std::sort(mms.begin(), mms.end(), [](auto* a, auto* b){return a->ask_order.price < b->ask_order.price;});
    else
        std::sort(mms.begin(), mms.end(), [](auto* a, auto* b){return a->bid_order.price > b->bid_order.price;});

    Qty mmLiquidity = 0;
    for(auto* mm : mms)
        mmLiquidity += (aggSide==Side::BUY ? mm->ask_order.qty : mm->bid_order.qty);

    Qty actualQty = std::min(tradeQty, mmLiquidity);
    if(actualQty < tradeQty) {
        std::cout << DIM << "         requested " << tradeQty
                  << " but only " << mmLiquidity << " available -> filled " << actualQty << RST << "\n";
    }

    // Compute agg ratio (using original requested qty for signal strength)
    double ratio = static_cast<double>(tradeQty) /
        (aggSide==Side::BUY ? mkt.aqty : mkt.bqty);

    std::cout << BMAG << "  [TRADE] " << RST << sideToString(aggSide)
              << " " << actualQty << " @ market\n";
    std::cout << DIM << "         agg_ratio = " << std::setprecision(2) << ratio << RST << "\n";

    // ── Match aggressor against MMs ──
    Qty remaining = actualQty;
    for(auto* mm : mms) {
        if(remaining <= 0) break;
        Qty orderQty = (aggSide==Side::BUY) ? mm->ask_order.qty : mm->bid_order.qty;
        Price matchPrice = (aggSide==Side::BUY) ? mm->ask_order.price : mm->bid_order.price;
        Qty fillQty = std::min(remaining, orderQty);

        // MM gets passive fill
        Side mmSide = (aggSide==Side::BUY) ? Side::SELL : Side::BUY;
        std::cout << mm->color << "  [" << mm->tag << "] " << RST
                  << "PASSIVE FILL: " << sideToString(mmSide) << " " << fillQty << "@" << matchPrice;
        if(mm == mms.front() && mms.size()>1)
            std::cout << GRN << " (best price, filled first!)" << RST;
        std::cout << "\n";
        mm->fill(mmSide, matchPrice, fillQty, logger);

        // Aggressor gets fill at SAME price
        if(aggressor) {
            aggressor->fill(aggSide, matchPrice, fillQty, logger);
        }

        remaining -= fillQty;
    }

    // ── LiquidityTakers react ──
    for(auto& lt : participants) {
        if(lt.type != Participant::TAKER) continue;
        if(ratio >= lt.threshold) {
            // Recalculate available MM liquidity for LT
            Qty ltAvail = 0;
            for(auto* mm : mms) {
                Qty oq = (aggSide==Side::BUY) ? mm->ask_order.qty : mm->bid_order.qty;
                ltAvail += oq;
            }
            Qty ltFill = std::min(lt.clip, ltAvail);

            if(ltFill > 0) {
                // Match LT against MMs at MM's actual prices
                Qty ltRem = ltFill;
                Price ltFillPrice = 0;
                Qty ltTotalFilled = 0;

                for(auto* mm : mms) {
                    if(ltRem <= 0) break;
                    Qty oq = (aggSide==Side::BUY) ? mm->ask_order.qty : mm->bid_order.qty;
                    Price mp = (aggSide==Side::BUY) ? mm->ask_order.price : mm->bid_order.price;
                    Qty f = std::min(ltRem, oq);
                    if(f <= 0) continue;

                    Side mmSide = (aggSide==Side::BUY) ? Side::SELL : Side::BUY;
                    mm->fill(mmSide, mp, f, logger);
                    lt.fill(aggSide, mp, f, logger);
                    ltFillPrice = mp; // for display
                    ltTotalFilled += f;

                    std::cout << mm->color << "  [" << mm->tag << "] " << RST
                              << DIM << "filled by " << lt.tag << ": " << sideToString(mmSide) << " " << f << "@" << mp << RST << "\n";
                    ltRem -= f;
                }

                std::cout << lt.color << "  [" << lt.tag << "] " << RST;
                if(aggSide==Side::BUY) {
                    std::cout << BGRN << "FOLLOW BUY " << ltTotalFilled << "@" << ltFillPrice << RST;
                    lt.bid_order = {true, ltFillPrice, ltTotalFilled};
                    lt.ask_order = {false, 0, 0};
                } else {
                    std::cout << BRED << "FOLLOW SELL " << ltTotalFilled << "@" << ltFillPrice << RST;
                    lt.bid_order = {false, 0, 0};
                    lt.ask_order = {true, ltFillPrice, ltTotalFilled};
                }
                std::cout << DIM << " (ratio " << ratio << " >= " << lt.threshold << ")" << RST << "\n";
            } else {
                std::cout << lt.color << "  [" << lt.tag << "] " << RST
                          << DIM << "TRIGGERED but no liquidity" << RST << "\n";
            }
        } else {
            std::cout << lt.color << "  [" << lt.tag << "] " << RST
                      << DIM << "SKIP (ratio " << ratio << " < " << lt.threshold << ")" << RST << "\n";
        }
    }

    markAllToMarket();
    updateAllMMs();
}

// ── Random trader ──
void randomTrade() {
    Participant& rnd = participants[4];
    Side side = (rand()%2) ? Side::BUY : Side::SELL;
    Qty qty = 10 + rand()%150;

    std::cout << BWHT << "  [RND] " << RST << "generates: " << sideToString(side) << " " << qty << "\n";
    executeTrade(side, qty, &rnd);
}

void showBBO() {
    std::cout << "\n" << BCYN << "  ┌─ MARKET " << RST;
    { for(int i=0;i<68;i++) std::cout << "─"; } std::cout << "\n";
    std::cout << "  │  " << BGRN << mkt.bqty << " × " << mkt.bid << RST
              << DIM << "  ◄── bid   ask ──►  " << RST
              << BRED << mkt.ask << " × " << mkt.aqty << RST
              << DIM << "   spread=" << (mkt.ask-mkt.bid) << RST << "\n";
    std::cout << BCYN << "  └─ " << RST << DIM << "fair=" << RST << BYEL
              << std::fixed << std::setprecision(2) << mkt.fairPrice() << RST
              << DIM << "  mid=" << mkt.mid() << RST << "\n";
}

void showOrderBook() {
    // Collect all MM orders to show a consolidated book view
    std::cout << "\n" << BOLD << "  Consolidated Order Book (MMs only):" << RST << "\n";
    std::cout << DIM << "    BIDS                           ASKS" << RST << "\n";
    std::cout << "    ";
    for(auto& p : participants) {
        if(p.type!=Participant::MAKER) continue;
        if(p.bid_order.active)
            std::cout << GRN << p.tag << " " << p.bid_order.qty << "@" << p.bid_order.price << RST << "  ";
        else
            std::cout << DIM << p.tag << " ---" << RST << "  ";
    }
    std::cout << "  │  ";
    for(auto& p : participants) {
        if(p.type!=Participant::MAKER) continue;
        if(p.ask_order.active)
            std::cout << RED << p.tag << " " << p.ask_order.qty << "@" << p.ask_order.price << RST << "  ";
        else
            std::cout << DIM << p.tag << " ---" << RST << "  ";
    }
    std::cout << "\n";
}

void showParticipants() {
    std::cout << "\n";
    // Header
    std::cout << BOLD << "  " << std::setw(16) << "Participant"
              << std::setw(8) << "Pos" << std::setw(10) << "VWAP"
              << std::setw(10) << "Real" << std::setw(10) << "Unreal"
              << std::setw(10) << "Total" << RST << "\n";
    std::cout << "  ";
    { for(int i=0;i<64;i++) std::cout << "─"; } std::cout << "\n";

    for(auto& p : participants) {
        std::cout << p.color << "  " << std::setw(16) << p.name << RST
                  << posC(p.pos.position_) << std::setw(8) << p.pos.position_ << RST;
        if(p.pos.position_)
            std::cout << DIM << std::setw(10) << std::setprecision(1) << p.vwap() << RST;
        else
            std::cout << std::setw(10) << "-";
        std::cout << pnlC(p.pos.real_pnl_) << std::setw(10) << std::setprecision(1) << p.pos.real_pnl_ << RST
                  << pnlC(p.pos.unreal_pnl_) << std::setw(10) << p.pos.unreal_pnl_ << RST
                  << pnlC(p.pos.total_pnl_) << BOLD << std::setw(10) << p.pos.total_pnl_ << RST
                  << "\n";
    }

    // Sum
    double totalPnl = 0;
    for(auto& p : participants) totalPnl += p.pos.total_pnl_;
    std::cout << "  ";
    { for(int i=0;i<64;i++) std::cout << "─"; } std::cout << "\n";
    std::cout << DIM << "  " << std::setw(54) << "Market total: " << RST
              << pnlC(totalPnl) << BOLD << std::setprecision(1) << totalPnl << RST << "\n";
}

void showAll() {
    showBBO();
    showOrderBook();
    showParticipants();
}

void showHelp() {
    std::cout << "\n" << BOLD << "Commands:" << RST << "\n";
    std::cout << DIM << "  Market:" << RST << "\n";
    std::cout << "    bid +1 / bid -1           ask +1 / ask -1\n";
    std::cout << "    bqty +100 / bqty -100     aqty +100 / aqty -100\n";
    std::cout << DIM << "  Trades:" << RST << "\n";
    std::cout << "    buy 80 / sell 80          Manual trade\n";
    std::cout << "    random                    Random generates one trade\n";
    std::cout << "    auto 5                    Random generates 5 trades in sequence\n";
    std::cout << DIM << "  Presets:" << RST << "\n";
    std::cout << "    buypressure / sellpressure / balanced / reset\n";
    std::cout << DIM << "  Config:" << RST << "\n";
    std::cout << "    config                    Show all participant configs\n";
    std::cout << "    set MMA thresh 0.8        Change MM-A threshold\n";
    std::cout << "    set LT1 thresh 0.3        Change LT-1 threshold\n";
    std::cout << DIM << "  Display:" << RST << "\n";
    std::cout << "    show / help / quit\n";
}

void showConfig() {
    std::cout << "\n" << BOLD << "  Participant Configs:" << RST << "\n";
    std::cout << "  " << std::setw(18) << "Name" << std::setw(10) << "Type"
              << std::setw(12) << "Threshold" << std::setw(8) << "Clip" << "\n";
    std::cout << "  ";
    { for(int i=0;i<48;i++) std::cout << "─"; } std::cout << "\n";
    for(auto& p : participants) {
        std::cout << p.color << "  " << std::setw(18) << p.name << RST
                  << std::setw(10) << (p.type==Participant::MAKER?"MAKER":p.type==Participant::TAKER?"TAKER":"RANDOM")
                  << std::setw(12) << std::setprecision(2) << p.threshold
                  << std::setw(8) << p.clip << "\n";
    }
    std::cout << "\n  " << DIM << "MM-A is aggressive (low threshold), MM-B is conservative (high threshold)" << RST << "\n";
    std::cout << "  " << DIM << "LT-1 is sensitive (low threshold), LT-2 is selective (high threshold)" << RST << "\n";
}

Participant* findParticipant(const std::string& key) {
    if(key=="MMA" || key=="mma") return &participants[0];
    if(key=="MMB" || key=="mmb") return &participants[1];
    if(key=="LT1" || key=="lt1") return &participants[2];
    if(key=="LT2" || key=="lt2") return &participants[3];
    return nullptr;
}

int main() {
    srand(time(nullptr));
    logger = new Common::Logger("demo_full_market.log");
    for(auto& p : participants) p.init();

    std::cout << BOLD << "\n";
    std::cout << "  ╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "  ║   Full Market Simulator (5 participants, real C++ book code)  ║\n";
    std::cout << "  ║   2× MarketMaker  +  2× LiquidityTaker  +  1× Random        ║\n";
    std::cout << "  ╚════════════════════════════════════════════════════════════════╝\n";
    std::cout << RST;

    showConfig();
    showHelp();
    updateAllMMs();
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
        std::cout << DIM << "═══ Step " << stepNum << " ═══" << RST << "\n";

        if(cmd=="quit"||cmd=="q"||cmd=="exit") break;
        else if(cmd=="help"||cmd=="h") { showHelp(); continue; }
        else if(cmd=="show"||cmd=="s") { showAll(); continue; }
        else if(cmd=="config") { showConfig(); continue; }
        else if(cmd=="bid") {
            int d; iss>>d; mkt.bid+=d;
            if(mkt.bid>=mkt.ask) mkt.bid=mkt.ask-1;
            std::cout << BCYN << "  [MKT] " << RST << "Bid → " << mkt.bid << "\n";
            markAllToMarket(); updateAllMMs();
        }
        else if(cmd=="ask") {
            int d; iss>>d; mkt.ask+=d;
            if(mkt.ask<=mkt.bid) mkt.ask=mkt.bid+1;
            std::cout << BCYN << "  [MKT] " << RST << "Ask → " << mkt.ask << "\n";
            markAllToMarket(); updateAllMMs();
        }
        else if(cmd=="bqty") { int d; iss>>d; mkt.bqty=std::max(Qty(10),Qty(int(mkt.bqty)+d));
            std::cout<<BCYN<<"  [MKT] "<<RST<<"Bid qty → "<<mkt.bqty<<"\n"; markAllToMarket(); updateAllMMs(); }
        else if(cmd=="aqty") { int d; iss>>d; mkt.aqty=std::max(Qty(10),Qty(int(mkt.aqty)+d));
            std::cout<<BCYN<<"  [MKT] "<<RST<<"Ask qty → "<<mkt.aqty<<"\n"; markAllToMarket(); updateAllMMs(); }
        else if(cmd=="buy") { Qty qty; iss>>qty; executeTrade(Side::BUY, qty, &participants[4]); }
        else if(cmd=="sell") { Qty qty; iss>>qty; executeTrade(Side::SELL, qty, &participants[4]); }
        else if(cmd=="random"||cmd=="rand") { randomTrade(); }
        else if(cmd=="auto") {
            int n=5; iss>>n;
            std::cout << BWHT << "  [RND] " << RST << "generating " << n << " random trades...\n\n";
            for(int i=0;i<n;i++) {
                std::cout << DIM << "  ── auto " << (i+1) << "/" << n << " ──" << RST << "\n";
                randomTrade();
                std::cout << "\n";
            }
        }
        else if(cmd=="buypressure") { mkt={100,102,500,50}; std::cout<<BCYN<<"  [MKT] buy pressure\n"<<RST; markAllToMarket(); updateAllMMs(); }
        else if(cmd=="sellpressure") { mkt={100,102,50,500}; std::cout<<BCYN<<"  [MKT] sell pressure\n"<<RST; markAllToMarket(); updateAllMMs(); }
        else if(cmd=="balanced") { mkt={100,102,300,300}; std::cout<<BCYN<<"  [MKT] balanced\n"<<RST; markAllToMarket(); updateAllMMs(); }
        else if(cmd=="reset") {
            mkt={100,102,300,300}; stepNum=0;
            for(auto& p:participants){p.pos=PositionInfo{};p.init();p.bid_order={};p.ask_order={};}
            std::cout<<BCYN<<"  [MKT] everything reset\n"<<RST; updateAllMMs();
        }
        else if(cmd=="set") {
            std::string who, what; double val;
            iss >> who >> what >> val;
            auto* p = findParticipant(who);
            if(p && what=="thresh") { p->threshold=val; std::cout<<"  "<<p->name<<" threshold → "<<val<<"\n"; updateAllMMs(); }
            else if(p && what=="clip") { p->clip=static_cast<Qty>(val); std::cout<<"  "<<p->name<<" clip → "<<p->clip<<"\n"; updateAllMMs(); }
            else std::cout << RED << "  Usage: set MMA/MMB/LT1/LT2 thresh/clip VALUE" << RST << "\n";
        }
        else { std::cout<<RED<<"  Unknown: "<<cmd<<". Type 'help'."<<RST<<"\n"; continue; }

        showAll();
    }

    std::cout << "\nFinal standings:\n";
    showParticipants();
    std::cout << "\nBye!\n";
    delete logger;
    return 0;
}
