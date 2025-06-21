
# Prefix-Autocomplete CLI Utility

A command-line utility written in C that reads a text file, builds a prefix trie from its contents, and enables interactive autocomplete-style prefix searches directly in the terminal using the ncurses library.

### Features
* <b>Prefix Trie Construction:</b> Reads the file from a given path and builds a prefix trie containing all unique words found in the text.
* <b>Interactive Search:</b> Uses the ncurses library to provide a dynamic, responsive terminal UI. Start typing a prefix and see an updated list of all words from the text that match your input.
* <b>Word Occurrence Counts:</b> The autocomplete result displays not only matching words but also the number of times each word appears in the original text.
* <b>CLI Interface:</b> Intuitive prompt for entering prefixes and easily browsing autocomplete suggestions.

### Build
> make trie


### Usage
> ./trie [path_to_text_file]