#ifndef _ZIP_FILE_SERVER_H_
#define _ZIP_FILE_SERVER_H_

#include "zip_filesystem.h"

#define CPPHTTPLIB_USE_POLL
#include <httplib.h>

using namespace httplib;

class ZipFileServer: public Server
{
protected:
  FileSystem* fs_handle;

public:
  ZipFileServer(): Server(), fs_handle(nullptr) {
      // 自定义文件处理器
      set_pre_routing_handler(std::bind(&ZipFileServer::pre_file_request, this, 
                                            std::placeholders::_1, std::placeholders::_2));
  }

  void set_filesystem_handle(FileSystem* fs) {
      fs_handle = fs;
  }

  HandlerResponse pre_file_request(const Request &req, Response &res){
    // File handler
    bool is_head_request = req.method == "HEAD";
    if ((req.method == "GET" || is_head_request) &&
        ex_handle_file_request(req, res, is_head_request)) {
        return HandlerResponse::Handled;
    }
    return HandlerResponse::Unhandled;
  };
  bool set_fs_base_dir(const std::string &mount_point){
    return set_fs_mount_point(mount_point, "./");
  }
  bool set_fs_mount_point(const std::string &mount_point, const std::string &dir,
                       Headers headers = Headers()){
    std::string mnt = !mount_point.empty() ? mount_point : "/";
    if (!mnt.empty() && mnt[0] == '/') {
      base_dirs_.push_back({mnt, std::string("@@@@") + dir, std::move(headers)});
      return true;
    }
    return false;
  }

  bool ex_handle_file_request(const Request &req, Response &res, bool head) {
    if (nullptr == fs_handle) {
      return false;
    }
    for (const auto &entry : base_dirs_) {
        // Prefix match, 
        if (req.path.compare(0, entry.mount_point.size(), entry.mount_point)) {
          continue;
        }
        std::string sub_path = req.path.substr(entry.mount_point.size());
        // 校验路径是否有效
        if (!detail::is_valid_path(sub_path)) {
          continue;
        }
        // in zip file system.
        auto path = entry.base_dir.substr(6) + sub_path;
        // need drop first / tag
        if (path[0] == '/') { path = path.substr(1);}
        // 
        if (path.back() == '/') { path += "index.html"; }
        if (path.length() == 0) { path = "index.html"; }
        // req的相应文件路径，在mount_point中，是否能够访问到
        if (!fs_handle->is_exist(path.c_str())) {
          continue;
        }
        File* zip_file = nullptr;
        // 打开文件
        if (0 != fs_handle->open(path.c_str(), &zip_file)) {
          continue;
        }
        FileInfo fi;
        memset(&fi, 0, sizeof(fi));
        // 获取解压后的大小
        if ((0 != zip_file->stat(&fi)) || (fi.file_size <= 0)) {
          // 释放文件句柄。
          zip_file->close();
          continue;
        }
        res.body.resize(static_cast<size_t>(fi.file_size));
        // 读取文件
        zip_file->read((uint8_t*)&res.body[0], static_cast<std::streamsize>(fi.file_size));

        std::map<std::string, std::string> null_map;
        auto type = detail::find_content_type(path, null_map);
        if (type) { res.set_header("Content-Type", type); }
        for (const auto &kv : entry.headers) {
            res.set_header(kv.first.c_str(), kv.second);
        }
        res.status = req.has_header("Range") ? 206 : 200;
        // 释放文件句柄。
        zip_file->close();
        return true;
    }
    return false;
  }
};


#endif
