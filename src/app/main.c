#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <locale.h>

#include "utility.h"
#include "huffman_node.h"
#include "huffman_processing.h"
#include "bitarray.h"
#include "io.h"

#include "timer.h"

#define COUNTOF(ARR) ((&(ARR))[1] - (ARR))

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
    int64_t length = bitarray_length(bitarray);
    int64_t read_length = MIN(length, 10'000);
    bool* buff = malloc(read_length * sizeof(bool));
    const int offset = 0;

    int64_t succesful_read = bitarray_read_bits(bitarray, offset, read_length - offset, buff);

    for (int64_t i = 0; i < succesful_read; ++i)
    {
        if (i % 8 == 0 && i != 0)
            printf("");

        printf("%d", buff[i]);
    }
    printf(" (%d)(%d)", length, succesful_read);

    free(buff);
}

void print_bitarr_old(const bitarray* bitarray)
{
    const int64_t length = MIN(bitarray_length(bitarray), 10'000);

    for (int64_t i = 0; i < length; ++i)
    {
        printf("%d", bitarray_value_at(bitarray, i));
    }
    printf(" (%d)", length);
}

int main5(void)
{
    const bool boolarr[] = { 0, 1, 1, 0, 1, 1 };
    bitarray bitarr = {0};
    FILE* fp = open_file_write("./bitarr_buff");

    bitarray_init_io_buffered(&bitarr, fp);
    bitarray_append_bit(&bitarr, 1);
    bitarray_append_bit(&bitarr, 1);
    const int64_t written = bitarray_append_bits(&bitarr, COUNTOF(boolarr), boolarr);

    printf("wrote: %ld\n", written);

    print_bitarr(&bitarr);
    puts("");

    bitarray_clean(&bitarr);

    return EXIT_SUCCESS;
}

int main(void)
{
    setlocale(LC_ALL, "");
    struct timer duration_timer = {};
    huffman_node tree_root = {0};
    bitarray arr = {0};
    const char* text_filename;
    const char* buff_filename;
    FILE* text_fp    = NULL;
    FILE* bitarr_buf = NULL;

    text_filename = "./input";
    buff_filename = "./bitarr_buff";
    text_fp  = open_file_read(text_filename);
    puts("Opened text file");

    start_timer(&duration_timer);
    dlinked_list_node* counted_nodes = huffman_count_stream_table(text_fp);
    end_timer(&duration_timer);

    rewind(text_fp);
    puts("Parsed for symbol-count:");
    doublyll_print(counted_nodes);
    printf("Took %'0.lf clocks\n", (double)timer_duration(&duration_timer));

    start_timer(&duration_timer);
    tree_root = huffman_construct_tree(counted_nodes);
    end_timer(&duration_timer);

    puts("Constructed tree");
    printf("Took %'0.lf clocks\n", (double)timer_duration(&duration_timer));
    huffman_node_print_depth(&tree_root);
    puts("");

    bitarr_buf = open_file_write(buff_filename);
    // bitarray_init_io_buffered(&arr, bitarr_buf);
    bitarray_init_in_mem(&arr);

    start_timer(&duration_timer);
    const bitarray_length_t processed = huffman_compress_stream(&tree_root, text_fp, &arr);
    end_timer(&duration_timer);

    const bitarray_length_t length = bitarray_length(&arr);

    printf("Compressed file (%llu bits)\n", processed);
    printf("compressed-length: %zu bytes\n", (length >> 3) + 1);
    printf("Took %'0.lf clocks\n", (double)timer_duration(&duration_timer));

    print_bitarr(&arr);
    puts("");

    // write compressed into file
    puts("Writing compressed into file...");
    FILE* comp_fp = open_file_write("compressed.huff");

    start_timer(&duration_timer);
    bitarray_write_file(&arr, comp_fp, -1);
    end_timer(&duration_timer);
    printf("Took %'0.lf clocks\n", (double)timer_duration(&duration_timer));

    rewind(comp_fp);

    // Restore into different bitarray
    puts("\nRestoring from file into bitarray");
    bitarray rest_arr = {0};
    bitarray_init_io_buffered(&rest_arr, bitarr_buf);
    // bitarray_init_in_mem(&rest_arr);

    start_timer(&duration_timer);
    bitarray_read_file(&rest_arr, comp_fp, -1);
    end_timer(&duration_timer);
    printf("Took %'0.lf clocks\n", (double)timer_duration(&duration_timer));

    close_file(comp_fp);
    print_bitarr(&rest_arr);
    puts("");

    // Write the restored bitarray into output so we know the whole process was successful or not
    close_file(text_fp);
    text_fp = open_file_write("./output");

    start_timer(&duration_timer);
    const size_t written = huffman_decompress_stream(&tree_root, &rest_arr, text_fp);
    end_timer(&duration_timer);

    puts("Decompressed bitarray");
    printf("Written into stream: %zu bytes\n", written);
    printf("Reached compression-ratio of: %0.1f%%\n", (int64_t)(length - (written * 8)) / (float)(written * 8) * 100 );
    printf("Took %'0.lf clocks\n", (double)timer_duration(&duration_timer));

    huffman_node_clean(&tree_root);
    close_file(text_fp);
    bitarray_clean(&arr);
    bitarray_clean(&rest_arr);

    return EXIT_SUCCESS;
}

int main4(void)
{
    const char* filename = "./input";
    FILE* fp = open_file_read(filename);
    puts("Opened file");

    dlinked_list_node* counted_nodes = huffman_count_stream_onepass(fp);
    rewind(fp);
    puts("Parsed file");

    doublyll_print(counted_nodes);

    huffman_node tree_root = huffman_construct_tree(counted_nodes);
    puts("Constructed tree");
    huffman_node_print_depth(&tree_root);
    puts("");

    bitarray arr = {0};
    bitarray_init_in_mem(&arr);

    const bitarray_length_t processed = huffman_compress_stream(&tree_root, fp, &arr);
    const bitarray_length_t length = bitarray_length(&arr);

    printf("Compressed file (%llu bits)\n", processed);
    printf("compressed-length: %zu bytes\n", (length >> 3) + 1);
    close_file(fp);

    print_bitarr(&arr);
    puts("");

    fp = open_file_write("./output");
    const size_t written = huffman_decompress_stream(&tree_root, &arr, fp);
    puts("Decompressed bitarray");
    printf("Written into stream: %zu bytes\n", written);
    printf("Reached compression-ratio of: %0.1f%%\n", (int64_t)(length - (written * 8)) / (float)(written * 8) * 100 );

    huffman_node_clean(&tree_root);
    bitarray_clean(&arr);
    close_file(fp);

    return EXIT_SUCCESS;
}

int main3(void)
{
    const size_t text_length = strlen((char*)test_text);
    printf("Text: (%lu)\n%s\n", text_length, test_text);

    dlinked_list_node* list_root = huffman_count_text(test_text, text_length);
    huffman_node huffman_root = huffman_construct_tree(list_root);

    bitarray my_bitarr = {0};
    bitarray_init_in_mem(&my_bitarr);

    huffman_compress(&huffman_root, text_length, test_text, &my_bitarr);
    printf("Bitarray: (%li)\n", bitarray_length(&my_bitarr));
    print_bitarr(&my_bitarr);
    puts("");

    FILE* fp = open_file_write("./bitarray.txt");
    bitarray_write_file(&my_bitarr, fp, -1);
    bitarray_clean(&my_bitarr);
    rewind(fp);

    bitarray file_read = {0};
    bitarray_init_in_mem(&file_read);
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
    bitarray_init_in_mem(&my_bitarr);

    const int64_t length = strlen((char*)test_text);
    for (int64_t idx = 0; idx < length; idx++)
    {
        int64_t path_len = 0;
        bool* path = huffman_node_path(&huffman_root, test_text[idx], &path_len);
        while (path_len-- > 0)
            bitarray_append_bit(&my_bitarr, path[path_len]);
    }

    int byte_count = bitarray_length(&my_bitarr) / (sizeof(char) * 8) + ((bitarray_length(&my_bitarr) % 8) > 0);
    printf("\nlength: %zu (%d)\n", bitarray_length(&my_bitarr), byte_count);
    printf("bit-array:\n");
    print_bitarr(&my_bitarr);
    puts("");

    char* buf = (char*)vector_get_start_addr_(my_bitarr.in_mem.content);
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

    bitarray_init_in_mem(&my_bitarr);
    bitarray_read_file(&my_bitarr, fp, -1);

    byte_count = bitarray_length(&my_bitarr) / (sizeof(char) * 8) + ((bitarray_length(&my_bitarr) % 8) > 0);
    printf("\nbitarray (from-file) (%lu | %d):\n", bitarray_length(&my_bitarr), byte_count);
    print_bitarr(&my_bitarr);
    puts("");

    bitarray_clean(&my_bitarr);
    close_file(fp);

    return EXIT_SUCCESS;
}

