#include "../libraries/ChildrenTree/ChildrenTree.hpp"
#include "pico/stdlib.h"

int main() {

    stdio_init_all();

    ChildrenTree *tree = new ChildrenTree(1);

    sleep_ms(10000);

    tree->add_child(2);
    tree->add_child(3);
    tree->add_any_child(2, 4);
    tree->add_any_child(2, 5);
    tree->add_any_child(4, 6);
    tree->add_child(7);
    tree->add_child(8);
    tree->add_child(9);
    tree->add_any_child(2, 10);
    tree->add_any_child(2, 11);
    uint32_t ptr = 0;
    tree->find_path_parent(5, &ptr);
    std::cout<<ptr;

    uint32_t *ptr = nullptr;
    tree->find_path_parent(5, ptr);
    std::cout<<ptr;

    tree->traverse();
    tree->remove_children(4);
    tree->traverse();

    printf("Does Node 2 exist: %d\n", tree->node_exists(2));

    while (true) { tight_loop_contents(); }
}
