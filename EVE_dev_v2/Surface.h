#ifndef SURFACE_H
#define SURFACE_H

#include <stdint.h>
#include <stdbool.h>

#include "Widgets.h"

GenericWidgetNode* srfCreateNode(GenericWidget *wd);

bool srfInsertAtBottom(GenericWidgetNode** head, GenericWidget *wd);

bool srfInsertAtTop(GenericWidgetNode** head, GenericWidget *wd);

bool srfInsertAtPosition(GenericWidgetNode** head, GenericWidget *wd, uint8_t pos);

bool srfDeleteAtBeginning(GenericWidgetNode** head);

bool srfDeleteAtEnd(GenericWidgetNode** head);

bool srfDeleteAtPosition(GenericWidgetNode** head, uint8_t pos);

bool srfReverseTraverseList(GenericWidgetNode** head);

bool srfForwardTraverseList(GenericWidgetNode** head);

uint32_t srfGetListSizeBytes(GenericWidgetNode** head);

void srfPrintListSize(GenericWidgetNode** head);

#endif // SURFACE_H

