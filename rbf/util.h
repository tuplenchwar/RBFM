//
// Created by Divya Meharwade on 22/01/24.
//

#ifndef COEN380_W24_G1_UTIL_H
#define COEN380_W24_G1_UTIL_H

#endif //COEN380_W24_G1_UTIL_H

#include <iostream>
#include <string>
#include <cassert>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#include "pfm.h"
#include "rbfm.h"

// Check whether a file exists
bool FileExistsConst(const string &fileName)
{
    struct stat stFileInfo;

    if(stat(fileName.c_str(), &stFileInfo) == 0) return true;
    else return false;
}