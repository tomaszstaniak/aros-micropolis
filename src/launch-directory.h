#ifndef MICROPOLIS_LAUNCH_DIRECTORY_H
#define MICROPOLIS_LAUNCH_DIRECTORY_H
#include <proto/dos.h>
#include <workbench/startup.h>
#include <string>
#include <vector>
// The engine's scenario paths are relative. Preserve explicit CLI arguments
// before entering PROGDIR, and restore the caller's lock before CRT cleanup.
class LaunchDirectory {
public:
    std::vector<std::string> arguments;
    LaunchDirectory()=default;
    LaunchDirectory(const LaunchDirectory&)=delete;
    LaunchDirectory& operator=(const LaunchDirectory&)=delete;
    ~LaunchDirectory(){if(directory_){CurrentDir(previous_);UnLock(directory_);}}
    bool prepare(int argc,char **argv,std::string &error){
        // argc==0 means argv is a WBStartup message, not a string array.
        // A project icon (or an icon dropped on ours) is the first extra
        // WBArg: a directory lock plus a name inside it.
        if(argc==0 && argv){
            auto *startup=(WBStartup *)argv;
            if(startup->sm_NumArgs>1){
                const WBArg &project=startup->sm_ArgList[1];
                char path[4096];
                if(!project.wa_Lock || !NameFromLock(project.wa_Lock,(STRPTR)path,sizeof path) ||
                   !AddPart((STRPTR)path,(CONST_STRPTR)project.wa_Name,sizeof path)){
                    error="Cannot resolve the city icon's path.";return false;
                }
                arguments.emplace_back(path);
            }
        }
        for(int i=1;i<argc && i<=3;++i){
            BPTR lock=Lock((CONST_STRPTR)argv[i],SHARED_LOCK);
            if(!lock){error="Cannot open launch argument: "+std::string(argv[i]);return false;}
            char path[4096];
            bool ok=NameFromLock(lock,(STRPTR)path,sizeof path);
            UnLock(lock);
            if(!ok){error="Launch path is too long or unavailable.";return false;}
            arguments.emplace_back(path);
        }
        directory_=Lock((CONST_STRPTR)"PROGDIR:",SHARED_LOCK);
        if(!directory_){error="Cannot open the program directory.";return false;}
        previous_=CurrentDir(directory_);return true;
    }
private:
    BPTR directory_=0,previous_=0;
};
#endif
