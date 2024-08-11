
#pragma once

#include <map>
#include <unordered_map>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <numeric>

#include "Usings.h"
#include "Order.h"
#include "OrderModify.h"
#include "OrderBookLevelInfos.h"
#include "Trade.h"

class OrderBook {
    private:

        struct OrderEntry {
            OrderPointer order_{ nullptr };
            OrderPointers::iterator location_;
        };

        struct LevelData {
            Quantity quantity_{};
            Quantity count_{};

            enum class Action{
                Add,
                Remove,
                Match,
            };
        };

        std::unordered_map<Price, LevelData> data_;
        std::map<Price, OrderPointers, std::greater<Price>> bids_;
        std::map<Price, OrderPointers, std::less<Price>> asks_;
        std::unordered_map<OrderId, OrderEntry> orders_;
        mutable std::mutex ordersMutex_;
        std::thread ordersPruneThread_;
        std::condition_variable shutdownConditionVariable_;
        std::atomic<bool> shutdown_{ false };

        void PruneGoodForDayOrders();

        void CancelOrders(OrderIds orderIds);
        void CancelOrderInternal(OrderId orderId);

        void OnOrderCancelled(OrderPointer order);
        void OnOrderAdded(OrderPointer order);
        void OnOrderMatched(Price price, Quantity quanity, bool isFullyFilled);
        void UpdateLevelData(Price price, Quantity quantity, LevelData::Action action);

        bool CanFullyFill(Side side, Price price, Quantity quantity) const;
        bool CanMatch(Side side, Price price) const;
        Trades MatchOrders();


    public:

        OrderBook();
        OrderBook(const OrderBook&) = delete;
        void operator=(const OrderBook&) = delete;
        OrderBook(OrderBook&&) = delete;
        void operator=(OrderBook&&) = delete;
        ~OrderBook();

        Trades AddOrder(OrderPointer order);
        void CancelOrder(OrderId orderId);
        Trades ModifyOrder(OrderModify order);

        std::size_t Size() const;
        OrderBookLevelInfos GetOrderInfos() const;

};
