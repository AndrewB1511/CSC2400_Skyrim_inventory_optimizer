#include "item.h"

#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

using namespace std;

static string trim(const string& s) {
    const size_t first = s.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    const size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

int scaleWeight(double weight) {
    double scaled = weight * WEIGHT_SCALE;
    if (!isfinite(scaled) || weight < 0 ||
        scaled > numeric_limits<int>::max() ||
        fabs(scaled - round(scaled)) > 1e-6) {
        throw runtime_error("Weight must be a nonnegative multiple of 0.25.");
    }
    return static_cast<int>(lround(scaled));
}

vector<Item> readItems(const string& filename) {
    ifstream file(filename);

    // Works when datasets are beside the .cpp files OR in a Data/ subfolder.
    if (!file && filename.find('/') == string::npos &&
        filename.find('\\') == string::npos) {
        file.clear();
        file.open("Data/" + filename);
    }
    if (!file) throw runtime_error("Cannot open dataset: " + filename);

    string line;
    if (!getline(file, line) ||
        trim(line) != "name,weight,value,category,max_copies") {
        throw runtime_error("Invalid CSV header in " + filename);
    }

    vector<Item> items;
    int lineNumber = 1;

    while (getline(file, line)) {
        ++lineNumber;
        if (trim(line).empty()) continue;

        stringstream ss(line);
        vector<string> fields;
        string field;

        while (getline(ss, field, ','))
            fields.push_back(trim(field));

        if (fields.size() != 5) {
            throw runtime_error(
                "Expected 5 fields at line " +
                to_string(lineNumber) + " of " + filename
            );
        }

        try {
            Item item;

            item.name = fields[0];
            item.weight = scaleWeight(stod(fields[1]));
            item.value = stoi(fields[2]);
            item.category = fields[3];
            item.maxCopies = stoi(fields[4]);

            bool categoryOk =
                item.category == "one_handed" ||
                item.category == "two_handed" ||
                item.category == "armor_set" ||
                item.category == "shield" ||
                item.category == "potion";

            if (item.name.empty() ||
                item.value < 0 ||
                item.maxCopies < 0 ||
                !categoryOk) {
                throw runtime_error(
                    "Invalid item information or category"
                );
            }

            items.push_back(item);
        }
        catch (const exception& e) {
            throw runtime_error(
                "Invalid record at line " +
                to_string(lineNumber) + " of " +
                filename + ": " + e.what()
            );
        }
    }

    return items;
}