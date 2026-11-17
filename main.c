#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

double nowMs(void) {
#ifdef _WIN32
    LARGE_INTEGER freq, cnt;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&cnt);
    return 1000.0 * (double)cnt.QuadPart / (double)freq.QuadPart;
#else
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
#endif
}

#define STACK_BYTES 32

typedef struct Node {
    int key, count, height;
    struct Node* left, * right;
} Node;

long long comparisons, movements;
int depth, maxDepth, nodeCount, maxNodes;

void reset(void) {
    comparisons = movements = 0;
    depth = maxDepth = nodeCount = maxNodes = 0;
}

void enter(void) {
    depth++;
    if (depth > maxDepth) maxDepth = depth;
}

void leave(void) { depth--; }

int randomNumber(void) {
    return ((rand() & 32767) << 15) | (rand() & 32767);
}

void swap(int* a, int* b) {
    if (a == b) return;
    int tmp = *a;
    *a = *b;
    *b = tmp;
    movements += 3;
}

void quickSort(int a[], int left, int right) {
    if (left >= right) return;
    enter();

    int mid = left + (right - left) / 2;
    swap(&a[mid], &a[right]);
    int pivot = a[right];
    movements++;

    int i = left;
    for (int j = left; j < right; j++) {
        comparisons++;
        if (a[j] < pivot) swap(&a[i++], &a[j]);
    }
    swap(&a[i], &a[right]);
    quickSort(a, left, i - 1);
    quickSort(a, i + 1, right);
    leave();
}

void arrayInsert(int a[], int* size, int value) {
    int i = 0;
    while (i < *size) {
        comparisons++;
        if (a[i] >= value) break;
        i++;
    }
    for (int j = *size; j > i; j--) {
        a[j] = a[j - 1];
        movements++;
    }
    a[i] = value;
    movements++;
    (*size)++;
}

void arrayDelete(int a[], int* size, int value) {
    int i = 0;
    while (i < *size) {
        comparisons++;
        if (a[i] >= value) break;
        i++;
    }
    if (i == *size) return;
    comparisons++;
    if (a[i] != value) return;

    for (int j = i; j < *size - 1; j++) {
        a[j] = a[j + 1];
        movements++;
    }
    (*size)--;
}

int height(Node* p) { return p ? p->height : 0; }

void updateHeight(Node* p) {
    int a = height(p->left), b = height(p->right);
    p->height = 1 + (a > b ? a : b);
}

Node* rotateRight(Node* p) {
    Node* q = p->left;
    p->left = q->right;
    q->right = p;
    updateHeight(p);
    updateHeight(q);
    return q;
}

Node* rotateLeft(Node* p) {
    Node* q = p->right;
    p->right = q->left;
    q->left = p;
    updateHeight(p);
    updateHeight(q);
    return q;
}

Node* balance(Node* p) {
    updateHeight(p);
    int diff = height(p->left) - height(p->right);
    if (diff > 1) {
        if (height(p->left->left) < height(p->left->right))
            p->left = rotateLeft(p->left);
        return rotateRight(p);
    }
    if (diff < -1) {
        if (height(p->right->right) < height(p->right->left))
            p->right = rotateRight(p->right);
        return rotateLeft(p);
    }
    return p;
}

Node* newNode(int value) {
    Node* p = malloc(sizeof(Node));
    if (!p) { fprintf(stderr, "Memory allocation failed\n"); exit(1); }
    p->key = value;
    p->count = p->height = 1;
    p->left = p->right = NULL;
    movements++;          
    nodeCount++;
    if (nodeCount > maxNodes) maxNodes = nodeCount;
    return p;
}

Node* avlInsert(Node* p, int value) {
    enter();
    if (!p) {
        p = newNode(value);
    }
    else {
        comparisons++;
        if (value < p->key) {
            p->left = avlInsert(p->left, value);
        }
        else {
            comparisons++;
            if (value > p->key) {
                p->right = avlInsert(p->right, value);
            }
            else {
                p->count++; 
                leave();
                return p;
            }
        }
        p = balance(p);
    }
    leave();
    return p;
}

Node* removeMin(Node* p) {
    enter();
    if (!p->left) {
        Node* next = p->right;
        free(p);
        nodeCount--;
        leave();
        return next;
    }
    p->left = removeMin(p->left);
    p = balance(p);
    leave();
    return p;
}

Node* avlDelete(Node* p, int value) {
    if (!p) return NULL;
    enter();
    comparisons++;
    if (value < p->key) {
        p->left = avlDelete(p->left, value);
    }
    else {
        comparisons++;
        if (value > p->key) {
            p->right = avlDelete(p->right, value);
        }
        else {
            if (p->count > 1) {
                p->count--;
                leave();
                return p;
            }
            if (!p->left || !p->right) {
                Node* child = p->left ? p->left : p->right;
                free(p);
                nodeCount--;
                leave();
                return child;
            }
            Node* next = p->right;
            while (next->left) next = next->left;
            p->key = next->key;
            p->count = next->count;
            movements++;     
            p->right = removeMin(p->right);
        }
    }
    p = balance(p);
    leave();
    return p;
}

void inorder(Node* p, int out[], int* index) {
    if (!p) return;
    inorder(p->left, out, index);
    for (int i = 0; i < p->count; i++) out[(*index)++] = p->key;
    inorder(p->right, out, index);
}

void freeTree(Node* p) {
    if (!p) return;
    freeTree(p->left);
    freeTree(p->right);
    free(p);
}

void experiment(int n) {
    int ops = n / 5, capacity = n + ops;
    int* data = malloc(n * sizeof(int));
    int* array = malloc(capacity * sizeof(int));
    int* result = malloc(capacity * sizeof(int));
    int* type = malloc(ops * sizeof(int));
    int* value = malloc(ops * sizeof(int));
    if (!data || !array || !result || !type || !value) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    for (int i = 0; i < n; i++) {
        data[i] = randomNumber() % (2 * n);
        array[i] = data[i];
    }
    for (int i = 0; i < ops; i++) {
        type[i] = randomNumber() % 2;     
        value[i] = randomNumber() % (2 * n);
    }

    reset();
    int size = n;
    double start = nowMs();
    quickSort(array, 0, n - 1);
    for (int i = 0; i < ops; i++) {
        if (type[i] == 0) arrayInsert(array, &size, value[i]);
        else arrayDelete(array, &size, value[i]);
    }
    double quickTime = nowMs() - start;
    long long quickCmp = comparisons, quickMove = movements;
    long long quickMem = (long long)capacity * sizeof(int) + (long long)maxDepth * STACK_BYTES;

    reset();
    Node* root = NULL;
    start = nowMs();
    for (int i = 0; i < n; i++) root = avlInsert(root, data[i]);
    for (int i = 0; i < ops; i++) {
        if (type[i] == 0) root = avlInsert(root, value[i]);
        else root = avlDelete(root, value[i]);
    }
    double avlTime = nowMs() - start;
    long long avlCmp = comparisons, avlMove = movements;
    long long avlMem = (long long)maxNodes * sizeof(Node) + (long long)maxDepth * STACK_BYTES;

    int count = 0, same = 1;
    inorder(root, result, &count);
    if (count != size) same = 0;
    for (int i = 0; same && i < size; i++)
        if (array[i] != result[i]) same = 0;

    printf("Data Size : %d\nOperations: %d\n\n", n, ops);
    printf("                  Quick Sort      AVL Tree\n");
    printf("------------------------------------------------\n");
    printf("Total Time        %-8.3f ms      %-8.3f ms\n", quickTime, avlTime);
    printf("Comparisons       %-15lld %lld\n", quickCmp, avlCmp);
    printf("Data Movements    %-15lld %lld\n", quickMove, avlMove);
    printf("Memory Usage      %-6lld Bytes    %-6lld Bytes\n", quickMem, avlMem);
    printf("\n");
    printf("Final Data Count  %-15d %d\n", size, count);
    printf("Same Result       %s\n\n", same ? "YES" : "NO (error!)");

    freeTree(root);
    free(data);
    free(array);
    free(result);
    free(type);
    free(value);
}

int main(void) {
    int sizes[] = { 1000, 2000, 4000, 8000, 16000, 20000 };
    srand(12345);
    for (int i = 0; i < 6; i++) experiment(sizes[i]);
    return 0;
}