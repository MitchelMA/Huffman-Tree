#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "huffman_node.h"
#include "huffman_processing.h"

static const uint8_t* test_text = u8"this is an example of a huffman tree";

void doublyll_print(dlinked_list_node* root)
{
    int leaf_count = 0;

    while (true)
    {
        leaf_count++;
        huffman_node_print_inline(&root->node);
        printf(" <-> ");

        if (root->next != NULL)
            root = root->next;
        else
            break;
    }
    printf("\b\b\b\b(%d)\n", leaf_count);
}

void print_path(const huffman_node* root, uint8_t symbol)
{
    int64_t path_len = 0;
    bool* bool_arr = huffman_node_path(root, symbol, &path_len);

    printf("path for '%c': ", symbol);

    path_len--;
    for (; path_len >= 0; path_len--)
        printf("%d", bool_arr[path_len]);
    puts("");
}

int main(void)
{
    dlinked_list_node* list_root = huffman_count_text(test_text, strlen((char*)test_text));
    doublyll_print(list_root);
    puts("");

    huffman_node huffman_root = huffman_construct_tree(list_root);

    huffman_node_print_depth(&huffman_root);
    puts("");
    print_path(&huffman_root, ' ');
    print_path(&huffman_root, 'a');
    print_path(&huffman_root, 'e');
    print_path(&huffman_root, 'f');
    print_path(&huffman_root, 'h');
    print_path(&huffman_root, 'i');
    print_path(&huffman_root, 'm');
    print_path(&huffman_root, 'n');
    print_path(&huffman_root, 's');
    print_path(&huffman_root, 't');
    print_path(&huffman_root, 'l');
    print_path(&huffman_root, 'o');
    print_path(&huffman_root, 'p');
    print_path(&huffman_root, 'r');
    print_path(&huffman_root, 'u');
    print_path(&huffman_root, 'x');

    huffman_node_clean(&huffman_root);

    puts("");


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
