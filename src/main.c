#include <stdio.h>
#include <stdlib.h>

#include "HuffmanNode.h"

int main(void)
{
    huffman_node a_node = {0};
    huffman_node_init_leaf(&a_node, 'a', 15);

    huffman_node b_node = {0};
    huffman_node_init_leaf(&b_node, 'b', 7);

    huffman_node c_node = {0};
    huffman_node_init_leaf(&c_node, 'c', 6);

    huffman_node d_node = {0};
    huffman_node_init_leaf(&d_node, 'd', 6);

    huffman_node e_node = {0};
    huffman_node_init_leaf(&e_node, 'e', 5);

    huffman_node de_branching = {0};
    huffman_node_init_branching(&de_branching);
    huffman_node_assign_left(&de_branching, &d_node);
    huffman_node_assign_right(&de_branching, &e_node);

    huffman_node bc_branching = {0};
    huffman_node_init_branching(&bc_branching);
    huffman_node_assign_left(&bc_branching, &b_node);
    huffman_node_assign_right(&bc_branching, &c_node);

    huffman_node bcde_branching = {0};
    huffman_node_init_branching(&bcde_branching);
    huffman_node_assign_left(&bcde_branching, &bc_branching);
    huffman_node_assign_right(&bcde_branching, &de_branching);

    huffman_node root = {0};
    huffman_node_init_branching(&root);
    huffman_node_assign_left(&root, &a_node);
    huffman_node_assign_right(&root, &bcde_branching);

    puts("Before clean:");
    huffman_node_print_depth(&root);
    puts("");
    printf("total-count: %llu\n", huffman_node_count(&root));

    huffman_node_clean(&root);

    puts("After clean:");
    huffman_node_print_depth(&root);
    puts("");
    printf("total-count: %llu\n", huffman_node_count(&root));

    return EXIT_SUCCESS;
}
