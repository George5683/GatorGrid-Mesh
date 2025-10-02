#ifndef CHILDREN_TREE_HPP
#define CHILDREN_TREE_HPP

#include <cstdint>
#include <cstdio>
#include <iostream>

class ChildrenTree {
    struct Node {
        uint32_t id;
        Node* parent;
        Node** children;
        uint8_t number_of_children;

        Node() : id(0), parent(nullptr), children(new Node*[4]), number_of_children(0) {}
        Node(uint32_t id) : id(id), parent(nullptr), children(new Node*[4]), number_of_children(0) {}

    };
    Node* head;
    uint32_t id;
public:
    ChildrenTree(uint32_t id) : id(id) {
        head = new Node(id);
    } 
    ~ChildrenTree();
    bool add_child(uint32_t child_id);
    bool add_any_child(uint32_t parent_id, uint32_t child_id);
    void remove_children(int id);
    bool remove_child(uint32_t id);

    bool update_node(uint32_t id, uint32_t children_id[4], uint8_t &number_of_children);

    void edit_head(uint32_t new_id);

    /**
     * @brief Get the list of children id from a given parent
     * 
     * @param [in] parent_id 
     * @param [out] children_id - must be allocated for max number of children - 4
     * @param [out] number_of_children 
     * @return true     parent node found
     * @return false    parent node not found
     */
    bool get_children(uint32_t parent_id, uint32_t children_id[4], uint8_t &number_of_children);

    Node* find_node(uint32_t id, Node* head);
    bool node_exists(uint32_t id);
    bool find_path_parent(uint32_t id, uint32_t *parent);
    bool find_parent_recursive(Node* node, uint32_t target);
    void traverse();
    void get_node_details(ChildrenTree *tree);
};

#endif