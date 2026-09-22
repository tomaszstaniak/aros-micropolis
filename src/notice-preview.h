#ifndef MICROPOLIS_NOTICE_PREVIEW_H
#define MICROPOLIS_NOTICE_PREVIEW_H
#include "messages.h"
#include <algorithm>
inline bool noticePreviewVisible(const CityMessage &m,bool noticesEnabled){return noticesEnabled&&m.picture&&m.hasLocation();}
inline void noticePreviewCamera(int x,int y,int columns,int rows,int &cameraX,int &cameraY){cameraX=std::clamp(x-columns/2,0,std::max(0,120-columns));cameraY=std::clamp(y-rows/2,0,std::max(0,100-rows));}
#endif
