#include <WebServer/base/FileUtil.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;
using namespace ywl;

AppendFile::AppendFile(const string& filename)
    : fp_(fopen(filename.c_str(), "ae"))
{
    fp_ = fopen(filename.c_str(), "ae");
    if (fp_ == NULL) {
        fprintf(stderr, "open %s failed!\n", filename.c_str());
        exit(1);
    }
    setbuffer(fp_, buffer_, sizeof buffer_);
}

AppendFile::~AppendFile()
{
    fclose(fp_);
}

void AppendFile::append(const char* logline, const size_t len)
{
    int nleft = len;
    size_t nwritten;
    const char* bufp = logline;
    while (nleft > 0)
    {
        nwritten = this->write(bufp, nleft);
        if (nwritten == 0)
        {
            int err = ferror(fp_);
            if (err)
                fprintf(stderr, "AppendFile::append() failed !\n");
            break;
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
}

void AppendFile::flush()
{
    fflush(fp_);
}

size_t AppendFile::write(const char* logline, size_t len)
{
    return fwrite_unlocked(logline, 1, len, fp_);
}
