#include <stdio.h>
#include <web_assets_basehttp.h>

#include "zip_file_server.h"

int main(int argc, char* argv[])
{
    ZipFileSystem zfs;
    // HTTP
    ZipFileServer svr;

    uint32_t assets_len = 0;
    const uint8_t* assets = NULL;
    get_web_assets_basehttp(&assets, &assets_len);

    if (!zfs.init(assets, assets_len)) {
        printf("init zip file system fail.\n");
        return -1;
    }
    svr.set_filesystem_handle(&zfs);
    svr.set_fs_base_dir("/static");
    // ajax request srv
    svr.Get("/hello", [](const Request& req, Response& res) {
      res.set_content("{\"id\": 123}", "application/json");
    });

    svr.listen("0.0.0.0", 8080);

    return 0;
}