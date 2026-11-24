#include "dynamic.h"

#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace {

// One legal selection from a group.
struct Choice {
    vector<pair<int, int>> picks;   // (item index, quantity)
    int weight = 0;
    long long damage = 0;
    long long armor = 0;
    long long healing = 0;

    long long score() const {
        return DAMAGE_WEIGHT * damage +
               ARMOR_WEIGHT * armor +
               HEALING_WEIGHT * healing;
    }
};

// Creates a choice and calculates its combat statistics.
Choice makeChoice(
    const vector<Item>& items,
    initializer_list<pair<int, int>> selections
) {
    Choice choice;

    for (const auto& entry : selections) {
        const int index = entry.first;
        const int quantity = entry.second;

        if (quantity <= 0)
            continue;

        const Item& item = items[index];

        choice.picks.push_back(entry);
        choice.weight += item.weight * quantity;

        long long amount = 1LL * item.value * quantity;

        if (item.category == "one_handed" ||
            item.category == "two_handed") {
            choice.damage += amount;
        }
        else if (item.category == "armor_set" ||
                 item.category == "shield") {
            choice.armor += amount;
        }
        else if (item.category == "potion") {
            choice.healing += amount;
        }
    }

    return choice;
}

// Generates legal selections from the item categories.
vector<vector<Choice>> makeGroups(
    const vector<Item>& items
) {
    vector<int> oneHanded;
    vector<int> twoHanded;
    vector<int> shields;
    vector<int> armorSets;
    vector<int> potions;

    for (int i = 0;
         i < static_cast<int>(items.size()); ++i) {

        const string& c = items[i].category;

        if (c == "one_handed")
            oneHanded.push_back(i);
        else if (c == "two_handed")
            twoHanded.push_back(i);
        else if (c == "shield")
            shields.push_back(i);
        else if (c == "armor_set")
            armorSets.push_back(i);
        else if (c == "potion")
            potions.push_back(i);
    }

    vector<vector<Choice>> groups;

    // HANDS:
    // Choose none, one one-handed weapon,
    // two one-handed weapons, one-handed + shield,
    // one two-handed weapon, or a shield alone.
    //
    // Two-handed weapons cannot be combined
    // with shields or one-handed weapons.

    vector<Choice> hands = {Choice{}};

    // Single one-handed weapons or two copies of one weapon.
    for (int i : oneHanded) {

        if (items[i].maxCopies >= 1)
            hands.push_back(
                makeChoice(items, {{i, 1}})
            );

        if (items[i].maxCopies >= 2)
            hands.push_back(
                makeChoice(items, {{i, 2}})
            );
    }

    // Two-handed weapons.
    for (int i : twoHanded) {

        if (items[i].maxCopies >= 1)
            hands.push_back(
                makeChoice(items, {{i, 1}})
            );
    }

    // Shields alone.
    for (int i : shields) {

        if (items[i].maxCopies >= 1)
            hands.push_back(
                makeChoice(items, {{i, 1}})
            );
    }

    // Dual-wielding and one-handed weapon + shield.
    for (size_t a = 0; a < oneHanded.size(); ++a) {

        int i = oneHanded[a];

        if (items[i].maxCopies < 1)
            continue;

        // Two different one-handed weapons.
        for (size_t b = a + 1;
             b < oneHanded.size(); ++b) {

            int j = oneHanded[b];

            if (items[j].maxCopies >= 1)
                hands.push_back(
                    makeChoice(items, {{i, 1}, {j, 1}})
                );
        }

        // One-handed weapon with a shield.
        for (int s : shields) {

            if (items[s].maxCopies >= 1)
                hands.push_back(
                    makeChoice(items, {{i, 1}, {s, 1}})
                );
        }
    }

    groups.push_back(std::move(hands));

    // ARMOR:
    // Choose at most one complete armor set.

    vector<Choice> armor = {Choice{}};

    for (int i : armorSets) {

        if (items[i].maxCopies >= 1)
            armor.push_back(
                makeChoice(items, {{i, 1}})
            );
    }

    groups.push_back(std::move(armor));

    // POTIONS:
    // Each potion type can be chosen
    // from 0 through maxCopies.

    for (int i : potions) {

        vector<Choice> choices;

        for (int q = 0;
             q <= items[i].maxCopies; ++q) {

            choices.push_back(
                makeChoice(items, {{i, q}})
            );
        }

        groups.push_back(std::move(choices));
    }

    return groups;
}

} // namespace

// Main bottom-up dynamic programming algorithm.
Solution optimize(
    const vector<Item>& items,
    int capacity
) {
    if (capacity < 0)
        throw runtime_error("Negative capacity.");

    const auto groups = makeGroups(items);

    const int G =
        static_cast<int>(groups.size());

    const long long IMPOSSIBLE =
        numeric_limits<long long>::min() / 4;

    // dp[g][w] stores the highest combat score
    // at EXACT weight w using the first g groups.

    vector<vector<long long>> dp(
        G + 1,
        vector<long long>(
            capacity + 1, IMPOSSIBLE
        )
    );

    // Records choices for backtracking.
    vector<vector<int>> chosen(
        G + 1,
        vector<int>(capacity + 1, -1)
    );

    // Base case: zero weight, zero score.
    dp[0][0] = 0;

    // Fill the DP table.
    for (int g = 1; g <= G; ++g) {

        const auto& options = groups[g - 1];

        for (int oldWeight = 0;
             oldWeight <= capacity; ++oldWeight) {

            if (dp[g - 1][oldWeight] == IMPOSSIBLE)
                continue;

            // Try every choice in the current group.
            for (int j = 0;
                 j < static_cast<int>(options.size());
                 ++j) {

                const Choice& c = options[j];

                // Reject overweight combinations.
                if (c.weight > capacity - oldWeight)
                    continue;

                const int w =
                    oldWeight + c.weight;

                const long long candidate =
                    dp[g - 1][oldWeight] + c.score();

                // Keep the higher combat score.
                if (candidate > dp[g][w]) {

                    dp[g][w] = candidate;
                    chosen[g][w] = j;
                }
            }
        }
    }

    // Find maximum score at any legal weight.
    long long bestScore = IMPOSSIBLE;
    int bestWeight = 0;

    // Prefer lighter inventories when scores tie.
    for (int w = 0; w <= capacity; ++w) {

        if (dp[G][w] > bestScore) {

            bestScore = dp[G][w];
            bestWeight = w;
        }
    }

    Solution result;

    result.weight = bestWeight;
    result.score = bestScore;
    result.quantities.assign(items.size(), 0);
    result.groupCount = G;

    for (const auto& options : groups) {
        result.optionCount +=
            static_cast<int>(options.size());
    }

    // BACKTRACKING:
    // Recover the items that produced the best score.

    int w = bestWeight;

    for (int g = G; g >= 1; --g) {

        int selectedIndex = chosen[g][w];

        if (selectedIndex < 0)
            throw runtime_error(
                "Could not reconstruct DP solution."
            );

        const Choice& c =
            groups[g - 1][selectedIndex];

        // Record selected items and quantities.
        for (const auto& p : c.picks) {
            result.quantities[p.first] += p.second;
        }

        // Record combat statistics.
        result.damage += c.damage;
        result.armor += c.armor;
        result.healing += c.healing;

        w -= c.weight;
    }

    if (w != 0)
        throw runtime_error(
            "Backtracking weight mismatch."
        );

    return result;
}

// Validate the final selected inventory.
bool validate(
    const vector<Item>& items,
    const Solution& result,
    int capacity
) {
    if (result.quantities.size() != items.size())
        return false;

    long long weight = 0;
    long long damage = 0;
    long long armor = 0;
    long long healing = 0;

    int oneHanded = 0;
    int twoHanded = 0;
    int armorSets = 0;
    int shields = 0;

    for (size_t i = 0; i < items.size(); ++i) {

        const Item& item = items[i];
        const int q = result.quantities[i];

        if (q < 0 || q > item.maxCopies)
            return false;

        weight += 1LL * q * item.weight;

        const long long value =
            1LL * q * item.value;

        if (item.category == "one_handed") {
            oneHanded += q;
            damage += value;
        }
        else if (item.category == "two_handed") {
            twoHanded += q;
            damage += value;
        }
        else if (item.category == "armor_set") {
            armorSets += q;
            armor += value;
        }
        else if (item.category == "shield") {
            shields += q;
            armor += value;
        }
        else if (item.category == "potion") {
            healing += value;
        }
        else {
            return false;
        }
    }

    // Check legal hand combinations.
    const bool legalHands =
        oneHanded <= 2 &&
        twoHanded <= 1 &&
        shields <= 1 &&
        !(twoHanded > 0 &&
          (oneHanded > 0 || shields > 0)) &&
        !(shields > 0 && oneHanded > 1);

    // Verify restrictions and calculated totals.
    return
        legalHands &&
        armorSets <= 1 &&
        weight <= capacity &&
        weight == result.weight &&
        damage == result.damage &&
        armor == result.armor &&
        healing == result.healing &&
        DAMAGE_WEIGHT * damage +
        ARMOR_WEIGHT * armor +
        HEALING_WEIGHT * healing == result.score;
}