#ifndef BLOCK_H 
#define BLOCK_H

#include <unordered_map>
#include <string>

using namespace std;

struct Block{
    Block(){}
    Block(string name){
        this->name = name;
    }
    Block(string name, bool is_leaf) {
        this->name = name;
        this->is_leaf = is_leaf;
    }
    Block(string name, long long width, long long height) {
        this->name = name;
        this->width = width;
        this->height = height;
    }
    Block(string name, long long width, long long height, int orientation) {
        this->name = name;
        this->width = width;
        this->height = height;
        this->orientation = orientation;
    }
    string name;
    bool is_leaf = true; // true: leaf block, false: non-leaf block
    long long width, height;
    long long x = 0, y = 0; // coordinates of the block in the floorplan
    int orientation = 0; // 0: normal, 1: rotated
    long long cx = 0, cy = 0; // center of the block
};

struct Pad{
    Pad(){}
    Pad(string name, long long x, long long y) {
        this->name = name;
        this->x = x;
        this->y = y;
    }
    string name;
    long long x, y;
};

struct Net{
    Net(){}
    Net(string name, int num_pins) {
        this->name = name;
        this->num_pins = num_pins;
    }
    string name;
    int num_pins;
    unordered_map<string, Pad*> pin_to_pad; 
    unordered_map<string, Block*> pin_to_block; 
};

#endif // BLOCK_H
