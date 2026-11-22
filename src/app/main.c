#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <locale.h>
#include <assert.h>

#include <command.h>
#include <option.h>
#include <command_tree.h>
#include <builtin-commands/help.h>

#include "utility.h"
#include "huffman_node.h"
#include "huffman_processing.h"
#include "bitarray.h"
#include "io.h"
#include "header.h"

static command_tree_s command_builder();
static int handle_compress(const command_s* command);
static int handle_decompress(const command_s* command);

int main(int argc, const char* argv[])
{
    setlocale(LC_ALL, "");

    command_tree_s command_tree = command_builder();
    int exit_code = EXIT_SUCCESS;

    if (!command_tree_parse_base(&command_tree, argc, argv))
    {
        print_global_help(stdout, &command_tree);
        exit_code = EXIT_FAILURE;
        goto exit_cleanup;
    }

    command_s* used_command = command_tree_get_called_command(&command_tree);
    assert(used_command != NULL);

    if (!command_parse(used_command))
    {

        exit_code = EXIT_SUCCESS;
        goto exit_cleanup;
    }

    if (handle_help_command(&command_tree, used_command))
    {
        exit_code = EXIT_SUCCESS;
        goto exit_cleanup;
    }

    if (command_is_of_flag(used_command, "--compress"))
    {
        exit_code = handle_compress(used_command);
        goto exit_cleanup;
    }

    if (command_is_of_flag(used_command, "--decompress"))
    {
        exit_code = handle_decompress(used_command);
        goto exit_cleanup;
    }

exit_cleanup:
    command_tree_clean(&command_tree);

    return exit_code;
}

command_tree_s
command_builder()
{
    command_tree_s tree = {0};
    command_tree_init(&tree, 3);
    command_tree_add_help(&tree);

    option_s buffered_option = {0};
    option_init(&buffered_option, false, OPTION_TYPE_BOOL, NULL);
    option_set_name(&buffered_option, "--buffered", 1, "-b");
    option_set_description(&buffered_option, "Wether the processing of compressing or decompressing is done through an io-stream, instead of in-memory");

    command_s compress_command = {0};
    command_init(&compress_command, 1);
    command_set_name(&compress_command, "--compress", 1, "-c");
    command_add_option(&compress_command, &buffered_option);

    command_tree_add_command(&tree, &compress_command);

    command_s decompress_command = {0};
    command_init(&decompress_command, 1);
    command_set_name(&decompress_command, "--decompress", 1, "-d");
    command_add_option(&decompress_command, &buffered_option);

    command_tree_add_command(&tree, &decompress_command);

    return tree;
}

int
handle_compress(
    const command_s* const command
)
{
    int param_count = 0;
    const char** params = command_get_parameters(command, &param_count);

    const bool is_io_buffered = command_read_bool_option(command, "--buffered");

    if (param_count <= 0)
        return EXIT_FAILURE;

    const char* const compressed_extension    = ".huff";
    const char* const bitarr_buffer_extension = ".buff";

    int exit_code = EXIT_SUCCESS;
    FILE* input_fp = NULL;
    FILE* output_fp = NULL;
    FILE* bitarr_buffer = NULL;

    huffman_node tree_root = {0};
    bitarray data_bit_array = {0};
    huffman_header header_data = {0};

    const char* const input_filename = params[0];
    char output_filename[256] = {0};
    char buffer_filename[256] = {0};

    uint8_t serialisation_buffer[2048] = {0};
    size_t serialisation_length = 0;

    strcpy(output_filename, input_filename);
    strcat(output_filename, compressed_extension);

    strcpy(buffer_filename, input_filename);
    strcat(buffer_filename, bitarr_buffer_extension);

    printf("output-filename: %s\n", output_filename);

    input_fp = open_file_read(input_filename);

    if (input_fp == NULL)
    {
        exit_code = EXIT_FAILURE;
        goto exit_cleanup_compress;
    }

    dlinked_list_node* const symbol_list = huffman_count_stream_table(input_fp);
    rewind(input_fp);
    tree_root = huffman_construct_tree(symbol_list);

    serialisation_length = huffman_serialise_tree(&tree_root, 2048, serialisation_buffer);
    header_data = header_create_empty();
    const bool wrote_header = header_add_entry(&header_data, 3, "sft", serialisation_length, (char*)serialisation_buffer);

    UNUSED(wrote_header);
    assert(wrote_header);

    if (is_io_buffered)
    {
        bitarr_buffer = open_file_write(buffer_filename);
        if (bitarr_buffer == NULL)
        {
            exit_code = EXIT_FAILURE;
            goto exit_cleanup_compress;
        }

        bitarray_init_io_buffered(&data_bit_array, bitarr_buffer);
    }
    else
    {
        bitarray_init_in_mem(&data_bit_array);
    }

    huffman_compress_stream(&tree_root, input_fp, &data_bit_array);

    output_fp = open_file_write(output_filename);
    if (output_fp == NULL)
    {
        exit_code = EXIT_FAILURE;
        goto exit_cleanup_compress;
    }

    header_flush_stream(&header_data, output_fp);
    bitarray_write_file(&data_bit_array, output_fp, -1);

exit_cleanup_compress:
    bitarray_clean(&data_bit_array);
    huffman_node_clean(&tree_root);
    header_clean(&header_data);

    if (input_fp != NULL)
        fclose(input_fp);
    if (output_fp != NULL)
        fclose(output_fp);
    if (bitarr_buffer != NULL)
        remove(buffer_filename);

    return exit_code;
}

int
handle_decompress(
    const command_s* const command
)
{
    int param_count = 0;
    const char** params = command_get_parameters(command, &param_count);

    const bool is_io_buffered = command_read_bool_option(command, "--buffered");

    if (param_count <= 0)
        return EXIT_FAILURE;

    const char* const decompressed_extension  = ".unhuff";
    const char* const bitarr_buffer_extension = ".buff";

    int exit_code = EXIT_SUCCESS;
    FILE* input_fp = NULL;
    FILE* output_fp = NULL;
    FILE* bitarr_buffer = NULL;

    huffman_node tree_root = {0};
    bitarray data_bit_array = {0};
    huffman_header header_data = {0};

    const char* const input_filename = params[0];
    char output_filename[256] = {0};
    char buffer_filename[256] = {0};

    uint8_t serialisation_buffer[2048] = {0};
    size_t serialisation_length = 0;

    strcpy(output_filename, input_filename);
    *strrchr(output_filename, '.') = '\0';
    strcat(output_filename, decompressed_extension);

    strcpy(buffer_filename, input_filename);
    strcat(buffer_filename, bitarr_buffer_extension);

    printf("output-filename: %s\n", output_filename);

    input_fp = open_file_read(input_filename);
    if (input_fp == NULL)
    {
        exit_code = EXIT_FAILURE;
        goto exit_cleanup_decompress;
    }

    header_data = parse_header_stream(input_fp);
    serialisation_length = header_get_value(&header_data, 3, "sft", 2048, (char*) serialisation_buffer);
    tree_root = huffman_deserialise_tree(serialisation_buffer, serialisation_length);

    if (is_io_buffered)
    {
        bitarr_buffer = open_file_write(buffer_filename);
        if (bitarr_buffer == NULL)
        {
            exit_code = EXIT_FAILURE;
            goto exit_cleanup_decompress;
        }

        bitarray_init_io_buffered(&data_bit_array, bitarr_buffer);
    }
    else {
        bitarray_init_in_mem(&data_bit_array);
    }

    bitarray_read_file(&data_bit_array, input_fp, -1);

    output_fp = open_file_write(output_filename);
    if (output_fp == NULL)
    {
        exit_code = EXIT_FAILURE;
        goto exit_cleanup_decompress;
    }

    huffman_decompress_stream(&tree_root, &data_bit_array, output_fp);

exit_cleanup_decompress:

    huffman_node_clean(&tree_root);
    header_clean(&header_data);
    bitarray_clean(&data_bit_array);

    if (input_fp != NULL)
        fclose(input_fp);

    if (output_fp != NULL)
        fclose(output_fp);

    if (bitarr_buffer != NULL)
        remove(buffer_filename);

    return exit_code;
}
