#ifndef MICROPOLIS_STARTUP_WINDOW_H
#define MICROPOLIS_STARTUP_WINDOW_H
#include "startup-model.h"
#include "bmp.h"
struct Window;
using StartupFilePicker=bool (*)(Window *,bool,char *,int);
std::unique_ptr<Micropolis> showStartup(Window *parent,const BmpImage &tiles,
    StartupModel::Factory factory,StartupFilePicker picker,std::string &error,
    bool returnToCity=false);
#endif
