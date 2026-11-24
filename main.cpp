#include "dynamic.h"
#include "item.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

struct Benchmark {
    Solution result;
    double totalMs = 0.0;
    double averageMs = 0.0;
    bool valid = false;
};

// Measure optimization time.
Benchmark runBenchmark(
    const vector<Item>& items,
    int capacity,
    int runs
) {
    Benchmark b;

    const auto start = chrono::steady_clock::now();

    for (int i = 0; i < runs; ++i) {
        b.result = optimize(items, capacity);
    }

    const auto end = chrono::steady_clock::now();

    b.totalMs =
        chrono::duration<double, milli>(
            end - start
        ).count();

    b.averageMs = b.totalMs / runs;

    b.valid = validate(
        items, b.result, capacity
    );

    return b;
}

// Display the selected inventory and statistics.
void printResults(
    const string& filename,
    const vector<Item>& items,
    const Benchmark& b,
    double capacity,
    int runs
) {
    cout << "\nSKYRIM DYNAMIC PROGRAMMING RESULTS\n";

    cout << "Dataset: " << filename << '\n';
    cout << "Item types: " << items.size() << '\n';
    cout << "Weight capacity: " << capacity << '\n';

    cout << "DP groups: "
         << b.result.groupCount << '\n';

    cout << "Selection options: "
         << b.result.optionCount << '\n';

    cout << "Scoring: "
         << DAMAGE_WEIGHT << "*damage + "
         << ARMOR_WEIGHT << "*armor rating + "
         << HEALING_WEIGHT << "*healing\n";

    cout << "\nSelected inventory:\n";

    bool any = false;

    for (size_t i = 0; i < items.size(); ++i) {

        const int qty = b.result.quantities[i];

        if (!qty)
            continue;

        any = true;

        string stat = "healing";

        if (items[i].category == "one_handed" ||
            items[i].category == "two_handed") {
            stat = "damage";
        }
        else if (items[i].category == "armor_set" ||
                 items[i].category == "shield") {
            stat = "armor_rating";
        }

        cout << "  " << qty
             << " x " << items[i].name
             << " [" << items[i].category << "]"

             << "  weight="
             << qty * items[i].weight /
                double(WEIGHT_SCALE)

             << "  " << stat << '='
             << 1LL * qty * items[i].value
             << '\n';
    }

    if (!any)
        cout << "  No items selected.\n";

    cout << "\nTOTAL WEIGHT: "
         << b.result.weight /
            double(WEIGHT_SCALE)
         << " / " << capacity << '\n';

    cout << "REMAINING CAPACITY: "
         << capacity -
            b.result.weight / double(WEIGHT_SCALE)
         << '\n';

    cout << "TOTAL WEAPON DAMAGE: "
         << b.result.damage << '\n';

    cout << "TOTAL ARMOR RATING: "
         << b.result.armor << '\n';

    cout << "TOTAL HEALING: "
         << b.result.healing << '\n';

    cout << "TOTAL COMBAT SCORE: "
         << b.result.score << '\n';

    cout << "Average optimization time ("
         << runs << " runs): "
         << b.averageMs << " ms\n";

    cout << "Total optimization time ("
         << runs << " runs): "
         << b.totalMs << " ms\n";

    cout << "Restrictions validation: "
         << (b.valid ? "PASS" : "FAIL")
         << '\n';
}

int main(int argc, char* argv[]) {
    try {
        // Select a dataset or run all datasets.
        const string input =
            (argc >= 2) ? argv[1] : "data_15.txt";

        const double weightLimit =
            (argc >= 3) ? stod(argv[2]) : 150.0;

        const int runs =
            (argc >= 4) ? stoi(argv[3]) : 100;

        if (runs <= 0)
            throw runtime_error("Runs must be positive.");

        const int capacity =
            scaleWeight(weightLimit);

        cout << fixed << setprecision(3);

        // RUN ALL EIGHT DATASETS.
        if (input == "all") {

            const vector<string> files = {
                "data_15.txt",
                "data_30.txt",
                "data_45.txt",
                "data_60.txt",
                "data_85.txt",
                "data_100.txt",
                "data_150.txt",
                "data_227.txt"
            };

            struct Row {
                string filename;
                int itemCount;
                Solution result;
                double averageMs;
                bool valid;
            };

            vector<Row> summary;

            cout << "SKYRIM DP BENCHMARK\n";
            cout << "Weight capacity: "
                 << weightLimit << '\n';
            cout << "Repetitions: "
                 << runs << '\n';

            for (const string& filename : files) {

                auto items = readItems(filename);

                Benchmark b = runBenchmark(
                    items, capacity, runs
                );

                cout << "\n========================================\n";

                printResults(
                    filename,
                    items,
                    b,
                    weightLimit,
                    runs
                );

                summary.push_back({
                    filename,
                    static_cast<int>(items.size()),
                    b.result,
                    b.averageMs,
                    b.valid
                });

                if (!b.valid)
                    return 1;
            }

            // Display final benchmark summary.
            cout << "\n========================================\n";
            cout << "FINAL BENCHMARK SUMMARY\n\n";

            cout << left << setw(19) << "File"
                 << right << setw(7) << "Items"
                 << setw(9) << "Damage"
                 << setw(9) << "Armor"
                 << setw(10) << "Healing"
                 << setw(10) << "Score"
                 << setw(10) << "Weight"
                 << setw(14) << "Avg ms"
                 << setw(9) << "Valid"
                 << '\n';

            for (const Row& row : summary) {

                cout << left << setw(19)
                     << row.filename

                     << right << setw(7)
                     << row.itemCount

                     << setw(9)
                     << row.result.damage

                     << setw(9)
                     << row.result.armor

                     << setw(10)
                     << row.result.healing

                     << setw(10)
                     << row.result.score

                     << setw(10)
                     << row.result.weight /
                        double(WEIGHT_SCALE)

                     << setw(14)
                     << row.averageMs

                     << setw(9)
                     << (row.valid ? "PASS" : "FAIL")
                     << '\n';
            }

            cout << "\nAll eight datasets completed.\n";
        }
        else {
            // RUN AN INDIVIDUAL DATASET.
            auto items = readItems(input);

            Benchmark b = runBenchmark(
                items, capacity, runs
            );

            printResults(
                input,
                items,
                b,
                weightLimit,
                runs
            );

            return b.valid ? 0 : 1;
        }
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}