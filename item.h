#ifndef ITEM_H
#define ITEM_H

#include <string>
#include <vector>

// Multiply weight by 4 so 0.25-weight increments have integer DP indices.
constexpr int WEIGHT_SCALE = 4;

struct Item {
    std::string name;
    int weight = 0;            // Scaled by WEIGHT_SCALE
    int value = 0;             // Damage, armor rating, or healing per unit
    std::string category;      // one_handed, two_handed, armor_set, shield, potion
    int maxCopies = 0;
};

int scaleWeight(double weight);
std::vector<Item> readItems(const std::string& filename);

#endif