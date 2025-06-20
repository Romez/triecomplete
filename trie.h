#include <sys/types.h>

typedef struct node_t {
    char c;
    size_t freq;
    size_t children_len;
    struct node_t** children;
} node_t;

typedef struct {
    char* word;
    size_t word_len;
    size_t freq;
} word_t;

typedef struct {
    word_t* words;
    size_t len;
    size_t cap;
} words_t;

node_t *load_trie(char *file_path);

words_t* trie_to_words(node_t *node);

word_t *find_word(node_t *root, char *text);

node_t *find_prefix(node_t *node, char *s);

void free_words(words_t *words);

words_t *prefix_to_words(node_t* root, char* prefix);

