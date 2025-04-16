
#ifndef CHILDREN_TREE_HPP
#define CHILDREN_TREE_HPP

#include <cstdint>

class ChildrenTree {
    struct Node {
        uint32_t id;
        Node* parent;
        Node** children;
        uint8_t number_of_children;

        Node() : id(0), parent(nullptr), children(nullptr), number_of_children(0) {}
        Node(uint32_t id) : id(id), parent(nullptr), children(nullptr), number_of_children(0) {}

    };
    Node* head;
    uint32_t id;
public:
    ChildrenTree(uint32_t id) : id(id) {
        head = new Node(id);
    } 
    ~ChildrenTree();
    bool add_child(uint32_t parent_id, uint32_t child_id);
    void remove_children(Node* node);

    Node* find_node(uint32_t id, Node* head);
    bool node_exists(uint32_t id);
    bool find_path_parent(uint32_t id, uint32_t* parent);
};

#endif