#pragma once

#include "Order.h"

class OrderModify {
    public:
        OrderModify(OrderId orderId, Side side, Price price, Quantity quantity)
        : orderId_ { orderId }
        , price_ { price }
        , side_ { side }
        , quantity_ { quantity}
        {}

        OrderId GetOrderId() const { return orderId_; }
        Price GetPrice() const { return price_; }
        Side GetSide() const { return side_; }
        Quantity GetQuantity() const { return quantity_; }
        OrderPointer ToOrderPointer(OrderType type) const {
            return std::make_shared<Order>(type, GetOrderId() , GetPrice(), GetSide(), GetQuantity());
        }

    private:
        OrderId orderId_;
        Side side_;
        Price price_;
        Quantity quantity_;
};


