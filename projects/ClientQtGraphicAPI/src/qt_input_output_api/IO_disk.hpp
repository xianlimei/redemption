/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2010-2013
   Author(s): Clément Moroldo

*/

#pragma once

#include <sys/ioctl.h>
#include <sys/statvfs.h>
#include <linux/hdreg.h>

#include "utils/fileutils.hpp"
#include "utils/log.hpp"
//#include "client_redemption/client_redemption_api.hpp"
#include "client_redemption/client_input_output_api/client_iodisk_api.hpp"
#include "core/RDP/channels/rdpdr.hpp"



class IODisk : public ClientIODiskAPI
{

public:
    IODisk() {}

    bool ifile_good(const char * new_path) override {
        std::ifstream file(new_path);
        if (file.good()) {
            file.close();
            return true;
        }
        file.close();
        return false;
    }

    bool ofile_good(const char * new_path) override {
        std::ofstream file(new_path, std::ios::out | std::ios::binary);
        if (file.good()) {
            file.close();
            return true;
        }
        file.close();
        return false;
    }

    bool dir_good(const char * new_path) override {
        struct stat buff_dir;
        return stat(new_path, &buff_dir);
    }

    void marke_dir(const char * new_path) override {
        mkdir(new_path, ACCESSPERMS);
    }

    FileStat get_file_stat(const char * file_to_request) override {
        struct stat buff;
        stat(file_to_request, &buff);

        FileStat fileStat;
        fileStat.LastAccessTime = UnixSecondsToWindowsTick(buff.st_atime);
        fileStat.LastWriteTime  = UnixSecondsToWindowsTick(buff.st_mtime);
        fileStat.CreationTime    = fileStat.LastWriteTime - 1;
        fileStat.ChangeTime     = UnixSecondsToWindowsTick(buff.st_ctime);
        fileStat.FileAttributes = fscc::FILE_ATTRIBUTE_ARCHIVE;

        if (S_ISDIR(buff.st_mode)) {
            fileStat.FileAttributes = fscc::FILE_ATTRIBUTE_DIRECTORY;
        } else {
            fileStat.AllocationSize = buff.st_size;
            fileStat.EndOfFile      = buff.st_size;
        }

        fileStat.NumberOfLinks  = buff.st_nlink;
        fileStat.DeletePending  = 1;
        fileStat.Directory      = 0;
        if (S_ISDIR(buff.st_mode)) {
            fileStat.Directory = 1;
        }

        return fileStat;
    }

    FileStatvfs get_file_statvfs(const char * file_to_request) override {
        struct statvfs buffvfs;
        statvfs(file_to_request, &buffvfs);

         uint64_t freespace(buffvfs.f_bfree * buffvfs.f_bsize);

         FileStatvfs fileStatvfs;
         fileStatvfs.TotalAllocationUnits           = freespace + 0x1000000;
         fileStatvfs.CallerAvailableAllocationUnits = freespace;
         fileStatvfs.AvailableAllocationUnits       = freespace;
         fileStatvfs.ActualAvailableAllocationUnits = freespace;

         fileStatvfs.BytesPerSector                 = buffvfs.f_bsize;
         fileStatvfs.MaximumComponentNameLength     = buffvfs.f_namemax;

        return fileStatvfs;
    }

//     erref::NTSTATUS read_data(const  std::string & file_to_tread,
//                               int file_size,
//                               int offset,
//                               std::unique_ptr<uint8_t[]> & ReadData,
//                               bool log_erro_on) override {
//
//         std::ifstream ateFile(file_to_tread, std::ios::binary| std::ios::ate);
//         if(ateFile.is_open()) {
//             if (file_size > ateFile.tellg()) {
//                 file_size = ateFile.tellg();
//             }
//             ateFile.close();
//
//             std::ifstream inFile(file_to_tread, std::ios::in | std::ios::binary);
//             if(inFile.is_open()) {
//                 ReadData = std::make_unique<uint8_t[]>(file_size+offset);
//                 inFile.read(reinterpret_cast<char *>(ReadData.get()), file_size+offset);
//                 inFile.close();
//             } else {
//                 if (log_erro_on) {
//                     LOG(LOG_WARNING, "  Can't open such file : \'%s\'.", file_to_tread.c_str());
//                 }
//                 return erref::NTSTATUS::STATUS_NO_SUCH_FILE;
//             }
//
//         } else {
//             if (log_erro_on) {
//                 LOG(LOG_WARNING, "  Can't open such file : \'%s\'.", file_to_tread.c_str());
//             }
//             return erref::NTSTATUS::STATUS_NO_SUCH_FILE;
//         }
//
//         return erref::NTSTATUS::STATUS_SUCCESS;
//     }
//
//     erref::NTSTATUS read_data(
//         std::string const& file_to_tread, int offset, byte_array data,
//         bool log_erro_on
//     ) override {
//         auto const fsz = filesize(file_to_tread.c_str());
//         if (fsz < 0) {
//             return erref::NTSTATUS::STATUS_NO_SUCH_FILE;
//         }
//         if (offset > fsz) {
//             return erref::NTSTATUS::STATUS_UNSUCCESSFUL;
//         }
//
//         auto const remaining = fsz - offset;
//
//         assert(std::size_t(remaining) <= data.size());
//
//         std::ifstream inFile(file_to_tread, std::ios::in | std::ios::binary);
//
//         if (inFile.fail()) {
//             if (log_erro_on) {
//                 LOG(LOG_WARNING, "  Can't open such file : \'%s\'.", file_to_tread.c_str());
//             }
//             // TODO or STATUS_ACCESS_DENIED
//             return erref::NTSTATUS::STATUS_NO_SUCH_FILE;
//         } else {
//             inFile.read(data.to_charp(), std::min(remaining, int(data.size())));
//         }
//
//         return erref::NTSTATUS::STATUS_SUCCESS;
//     }

    bool set_elem_from_dir(std::vector<std::string> & elem_list, const std::string & str_dir_path) override {
        elem_list.clear();

        DIR *dir;
        struct dirent *ent;
        std::string ignored1("..");
        std::string ignored2(".");

        if ((dir = opendir (str_dir_path.c_str())) != nullptr) {

            try {
                while ((ent = readdir (dir)) != nullptr) {

                    std::string current_name = std::string (ent->d_name);

                    if (!(current_name == ignored1) && !(current_name == ignored2)) {
                        elem_list.push_back(current_name);
                    }
                }
            } catch (Error & e) {
                LOG(LOG_WARNING, "readdir error: (%u) %s", e.id, e.errmsg());
            }
            closedir (dir);

            return true;

        } else {
            return false;
        }
    }

    int get_device(const char * file_path) override {
        return open(file_path, O_RDONLY | O_NONBLOCK);
    }

    uint32_t get_volume_serial_number(int device) override {
        static struct hd_driveid hd;
        ioctl(device, HDIO_GET_IDENTITY, &hd);
        return this->string_to_hex32(hd.serial_no);
    }

    bool write_file(const char * file_to_write, const char * data, int data_len) override {
        std::ofstream oFile(file_to_write, std::ios::out | std::ios::binary);
        if (oFile.good()) {
            oFile.write(data, data_len);
            oFile.close();
            return true;
        }
        return false;
    }

    bool remove_file(const char * file_to_remove) override {
        return remove(file_to_remove) != 0;
    }

    bool rename_file(const char * file_to_rename,  const char * new_name) override {
        return rename(file_to_rename, new_name) != 0;
    }


};

