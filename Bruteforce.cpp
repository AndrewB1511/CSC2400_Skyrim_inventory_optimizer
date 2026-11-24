
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>

using namespace std;

// Maximum weight the player can carry
const double MAX_CAPACITY = 150.0;

// Store each Skyrim item's information
struct Item {
    string name;
    double weight;
    int value;
    string category;
    int maxCopies;
};

// Store all items from the file
vector<Item> items;

// Keep track of the items selected
vector<int> selected;
vector<int> bestSelected;

// Store the best inventory results
int bestValue = -1;
double bestWeight = 0.0;
int bestPotions = -1;
long long combinationsChecked = 0;

// Use Brute Force to check every valid inventory combination
void BruteForce(int index, double currentWeight, int currentValue,
                int currentPotions, bool weaponChosen,
                bool oneHandedChosen, bool shieldChosen, bool armorChosen) {

    // Check if we have gone through all items
    if (index == (int)items.size()) {

        // A shield can only be used with a one-handed weapon
        if (shieldChosen && !oneHandedChosen) {
            return;
        }

        // Count this valid combination
        combinationsChecked++;

        // Save the combination with the highest attack and defense
        // If tied, choose more potions, then the lighter inventory
        if (currentValue > bestValue ||
            (currentValue == bestValue && currentPotions > bestPotions) ||
            (currentValue == bestValue && currentPotions == bestPotions &&
             currentWeight < bestWeight)) {

            bestValue = currentValue;
            bestWeight = currentWeight;
            bestPotions = currentPotions;
            bestSelected = selected;
        }

        return;
    }

    // Get the current item and its maximum copies
    Item item = items[index];
    int limit = item.maxCopies;

    // Only allow one weapon
    if (item.category == "one_handed" || item.category == "two_handed") {

        // Cannot select another weapon if one is already chosen
        if (weaponChosen || (item.category == "two_handed" && shieldChosen)) {
            limit = 0;
        }
    }

    // Only allow one complete armor set
    else if (item.category == "armor_set") {
        if (armorChosen) {
            limit = 0;
        }
    }

    // Only allow one shield with a one-handed weapon
    else if (item.category == "shield") {
        if (shieldChosen || (weaponChosen && !oneHandedChosen)) {
            limit = 0;
        }
    }

    // Try selecting different amounts of the current item
    for (int copies = 0; copies <= limit; copies++) {

        // Calculate the new total weight
        double newWeight = currentWeight + copies * item.weight;

        // Stop if the weight goes over 150
        if (newWeight > MAX_CAPACITY) {
            break;
        }

        // Save how many copies we selected
        selected[index] = copies;

        // Check if the current item is a weapon
        bool isWeapon = (item.category == "one_handed" ||
                         item.category == "two_handed");

        // Count the number of healing potions selected
        int newPotions = currentPotions;

        if (item.category == "potion") {
            newPotions += copies;
        }

        // Move to the next item and check more combinations
        BruteForce(index + 1,
                   newWeight,
                   currentValue + copies * item.value,
                   newPotions,
                   weaponChosen || (isWeapon && copies > 0),
                   oneHandedChosen || (item.category == "one_handed" && copies > 0),
                   shieldChosen || (item.category == "shield" && copies > 0),
                   armorChosen || (item.category == "armor_set" && copies > 0));
    }

    // Reset the current item before going back
    selected[index] = 0;
}

int main() {
    string filename;

    // Ask the user for the data file
    cout << "Enter the filename: ";
    getline(cin, filename);

    // Open the file
    ifstream inputFile(filename);

    // Check if the file opened correctly
    if (!inputFile) {
        cout << "Error opening file!" << endl;
        return 1;
    }

    string line;

    // Skip the first line containing column names
    getline(inputFile, line);

    // Read each item from the file
    while (getline(inputFile, line)) {

        // Skip empty lines
        if (line.empty()) {
            continue;
        }

        stringstream ss(line);
        Item item;

        string weightText, valueText, copiesText;

        // Read the item's information separated by commas
        getline(ss, item.name, ',');
        getline(ss, weightText, ',');
        getline(ss, valueText, ',');
        getline(ss, item.category, ',');
        getline(ss, copiesText);

        // Convert the numbers from text
        item.weight = stod(weightText);
        item.value = stoi(valueText);
        item.maxCopies = stoi(copiesText);

        // Make sure the item belongs to an allowed category
        if (item.category != "one_handed" &&
            item.category != "two_handed" &&
            item.category != "armor_set" &&
            item.category != "shield" &&
            item.category != "potion") {

            cout << "Unknown category: " << item.name << endl;
            return 1;
        }

        // Check if the item is already in the list
        bool duplicate = false;

        for (int i = 0; i < (int)items.size(); i++) {
            if (items[i].name == item.name) {
                duplicate = true;
                break;
            }
        }

        // Check for invalid items or quantities
        if (duplicate || item.name.empty() || item.weight < 0 ||
            item.value < 0 || item.maxCopies < 0 ||
            (item.category == "potion" && item.maxCopies > 5) ||
            (item.category != "potion" && item.maxCopies != 1)) {

            cout << "Invalid or duplicate item: " << item.name << endl;
            return 1;
        }

        // Add the item to our list
        items.push_back(item);
    }

    // Close the file after reading
    inputFile.close();

    cout << "\nMaximum carrying capacity: " << MAX_CAPACITY << endl;
    cout << "Items loaded: " << items.size() << endl;

    // Make sure the file contains items
    if (items.empty()) {
        cout << "No items found in the file." << endl;
        return 1;
    }

    // Prepare the vectors to store selected items
    selected.resize(items.size(), 0);
    bestSelected.resize(items.size(), 0);

    // Start measuring the Brute Force execution time
    auto start = chrono::steady_clock::now();

    // Start checking all possible valid combinations
    BruteForce(0, 0.0, 0, 0, false, false, false, false);

    // Stop measuring execution time
    auto end = chrono::steady_clock::now();

    // Calculate the execution time in milliseconds
    chrono::duration<double, milli> elapsed = end - start;

    // Store the attack and defense of the best inventory
    int attack = 0;
    int defense = 0;

    cout << "\nBest attack and defense inventory found:\n";

    // Display the items selected in the best inventory
    for (int i = 0; i < (int)items.size(); i++) {

        if (bestSelected[i] > 0) {
            cout << bestSelected[i] << " x " << items[i].name << endl;

            // Add the damage from the selected weapon
            if (items[i].category == "one_handed" ||
                items[i].category == "two_handed") {

                attack += items[i].value;
            }

            // Add the defense from armor and shields
            if (items[i].category == "armor_set" ||
                items[i].category == "shield") {

                defense += items[i].value;
            }
        }
    }

    // Display the final inventory results
    cout << "\nTotal weight: " << bestWeight << " / " << MAX_CAPACITY << endl;
    cout << "Weapon attack damage: " << attack << endl;
    cout << "Armor and shield defense rating: " << defense << endl;
    cout << "Attack and defense total: " << bestValue << endl;
    cout << "Healing potions selected: " << bestPotions << endl;
    cout << "Valid combinations checked: " << combinationsChecked << endl;

    // Display execution time with three decimal places
    cout << fixed << setprecision(3);
    cout << "Execution time: " << elapsed.count() << " ms" << endl;

    return 0;
}
