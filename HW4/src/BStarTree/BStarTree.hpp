#pragma once
#include <vector>
#include <limits>
#include <memory>
#include <unordered_map>
#include <cassert>
#include <string>
#include "../Block.h"


template <typename T>
class BStarTree;

/**
 * @brief A very simple segment tree for RMQ from TAs
 */
template <typename T>
class SegmentTree
{
    struct Node
    {
        T data, tag;
        bool hasTag;

        Node() : data(0), tag(0), hasTag(false) {}
    };

    size_t n;
    std::vector<Node> seg;

    T getVal(size_t id) const
    {
        return (seg[id].hasTag) ? seg[id].tag : seg[id].data;
    }

    void pull(size_t id)
    {
        seg[id].data = std::max(getVal(id * 2), getVal(id * 2 + 1));
    }

    void push(size_t id)
    {
        if (seg[id].hasTag)
        {
            seg[id].data = getVal(id);
            seg[id].hasTag = false;
            seg[id * 2].tag = seg[id].tag;
            seg[id * 2].hasTag = true;
            seg[id * 2 + 1].tag = seg[id].tag;
            seg[id * 2 + 1].hasTag = true;
        }
    }

    T query(size_t ql, size_t qr, size_t l, size_t r, size_t id = 1)
    {
        if (ql > r || qr < l)
            return std::numeric_limits<T>::min();
        if (ql <= l && qr >= r)
            return getVal(id);

        push(id);
        int mid = (l + r) / 2;
        return std::max(query(ql, qr, l, mid, id * 2), query(ql, qr, mid + 1, r, id * 2 + 1));
    }

    void update(T val, size_t ql, size_t qr, size_t l, size_t r, size_t id = 1)
    {
        if (ql > r || qr < l)
            return;
        if (ql <= l && qr >= r)
        {
            seg[id].tag = val;
            seg[id].hasTag = true;
            return;
        }

        push(id);
        size_t mid = (l + r) / 2;
        update(val, ql, qr, l, mid, id * 2);
        update(val, ql, qr, mid + 1, r, id * 2 + 1);
        pull(id);
    }

public:
    SegmentTree() : n(0) {}

    void init(size_t n_)
    {
        n = n_;
        seg.assign(n_ * 4, {});
    }

    T query(size_t ql, size_t qr)
    {
        return query(ql, qr, 0ULL, n - 1);
    }

    void update(size_t ql, size_t qr, T val)
    {
        update(val, ql, qr, 0ULL, n - 1);
    }
};

/**
 * @brief The node structure of the B*-tree
 */
template <typename T>
struct Node
{
    using ptr = std::unique_ptr<Node>;

    T x, y;
    T width, height;
    std::string name;
    int rotated = 0;
    Node *lchild, *rchild;

    // is island (預設是切 vertical)
    bool island = false;
    bool isSymSelf = false;
    BStarTree<T> *islandTree = nullptr;
    Node<T> * parent = nullptr;
    vector<Node<T> *> symPairs;
    vector<Node<T> *> symSelf;
    vector<Node<T> *> innerNodes;
    vector<Node<T> *> inorder;
    vector<Node<T> *> preorder;
    T symSelfWidth = 0, symSelfHeight = 0;

    Node() : x(0), y(0), width(0), height(0), lchild(nullptr), rchild(nullptr) {}

    void setPosition(T x_, T y_)
    {
        x = x_;
        y = y_;
    }

    void setShape(T width_, T height_)
    {
        width = width_;
        height = height_;
    }

    void setSymSelfShape(T width_, T height_)
    {
        symSelfWidth = width_;
        symSelfHeight = height_;
    }


    void inorderTraversal(Node<int>* node, vector<Node<int>*>& inorder) {
        if (node == nullptr) return;
        inorderTraversal(node->lchild, inorder);
        inorder.push_back(node);
        inorderTraversal(node->rchild, inorder);
    }
    
    void preorderTraversal(Node<int>* node, vector<Node<int>*>& preorder) {
        if (node == nullptr) return;
        preorder.push_back(node);
        preorderTraversal(node->lchild, preorder);
        preorderTraversal(node->rchild, preorder);
    }
    
    void buildInitialTree(vector<Node<int>*>& nodes) {
        Node<int>* root = nodes[0];
        Node<int>* currentHead = root;
        for (int i = 1; i < nodes.size(); ++i) {
            Node<int>* node = nodes[i];
            int nodeWidth = max(node->width, node->height);
            currentHead->lchild = node;
            currentHead = node;
        }
    }

    void initIsland(SymGroup *group)
    {
        this->island = true;
        this->islandTree = new BStarTree<T>();
        this->rotated = 0;

        // pair 只要記錄其中一邊就好
        for (auto* pair : group->symPairs)
        {
            Node<T>* node1 = new Node<T>();
            node1->name = pair->block1->name;
            node1->rotated = 0;
            node1->setShape(pair->block1->width, pair->block1->height);
            this->symPairs.push_back(node1);
            this->innerNodes.push_back(node1);
        }

        // 先建樹，因為 symSelf 一定要在 Right Most Node 或 Left Most Node，建完再加進去
        sort(this->symPairs.begin(), this->symPairs.end(), [](Node<T>* a, Node<T>* b) {
            return max(a->width, a->height) > max(b->width, b->height);
        });
        buildInitialTree(this->symPairs);
        preorderTraversal(this->symPairs[0], preorder);
        inorderTraversal(this->symPairs[0], inorder);
        this->islandTree->buildTree(this->preorder, this->inorder);

        // 預設是切 vertical，要找出最右邊的 node 加入 symSelf
        if(this->rotated == 0)
        {
            Node<T>* rightMost = this->islandTree->root;
            while (rightMost->rchild) rightMost = rightMost->rchild;
        
            // 把 symSelf 加進去
            for (auto* block : group->symSelf)
            {
                Node<T>* newNode = new Node<T>();
                newNode->name = block->name;
                newNode->rotated = 0;
                newNode->isSymSelf = true;
                newNode->parent = this;
                newNode->setSymSelfShape(block->width, block->height);
                newNode->setShape(block->width / 2, block->height);
        
                rightMost->rchild = newNode;
                rightMost = newNode;
                this->symSelf.push_back(newNode);
                this->innerNodes.push_back(newNode);
            }
            
            this->islandTree->setPosition();
            auto [width, height] = this->islandTree->getWidthHeight(this->islandTree->root);
            this->setShape(width * 2, height);
        }
        else
        {
            Node<T>* leftMost = this->islandTree->root;
            while (leftMost->lchild)
            leftMost = leftMost->lchild;

            // 把 symSelf 加進去
            for (auto* block : group->symSelf)
            {
                Node<T>* newNode = new Node<T>();
                newNode->name = block->name;
                newNode->rotated = 0;
                newNode->isSymSelf = true;
                newNode->parent = this;
                newNode->setSymSelfShape(block->width, block->height);
                newNode->setShape(block->width, block->height / 2);

                leftMost->lchild = newNode;
                leftMost = newNode;
                this->symSelf.push_back(newNode);
                this->innerNodes.push_back(newNode);
            }

            this->islandTree->setPosition();
            auto [width, height] = this->islandTree->getWidthHeight(this->islandTree->root);
            this->setShape(width, height * 2);
        }
        // cout << preorder.size() << " " << inorder.size() << endl;
        this->preorder.clear();
        this->inorder.clear();
        preorderTraversal(this->islandTree->root, this->preorder);
        inorderTraversal(this->islandTree->root, this->inorder);
        this->islandTree->buildTree(this->preorder, this->inorder);
    }

    void buildIslandTree()
    {
        this->islandTree->buildTree(this->preorder, this->inorder);
        this->islandTree->setPosition();
        auto [width, height] = this->islandTree->getWidthHeight(this->islandTree->root);
        this->rotated == 0 ? this->setShape(width * 2, height) : this->setShape(width, height * 2);
    }
};


/**
 * @brief A B*-tree to calculate the coordinates of nodes and the area of placement
 */
template <typename T>
class BStarTree
{
    std::unordered_map<Node<T> *, int64_t> toInorderIdx;

    Node<T> *buildTree(const std::vector<Node<T> *> &preorder, const std::vector<Node<T> *> &inorder, size_t &i, int64_t l, int64_t r)
    {
        if (l > r || i >= preorder.size())
            return nullptr;

        Node<T> *node = preorder[i++];

        if (node->island) node->buildIslandTree();
        
        assert(toInorderIdx.count(node) > 0 && "Node not found in inorder map.");
        int64_t idx = toInorderIdx[node];
        node->lchild = buildTree(preorder, inorder, i, l, idx - 1);
        node->rchild = buildTree(preorder, inorder, i, idx + 1, r);
        return node;
    }

    T getTotalWidth(Node<T> *node) const
    {
        if (!node)
            return 0;
        
        T maxWidth = std::max(node->width, node->height);
        return maxWidth + getTotalWidth(node->lchild) + getTotalWidth(node->rchild);
    }

    void setPosition(Node<T> *node, T startX)
    {
        if (!node)
            return;
    
        T nodeWidth = node->width;
        T nodeHeight = node->height;
    
        if(node->rotated && !node->island){
            nodeWidth = node->height, nodeHeight = node->width;
        }
            
        if(node->isSymSelf)
        {
            if(node->rotated == 0 && node->parent->rotated == 0)
            {
                nodeWidth = node->symSelfWidth / 2;
                nodeHeight = node->symSelfHeight;
            }
            else if(node->rotated == 1 && node->parent->rotated == 1)
            {
                nodeWidth = node->symSelfHeight;
                nodeHeight = node->symSelfWidth / 2;
            }
            else if(node->rotated == 0 && node->parent->rotated == 1)
            {
                nodeWidth = node->symSelfWidth;
                nodeHeight = node->symSelfHeight / 2;
            }
            else
            {
                nodeWidth = node->symSelfHeight / 2;
                nodeHeight = node->symSelfWidth;
            }
        }

        T endX = startX + nodeWidth;
        T y = contourH.query(startX, endX - 1);


        if (node->island)
        {
            // vertical
            if(node->rotated == 0){
                for (int i = 0; i < nodeWidth / 2; ++i)
                {
                    T lx = -i;
                    T rx = i;
                    T absX = startX + nodeWidth / 2;
                    T dy = node->islandTree->contourH.query(rx, rx);
                    contourH.update(absX + rx, absX + rx, y + dy);
                    contourH.update(absX + lx, absX + lx, y + dy);
                }
            } else {
                for (int i = 0; i < nodeWidth; ++i)
                {
                    T dy = node->islandTree->contourH.query(i, i);
                    T absX = startX;
                    contourH.update(startX + i, startX + i, y + dy + nodeHeight / 2);
                }
            }
        }
        else
        {
            contourH.update(startX, endX - 1, y + nodeHeight);
        }
        

        node->setPosition(startX, y);
        if(node->island) node->islandTree->setPosition();
    
        setPosition(node->lchild, endX);  
        setPosition(node->rchild, startX); 
    }
    
public:
    SegmentTree<T> contourH;

    std::pair<T, T> getWidthHeight(Node<T> *node) const
    {
        if (!node)
            return {0, 0};
    
        T nodeWidth = node->width;
        T nodeHeight = node->height;

        if(node->rotated && !node->island)
            nodeWidth = node->height, nodeHeight = node->width;
    
        auto [lMaxWidth, lMaxHeight] = getWidthHeight(node->lchild);
        auto [rMaxWidth, rMaxHeight] = getWidthHeight(node->rchild);
    
        T maxWidth = std::max({lMaxWidth, rMaxWidth, node->x + nodeWidth});
        T maxHeight = std::max({lMaxHeight, rMaxHeight, node->y + nodeHeight});
        return {maxWidth, maxHeight};
    }
    

    Node<T> *root;

    BStarTree() : root(nullptr) {}

    void buildTree(const std::vector<Node<T> *> &preorder, const std::vector<Node<T> *> &inorder)
    {
        // if(preorder.size() != inorder.size())
            // cout << preorder.size() << " " << inorder.size() << endl;
        assert(preorder.size() == inorder.size() && "The size of preorder and inorder must be the same.");
        int64_t n = inorder.size();

        toInorderIdx.clear();
        for (int64_t i = 0; i < n; ++i)
            toInorderIdx[inorder[i]] = i;

        size_t i = 0;
        root = buildTree(preorder, inorder, i, 0LL, n - 1);
    }

    void setPosition()
    {
        contourH.init(getTotalWidth(root)*2);
        setPosition(root, 0);
    }

    T getArea() const
    {
        auto [width, height] = getWidthHeight(root);
        // cout << "width: " << width << " height: " << height << endl;
        // cout << "area: " << width * height << endl;
        return width * height;
    }

    void debugPreorder(Node<T> *node)
    {
        if (!node)
            return;

        cout << node->name << " ";
        debugPreorder(node->lchild);
        debugPreorder(node->rchild);
    }
};
