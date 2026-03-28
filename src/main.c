#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "huffman_node.h"
#include "huffman_processing.h"
#include "bitarray.h"
#include "io.h"

static const uint8_t* test_text = u8"this is an example of a huffman tree";
// static const uint8_t* test_text_back = u8"this is an example of a huffman treeloprloprhimnst";

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

void print_compressed_text(const uint8_t* text, const huffman_node* tree)
{
    const int64_t length = strlen((char*)text);
    for (int64_t idx = 0; idx < length; idx++)
    {
        int64_t path_len = 0;
        bool* path = huffman_node_path(tree, text[idx], &path_len);
        while (path_len-- > 0)
            printf("%d", path[path_len]);

        printf("");
    }
    puts("");
}

void print_uncompressed_text(const uint8_t* text)
{
    const int64_t length = strlen((char*)text);
    for (int64_t idx = 0; idx < length; idx++)
    {
        for (int off = 7; off >= 0; off--)
        {
            int mask = (1 << off);
            int value = text[idx] & mask;
            printf("%d", value > 0);
        }
        printf("");
    }

    puts("");
}

void print_bitarr(const bitarray* bitarray)
{
    for (int64_t i = 0; i < (int64_t)bitarray->length; i++)
    {
        if (i % 8 == 0 && i != 0)
            printf("");

        printf("%d", bitarray_value_at(bitarray, i));
    }
}

int main(void)
{
    const size_t text_length = strlen((char*)test_text);
    printf("Text: (%lu)\n%s\n", text_length, test_text);

    dlinked_list_node* list_root = huffman_count_text(test_text, text_length);
    huffman_node huffman_root = huffman_construct_tree(list_root);

    bitarray my_bitarr = huffman_compress(&huffman_root, text_length, test_text);
    printf("Bitarray: (%li)\n", my_bitarr.length);
    print_bitarr(&my_bitarr);
    puts("");

    FILE* fp = open_file_write("./bitarray.txt");
    bitarray_write_file(&my_bitarr, fp, -1);
    bitarray_clean(&my_bitarr);
    rewind(fp);

    bitarray file_read = {0};
    bitarray_init(&file_read);
    bitarray_read_file(&file_read, fp, -1);
    close_file(fp);

    char BUFF[1024] = {0};

    huffman_decompress(&huffman_root, &file_read, 1024, (uint8_t*)BUFF);
    huffman_node_clean(&huffman_root);
    bitarray_clean(&file_read);

    printf("Recovered: (%lu)\n%s\n", strlen(BUFF), BUFF);

    return EXIT_SUCCESS;
}

int main2(void)
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

    puts("");

    printf("text: %s\n", test_text);
    puts("compressed:");
    print_compressed_text(test_text, &huffman_root);
    puts("uncompressed:");
    print_uncompressed_text(test_text);


    bitarray my_bitarr = {0};
    bitarray_init(&my_bitarr);

    const int64_t length = strlen((char*)test_text);
    for (int64_t idx = 0; idx < length; idx++)
    {
        int64_t path_len = 0;
        bool* path = huffman_node_path(&huffman_root, test_text[idx], &path_len);
        while (path_len-- > 0)
            bitarray_append_bit(&my_bitarr, path[path_len]);
    }

    int byte_count = my_bitarr.length / (sizeof(char) * 8) + ((my_bitarr.length % 8) > 0);
    printf("\nlength: %zu (%d)\n", my_bitarr.length, byte_count);
    printf("bit-array:\n");
    print_bitarr(&my_bitarr);
    puts("");

    char* buf = (char*)vector_get_start_addr_(my_bitarr.content);
    puts("\nbinary-char-print-test:");
    for (int i = 0; i < byte_count; i++)
        printf("%08hhb", buf[i]);
    puts("|");

    puts("\nchar-print-test:");
    for (int i = 0; i < byte_count; i++)
        printf("%c", buf[i]);
    puts("|");

    FILE* fp = open_file_write("./bitarray.txt");
    bitarray_write_file(&my_bitarr, fp, -1);
    rewind(fp);

    bitarray_clean(&my_bitarr);
    huffman_node_clean(&huffman_root);

    bitarray_init(&my_bitarr);
    bitarray_read_file(&my_bitarr, fp, -1);

    byte_count = my_bitarr.length / (sizeof(char) * 8) + ((my_bitarr.length % 8) > 0);
    printf("\nbitarray (from-file) (%lu | %d):\n", my_bitarr.length, byte_count);
    print_bitarr(&my_bitarr);
    puts("");

    bitarray_clean(&my_bitarr);
    close_file(fp);

    return EXIT_SUCCESS;
}

