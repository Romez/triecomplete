#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "trie.h"

#define READ_BUF_SIZE 1024
#define WORD_BUF_SIZE 32

char word_buf[WORD_BUF_SIZE];
int word_buf_len = 0;

node_t* alloc_node(char c) {
    node_t* node = malloc(sizeof(node_t));

    node->c = c;
    node->children = NULL;
    node->children_len = 0;
    node->freq = 0;

    return node;
}

ssize_t find_child(node_t* node, char c) {
    for (size_t i = 0; i < node->children_len; i++) {
        if (node->children[i]->c == c) {
            return i;
        }
    }

    return -1;
}

node_t* add_child(node_t* root, char c) {
    node_t* child = alloc_node(c);

    size_t new_children_len = root->children_len + 1;

    root->children = realloc(root->children, new_children_len * sizeof(node_t*));
    root->children[root->children_len++] = child;

    return child;
}

void add_word(node_t* node, char* word_buf) {
    char c = *word_buf;

    if (c == '\0') {
        node->freq++;
    }
    else {
        ssize_t idx = find_child(node, c);
        node_t* child = idx == -1 ? add_child(node, c) : node->children[idx];

        add_word(child, &word_buf[1]);
    }
}

void add_text(node_t* root, char* text, size_t text_len) {
    for (size_t i = 0; i < text_len; i++) {
        unsigned char c = text[i];

        if ('A' <= c && c <= 'Z') {
            assert(word_buf_len < WORD_BUF_SIZE);
            word_buf[word_buf_len++] = tolower(c);
        } else if ('a' <= c && c <= 'z') {
            assert(word_buf_len < WORD_BUF_SIZE);
            word_buf[word_buf_len++] = c;
        } else {
            if (word_buf_len > 0) {
                add_word(root, word_buf);
                memset(word_buf, 0, WORD_BUF_SIZE);
                word_buf_len = 0;
            }
        }
    }

    if (word_buf_len > 0) {
        add_word(root, word_buf);
        memset(word_buf, 0, WORD_BUF_SIZE);
        word_buf_len = 0;
    }
}

node_t *find_prefix(node_t* node, char* s) {
    if (*s == '\0') {
        return node;
    }
    
    ssize_t idx = find_child(node, *s);

    if (idx != -1) {
        node_t* child = node->children[idx];
        return find_prefix(child, s + 1);
    } else {
        return NULL;
    }
}

node_t* load_trie(char* file_path) {
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("in file: err: open file");
        exit(1);
    }

    node_t* root = alloc_node('\0');

    char read_buf[READ_BUF_SIZE];

    ssize_t bytes_read = 0;
    do {
        bytes_read = read(fd, read_buf, READ_BUF_SIZE);
        if (bytes_read == -1) {
            perror(file_path);
            exit(EXIT_FAILURE);
        }

        if (bytes_read > 0) {
            add_text(root, read_buf, bytes_read);
        }
    } while (bytes_read);

    close(fd);

    return root;
}

// Words -----------------

words_t* allocate_words() {
    words_t* words = malloc(sizeof(words_t));
    if (words == NULL) {
        perror("malloc words");
        exit(EXIT_FAILURE);
    }

    size_t initial_cap = 32;

    word_t* words_arr = malloc(initial_cap * sizeof(word_t));
    if (words_arr == NULL) {
        perror("malloc words");
        exit(EXIT_FAILURE);
    }

    words->words = words_arr;
    words->cap = initial_cap;
    words->len = 0;

    return words;
}

void free_words(words_t* words) {
    free(words->words);
    free(words);
}

int cmp_words_freq(const void* w1, const void* w2) {
    return ((word_t*)w2)->freq - ((word_t*)w1)->freq;
}

void append_word(words_t* words, char* word_buf, size_t word_buf_len, size_t freq) {
    if (words->len == words->cap) {
        size_t new_cap = words->cap + 32;

        words->words = realloc(words->words, new_cap * sizeof(word_t));
        words->cap = new_cap;
    }

    word_t word = {
        .word_len = word_buf_len,
        .freq = freq,
    };

    word.word = malloc(word_buf_len);
    if (word.word == NULL) {
        perror("malloc word");
        exit(EXIT_FAILURE);
    }

    memcpy(word.word, word_buf, word_buf_len);

    words->words[words->len++] = word;
}

void trie_to_words_iter(node_t* node, words_t* words) {
    if (node->freq > 0 && word_buf_len > 0) {
        append_word(words, word_buf, word_buf_len, node->freq);
    }

    for (size_t i = 0; i < node->children_len; i++) {
        node_t* child = node->children[i];

        if (child->c != '\0') {
            word_buf[word_buf_len++] = child->c;

            trie_to_words_iter(child, words);

            word_buf_len--;
        }
    }
}

words_t* trie_to_words(node_t* node) {
    memset(word_buf, 0, WORD_BUF_SIZE);
    word_buf_len = 0;

    words_t* words = allocate_words();

    trie_to_words_iter(node, words);

    qsort(words->words, words->len, sizeof(word_t), cmp_words_freq);

    return words;
}

words_t *prefix_to_words(node_t* root, char* prefix) {
    node_t* node = find_prefix(root, prefix);

    if (node != NULL) {
        size_t prefix_len = strlen(prefix);
        
        memset(word_buf, 0, WORD_BUF_SIZE);

        memcpy(word_buf, prefix, prefix_len);
        
        word_buf_len = prefix_len;

        words_t* words = allocate_words();

        trie_to_words_iter(node, words);

        qsort(words->words, words->len, sizeof(word_t), cmp_words_freq);

        return words;        
    } else {
        return NULL;
    }
}

word_t* find_word(node_t* root, char* text) {
    node_t* curr_node = root;
    char* word_tail = text;

next_child:
    while (*word_tail != '\0') {
        for (size_t i = 0; i < curr_node->children_len; i++) {
            node_t* child = curr_node->children[i];

            if (child->c == *word_tail) {
                curr_node = child;

                word_tail++;

                goto next_child;
            }
        }

        return NULL;
    }

    if (curr_node->freq == 0) {
        return NULL;
    }

    word_t* word = malloc(sizeof(word_t));
    if (word == NULL) {
        perror("malloc word");
        exit(1);
    }

    word->word_len = strlen(text);
    word->freq = curr_node->freq;

    word->word = malloc(word->word_len);
    if (word->word == NULL) {
        perror("malloc word");
        exit(1);
    }

    strcpy(word->word, text);

    return word;
}
