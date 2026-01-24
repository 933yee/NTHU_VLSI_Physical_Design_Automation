#ifndef DIE_TECH_LIB_H 
#define DIE_TECH_LIB_H

#include <unordered_map>
#include <string>

using namespace std;

struct Die;
struct Tech;
struct Net;
struct Cell;

struct Die {
    Die() {}
    Die(Tech* tech, long long max_area) : tech(tech), max_area(max_area) {}
    void update_test() {
        area_test = area;
    }
    long long area = 0, area_test = 0;
    // 不會變的
    Tech* tech;
    long long max_area = 0;
    unordered_map<string, Cell*> cells_map;
};

struct Tech {
    // lib cell name -> area
    Tech() {}
    unordered_map<string, long long> lib_area_map;
};

struct Net {
    Net() {}
    Net(long long weight) : weight(weight) {}
    void update_test() {
        cells_in_A_test = cells_in_A;
        cells_in_B_test = cells_in_B;
    }
    // 會變的
    unordered_map<string, Cell*> cells_in_A, cells_in_B; 
    unordered_map<string, Cell*> cells_in_A_test, cells_in_B_test;
    // 不會變的
    long long weight = 0;
    unordered_map<string, Cell*> cells;
    long long max_area_in_A = 0, max_area_in_B = 0;
};

struct Cell{
    Cell() {}
    Cell(long long area_in_A, long long area_in_B, string cell_name) : area_in_A(area_in_A), area_in_B(area_in_B), cell_name(cell_name){}
    void update_test() {
        die_test = die;
        gain_test = gain;
    }
    // 會變的
    string die, die_test;
    long long gain = 0, gain_test = 0;
    bool is_locked = false;
    // 不會變的
    long long area_in_A = 0, area_in_B = 0;
    long long connected_weight = 0;
    string cell_name;
    vector<Net*> nets;
};

#endif // DIE_TECH_LIB_H
