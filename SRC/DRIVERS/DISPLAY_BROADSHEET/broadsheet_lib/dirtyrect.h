
#ifndef __DIRTYRECT_H__
#define __DIRTYRECT_H__


typedef void* HDIRTYRECT;
typedef void (*CBWORK)(RECT rect);


HDIRTYRECT DirtyRect_Init(CBWORK cbWork);
void DirtyRect_Add(HDIRTYRECT hDR, RECT rect);
void DirtyRect_Destroy(HDIRTYRECT hDR);


#endif	//__DIRTYRECT_H__

