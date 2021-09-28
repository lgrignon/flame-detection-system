//
//  main.cpp
//  FlameDetection
//
//  Created by liberize on 14-4-4.
//  Copyright (c) 2014年 liberize. All rights reserved.
//

#include <cstdio>
#include <string>
#include <iostream>
#include <filesystem>

#include "common.h"
#include "VideoHandler.h"
#include "FlameDetector.h"

#ifdef TRAIN_MODE
bool trainComplete = false;
#endif

using std::cout, std::endl, std::string;
namespace fs = std::filesystem;

VideoHandler* videoHandler = NULL;

int main(int argc, const char* argv[])
{
    cout << "current dir: " << string(fs::current_path()) << endl;
    
    VideoHandler handler(string(PROJECT_DIR) + "/clips/5.avi");
    videoHandler = &handler;
    
    int ret = handler.handle();
    
    switch (ret) {
    case VideoHandler::STATUS_FLAME_DETECTED:
        cout << "Flame detected." << endl;
        break;
    case VideoHandler::STATUS_OPEN_CAP_FAILED:
        cout << "Open capture failed." << endl;
        break;
    case VideoHandler::STATUS_NO_FLAME_DETECTED:
        cout << "No flame detected." << endl;
        break;
    default:
        break;
    }

    return 0;
}
