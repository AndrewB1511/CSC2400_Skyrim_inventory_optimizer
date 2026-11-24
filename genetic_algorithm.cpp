//created with help from claude ai sonnet version 5.5
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

enum Kind { KIND_OTHER = 0, KIND_ARMOR, KIND_TWO, KIND_SHIELD, KIND_ONE };

static int kindOf(const std::string& cat) {
    if (cat == "armor_set") return KIND_ARMOR;
    if (cat == "two_handed") return KIND_TWO;
    if (cat == "shield") return KIND_SHIELD;
    if (cat == "one_handed") return KIND_ONE;
    return KIND_OTHER;
}
//creating the Item struct to store each items values from the data sets.
struct Item {
    std::string name;
    double weight = 0;
    double value = 0;
    std::string category;
    int maxCopies = 1;
    int kind = KIND_OTHER;
};

struct Individual {
    std::vector<int> genes;  // genes[i] = copies of item i taken
    double weight = 0;
    double value = 0;
};

static std::vector<Item> items;
static const double MAX_CAPACITY = 150.0;
static const double capacity = MAX_CAPACITY;
static std::mt19937 rng;

static const double EPS = 1e-9;

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

//creating the function that reads our data files and stores each value into their Item slots
static bool loadItems(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        std::cerr << "Error: cannot open '" << path << "'\n";
        return false;
    }
    std::string line;
    bool first = true;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        line = trim(line);
        if (line.empty()) continue;
        if (first) {  // header row
            first = false;
            continue;
        }
        std::vector<std::string> f;
        std::stringstream ss(line);
        std::string cell;
        while (std::getline(ss, cell, ',')) f.push_back(trim(cell));
        if (f.size() < 5) {
            std::cerr << "Warning: line " << lineNo << " skipped (expected 5 fields)\n";
            continue;
        }
        try {
            Item it;
            it.name = f[0];
            it.weight = std::stod(f[1]);
            it.value = std::stod(f[2]);
            it.category = f[3];
            it.maxCopies = std::stoi(f[4]);
            it.kind = kindOf(it.category);
            if (it.weight < 0 || it.maxCopies < 0) throw std::runtime_error("negative");
            items.push_back(it);
        } catch (...) {
            std::cerr << "Warning: line " << lineNo << " skipped (bad number)\n";
        }
    }
    return !items.empty();
}

// Highest allowed count for item i. Items worth nothing are never taken.
static int geneMax(size_t i) {
    return items[i].value > 0 ? items[i].maxCopies : 0;
}

static void evaluate(Individual& ind) {
    ind.weight = 0;
    ind.value = 0;
    for (size_t i = 0; i < items.size(); ++i) {
        ind.weight += ind.genes[i] * items[i].weight;
        ind.value += ind.genes[i] * items[i].value;
    }
}
//creating the struct for loadout rules so only one or 2 one handed weapons with or without a shield or one 2 handed and armor.
struct Counts {
    int armor = 0, two = 0, shield = 0, one = 0;
};
//creating the func that counts when an armor or weapon is chosen.
static void bump(Counts& c, int kind, int d) {
    switch (kind) {
        case KIND_ARMOR: c.armor += d; break;
        case KIND_TWO: c.two += d; break;
        case KIND_SHIELD: c.shield += d; break;
        case KIND_ONE: c.one += d; break;
        default: break;
    }
}

static Counts countCategories(const std::vector<int>& g) {
    Counts c;
    for (size_t i = 0; i < items.size(); ++i) bump(c, items[i].kind, g[i]);
    return c;
}

// 1 armor max; hands used (two-handed = 2, shield = 1, one-handed = 1) <= 2; 1 shield max.
static bool loadoutOk(const Counts& c) {
    return c.armor <= 1 && c.shield <= 1 && 2 * c.two + c.shield + c.one <= 2;
}

// Make an individual valid: first fix loadout violations (armor / hands),
// then drop random copies until it fits the weight limit, then top it up
// with random items that still fit both the weight and the loadout rules.
static void repairAndFill(Individual& ind, const std::vector<int>& order) {
    Counts c = countCategories(ind.genes);

    // Loadout repair: remove random offending items.
    while (!loadoutOk(c)) {
        std::vector<int> cand;
        for (int i : order) {
            if (ind.genes[i] <= 0) continue;
            int k = items[i].kind;
            bool offends = (c.armor > 1) ? (k == KIND_ARMOR)
                                         : (k == KIND_TWO || k == KIND_SHIELD || k == KIND_ONE);
            if (offends) cand.push_back(i);
        }
        if (cand.empty()) break;
        int i = cand[rng() % cand.size()];
        ind.genes[i]--;
        bump(c, items[i].kind, -1);
    }

    evaluate(ind);

    // Weight repair: remove random taken copies while overweight.
    while (ind.weight > capacity + EPS) {
        std::vector<int> taken;
        for (int i : order)
            if (ind.genes[i] > 0) taken.push_back(i);
        if (taken.empty()) break;
        int i = taken[rng() % taken.size()];
        ind.genes[i]--;
        bump(c, items[i].kind, -1);
        ind.weight -= items[i].weight;
        ind.value -= items[i].value;
    }

    // Fill: try items in random order, add copies while they fit.
    std::vector<int> shuffled = order;
    std::shuffle(shuffled.begin(), shuffled.end(), rng);
    for (int i : shuffled) {
        int mx = geneMax(i);
        while (ind.genes[i] < mx && ind.weight + items[i].weight <= capacity + EPS) {
            bump(c, items[i].kind, 1);
            if (!loadoutOk(c)) {
                bump(c, items[i].kind, -1);
                break;
            }
            ind.genes[i]++;
            ind.weight += items[i].weight;
            ind.value += items[i].value;
        }
    }
}

static Individual randomIndividual(const std::vector<int>& order) {
    Individual ind;
    ind.genes.assign(items.size(), 0);
    for (size_t i = 0; i < items.size(); ++i) {
        int mx = geneMax(i);
        // Bias towards sparse solutions so most start near feasible.
        if (mx > 0 && (rng() % 100) < 30) ind.genes[i] = rng() % (mx + 1);
    }
    repairAndFill(ind, order);
    return ind;
}

static const Individual& tournament(const std::vector<Individual>& pop, int k) {
    const Individual* best = &pop[rng() % pop.size()];
    for (int i = 1; i < k; ++i) {
        const Individual& c = pop[rng() % pop.size()];
        if (c.value > best->value) best = &c;
    }
    return *best;
}

int main(int argc, char** argv) {
    std::string path;
    int popSize = 200;
    int generations = 500;
    double mutRate = 0.02;
    unsigned seed = std::random_device{}();

    if (argc >= 2) {
        path = argv[1];
        if (argc > 2) popSize = std::atoi(argv[2]);
        if (argc > 3) generations = std::atoi(argv[3]);
        if (argc > 4) mutRate = std::atof(argv[4]);
        if (argc > 5) seed = (unsigned)std::strtoul(argv[5], nullptr, 10);
    } else {
        std::cout << "Data file path: ";
        std::getline(std::cin, path);
        path = trim(path);
    }

    if (popSize < 4 || generations < 1 || mutRate < 0 || mutRate > 1) {
        std::cerr << "Error: invalid parameters.\n";
        return 1;
    }
    if (!loadItems(path)) return 1;

    rng.seed(seed);
    const size_t n = items.size();
    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);

    std::cout << "Loaded " << n << " items from " << path << "\n"
              << "Capacity: " << capacity << " | population: " << popSize
              << " | generations: " << generations << " | mutation: " << mutRate
              << " | seed: " << seed << "\n\n";

    std::vector<Individual> pop;
    pop.reserve(popSize);
    for (int i = 0; i < popSize; ++i) pop.push_back(randomIndividual(order));

    auto bestOf = [](const std::vector<Individual>& p) {
        return std::max_element(p.begin(), p.end(),
                                [](const Individual& a, const Individual& b) { return a.value < b.value; });
    };

    Individual best = *bestOf(pop);
    const int elites = std::max(1, popSize / 20);
    const int printEvery = std::max(1, generations / 10);

    for (int gen = 1; gen <= generations; ++gen) {
        std::sort(pop.begin(), pop.end(),
                  [](const Individual& a, const Individual& b) { return a.value > b.value; });

        std::vector<Individual> next(pop.begin(), pop.begin() + elites);  // elitism
        while ((int)next.size() < popSize) {
            const Individual& p1 = tournament(pop, 3);
            const Individual& p2 = tournament(pop, 3);

            Individual child;
            child.genes.resize(n);
            for (size_t i = 0; i < n; ++i)  // uniform crossover
                child.genes[i] = (rng() & 1) ? p1.genes[i] : p2.genes[i];

            std::uniform_real_distribution<double> U(0.0, 1.0);
            for (size_t i = 0; i < n; ++i) {  // mutation
                int mx = geneMax(i);
                if (mx > 0 && U(rng) < mutRate) child.genes[i] = rng() % (mx + 1);
            }

            repairAndFill(child, order);
            next.push_back(std::move(child));
        }
        pop = std::move(next);

        const Individual& gb = *bestOf(pop);
        if (gb.value > best.value) best = gb;

        if (gen == 1 || gen % printEvery == 0 || gen == generations)
            std::cout << "Gen " << std::setw(5) << gen << "  best value: " << best.value
                      << "  weight: " << best.weight << "\n";
    }

    std::cout << "\n=== Best solution ===\n";
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "Total value : " << best.value << "\n";
    std::cout << "Total weight: " << best.weight << " / " << capacity << "\n\n";
    std::cout << "Items chosen:\n";
    for (size_t i = 0; i < n; ++i) {
        if (best.genes[i] > 0)
            std::cout << "  " << best.genes[i] << " x " << items[i].name << " (" << items[i].category
                      << ", w=" << items[i].weight << ", v=" << items[i].value << ")\n";
    }
    return 0;
}
