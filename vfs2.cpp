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

using namespace std;

// Замените это на ваш собственный способ получения данных из cron
vector<string> get_cron_jobs() {
  vector<string> jobs;
  // Здесь должен быть ваш код для чтения данных из crontab или другого источника
  // Пример:  Предположим, что данные хранятся в файле /etc/crontab
  FILE *fp = fopen("/etc/crontab", "r");
  if (fp != NULL) {
    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL) {
      jobs.push_back(line); // Добавляем строку из crontab в вектор
    }
    fclose(fp);
  } else {
    cerr << "Ошибка открытия /etc/crontab" << endl;
  }
  return jobs;
}


static int xmp_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    vector<string> jobs = get_cron_jobs();
    int job_index = 0;
    // Проверяем, является ли путь именем файла(задачи)
    stringstream ss(path);
    string segment;
    ss >> segment; // пропускаем начальный /
    ss >> segment; // имя файла(задачи)

    if(ss && job_index < jobs.size()){
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = jobs[job_index].length();
        return 0;
    }

    return -ENOENT;
}

static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;
    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    vector<string> jobs = get_cron_jobs();
    for (size_t i = 0; i < jobs.size(); ++i) {
      // Создаем имена файлов из номеров задач (или используйте более подходящую схему наименования)
      stringstream filename;
      filename << "job_" << i + 1;
      filler(buf, filename.str().c_str(), NULL, 0);
    }

    return 0;
}


static int xmp_open(const char *path, struct fuse_file_info *fi) {
    // Проверка доступа
    return 0;
}


static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
                     struct fuse_file_info *fi) {
    vector<string> jobs = get_cron_jobs();
    int job_index = 0;
    stringstream ss(path);
    string segment;
    ss >> segment;
    ss >> segment;

    if(ss && job_index < jobs.size()){
        size_t len = jobs[job_index].size();
        if (offset < len) {
            size_t count = (size > len - offset) ? len - offset : size;
            memcpy(buf, jobs[job_index].c_str() + offset, count);
            return count;
        } else {
            return 0;
        }
    }

    return -ENOENT;
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

