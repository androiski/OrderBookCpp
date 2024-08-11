#include "OrderBook.h"

#include <numeric>
#include <chrono>
#include <ctime>

void OrderBook::PruneGoodForDayOrders(){
    using namespace std::chrono;
    const auto end = hours(16);


    while (true){
        const auto now = system_clock::now();
        const auto now_c = system_clock::to_time_t(now);
        std::tm now_parts;
        localtime_s(&now_parts, &now_c);

        if (now_parts.tm_hour >= end.count()) now_parts.tm_mday += 1;

        now_parts.tm_hour = end.count();
        now_parts.tm_min = 0;
        now_parts.tm_sec = 0;

        auto next = system_clock::from_time_t(mktime(&now_parts));
        auto till = next - now + milliseconds(100);

        {
            std::unique_lock ordersLock{ ordersMutex_ };

            if (shutdown_.load(std::memory_order_acquire) ||
            shutdownConditionVariable_.wait_for(ordersLock, till) == std::cv_status::no_timeout)
                return;
        }

        OrderIds orderIds;

        {
            std::scoped_lock ordersLock{ ordersMutex_ };

            for (const auto& [_, entry] : orders_){
                const auto& [order, _] = entry;

                if (order->GetOrderType() != OrderType::GoodForDay) continue;

                orderIds.push_back(order->GetOrderId());
            }
        }
        CancelOrders(orderIds);
    }
}

void OrderBook::CancelOrders(OrderIds orderIds){
    std::scoped_lock orderLocks{ ordersMutex_ };
    for (const auto& orderId : orderIds) CancelOrderInternal(orderId);
}

void OrderBook::CancelOrderInternal(OrderId orderId){
    if (!orders_.contains(orderId)) return;

    const auto [order, iterator] = orders_.at(orderId);
    orders_.erase(orderId);

    if (order->GetSide() == Side::Sell){
        auto price = order->GetPrice();
        auto& orders = asks_.at(price);
        orders.erase(iterator);
        if (orders.empty()) asks_.erase(price);
    }else{
        auto price = order->GetPrice();
        auto& orders = bids_.at(price);
        orders.erase(iterator);
        if (orders.empty()) bids_.erase(price);
    }

    OnOrderCancelled(order);
}

void OrderBook::OnOrderCancelled(OrderPointer order){
    UpdateLevelData(order->GetPrice(), order->GetRemainingQuantity(), LevelData::Action::Remove);
}

void OrderBook::OnOrderAdded(OrderPointer order){
    UpdateLevelData(order->GetPrice(), order->GetInitialQuanity(), LevelData::Action::Add);
}

void OrderBook::OnOrderMatched(Price price, Quantity quantity, bool isFullyFilled){
    UpdateLevelData(price, quantity, isFullyFilled ? LevelData::Action::Remove : LevelData::Action::Match);
}

void OrderBook::UpdateLevelData(Price price, Quantity quantity, LevelData::Action action){
    auto& data = data_[price];

    data.count_ += action == LevelData::Action::Remove ? -1 : action == LevelData::Action::Add ? 1 : 0;
    if (action == LevelData::Action::Remove || action == LevelData::Action::Match){
        data.quantity_ -= quantity;
    }
    else{
        data.quantity_ += quantity;
    }

    if (data.count_ == 0) data_.erase(price);
}

bool OrderBook::CanFullyFill(Side side, Price price, Quantity quantity) const{
    if (!CanMatch(side, price)) return false;

    std::optional<Price> threshold;

    if (side == Side::Buy){
        const auto [askPrice, _] = *asks_.begin();
        threshold = askPrice;
    }
    else{
        const auto [bidPrice, _] = *bids_.begin();
        threshold = bidPrice;
    }

    for (const auto& [levelPrice, levelData] : data_){
        if (threshold.has_value() &&
            (side == Side::Buy && threshold.value() > levelPrice) ||
            (side == Side::Sell && threshold.value() < levelPrice))
                continue;

        if ((side == Side::Buy && levelPrice > price) ||
            (side == Side::Sell && levelPrice < price))
                continue;

        if (quantity <= levelData.quantity_) return true;

        quantity -= levelData.quantity_;
    }

    return false;

}

bool OrderBook::CanMatch(Side side, Price price) const {
    if (side == Side::Buy){
        if (asks_.empty()) return false;

        const auto& [bestAsk, _] = *asks_.begin();
        return price >= bestAsk;
    }
    else{
        if (bids_.empty()) return false;

        const auto& [bestBid, _] = *bids_.begin();
        return price <= bestBid;
    }
}