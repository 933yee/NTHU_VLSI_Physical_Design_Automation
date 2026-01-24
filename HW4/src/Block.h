#ifndef BLOCK_H 
#define BLOCK_H

#include <unordered_map>
#include <string>

using namespace std;

struct Block{
    Block(){}
    Block(string name_, int width_, int height_): name(name_), width(width_), height(height_) {}
    string name;
    int width, height;
    bool isOrphan = true; 
};

struct SymPair{
    SymPair(){}
    SymPair(Block* block1_, Block* block2_): block1(block1_), block2(block2_) {}
    Block* block1;
    Block* block2;
};

struct SymGroup{
    SymGroup(){}
    SymGroup(string name_, int size_): name(name_), size(size_) {}
    string name;
    vector<SymPair*> symPairs;
    vector<Block*> symSelf;
    int size;
};

#endif // BLOCK_H
