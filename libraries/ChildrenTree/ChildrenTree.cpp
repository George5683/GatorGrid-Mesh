#include "ChildrenTree.hpp"
#include "vector"

// Node can't have more than 4 children, 3 at most so 4th can get 

ChildrenTree::~ChildrenTree() {
    if (!head) {
        return;
    }
    remove_children(head->id);
    delete head;
    head = nullptr;
}

/*
void ChildrenTree::remove_children(Node* node) {
    if (!node) {
        return;
    }
    for (int i = 0; i < node->number_of_children; i++) {
        remove_children(node->children[i]);
        delete node->children[i];
    }
    node->number_of_children = 0;
}*/

bool ChildrenTree::remove_node(uint32_t id) {
    Node* node = this->find_node(id, this->head);
    if (node == nullptr) {
        return false;
    }

    Node **children = node->children;
    Node* newParent = this->head;
    int childrenToAdd = node->number_of_children;
    int index = 0;
    /*
    while (childrenToAdd > 0) {
        // Try adding to deleted nodes parent
        while (newParent->number_of_children < 4) {
            int id = children[index]->id;
            this->add_any_child(id, newParent->id);
            index++;
            childrenToAdd--;
        }
        
    }
    */

    //current method for reshaping tree, delete then add node again
   for (int i = 0; i < childrenToAdd; i++) {
        int id = children[i]->id;
        this->remove_node(id);
        add_child(id);
   }
    return true;
}

void ChildrenTree::remove_children(int id) {
    Node* node = this->find_node(id, this->head);
    for (int i = 0; i < node->number_of_children; i++) {
        for (int j = 0; j < node->children[i]->number_of_children; j++) {
            int childId = node->children[i]->id;
            add_any_child(node->id, childId);
            delete node->children[i]->children[j];
        }
        remove_children(node->children[i]->id);
        delete node->children[i];
    }
    node->number_of_children = 0;
}


bool ChildrenTree::add_child(uint32_t child_id) {
    Node* child = new Node(child_id);
    child->parent = head;
    std::cout << child->parent->id<<"here\n";
    Node **newChildren = new Node*[head->number_of_children+1];
    if (head->number_of_children != 0) {
        for (int i = 0; i < head->number_of_children; ++i)
            newChildren[i] = head->children[i];
        delete head->children;
    }
    newChildren[head->number_of_children] = child;
    head->children = newChildren;
    head->number_of_children+=1;
    if (head->number_of_children > 3) {
        std::cout<< "\nWARNING: Number of children for head is greater than 3\n";
    }
    return true;
}

bool ChildrenTree::add_any_child(uint32_t parent_id, uint32_t child_id) {
    bool success = false;
    if (parent_id == id) {
        Node* child = new Node(child_id);
        head->children[head->number_of_children++] = child;
        if (head->number_of_children > 3) {
            std::cout<< "\nWARNING: Number of children for head is greater than 3\n";
        }
        success = true;
    } else {
        Node* parent = find_node(parent_id, head);
        std::cout << "\nPID: " << parent->id;
        if (!parent) {
            success = false;
        } else {
            Node* child = new Node(child_id);
            child->parent = parent;
            Node **newChildren = new Node*[head->number_of_children+1];
            if (parent->number_of_children != 0) {
                for (int i = 0; i < parent->number_of_children; ++i)
                newChildren[i] = parent->children[i];
                delete parent->children;
            }
            newChildren[parent->number_of_children] = child;
            parent->children = newChildren;
            parent->number_of_children+=1;
            if (parent->number_of_children > 3) {
                std::cout<< "\nWARNING: Number of children for node " << parent->id <<" is greater than 3\n";
            }
        }

    }
    return success;
}

ChildrenTree::Node* ChildrenTree::find_node(uint32_t id, Node* head) {
    if (head->number_of_children == 0) {
        return nullptr;
    }
    for (int i = 0; i < head->number_of_children; i++) {
        if (head->children[i]->id == id) {
            return head->children[i];
        } else {
            Node* ret_val = find_node(id, head->children[i]);
            if (ret_val) {
                return ret_val;
            }
        }
    }
    return nullptr;
}

bool ChildrenTree::node_exists(uint32_t id) {
    if (find_node(id, head) != nullptr) {
        return true;
    }
    return false;
}

/**
 * @brief Returns true or false if path exists, and parent is overwritten as the path's parent
 * 
 * @param id node to find
 * @param parent of the path to node to find
 * @return true 
 * @return false 
 */
 // Not finished
 bool ChildrenTree::find_path_parent(uint32_t id, uint32_t *parent) {
    printf("Checking tree for parent of the path to %u\n", id);
    if (!head || !find_node(id, head)) {
        // either empty tree or id not in tree
        return false;
    }
    return find_parent_recursive(head, id, parent);
}

//Not finished
bool ChildrenTree::find_parent_recursive(Node* node, uint32_t target, uint32_t *parent) {
    for (int i = 0; i < node->number_of_children; ++i) {
        Node* child = node->children[i];
        if (child->id == target) {
            // this node is the parent of the target
            parent = &node->id;
            return true;
        }
        // otherwise, recurse into that child’s subtree
        if (find_parent_recursive(child, target, parent)) {
            return true;
        }
    }
    return false;
}

void ChildrenTree::traverse() {
    Node *root = this->head;
    if (root == nullptr) {
        return;
    }    

    std::vector<Node*> queue;
    queue.push_back(root);

    while (!queue.empty()) {
        Node* current = queue.front();
        queue.erase(queue.begin());

        std::cout << "\nNode ID: " << current->id << " | Children: " << (int)current->number_of_children << " | Parent ID: " << (current->parent ? std::to_string(current->parent->id) : "None") <<std::endl;

        if (current->children != nullptr) {
            for (int i = 0; i < current->number_of_children; ++i) {
                queue.push_back(current->children[i]);
            }
        }
    }
}

void ChildrenTree::get_node_details(ChildrenTree* tree) {
    Node* node = tree->head;
    if (node == this->head) {
        std::cout << "Head\n";
    }
    else {
        std::cout << "Not Head\n";
    }
    
    std::cout << "Number of Children: "<<(int)node->number_of_children;
    if(node->number_of_children == 0) std::cout << "No Children";
    std::cout << "\nID: " << node->id;
}