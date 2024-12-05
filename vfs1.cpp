#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <vector>
#include <string>
#include <sstream>
#include <unistd.h>


struct CronJob {
    std::string schedule;
    std::string command;
};

std::pair<std::string, std::vector<CronJob>> getCronData() {
    std::vector<CronJob> jobs;
    std::string username;
    username = "Denis";
    jobs.push_back({ "*/5 * * * *", "echo 'Test job every 5 minutes'" });

    return { username, jobs };
}


static int vfs_getattr(const char* path, struct stat* stbuf) {
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    int jobIndex = atoi(path + 1);
    auto [username, jobs] = getCronData();
    if (jobIndex >= 0 && jobIndex < jobs.size()) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = jobs[jobIndex].command.length() + jobs[jobIndex].schedule.length();
        return 0;
    }
    return -ENOENT;
}

static int vfs_readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi) {
    (void)offset;
    (void)fi;
    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    std::vector<CronJob> jobs = get_cron_jobs();
    for (size_t i = 0; i < jobs.size(); ++i) {
        char entry[10];
        sprintf(entry, "%zu", i);
        filler(buf, entry, NULL, 0);
    }
    return 0;
}

static int vfs_open(const char* path, struct fuse_file_info* fi) {
    if (atoi(path + 1) < 0) return -ENOENT;
    return 0;
}

static int vfs_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    int jobIndex = atoi(path + 1);
    std::vector<CronJob> jobs = get_cron_jobs();
    if (jobIndex >= 0 && jobIndex < jobs.size()) {
        std::string data = jobs[jobIndex].schedule + "\n" + jobs[jobIndex].command;
        size_t len = data.length();
        if (offset < len) {
            size_t count = std::min((size_t)(len - offset), size);
            memcpy(buf, data.c_str() + offset, count);
            return count;
        }
    }
    return 0;
}

static const struct fuse_operations vfs_oper = {
    .getattr = vfs_getattr,
    .readdir = vfs_readdir,
    .open = vfs_open,
    .read = vfs_read
};


int main(int argc, char* argv[]) {
    return fuse_main(argc, argv, &vfs_oper, NULL);
}

