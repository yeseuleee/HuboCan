/*
 * Copyright (c) 2015, Georgia Tech Research Corporation
 * All rights reserved.
 *
 * Author(s): Michael X. Grey <greyxmike@gmail.com>
 *
 * Humanoid Robotics Lab
 *
 * Directed by Prof. Mike Stilman <mstilman@cc.gatech.edu>
 *
 * This file is provided under the following "BSD-style" License:
 *   Redistribution and use in source and binary forms, with or
 *   without modification, are permitted provided that the following
 *   conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 *   USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 *   AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *   POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>

#include "HuboRT/utils.hpp"

size_t HuboRT::split_components(const std::string &text, StringArray &array, char delimiter)
{
    array.resize(0);
    size_t pos=0, last_pos=0, count=0;
    while(std::string::npos != (pos = text.find(delimiter, last_pos)))
    {
        ++count;
        array.push_back(text.substr(last_pos, pos-last_pos));
        last_pos = pos+1;
    }
    
    return count;
}

StringArray HuboRT::grab_files_in_dir(const std::string &directory)
{
    StringArray result;

    DIR* dptr = opendir(directory.c_str());
    if(dptr == NULL)
    {
        return result;
    }

    struct dirent* entry;
    while( NULL != (entry = readdir(dptr)) )
    {
        if(DT_REG == entry->d_type)
        {
            result.push_back(entry->d_name);
        }
    }

    return result;
}

StringArray HuboRT::grab_dirs_in_dir(const std::string &directory)
{
    StringArray result;

    DIR* dptr = opendir(directory.c_str());
    if(dptr == NULL)
    {
        return result;
    }

    struct dirent* entry;
    while( NULL != (entry = readdir(dptr)) )
    {
        if(DT_DIR == entry->d_type)
        {
            result.push_back(entry->d_name);
        }
    }

    return result;
}
