#ifndef DYNAMIC_H
#define DYNAMIC_H

#include "item.h"
#include <vector>

// Project scoring weights. Agree on these with your group before benchmarking.
// Use HEALING_WEIGHT = 0 to score only damage + armor rating.
constexpr long long DAMAGE_WEIGHT = 1;
constexpr long long ARMOR_WEIGHT = 1;
constexpr long long HEALING_WEIGHT = 1;

struct Solution {
    int weight = 0;                // Scaled weight
    long long damage = 0;          // Weapon damage (sum if dual-wielding)
    long long armor = 0;           // Armor set rating + shield rating
    long long healing = 0;         // Total potion healing
    long long score = 0;           // Weighted damage + armor + healing

    std::vector<int> quantities;

    int groupCount = 0;
    int optionCount = 0;
};

Solution optimize(
    const std::vector<Item>& items,
    int capacity
);

bool validate(
    const std::vector<Item>& items,
    const Solution& result,
    int capacity
);

#endif