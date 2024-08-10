#pragma once

#include <list>
#include <exception>
#include <format>
#include <iostream>
#include <memory>

#include "OrderType.h"
#include "Side.h"
#include "Usings.h"
#include "Constants.h"


class Order {
    public:
        Order(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity)
        : orderType_ {orderType}
        , orderId_ {orderId}
        , side_ {side}
        , price_ {price}
        , initialQuantity_ {quantity}
        , remainingQuantity_ {quantity}
        {}

        OrderId GetOrderId() const { return orderId_;}
        Side GetSide() const { return side_;}
        Price GetPrice() const { return price_;}
        OrderType GetOrderType() const {return orderType_;}
        Quantity GetInitialQuanity() const {return initialQuantity_;}
        Quantity GetRemainingQuantity() const {return remainingQuantity_;}
        Quantity GetFilledlQuanity() const {return GetInitialQuanity() - GetRemainingQuantity();}
        bool isFilled() const { return GetRemainingQuantity();}
        void Fill(Quantity quantity){
            if (quantity > GetRemainingQuantity())
                throw std::logic_error(std::format("Order ({}) cannot be filled for more than its remaining quantity of ({}).", GetOrderId(), GetRemainingQuantity()) );

            remainingQuantity_ -= quantity;
        }
    private:
        OrderType orderType_;
        OrderId orderId_;
        Side side_;
        Price price_;
        Quantity initialQuantity_;
        Quantity remainingQuantity_;
};


using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;