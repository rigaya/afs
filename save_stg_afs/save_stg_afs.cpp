// -----------------------------------------------------------------------------------------
// 自動フィールドシフト 高速化版 by rigaya
// -----------------------------------------------------------------------------------------
// The MIT License
//
// Copyright (c) 2012-2016 rigaya
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// --------------------------------------------------------------------------------------------

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include "../afs/afs.h"
#include "../afs/afs_stg.h"
#include "aviutl_sav_parser.h"

void write_stg_file(bool bForVF, const std::string& output_file_name, FilterConfig *afs_conf) {
    int proc_mode = 0x00;
    if (afs_conf->other_data.size() > 0) {
        AFS_EX_DATA *ex_data = (AFS_EX_DATA *)afs_conf->other_data.data();
        proc_mode = ex_data->proc_mode;
    }
    write_stg_file(bForVF, output_file_name.c_str(),
        afs_conf->track_data.data(), afs_conf->track_data.size(),
        afs_conf->check_data.data(), afs_conf->check_data.size(),
        proc_mode);
}

int save_stg_afs(const char *filename) {
    int ret = 1;
    using namespace std;
    std::string output_file_name = std::string(filename) + ".afs.ini";
    ifstream input_file(filename, ios::in | ios::binary);
    if (input_file.good()) {
        vector<char> file_data;
        file_data.resize((size_t)input_file.seekg(0, ios::end).tellg() + 1, '\0');
        input_file.seekg(0, ios::beg).read(&file_data[0], static_cast<std::streamsize>(file_data.size()));

        FilterConfigData filter_config;
        auto size_read = filter_config.parse(vector<char>(file_data.begin(), file_data.end()), 0);
        if (size_read > 0) {
            auto afs_conf = filter_config.find_filter("自動フィールドシフト");
            if (afs_conf) {
                write_stg_file(false, output_file_name, afs_conf);
            }
            afs_conf = filter_config.find_filter("自動フィールドシフトVF");
            if (afs_conf) {
                write_stg_file(true, output_file_name, afs_conf);
            }
            ret = 0;
        }
    }
    return ret;
}

void print_help() {
    fprintf(stderr,
        "save_stg_afs.exe by rigaya\n"
        " Aviutlのcfgファイルから、自動フィールドシフトの設定を保存します。\n"
        "\n"
        " Aviutlフォルダに置いてダブルクリックすれば、\n"
        " フォルダ内の全ての*.cfgを処理対象とします。\n"
        "\n"
        " あるいは、自動フィールドシフトの設定を保存したい\n"
        " cfgファイルをドラッグドロップしてください。\n");
}

int main(int argc, char **argv) {
    int ret = 0;
    int count = 0;
    if (argc == 1) {
        char curdir[1024];
        GetCurrentDirectory(_countof(curdir), curdir);
        WIN32_FIND_DATA win32fd = { 0 };
        HANDLE hnd = FindFirstFile((std::string(curdir) + "\\*.cfg").c_str(), &win32fd);
        if (hnd != INVALID_HANDLE_VALUE) {
            do {
                if (0 == (win32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    ret |= save_stg_afs((std::string(curdir) + "\\" + win32fd.cFileName).c_str());
                    count++;
                }
            } while (FindNextFile(hnd, &win32fd));
        }
    } else {
        for (int i_arg = 1; i_arg < argc; i_arg++) {
            ret |= save_stg_afs(argv[i_arg]);
            count++;
        }
    }
    if (count == 0)
        print_help();
    return ret;
}
