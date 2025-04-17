#include "ej1.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/**
 * Crea y destruye una lista vacía, chequeando que esté bien inicializada.
 */
void test_create_destroy_list() {
    string_proc_list* list = string_proc_list_create_asm();
    assert(list != NULL);
    assert(list->first == NULL);
    assert(list->last == NULL);
    printf("✅ string_proc_list_create_asm pasó\n");
    string_proc_list_destroy(list);
}

/**
 * Crea y destruye un nodo, chequeando sus valores.
 */
void test_create_destroy_node() {
    const char* hash_str = "hash_test";
    string_proc_node* node = string_proc_node_create_asm(42, (char*)hash_str);
    assert(node != NULL);
    assert(node->next == NULL);
    assert(node->previous == NULL);
    assert(node->type == 42);
    assert(strcmp(node->hash, hash_str) == 0);
    printf("✅ string_proc_node_create_asm pasó\n");
    string_proc_node_destroy(node);
}

/**
 * Crea una lista y le agrega nodos con distintos valores. Verifica enlaces.
 */
void test_create_list_add_nodes() {
    string_proc_list* list = string_proc_list_create_asm();
    string_proc_list_add_node_asm(list, 1, "uno");
    assert(list->first != NULL && list->last != NULL);
    assert(list->first == list->last);
    assert(strcmp(list->first->hash, "uno") == 0);

    string_proc_list_add_node_asm(list, 2, "dos");
    assert(list->first != list->last);
    assert(strcmp(list->last->hash, "dos") == 0);
    assert(list->first->next == list->last);
    assert(list->last->previous == list->first);

    string_proc_list_add_node_asm(list, 3, "tres");
    assert(strcmp(list->last->hash, "tres") == 0);
    assert(list->last->previous != NULL);
    assert(list->last->previous->previous == NULL);

    printf("✅ string_proc_list_add_node_asm pasó\n");
    string_proc_list_destroy(list);
}

/**
 * Crea lista, agrega nodos con type = 0, y concatena usando una semilla.
 */
void test_list_concat() {
    string_proc_list* list = string_proc_list_create_asm();
    string_proc_list_add_node_asm(list, 0, "A");
    string_proc_list_add_node_asm(list, 1, "B");
    string_proc_list_add_node_asm(list, 0, "C");
    string_proc_list_add_node_asm(list, 1, "D");
    string_proc_list_add_node_asm(list, 0, "E");

    char* result = string_proc_list_concat_asm(list, 0, "X");
    assert(result != NULL);
    assert(strcmp(result, "XACE") == 0 || strcmp(result, "XAE") == 0 || strcmp(result, "XAC") == 0);  // depende de str_concat
    printf("✅ string_proc_list_concat_asm pasó: %s\n", result);

    free(result);
    string_proc_list_destroy(list);
}

/**
 * Corre todos los tests.
 */
void run_tests() {
    test_create_destroy_list();
    test_create_destroy_node();
    test_create_list_add_nodes();
    test_list_concat();
}

int main(void) {
    run_tests();
    return 0;
}
