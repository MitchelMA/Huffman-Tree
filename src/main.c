#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "huffman_node.h"
#include "huffman_processing.h"

static const uint8_t* test_text = u8"this is an example of a huffman tree";

int main(void)
{
    dlinked_list_node* list_root = huffman_count_text(test_text, strlen((char*)test_text));
    int leaf_count = 0;

    while (true)
    {
        leaf_count++;
        huffman_node_print_inline(&list_root->node);
        printf(" -> ");

        huffman_node_clean(&list_root->node);

        if (list_root->next != NULL)
            list_root = list_root->next;
        else
            break;
    }
    printf(" (%d)\n", leaf_count);

    while (list_root->prev != NULL)
        list_root = list_root->prev;
    
    doublyll_clean(list_root);

    return EXIT_SUCCESS;
}

int main2(void)
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

    int64_t path_len = 0;
    uint8_t target = 'd';
    bool* path = huffman_node_path(&root, target, &path_len);

    puts("Before clean:");
    huffman_node_print_depth(&root);
    puts("");
    printf("total-count: %llu\n", huffman_node_count(&root));
    printf("length: %lli\n", path_len);
    printf("Path of '%c': ", target);

    if (path_len > 0)
        for (int64_t idx = path_len - 1; idx >= 0; idx--)
            printf("%d", path[idx]);
    else
        printf("-");

    puts("");

    huffman_node_clean(&root);

    puts("After clean:");
    huffman_node_print_depth(&root);
    puts("");
    printf("total-count: %llu\n", huffman_node_count(&root));

    return EXIT_SUCCESS;
}
