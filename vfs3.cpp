#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <stdexcept>


using namespace std;

// Структура для кэширования данных из crontab
struct CronJobData {
    vector<string> jobs;
    bool valid;
};

CronJobData get_cron_jobs() {
  CronJobData data;
  data.valid = false;
  FILE *fp = fopen("/etc/crontab", "r");
  if (fp != NULL) {
    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL) {
      data.jobs.push_back(line);
    }
    fclose(fp);
    data.valid = true;
  } else {
    cerr << "Ошибка открытия /etc/crontab" << endl;
  }
  return data;
}


static int xmp_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    static CronJobData cronData = get_cron_jobs();
    if (!cronData.valid) return -EIO;

    size_t job_index;
    try {
      job_index = stoi(string(path).substr(4)) -1 ; // Извлекаем индекс задачи из пути
      if (job_index >= cronData.jobs.size() || job_index < 0) return -ENOENT;
    } catch (const invalid_argument& e) {
      return -ENOENT;
    } catch (const out_of_range& e) {
      return -ENOENT;
    }

    stbuf->st_mode = S_IFREG | 0444;
    stbuf->st_nlink = 1;
    stbuf->st_size = cronData.jobs[job_index].length();
    return 0;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;
    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    static CronJobData cronData = get_cron_jobs();
    if (!cronData.valid) return -EIO;

    for (size_t i = 0; i < cronData.jobs.size(); ++i) {
      stringstream filename;
      filename << "job_" << i + 1;
      filler(buf, filename.str().c_str(), NULL, 0);
    }
    return 0;
}


static int xmp_open(const char *path, struct fuse_file_info *fi) {
    return 0;
}


static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
                     struct fuse_file_info *fi) {
    static CronJobData cronData = get_cron_jobs();
    if (!cronData.valid) return -EIO;

    size_t job_index;
    try {
      job_index = stoi(string(path).substr(4)) - 1; // Извлекаем индекс задачи из пути
      if (job_index >= cronData.jobs.size() || job_index < 0) return -ENOENT;
    } catch (const invalid_argument& e) {
      return -ENOENT;
    } catch (const out_of_range& e) {
      return -ENOENT;
    }

    size_t len = cronData.jobs[job_index].size();
    if (offset < len) {
        size_t count = (size > len - offset) ? len - offset : size;
        memcpy(buf, cronData.jobs[job_index].c_str() + offset, count);
        return count;
    } else {
        return 0;
    }
}


static struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open = xmp_open,
    .read = xmp_read,
};


int main(int argc, char *argv[]) {
    return fuse_main(argc, argv, &xmp_oper, NULL);
}
