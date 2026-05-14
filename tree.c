/**
 * 实验：目录树查看器（仿 Linux tree 命令）
 * 学号：2504020344  姓名：张紫涵
 * 说明：请补全所有标记为 TODO 的函数体，不要修改其他代码。
 * 目录树查看器（仿 Linux tree 命令）
 * 完整实现版本（C语言，左孩子右兄弟二叉树）
 * 编译：gcc -o tree tree.c -std=c99
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

// ================== 二叉树结点定义 ==================
typedef struct FileNode {
    char *name;                  // 文件/目录名
    int isDir;                   // 1:目录 0:文件
    struct FileNode *firstChild; // 左孩子：第一个子项
    struct FileNode *nextSibling;// 右兄弟：下一个同层项
} FileNode;

// ================== 函数声明 ==================
FileNode* createNode(const char *name, int isDir);
int cmpNode(const void *a, const void *b);
FileNode* buildTree(const char *path);
void printTree(FileNode *node, const char *prefix, int isLast);
int countNodes(FileNode *root);
int countLeaves(FileNode *root);
int treeHeight(FileNode *root);
void countDirFile(FileNode *root, int *dirs, int *files);
void freeTree(FileNode *root);
char* getBaseName(void);

// ================== 需要补全的函数 ==================

// 创建新结点（分配内存、复制字符串、初始化指针）
FileNode* createNode(const char *name, int isDir) {
   FileNode *node = (FileNode*)malloc(sizeof(FileNode));
     if (!node) {
         perror("malloc failed");
         return NULL;
     }
     node->name = strdup(name);
     node->isDir = isDir;
     node->firstChild = NULL;
     node->nextSibling = NULL;
     return node;
}

// 比较函数，用于 qsort 对子项按名称排序
int cmpNode(const void *a, const void *b) {
   FileNode *na = *(FileNode**)a;
     FileNode *nb = *(FileNode**)b;
     return strcmp(na->name, nb->name);
}

// 递归构建目录树（核心难点）
FileNode* buildTree(const char *path) {
    DIR *dir = opendir(path);
     if (!dir) {
         perror("opendir failed");
         return NULL;
     }
     struct dirent *entry;
     int capacity = 16, count = 0;
     FileNode **children = (FileNode**)malloc(capacity * sizeof(FileNode*));
     if (!children) {
         perror("malloc failed");
         closedir(dir);
         return NULL;
     }
     while ((entry = readdir(dir)) != NULL) {
         // 跳过 . 和 ..
         if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
             continue;
             char fullPath[1024];
         snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);
         struct stat st;
         if (stat(fullPath, &st) == -1) {
             perror("stat failed");
             continue;
         }
         int isDir = S_ISDIR(st.st_mode);
         FileNode *child = createNode(entry->d_name, isDir);
         if (!child) continue;
         // 若是目录，递归构建子树
         if (isDir) {
             child->firstChild = buildTree(fullPath);
         }
         if (count >= capacity) {
             capacity *= 2;
             children = (FileNode**)realloc(children, capacity * sizeof(FileNode*));
             if (!children) {
                 perror("realloc failed");
                 free(child);
                 break;
             }
         }
         children[count++] = child;
     }
     closedir(dir);
     // 对子结点数组排序
     qsort(children, count, sizeof(FileNode*), cmpNode);
     // 链接成兄弟链表
     FileNode *head = NULL, *tail = NULL;
     for (int i = 0; i < count; i++) {
         if (!head) {
             head = children[i];
             tail = children[i];
         } else {
             tail->nextSibling = children[i];
             tail = children[i];
         }
     }
     free(children);
     return head;

}

// 树形输出（仿 tree 命令）
void printTree(FileNode *node, const char *prefix, int isLast) {
   if (!node) return;
     printf("%s%s%s\n", prefix, isLast ? "/-- " : "+-- ", node->name);
     char newPrefix[1024];
     snprintf(newPrefix, sizeof(newPrefix), "%s%s", prefix, isLast ? "    " : "|   ");
     FileNode *child = node->firstChild;
     while (child) {
         printTree(child, newPrefix, child->nextSibling == NULL);
         child = child->nextSibling;
     }
}

// 统计二叉树结点总数
int countNodes(FileNode *root) {
   if (!root) return 0;
     return 1 + countNodes(root->firstChild) + countNodes(root->nextSibling);
}

// 统计叶子结点数（firstChild == NULL 的结点）
int countLeaves(FileNode *root) {
  if (!root) return 0;
     int cnt = 0;
     if (!root->firstChild) cnt = 1;
     return cnt + countLeaves(root->firstChild) + countLeaves(root->nextSibling);
}

// 计算二叉树高度（根深度为1，空树高度为0）
int treeHeight(FileNode *root) {
    if (!root) return 0;
     int maxChildH = 0;
     FileNode *child = root->firstChild;
     while (child) {
         int h = treeHeight(child);
         if (h > maxChildH) maxChildH = h;
         child = child->nextSibling;
     }
     return 1 + maxChildH;
}

// 统计目录数和文件数（遍历整棵树）
void countDirFile(FileNode *root, int *dirs, int *files) {
    if (!root) return;
     if (root->isDir) (*dirs)++;
     else (*files)++;
     countDirFile(root->firstChild, dirs, files);
     countDirFile(root->nextSibling, dirs, files);
}

// 释放整棵树的内存
void freeTree(FileNode *root) {
   if (!root) return;
     freeTree(root->firstChild);
     freeTree(root->nextSibling);
     free(root->name);
     free(root);
}

// 获取当前工作目录的“基本名称”（用于显示根结点名）
char* getBaseName(void) {
    char *cwd = getcwd(NULL, 0);
     if (!cwd) {
         perror("getcwd failed");
         exit(EXIT_FAILURE);
     }
     char *base = strrchr(cwd, '/');
     if (base) return strdup(base + 1);
     return strdup(cwd);
}

int main(int argc, char *argv[]) {
    char targetPath[1024];
    if (argc >= 2) {
        strncpy(targetPath, argv[1], sizeof(targetPath)-1);
        targetPath[sizeof(targetPath)-1] = '\0';
    } else {
        if (getcwd(targetPath, sizeof(targetPath)) == NULL) {
            perror("getcwd");
            return 1;
        }
    }

    int len = strlen(targetPath);
    if (len > 0 && targetPath[len-1] == '/')
        targetPath[len-1] = '\0';

    struct stat st;
    if (stat(targetPath, &st) != 0) {
        perror("stat");
        return 1;
    }
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "错误: %s 不是目录\n", targetPath);
        return 1;
    }

    FileNode *root = buildTree(targetPath);
    if (!root) {
        fprintf(stderr, "无法构建目录树\n");
        return 1;
    }

    // 输出根目录名
    char *displayName = NULL;
    if (argc >= 2) {
        displayName = root->name;
    } else {
        displayName = getBaseName();
    }
    printf("%s/\n", displayName);
    if (argc < 2) free(displayName);

    FileNode *child = root->firstChild;
    int childCount = 0;
    FileNode *tmp = child;
    while (tmp) { childCount++; tmp = tmp->nextSibling; }
    int idx = 0;
    while (child) {
        int isLast = (++idx == childCount);
        printTree(child, "", isLast);
        child = child->nextSibling;
    }

    int dirs = 0, files = 0;
    countDirFile(root, &dirs, &files);
    printf("\n%d directories, %d files\n", dirs, files);
    printf("Total nodes: %d\n", countNodes(root));
    printf("Leaf nodes: %d\n", countLeaves(root));
    printf("Tree height: %d\n", treeHeight(root));

    freeTree(root);
    return 0;
}




