#include "tLib.h"

void tNodeInit(tNode *node)													//让节点的前后指针都指向自己，表示“孤立节点”。
{
	node->nextNode=node;
	node->preNode=node;
}

void tListInit(tList *list)											
{
	list->firstnode=&(list->headNode); // 头节点的 next 指向自己
	list->lastnode=&(list->headNode);// 头节点的 pre 指向自己
	list->nodeCount=0;
}

uint32_t tListCount(tList *list)										//返回链表里节点的数量
{
	return list->nodeCount;
}

void tListAddFirst(tList *list, tNode *node)					//往链表的头部插入一个节点
{
	node->nextNode=list->firstnode;											 // 新节点 next 指向原第一个节点
	node->preNode=&(list->headNode);										 // 新节点 pre 指向头节点
	
	list->firstnode->preNode=node;											// 原第一个节点 pre 指向新节点
	list->firstnode=node;																// 让头节点后面直接接上新节点，新节点成为第一个
	
	list->nodeCount++;																 // 节点数加 1
}

void tListAddLast(tList *list, tNode *node)          // 把节点 node 插到链表 list 的尾部
{
    node->nextNode = &(list->headNode);              // 新节点的后继指向头节点（循环链表）
    node->preNode  = list->lastnode;                 // 新节点的前驱指向原最后一个节点

    list->lastnode->nextNode = node;                 // 原最后一个节点的后继改为新节点
    list->lastnode = node;                           // 头节点的前驱改为新节点，新节点成为最后一个

    list->nodeCount++;                               // 链表节点数加 1
}

void tListInsertAfter(tList *list, tNode *nodeAfter, 
                      tNode *nodeToInsert)              // 把 nodeToInsert 插到 nodeAfter 后面
{
    nodeToInsert->nextNode = nodeAfter->nextNode;       // 新节点的后继指向 nodeAfter 原来的后继
    nodeToInsert->preNode  = nodeAfter;                 // 新节点的前驱指向 nodeAfter

    nodeAfter->nextNode->preNode = nodeToInsert;        // 原后继节点的前驱改为新节点
    nodeAfter->nextNode = nodeToInsert;                 // nodeAfter 的后继改为新节点

    list->nodeCount++;                                  // 链表节点数加 1
}

void tListRemoveNode(tList *list, tNode *node)
{
    if (list->nodeCount > 0)                            // 链表非空才操作
    {
        node->preNode->nextNode = node->nextNode;        // 前驱的后继指向后继
        node->nextNode->preNode = node->preNode;         // 后继的前驱指向前驱
        list->nodeCount--;                               // 节点数减 1
    }
    return;
}

tNode * tListRemoveFirst(tList *list)									//Remove the first node of the list
{
	tNode *node=(tNode *)0;
	if(list->nodeCount>0)
	{
		node=list->firstnode;
		
		list->firstnode->nextNode->preNode=&(list->headNode);
		list->firstnode=list->firstnode->nextNode;
		
		list->nodeCount--;
	}
	return node;
}

void tListRemoveAll(tList *list)
{
    tNode *nextNode = list->firstnode;      // 从第一个节点开始
    tNode *currentNode;

    while (list->nodeCount > 0)             // 只要还有节点
    {
        currentNode = nextNode;             // 当前要摘除的节点
        nextNode = nextNode->nextNode;      // 先保存下一个节点，防止断链

        currentNode->nextNode = currentNode; // 让当前节点自己指自己
        currentNode->preNode  = currentNode; // 变成孤立节点

        list->nodeCount--;                   // 节点数减 1
    }

    list->firstnode = &(list->headNode);    // 头节点 next 指回自己
    list->lastnode  = &(list->headNode);    // 头节点 pre 指回自己
}

tNode * tListFirst(tList *list)											//返回链表中的第一个节点
{
	if(list->nodeCount>0)
		return list->firstnode;
	else
		return ((tNode *)0);
}

tNode * tListLast(tList *list)											//返回链表中的最后一个节点
{
	if(list->nodeCount>0)
		return list->lastnode;
	else
		return ((tNode *)0);
}

tNode *tListPre(tList *list, tNode *node)
{
    if (node->preNode == node)      // 节点的前驱指向自己 → 孤立节点
        return ((tNode *)0);         // 返回 NULL
    else
        return node->preNode;        // 返回前驱节点
}

tNode *tListNext(tList *list, tNode *node)
{
    if (node->nextNode == node)     // 节点的后继指向自己 → 孤立节点
        return ((tNode *)0);         // 返回 NULL
    else
        return node->nextNode;       // 返回后继节点
}
