/*
 * tilepacker.cpp
 *  A k-d tree based bin packer that organizes rectangular tiles to fit
 *  neatly into one texture.
 *
 * Copyright © 2018, Alex Dawson. All rights reserved.
 */

#include "compat.h"
#include "build.h"
#include "tilepacker.h"

typedef struct TreeNode
{
    struct TreeNode *pParent, *pChild0, *pChild1;
    TileRect rect;
    uint32_t maxSide;
    uint32_t tileUID;
} TreeNode;

// allocate all the memory we could ever need up front to avoid dynamic allocation
#define NUM_NON_ROOT_NODES MAXPACKEDTILES*2
#define NUM_NODES NUM_NON_ROOT_NODES+MAXTILESHEETS
TreeNode nodes[NUM_NODES];
uint32_t heapNodes = 0;
uint32_t nextTreeNodeIndex = NUM_NON_ROOT_NODES-1;

Tile tiles[MAXPACKEDTILES];

// node rejection queue implemented using a circular buffer
#define MAX_REJECTS (MAXPACKEDTILES-1)
TreeNode rejectQueue[MAX_REJECTS];
uint32_t rejectQueueHeadIndex = 0;
uint32_t numRejected = 0;

#if 0
static void maxheap_bubbleUp(uint32_t nodeIndex)
{
    while (true)
    {
        if (nodeIndex == 0)
        {
            return;
        }

        uint32_t parentIndex = (nodeIndex-1)/2;
        if (nodes[parentIndex].maxSide >= nodes[nodeIndex].maxSide)
        {
            return;
        }

        // bubble up
        TreeNode temp = nodes[nodeIndex];
        nodes[nodeIndex] = nodes[parentIndex];
        nodes[parentIndex] = temp;

        nodeIndex = parentIndex;
    }
}
#endif

static void maxheap_bubbleDown(uint32_t nodeIndex)
{
    while (true)
    {
        uint32_t largestChildIndex = 2*nodeIndex + 1;
        if (largestChildIndex >= heapNodes)
        {
            return;
        }

        uint32_t rightChildIndex = largestChildIndex+1;
        if (rightChildIndex < heapNodes &&
            nodes[rightChildIndex].maxSide > nodes[largestChildIndex].maxSide)
        {
            largestChildIndex = rightChildIndex;
        }

        if (nodes[largestChildIndex].maxSide <= nodes[nodeIndex].maxSide)
        {
            return;
        }

        // bubble down
        TreeNode temp = nodes[nodeIndex];
        nodes[nodeIndex] = nodes[largestChildIndex];
        nodes[largestChildIndex] = temp;

        nodeIndex = largestChildIndex;
    }
}

static void maxheap_buildHeap()
{
    for (int i = (heapNodes-2)/2; i >= 0; --i)
    {
        maxheap_bubbleDown(i);
    }
}

static TreeNode* maxheap_pop()
{
    if (heapNodes == 0)
    {
        return NULL;
    }

    // swap the root and the last node
    TreeNode temp = nodes[0];
    nodes[0] = nodes[heapNodes-1];
    nodes[heapNodes-1] = temp;
    --heapNodes;

    maxheap_bubbleDown(0);

    return nodes+heapNodes;
}

static TreeNode* maxheap_reserveNode(TreeNode *pParent,
                                     TreeNode *pChild0,
                                     TreeNode *pChild1,
                                     TileRect rectangle,
                                     uint32_t tileUID)
{
    if (heapNodes == nextTreeNodeIndex+1)
    {
        // our tree and heap are going to collide
#ifdef DEBUGGINGAIDS
        OSD_Printf("tilepacker: maxheap_reserveNode(): out of nodes\n");
#endif
        return NULL;
    }

    nodes[heapNodes] = {(TreeNode*) pParent,
                        (TreeNode*) pChild0,
                        (TreeNode*) pChild1,
                        rectangle,
                        rectangle.width >= rectangle.height ? rectangle.width : rectangle.height,
                        tileUID};
    ++heapNodes;

    return nodes+heapNodes-1;
}

static TreeNode* kdtree_reserveNode(TreeNode *pParent,
                                    TreeNode *pChild0,
                                    TreeNode *pChild1,
                                    TileRect rectangle,
                                    uint32_t tileUID)
{
    if (nextTreeNodeIndex == heapNodes-1)
    {
        // our tree and heap are going to collide
#ifdef DEBUGGINGAIDS
        OSD_Printf("tilepacker: kdtree_reserveNode(): out of nodes\n");
#endif
        return NULL;
    }

    nodes[nextTreeNodeIndex] = {(TreeNode*) pParent,
                                (TreeNode*) pChild0,
                                (TreeNode*) pChild1,
                                rectangle,
                                rectangle.width >= rectangle.height ? rectangle.width : rectangle.height,
                                tileUID};
    --nextTreeNodeIndex;

    return nodes+nextTreeNodeIndex+1;
}

static char kdtree_add(uint32_t treeIndex, TreeNode *pNode)
{
    TreeNode *pCurrentNode = nodes+NUM_NODES-treeIndex-1;
    while (true)
    {
        // is this node large enough to contain the currentNode?
        if (pCurrentNode->rect.width >= pNode->rect.width &&
            pCurrentNode->rect.height >= pNode->rect.height)
        {
            if (pCurrentNode->tileUID != (uint32_t) -1)
            {
                // if we're not a leaf node, continue to tunnel down until we reach a leaf on the 0-side of the tree
                pCurrentNode = pCurrentNode->pChild0;
                continue;
            }

            // otherwise, we have the node we want to split to insert our new node
            break;
        }

        // climb out until we find an unexplored 1-side branch to tunnel down
        TreeNode *lastNode;
        do
        {
            lastNode = pCurrentNode;
            pCurrentNode = pCurrentNode->pParent;

            if (pCurrentNode == NULL)
            {
                // we've fully explored the tree and asked for the root's parent
                return false;
            }
        } while (pCurrentNode->pChild1 == lastNode || !pCurrentNode->pChild1);

        pCurrentNode = pCurrentNode->pChild1;
    }

    // assign the empty leaf node the tileUID we want to add, then create children to split the node's remaining space
    pCurrentNode->tileUID = pNode->tileUID;
    tiles[pNode->tileUID].tilesheetID = treeIndex;
    tiles[pNode->tileUID].rect = {pCurrentNode->rect.u,
                                  pCurrentNode->rect.v,
                                  pNode->rect.width,
                                  pNode->rect.height};

    uint32_t rightSideWidth = pCurrentNode->rect.width - pNode->rect.width;
    uint32_t bottomSideHeight = pCurrentNode->rect.height - pNode->rect.height;
    TileRect rect0 = {pCurrentNode->rect.u+pNode->rect.width, pCurrentNode->rect.v, rightSideWidth, pCurrentNode->rect.height};
    TileRect rect1 = {pCurrentNode->rect.u, pCurrentNode->rect.v+pNode->rect.height, pNode->rect.width, bottomSideHeight};
    // decide which way to split
    if (rightSideWidth < bottomSideHeight)
    {
        //POGOTODO: instead of creating two new children and having the tree contain filled nodes
        //          I should usurp the place of pCurrentNode with the largest child,
        //          since the smaller child can always be contained within the larger one.
        //          This requires an additional sort/bubbling step and always splitting width/height
        //          based on level rather than max in order for the organization to work.

        // we'll have a more width-confined space to our right, so chop it off horizontally
        // rather than create a tall, potentially narrow empty area to have to fill later
        rect0 = {pCurrentNode->rect.u, pCurrentNode->rect.v+pNode->rect.height, pCurrentNode->rect.width, bottomSideHeight};
        rect1 = {pCurrentNode->rect.u+pNode->rect.width, pCurrentNode->rect.v, rightSideWidth, pNode->rect.height};
    }
    // it's important that the larger area become child0
    pCurrentNode->pChild0 = kdtree_reserveNode(pCurrentNode,
                                               NULL,
                                               NULL,
                                               rect0,
                                               -1);
    pCurrentNode->pChild1 = kdtree_reserveNode(pCurrentNode,
                                               NULL,
                                               NULL,
                                               rect1,
                                               -1);

    return true;
}

static char rejectQueue_add(TreeNode *pNode)
{
    if (numRejected >= MAX_REJECTS)
    {
        return false;
    }

    rejectQueue[(rejectQueueHeadIndex+numRejected) % MAX_REJECTS] = *pNode;
    ++numRejected;
    return true;
}

static TreeNode* rejectQueue_remove()
{
    if (numRejected == 0)
    {
        return NULL;
    }

    --numRejected;
    TreeNode* pNode = rejectQueue+rejectQueueHeadIndex;
    rejectQueueHeadIndex = (rejectQueueHeadIndex+1) % MAX_REJECTS;
    return pNode;
}

/*static void tilepacker_deleteTree()
{
    //POGOTODO: this
}*/

void tilepacker_initTilesheet(uint32_t tilesheetID, uint32_t tilesheetWidth, uint32_t tilesheetHeight)
{
    //POGOTODO: delete the tree if it's already been initialized

    nodes[NUM_NODES-tilesheetID-1] = {(TreeNode*) 0,
                                      (TreeNode*) 0,
                                      (TreeNode*) 0,
                                      {0, 0, tilesheetWidth, tilesheetHeight},
                                      tilesheetWidth >= tilesheetHeight ? tilesheetWidth : tilesheetHeight,
                                      (uint32_t) -1}; // use the maximum uint32_t to indicate this node is empty space
}

void tilepacker_addTile(uint32_t tileUID, uint32_t tileWidth, uint32_t tileHeight)
{
    if (nextTreeNodeIndex < heapNodes)
    {
        // cannot reserve any more tiles!
#ifdef DEBUGGINGAIDS
        OSD_Printf("tilepacker: tilepacker_addTile(): out of nodes\n");
#endif
        return;
    }
    if (tileWidth == 0 ||
        tileHeight == 0)
    {
        // don't allow adding tiles with a width or height of 0
        return;
    }

    maxheap_reserveNode((TreeNode*) 0,
                        (TreeNode*) 0,
                        (TreeNode*) 0,
                        {0, 0, tileWidth, tileHeight},
                        tileUID);
}

char tilepacker_pack(uint32_t tilesheetID)
{
    if (tilesheetID >= MAXTILESHEETS)
    {
        return false;
    }

    for (uint32_t numLeft = numRejected; numLeft > 0; --numLeft)
    {
        TreeNode *pNode = rejectQueue_remove();
        char success = kdtree_add(tilesheetID, pNode);
        if (!success)
        {
            rejectQueue_add(pNode);
        }
    }

    maxheap_buildHeap();
    for (TreeNode *pNode = maxheap_pop(); pNode != NULL; pNode = maxheap_pop())
    {
        char success = kdtree_add(tilesheetID, pNode);
        if (!success)
        {
            rejectQueue_add(pNode);
        }
    }

    return numRejected == 0;
}

void tilepacker_discardRejects()
{
    numRejected = 0;
}

char tilepacker_getTile(uint32_t tileUID, Tile *pOutput)
{
    if (tileUID >= MAXPACKEDTILES)
    {
        return false;
    }

    Tile tile = tiles[tileUID];
    if (tile.rect.width == 0)
    {
        // that tileUID has not been packed or didn't fit
        return false;
    }

    if (pOutput)
    {
        *pOutput = tile;
    }
    return true;
}
