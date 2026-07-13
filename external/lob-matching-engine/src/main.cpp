#include "matching_engine.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {
    string inputFile = "data/sample_orders.csv";
    if (argc > 1) inputFile = argv[1];

    MatchingEngine engine;
    vector<Trade> allTrades;

    ifstream file(inputFile);
    if (!file.is_open()) {
        cerr << "Error: cannot open " << inputFile << endl;
        return 1;
    }

    string line;
    getline(file, line); // header

    while (getline(file, line)) {
        istringstream ss(line);
        string token, sideStr, typeStr;
        uint64_t timestamp, quantity;
        int64_t price;

        getline(ss, token, ','); timestamp = stoull(token);
        getline(ss, sideStr, ',');
        getline(ss, typeStr, ',');
        getline(ss, token, ','); price = stoll(token);
        getline(ss, token, ','); quantity = stoull(token);

        Side side = (sideStr == "BUY") ? Side::BUY : Side::SELL;
        OrderType type = OrderType::LIMIT;
        if (typeStr == "MARKET") type = OrderType::MARKET;
        else if (typeStr == "IOC") type = OrderType::IOC;

        auto result = engine.submitOrder(side, type, price, quantity);
        for (auto& t : result.trades)
            allTrades.push_back(t);
    }

    cout << "buy_order_id,sell_order_id,price,quantity,timestamp" << endl;
    for (auto& t : allTrades)
        cout << t.buyOrderId << "," << t.sellOrderId << ","
             << t.price << "," << t.quantity << "," << t.timestamp << endl;

    cout << "\nDepth Snapshot (Top 5):" << endl;
    cout << "BIDS:" << endl;
    for (auto& lvl : engine.getOrderBook().getDepthSnapshot(5, Side::BUY))
        cout << "  " << lvl.price << " x " << lvl.quantity << endl;
    cout << "ASKS:" << endl;
    for (auto& lvl : engine.getOrderBook().getDepthSnapshot(5, Side::SELL))
        cout << "  " << lvl.price << " x " << lvl.quantity << endl;

    return 0;
}
