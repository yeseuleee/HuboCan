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

#include <fstream>
#include <ostream>
#include <iostream>

#include "HuboCan/DdParser.hpp"

namespace HuboCan {

DdParser::DdParser()
{
    _initialize();
}

void DdParser::_initialize()
{
    reset();
    _status = DD_END_FILE;
    error_output_stream = &std::cerr;
    _error_input_stream = new std::stringstream;
}

bool DdParser::load_file(const std::string &filename)
{
    return _push_back_file(filename, false);
}

bool DdParser::_push_back_file(const std::string &filename, bool inclusion, const std::string &parent_filename)
{
    std::string full_filename = filename;

    if(_status == DD_ERROR)
        return false;
    
    if(inclusion)
    {
        if(full_filename.at(0) != '/')
        {
            size_t base_directory = parent_filename.rfind('/');
            if(base_directory != std::string::npos)
                full_filename = parent_filename.substr(0, base_directory+1) + full_filename;
        }

        for(size_t i=0; i<_filename_list.size(); ++i)
        {
            if( full_filename == _filename_list[i] )
            {
                error() << "Infinite loop found after including '" << full_filename << "'! Aborting!";
                report_error();
                return false;
            }
        }
    }

    _filename_list.push_back(full_filename);

    std::ifstream filestream;
    filestream.open(full_filename.c_str());

    if(!filestream.is_open())
    {
        error() << "Could not open '" << filename << "'! (Resolved to '" << full_filename << "')";
        report_error();
        return false;
    }
    
    if(inclusion)
    {
        _contents.erase(_contents.end());
    }

    size_t count = 0;
    while(filestream.good())
    {
        ++count;
        DdLine new_line;
        new_line.file = full_filename;
        new_line.line_num = count;

        std::getline(filestream, new_line.data);

        _contents.push_back(new_line);
        _next_index = _contents.size();

        StringArray line_check = get_string_components(new_line.data);
        if(_status == DD_ERROR)
            return false;

        if(!_inclusion_check(line_check, full_filename))
            return false;
    }

    if(_status == DD_ERROR)
        return false;

    _next_index = 0;
    _status = DD_OKAY;
    return true;
}

bool DdParser::_inclusion_check(StringArray &line, std::string parent_filename)
{
    if(line.size() > 0)
    {
        if(line[0] == "include")
        {
            if(line.size() > 1)
            {
                if(!_push_back_file(line[1], true, parent_filename))
                {
                    return false;
                }
            }
            else
            {
                error() << "The keyword 'include' must be followed by a file name!";
                report_error();
                return false;
            }
        }
    }
    return true;
}

StringArray DdParser::get_string_components(const std::string &line)
{
    StringArray result;

    size_t progress = 0;
    std::string remaining = line;
    size_t comment = remaining.find("#",0);
    remaining = remaining.substr(0, comment);

    while(remaining.size() > 0)
    {
        progress += clear_front_back_whitespace(remaining);

        if(remaining.size() > 0)
        {
            size_t count=0;
            
            if(remaining.at(0) == '\"')
            {
                ++count;
                while(remaining.at(count) != '\"')
                {
                    ++count;
                    
                    if(count >= remaining.size())
                    {
                        error() << "Unclosed quotes starting at character #" << progress-1;
                        report_error();
                        result.clear();
                        return result;
                    }
                }
                
                if(count+1 < remaining.size())
                {
                    if(!isspace(remaining.at(count+1)))
                    {
                        error() << "Found illegal quotes at character #" << progress+count+1;
                        report_error();
                        result.clear();
                        return result;
                    }
                }
                
                progress += count+1;
                std::string component = remaining.substr(1, count-1);
                remaining = remaining.substr(count+1, std::string::npos);
                result.push_back(component);
            }
            else
            {
                while(!isspace(remaining.at(count)))
                {
                    if(remaining.at(count) == '\"')
                    {
                        error() << "Found illegal quotes at character #" << progress+count+1;
                        report_error();
                        result.clear();
                        return result;
                    }
                    
                    ++count;
                    if(count >= remaining.size())
                        break;
                }
                
                progress += count;
                std::string component = remaining.substr(0, count);
                remaining = remaining.substr(count, std::string::npos);
                result.push_back(component);
            }
        }
    }

    return result;
}

size_t DdParser::clear_front_back_whitespace(std::string &line)
{
    clear_back_whitespace(line);
    return clear_front_whitespace(line);
}

size_t DdParser::clear_front_whitespace(std::string &line)
{
    if(line.size() == 0)
        return 0;

    size_t count=0;
    while(isspace(line.at(count)))
    {
        ++count;
        if(count >= line.size() )
            return 0;
    }

    line = line.substr(count, std::string::npos);
    
    return count;
}

void DdParser::clear_back_whitespace(std::string &line)
{
    if(line.size() == 0)
        return;

    size_t count = line.size()-1;
    while(isspace(line.at(count)))
    {
        --count;
        if(count == std::string::npos)
            return;
    }

    line = line.substr(0, count+1);
}

void DdParser::report_error()
{
    if(_next_index-1 < _contents.size())
    {
        (*error_output_stream) << "Error! " << _contents[_next_index-1];
    }
    else
    {
        (*error_output_stream) << "next_index: " << _next_index-1 << ", size: " << _contents.size() << std::endl;
    }
    
    (*error_output_stream) << error().str() << std::endl;
    error().str("");
    error().clear();
    
    _status = DD_ERROR;
}

dd_result_t DdParser::next_line(StringArray &components)
{
    components.clear();
    
    if(_status == DD_ERROR)
        return DD_ERROR;
    
    while(   components.size() == 0
          && _next_index < _contents.size())
    {
        components = get_string_components(_contents[_next_index].data);
        ++_next_index;
    }
    
    if(_status == DD_ERROR)
        return DD_ERROR;
    
    if(_next_index >= _contents.size())
    {
        _status = DD_END_FILE;
        return _status;
    }
    
    if(components[0] == "begin")
    {
        if(_current_device_type != "")
        {
            error() << "The keyword 'begin' is being used inside of a device's description!";
            report_error();
            return _status;
        }
        
        if(components.size() < 2)
        {
            error() << "The keyword 'begin' must be followed by a device type!";
            report_error();
            _current_device_type = "";
            return _status;
        }
        else
        {
            _current_device_type = components[1];
        }

        _status = DD_BEGIN_DEVICE;
        return _status;
    }
    
    if(components[0] == "end")
    {
        if(components.size() < 2)
        {
            error() << "The keyword 'end' must be followed by a device type!";
            report_error();
            return _status;
        }
        else
        {
            if(components[1] == _current_device_type)
            {
                _current_device_type = "";
            }
            else
            {
                error() << "The device type accompanying this 'end' does not match the last 'begin'!";
                report_error();
                return _status;
            }
        }
        
        _status = DD_END_DEVICE;
        return _status;
    }
    
    _status = DD_OKAY;
    return _status;
}



bool DdParser::current_line(StringArray &components)
{
    components.clear();

    if(_status == DD_ERROR)
        return false;

    size_t backtrack = 1;

    while(components.empty())
    {

        if(_next_index-backtrack < _contents.size())
        {
            components = get_string_components(_contents[_next_index-backtrack].data);
        }
        else
        {
            return false;
        }

        ++backtrack;
    }

    return true;
}

} // namespace HuboCan
