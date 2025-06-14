/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_COMPRESSION_HUFFMAN_H
#define COMS_COMPRESSION_HUFFMAN_H

#include <stdio.h>
#include <string.h>

#include "../stdlib/Types.h"
#include "../utils/BitUtils.h"
#include "../utils/EndianUtils.h"

struct HuffmanNode {
    HuffmanNode* left;
    HuffmanNode* right;
    int32 frequency;
    byte character;
};

struct Huffman {
    HuffmanNode pool[512];
    HuffmanNode priority_queue[511];
    HuffmanNode** pq; // 1 indexed array (see huffman_init)
    int32 node_count;
    int32 pq_end;

    char buffer[1024]; // Contains the actual table data
    char* code[256];   // Contains a pointer per ASCII character to the huffman code sequence
};

// We could combine this function with the one below but this would introduce a if != 0 check for the frequency
// I would assume the current version is faster since we avoid a branch
static inline
HuffmanNode* huffman_node_create(Huffman* hf, int32 frequency, byte character, HuffmanNode* left, HuffmanNode* right) noexcept
{
    HuffmanNode* node = hf->pool + hf->node_count++;
    node->character = character;
    node->frequency = frequency;

    return node;
}

// Same as other function but frequency = 0
static inline
HuffmanNode* huffman_node_create(Huffman* hf, byte character, HuffmanNode* left, HuffmanNode* right) noexcept
{
    HuffmanNode* node = hf->pool + hf->node_count++;
    node->left = left;
    node->right = right;
    node->frequency = left->frequency + right->frequency;

    return node;
}

static inline
void huffman_node_insert(Huffman* hf, HuffmanNode* node) noexcept
{
    int32 child_id;
    int32 parent_id = hf->pq_end++;

    while ((child_id = parent_id / 2) && hf->pq[child_id]->frequency <= node->frequency) {
        hf->pq[parent_id] = hf->pq[child_id];
        parent_id = child_id;
    }

    hf->pq[parent_id] = node;
}

static
HuffmanNode* huffman_node_remove(Huffman* hf) noexcept
{
    int32 parent_id = 1;
    int32 left_child_id;
    HuffmanNode* min_node = hf->pq[parent_id];

    if (hf->pq_end < 2) {
        return 0;
    }

    --hf->pq_end;

    while ((left_child_id = parent_id * 2) < hf->pq_end) {
        // Branchless increment
        left_child_id += (int32) (left_child_id + 1 < hf->pq_end
            && hf->pq[left_child_id + 1]->frequency < hf->pq[left_child_id]->frequency
        );

        hf->pq[parent_id] = hf->pq[left_child_id];
        parent_id = left_child_id;
    }

    hf->pq[parent_id] = hf->pq[hf->pq_end];

    return min_node;
}

static
int64 huffman_code_build(Huffman* hf, HuffmanNode* root, char* code, int32 length, char* code_buffer, int32* buffer_position) noexcept
{
    if (root->character) {
        code[length] = 0;
        memcpy(&code_buffer[*buffer_position], code, length + 1);
        hf->code[root->character] = &code_buffer[*buffer_position];
        *buffer_position += length + 1;

        return;
    }

    code[length] = '0'; huffman_code_build(hf, root->left,  code, length + 1, code_buffer, buffer_position);
    code[length] = '1'; huffman_code_build(hf, root->right, code, length + 1, code_buffer, buffer_position);
}

void huffman_init(Huffman* hf, const byte* in) noexcept
{
    int32 frequency[256] = {0};
    int32 buffer_position = 0;
    char temp_code[16];

    // We artificially force the root element (usually the 0 element) to have the index 1.
    hf->pq = (HuffmanNode **) (hf->priority_queue - 1);

    while (*in) {
        ++frequency[(byte) *in++];
    }

    for (int32 i = 0; i < 256; ++i) {
        if (frequency[i]) {
            huffman_node_insert(hf, huffman_node_create(hf, frequency[i], i, NULL, NULL));
        }
    }

    while (hf->pq_end > 2) {
        huffman_node_insert(hf, huffman_node_create(hf, 0, huffman_node_remove(hf), huffman_node_remove(hf)));
    }

    huffman_code_build(hf, hf->pq[1], temp_code, 0, hf->buffer, &buffer_position);
}

inline
void huffman_dump(const Huffman* hf, byte* out)
{
    // dump the char -> code relations as relative indices
    for (uint32 i = 0; i < ARRAY_COUNT(hf->code); ++i) {
        *((int64 *) out) = hf->code[i]
            ? SWAP_ENDIAN_LITTLE(hf->code[i] - hf->buffer)
            : SWAP_ENDIAN_LITTLE(-1);

        out += sizeof(int64);
    }

    // dump the table codes
    memcpy(out, hf->buffer, sizeof(char) * ARRAY_COUNT(hf->buffer));
}

inline
void huffman_load(Huffman* hf, const byte* in)
{
    // load the char -> code relations and convert relative indices to pointers
    for (uint32 i = 0; i < ARRAY_COUNT(hf->code); ++i) {
        int64 value = SWAP_ENDIAN_LITTLE(*((int64 *) in));
        in += sizeof(value);

        if (value > -1) {
            hf->code[i] = hf->buffer + value;
        }
    }

    // load the table codes
    memcpy(hf->buffer, in, sizeof(char) * ARRAY_COUNT(hf->buffer));
}

inline
int64 huffman_encode(Huffman* hf, const byte* in, byte* out) noexcept
{
    uint64 bit_length = 0;
    int32 pos_bit = 0;

    while (*in) {
        const char* code = hf->code[*in++];

        while (*code) {
            if (*code == '1') {
                BIT_SET_L2R(*out, pos_bit, 1);
            }

            ++code;
            ++bit_length;

            // Make sure it wraps around to 0 for pos_bit > 7
            pos_bit = MODULO_2(++pos_bit, 8);
            if (pos_bit == 0) {
                ++out;
            }
        }
    }

    return bit_length;
}

inline
int64 huffman_decode(Huffman* hf, const byte* in, byte* out, uint64 bit_length) noexcept
{
    HuffmanNode* current = hf->pq[1];
    int32 pos_bit = 0;
    byte* start = out;

    while (pos_bit < bit_length) {
        // Branchless version of checking if bit is set and then updating current
        int32 bit = BITS_GET_8_L2R(*in, pos_bit, 1);
        current = (HuffmanNode *) (((uintptr_t) current->left & ~bit) | ((uintptr_t) current->right & bit));

        if (current->character) {
            *out++ = current->character;
            current = hf->pq[1];
        }

        pos_bit = MODULO_2(++pos_bit, 8);
        if (pos_bit == 0) {
            ++in;
        }
    }

    *out = '\0';

    // -1 for the \0 character which is not part of the length
    return out - start - 1;
}

#endif