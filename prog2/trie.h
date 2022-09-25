#ifndef H_TRIE_H
#define H_TRIE_H

struct TrieNode;
struct TrieNode *getNode(void);
void insert(struct TrieNode *root, const char *key);
bool search(struct TrieNode *root, const char *key);

#endif
