// list.c

#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>



// Node constructor
Node* createNode(void* pItem) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        return NULL; // Memory allocation failed
    }
    newNode->pItem = pItem;
    newNode->pNext = NULL;
    newNode->pPrev = NULL;
    return newNode;
}

List* List_create() {
    List* pList = (List*)malloc(sizeof(List));
    if (pList == NULL) {
        return NULL; // Memory allocation failed
    }
    pList->pFirstNode = NULL;
    pList->pLastNode = NULL;
    pList->pCurrentNode = NULL;
    pList->count = 0;
    pthread_mutex_init(&pList->mutex, NULL);
    pthread_cond_init(&pList->cond, NULL);
    return pList;
}

int List_count(List* pList) {
    if (pList == NULL) {
        return LIST_FAIL;
    }
    pthread_mutex_lock(&pList->mutex);
    int count = pList->count;
    pthread_mutex_unlock(&pList->mutex);
    return count;
}

void* List_first(List* pList) {
    if (pList == NULL) {
        return NULL;
    }
    pthread_mutex_lock(&pList->mutex);
    if (pList->pFirstNode != NULL) {
        pList->pCurrentNode = pList->pFirstNode;
        void* item = pList->pFirstNode->pItem;
        pthread_mutex_unlock(&pList->mutex);
        return item;
    } else {
        pthread_mutex_unlock(&pList->mutex);
        return NULL;
    }
}

void* List_last(List* pList) {
    if (pList == NULL) {
        return NULL;
    }
    pthread_mutex_lock(&pList->mutex);
    if (pList->pLastNode != NULL) {
        pList->pCurrentNode = pList->pLastNode;
        void* item = pList->pLastNode->pItem;
        pthread_mutex_unlock(&pList->mutex);
        return item;
    } else {
        pthread_mutex_unlock(&pList->mutex);
        return NULL;
    }
}

void* List_next(List* pList) {
    if (pList == NULL || pList->pCurrentNode == NULL) {
        return NULL;
    }
    pthread_mutex_lock(&pList->mutex);
    if (pList->pCurrentNode->pNext != NULL) {
        pList->pCurrentNode = pList->pCurrentNode->pNext;
        void* item = pList->pCurrentNode->pItem;
        pthread_mutex_unlock(&pList->mutex);
        return item;
    } else {
        pthread_mutex_unlock(&pList->mutex);
        return NULL;
    }
}

void* List_prev(List* pList) {
    if (pList == NULL || pList->pCurrentNode == NULL) {
        return NULL;
    }
    pthread_mutex_lock(&pList->mutex);
    if (pList->pCurrentNode->pPrev != NULL) {
        pList->pCurrentNode = pList->pCurrentNode->pPrev;
        void* item = pList->pCurrentNode->pItem;
        pthread_mutex_unlock(&pList->mutex);
        return item;
    } else {
        pthread_mutex_unlock(&pList->mutex);
        return NULL;
    }
}

void* List_curr(List* pList) {
    if (pList == NULL || pList->pCurrentNode == NULL) {
        return NULL;
    }
    pthread_mutex_lock(&pList->mutex);
    void* item = pList->pCurrentNode->pItem;
    pthread_mutex_unlock(&pList->mutex);
    return item;
}

int List_insert_after(List* pList, void* pItem) {
    if (pList == NULL || pItem == NULL) {
        return LIST_FAIL;
    }
    pthread_mutex_lock(&pList->mutex);
    Node* newNode = createNode(pItem);
    if (newNode == NULL) {
        pthread_mutex_unlock(&pList->mutex);
        return LIST_FAIL; // Memory allocation failed
    }
    if (pList->count == 0) {
        pList->pFirstNode = newNode;
        pList->pLastNode = newNode;
        pList->pCurrentNode = newNode;
    } else {
        newNode->pNext = pList->pCurrentNode->pNext;
        newNode->pPrev = pList->pCurrentNode;
        if (pList->pCurrentNode->pNext != NULL) {
            pList->pCurrentNode->pNext->pPrev = newNode;
        }
        pList->pCurrentNode->pNext = newNode;
        if (pList->pCurrentNode == pList->pLastNode) {
            pList->pLastNode = newNode;
        }
    }
    pList->count++;
    pthread_cond_signal(&pList->cond); // Signal that an item has been added
    pthread_mutex_unlock(&pList->mutex);
    return LIST_SUCCESS;
}

int List_insert_before(List* pList, void* pItem) {
    if (pList == NULL || pItem == NULL) {
        return LIST_FAIL;
    }
    pthread_mutex_lock(&pList->mutex);
    Node* newNode = createNode(pItem);
    if (newNode == NULL) {
        pthread_mutex_unlock(&pList->mutex);
        return LIST_FAIL; // Memory allocation failed
    }
    if (pList->count == 0) {
        pList->pFirstNode = newNode;
        pList->pLastNode = newNode;
        pList->pCurrentNode = newNode;
    } else {
        newNode->pNext = pList->pCurrentNode;
        newNode->pPrev = pList->pCurrentNode->pPrev;
        if (pList->pCurrentNode->pPrev != NULL) {
            pList->pCurrentNode->pPrev->pNext = newNode;
        }
        pList->pCurrentNode->pPrev = newNode;
        if (pList->pCurrentNode == pList->pFirstNode) {
            pList->pFirstNode = newNode;
        }
    }
    pList->count++;
    pthread_cond_signal(&pList->cond); // Signal that an item has been added
    pthread_mutex_unlock(&pList->mutex);
    return LIST_SUCCESS;
}

int List_append(List* pList, void* pItem) {
    if (pList == NULL || pItem == NULL) {
        return LIST_FAIL;
    }
    pthread_mutex_lock(&pList->mutex);
    Node* newNode = createNode(pItem);
    if (newNode == NULL) {
        pthread_mutex_unlock(&pList->mutex);
        return LIST_FAIL; // Memory allocation failed
    }
    if (pList->count == 0) {
        pList->pFirstNode = newNode;
        pList->pLastNode = newNode;
        pList->pCurrentNode = newNode;
    } else {
        newNode->pPrev = pList->pLastNode;
        pList->pLastNode->pNext = newNode;
        pList->pLastNode = newNode;
    }
    pList->count++;
    pthread_cond_signal(&pList->cond); // Signal that an item has been added
    pthread_mutex_unlock(&pList->mutex);
    return LIST_SUCCESS;
}

int List_prepend(List* pList, void* pItem) {
    if (pList == NULL || pItem == NULL) {
        return LIST_FAIL;
    }
    pthread_mutex_lock(&pList->mutex);
    Node* newNode = createNode(pItem);
    if (newNode == NULL) {
        pthread_mutex_unlock(&pList->mutex);
        return LIST_FAIL; // Memory allocation failed
    }
    if (pList->count == 0) {
        pList->pFirstNode = newNode;
        pList->pLastNode = newNode;
        pList->pCurrentNode = newNode;
    } else {
        newNode->pNext = pList->pFirstNode;
        pList->pFirstNode->pPrev = newNode;
        pList->pFirstNode = newNode;
    }
    pList->count++;
    pthread_cond_signal(&pList->cond); // Signal that an item has been added
    pthread_mutex_unlock(&pList->mutex);
    return LIST_SUCCESS;
}

void* List_remove(List* pList) {
    if (pList == NULL || pList->pCurrentNode == NULL) {
        return NULL;
    }
    pthread_mutex_lock(&pList->mutex);
    Node* removedNode = pList->pCurrentNode;
    void* item = removedNode->pItem;
    if (removedNode->pPrev != NULL) {
        removedNode->pPrev->pNext = removedNode->pNext;
    } else {
        pList->pFirstNode = removedNode->pNext;
    }
    if (removedNode->pNext != NULL) {
        removedNode->pNext->pPrev = removedNode->pPrev;
    } else {
        pList->pLastNode = removedNode->pPrev;
    }
    free(removedNode);
    pList->count--;
    pthread_mutex_unlock(&pList->mutex);
    return item;
}

void List_concat(List* pList1, List* pList2) {
    if (pList1 == NULL || pList2 == NULL || pList1 == pList2) {
        return;
    }
    pthread_mutex_lock(&pList1->mutex);
    pthread_mutex_lock(&pList2->mutex);
    if (pList1->count == 0) {
        pList1->pFirstNode = pList2->pFirstNode;
        pList1->pLastNode = pList2->pLastNode;
        pList1->pCurrentNode = pList2->pCurrentNode;
    } else if (pList2->count > 0) {
        pList1->pLastNode->pNext = pList2->pFirstNode;
        if (pList2->pFirstNode != NULL) {
            pList2->pFirstNode->pPrev = pList1->pLastNode;
        }
        pList1->pLastNode = pList2->pLastNode;
    }
    pList1->count += pList2->count;
    pList2->count = 0;
    pList2->pFirstNode = NULL;
    pList2->pLastNode = NULL;
    pList2->pCurrentNode = NULL;
    pthread_cond_signal(&pList1->cond); // Signal that items have been added
    pthread_mutex_unlock(&pList2->mutex);
    pthread_mutex_unlock(&pList1->mutex);
}

void List_free(List* pList, FREE_FN pItemFreeFn) {
    if (pList == NULL) {
        return;
    }
    pthread_mutex_lock(&pList->mutex);
    Node* currentNode = pList->pFirstNode;
    while (currentNode != NULL) {
        Node* nextNode = currentNode->pNext;
        if (pItemFreeFn != NULL) {
            pItemFreeFn(currentNode->pItem);
        }
        free(currentNode);
        currentNode = nextNode;
    }
    pthread_mutex_unlock(&pList->mutex);
    pthread_mutex_destroy(&pList->mutex);
    pthread_cond_destroy(&pList->cond);
    free(pList);
}

void* List_trim(List* pList) {
    if (pList == NULL) {
        return NULL;
    }
    pthread_mutex_lock(&pList->mutex);
    if (pList->pLastNode == NULL) {
        pthread_mutex_unlock(&pList->mutex);
        return NULL;
    }
    Node* lastNode = pList->pLastNode;
    void* item = lastNode->pItem;
    if (pList->pLastNode->pPrev != NULL) {
        pList->pLastNode = pList->pLastNode->pPrev;
        pList->pLastNode->pNext = NULL;
    } else {
        pList->pFirstNode = NULL;
        pList->pLastNode = NULL;
        pList->pCurrentNode = NULL;
    }
    free(lastNode);
    pList->count--;
    pthread_mutex_unlock(&pList->mutex);
    return item;
}

void* List_search(List* pList, COMPARATOR_FN pComparator, void* pComparisonArg) {
    if (pList == NULL || pComparator == NULL) {
        return NULL;
    }
    pthread_mutex_lock(&pList->mutex);
    Node* currentNode = pList->pCurrentNode;
    while (currentNode != NULL) {
        if (pComparator(currentNode->pItem, pComparisonArg)) {
            pList->pCurrentNode = currentNode;
            void* item = currentNode->pItem;
            pthread_mutex_unlock(&pList->mutex);
            return item;
        }
        currentNode = currentNode->pNext;
    }
    pthread_mutex_unlock(&pList->mutex);
    return NULL;
}
