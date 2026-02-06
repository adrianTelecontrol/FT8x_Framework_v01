#include <xdc/runtime/System.h>

#include <stdlib.h>
#include <string.h>
#include "Surface.h"


GenericWidgetNode* srfCreateNode(GenericWidget *wd)
{

    GenericWidgetNode* node = (GenericWidgetNode *)malloc((sizeof(GenericWidgetNode)));
    if(node == NULL)
        return NULL;

    node->psNext = NULL;
    node->psPrev = NULL;
    node->sWidget.pvWidget = NULL;
    switch (wd->eWidgetType) 
    {
        case WD_TYPE_BUTTON:
            node->sWidget.pvWidget = (Button *)malloc(sizeof(Button));
            memcpy(node->sWidget.pvWidget, wd->pvWidget, sizeof(Button));
        break;

        case WD_TYPE_RECT:
            node->sWidget.pvWidget = (Rectangle *)malloc(sizeof(Rectangle));
            memcpy(node->sWidget.pvWidget, wd->pvWidget, sizeof(Rectangle));
        break;

        default:
        break;
    }

    if(node->sWidget.pvWidget != NULL)
    {
        node->sWidget.eWidgetType = wd->eWidgetType;
    }

    return node; 
}

bool srfInsertAtBottom(GenericWidgetNode** head, GenericWidget *wd)
{
    GenericWidgetNode *newNode = srfCreateNode(wd);

    // Check if head is empty
    if(*head == NULL)
    {
        *head = newNode;
    }

    newNode->psNext = *head;
    (*head)->psPrev = newNode;
    *head = newNode;

    return true;
}

bool srfInsertAtTop(GenericWidgetNode** head, GenericWidget *wd)
{
    srfPrintListSize(head);
    GenericWidgetNode *newNode = srfCreateNode(wd);

    // Check if head is empty
    if(*head == NULL)
    {
        *head = newNode;
        return true;
    }

    GenericWidgetNode *temp = *head;
    while(temp->psNext != NULL)
    {
        temp = temp->psNext;
    }
    temp->psNext = newNode;
    newNode->psPrev = temp;

    return true;
}

bool srfInsertAtPosition(GenericWidgetNode** head, GenericWidget *wd, uint8_t pos)
{
    if(pos == 1)
    {
        srfInsertAtBottom(head, wd);
        return true;
    }
    GenericWidgetNode *newNode = srfCreateNode(wd);
    GenericWidgetNode *temp = *head;

    uint8_t i = 0;
    for(i = 0; temp != NULL && i < pos - 1; i++)
    {
        temp = temp->psNext;
    }

    if(temp == NULL)
    {
        System_printf("Position greater than the number of nodes");
        System_flush();
        return false;
    }

    newNode->psNext = temp->psNext;
    newNode->psPrev = temp;
    if(temp->psNext != NULL)
    {
        temp->psNext->psPrev = newNode;
    }
    temp->psNext = newNode;

    return true;
}

bool srfDeleteAtBeginning(GenericWidgetNode** head)
{
    if(*head == NULL)
    {
        return true;
    }

    GenericWidgetNode *temp = *head;
    *head = (*head)->psNext;
    if(*head != NULL)
    {
        (*head)->psPrev = NULL;
    }
    if(temp->sWidget.pvWidget != NULL)
        free(temp->sWidget.pvWidget);
    free(temp);
}

bool srfDeleteAtEnd(GenericWidgetNode** head)
{
    if(*head == NULL)
    {
        return true;
    }

    GenericWidgetNode *temp = *head;
    if(temp->psNext == NULL)
    {
        *head = NULL;
        free(temp);
        return true;
    }
    while (temp->psNext != NULL) 
    {
        temp = temp->psNext;
    }
    temp->psPrev->psNext = NULL;
    if(temp->sWidget.pvWidget != NULL)
        free(temp->sWidget.pvWidget);
    free(temp);

    return true;
}

bool srfDeleteAtPosition(GenericWidgetNode** head, uint8_t pos)
{
    if(*head == NULL)
    {
        return true;
    }
    GenericWidgetNode* temp = *head;
    if(pos == 1)
    {
        srfDeleteAtBeginning(head);
        return true;
    }
    unsigned int i = 0;
    for (i = 0; temp != NULL && i < pos - 1; i++) 
    {
        temp = temp->psNext;
    }
    if(temp == NULL)
    {
        System_printf("Position greater than the number of nodes");
        System_flush();
        return false;
    }
    if(temp->psNext != NULL)
    {
        temp->psNext->psPrev = temp->psPrev;
    }
    if(temp->psPrev != NULL)
    {
        temp->psPrev->psNext = temp->psNext;
    }

    if(temp->sWidget.pvWidget != NULL)
        free(temp->sWidget.pvWidget);
    free(temp);
    return true;
}

bool srfReverseTraverseList(GenericWidgetNode** head)
{
    if(*head == NULL)
    {
        System_printf("List is empty\n");
        System_flush();
        return false;
    }
    
    GenericWidgetNode *temp = *head;
    
    // Move to the last node
    while(temp->psNext != NULL)
    {
        temp = temp->psNext;
    }
    
    uint8_t position = 0;
    
    // Traverse backwards using psPrev
    while(temp != NULL)
    {
        temp = temp->psPrev;
        position++;
    }
    
    return true;
}

bool srfForwardTraverseList(GenericWidgetNode** head)
{
    if(*head == NULL)
    {
        System_printf("List is empty\n");
        System_flush();
        return false;
    }
    
    GenericWidgetNode *temp = *head;
    uint8_t position = 0;
    
    while(temp != NULL)
    {
        temp = temp->psNext;
        position++;
    }
    
    return true;
}

uint32_t srfGetListSizeBytes(GenericWidgetNode** head)
{
    if(*head == NULL)
        return 0;
    
    uint32_t totalSize = 0;
    GenericWidgetNode *temp = *head;
    
    while(temp != NULL)
    {
        // Size of the node itself
        totalSize += sizeof(GenericWidgetNode);
        
        // Size of the widget data
        if(temp->sWidget.pvWidget != NULL)
        {
            switch(temp->sWidget.eWidgetType)
            {
                case WD_TYPE_BUTTON:
                    totalSize += sizeof(Button);
                    break;
                    
                case WD_TYPE_RECT:
                    totalSize += sizeof(Rectangle);
                    break;
                    
                default:
                    break;
            }
        }
        
        temp = temp->psNext;
    }
    
    return totalSize;
}

void srfPrintListSize(GenericWidgetNode** head)
{
    if(*head == NULL)
    {
        System_printf("List is empty (0 bytes)\n");
        System_flush();
        return;
    }
    
    uint32_t totalSize = 0;
    uint32_t nodeCount = 0;
    uint32_t buttonCount = 0;
    uint32_t rectCount = 0;
    
    GenericWidgetNode *temp = *head;
    
    while(temp != NULL)
    {
        nodeCount++;
        totalSize += sizeof(GenericWidgetNode);
        
        if(temp->sWidget.pvWidget != NULL)
        {
            switch(temp->sWidget.eWidgetType)
            {
                case WD_TYPE_BUTTON:
                    totalSize += sizeof(Button);
                    buttonCount++;
                    break;
                    
                case WD_TYPE_RECT:
                    totalSize += sizeof(Rectangle);
                    rectCount++;
                    break;
                    
                default:
                    break;
            }
        }
        
        temp = temp->psNext;
    }
    
    // System_printf("\n=== List Memory Usage ===\n");
    // System_printf("Total Nodes:      %u\n", nodeCount);
    // System_printf("  - Buttons:      %u (%u bytes each)\n", buttonCount, sizeof(Button));
    // System_printf("  - Rectangles:   %u (%u bytes each)\n", rectCount, sizeof(Rectangle));
    // System_printf("Node Overhead:    %u bytes (%u bytes each)\n", 
    //               nodeCount * sizeof(GenericWidgetNode), sizeof(GenericWidgetNode));
    // System_printf("------------------------\n");
    // System_printf("Total List Size:  %u bytes\n", totalSize);
    // System_printf("========================\n\n");
    // System_flush();
}