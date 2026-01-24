#ifndef LIB_H
#define LIB_H

#include <unordered_map>
#include <string>

using namespace std;

struct Cell{
    Cell() : name(""), width(0), height(0), x(0.0), y(0.0), ori_x(0.0), ori_y(0.0), optimal_x(0.0), optimal_y(0.0) {}
    Cell(const string& name, int width, int height, double x, double y)
        : name(name), width(width), height(height), x(x), y(y),
          ori_x(x), ori_y(y), optimal_x(x), optimal_y(y) {}
    
    string name;
    int width, height;
    double x, y;
    int weight = 1;
    
    double ori_x, ori_y;     
    double optimal_x, optimal_y; 
};


struct Blockage{
    Blockage() : name(""), x(0.0), y(0.0), width(0), height(0) {}
    Blockage(const string& name, double x, double y, int width, int height)
        : name(name), x(x), y(y), width(width), height(height) {}
    string name;
    double x, y;
    int width, height;
};

struct Row{
    Row() : name(""), site_width(0), height(0), x(0), y(0), site_num(0) {}
    Row(const string& name, int site_width, int height, int x, int y, int site_num)
        : name(name), site_width(site_width), height(height), x(x), y(y), site_num(site_num) {}
    string name;
    int site_width;
    int height, x, y, site_num;
};

struct Cluster {
    double x, q;
    int weight, width;
    vector<Cell*> members;
     Cluster* prev = nullptr;
    Cluster(double _x, Cluster* pred = nullptr) : x(_x), q(0), weight(0), width(0), prev(pred) {}
};

struct SubRow {
    double min_x, max_x; 
    double y;
    int free_space;
    int site_width = 0;
    vector<Cell*> placed_cells;

    Cluster* last_cluster = nullptr;

    SubRow(double _min_x, double _max_x, double _y, int _site_width = 0)
        : min_x(_min_x), max_x(_max_x), y(_y), free_space(_max_x - _min_x), site_width(_site_width) {}
};


#endif // LIB_H
