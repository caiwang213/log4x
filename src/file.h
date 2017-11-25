/**
 * @file      file.h
 * @copyright Copyright (c) 2017, UT Technology Co., Ltd. All Rights Reserved.
 * @brief     brief
 * @author    caiwang213@qq.com
 * @date      2017-11-25 21:54:17
 *
 * @note
 *  file.h defines
 */
#ifndef __FILE_H__
#define __FILE_H__

/* #include <iostream> */
/* #include <fstream> */
/* #include <sstream> */
/* #include <thread> */
/* #include <mutex> */
#include <string>
#include <list>
/* #include <vector> */
/* #include <queue> */
/* #include <deque> */
/* #include <map> */
/* #include <algorithm> */
/* #include <condition_variable> */

/* #include <string.h> */
/* #include <time.h> */
/* #include <stdarg.h> */
/* #include <sys/timeb.h> */

#ifdef WIN32
/* #include <windows.h> */
/* #include <shlwapi.h> */
/* #pragma comment(lib, "shlwapi") */
/* #pragma warning(disable:4200) */
/* #pragma warning(disable:4996) */
#else
#include <unistd.h>
#include <fcntl.h>
/* #include <sys/stat.h> */
/* #include <dirent.h> */
/* #include <sys/syscall.h> */
#endif
namespace log4x
{
class File
{
public:
    File();
    ~File();

public:
    inline bool        isOpen();
    inline long        open(const char *path, const char * mod);
    inline void        clean(int index, int len);
    inline void        close();
    inline void        write(const char * data, size_t len);
    inline void        flush();

    inline std::string readLine();
    inline bool        removeFile(const std::string & path);
    inline std::string readContent();
public:
    FILE *_file;
};

File::File()
{
    _file = NULL;
}

File::~File()
{
    close();
}

inline bool
File::isOpen()
{
    return _file != NULL;
}

inline long
File::open(const char *path, const char * mod)
{
    if (_file != NULL)
    {
        fclose(_file);
        _file = NULL;
    }

    _file = fopen(path, mod);
    if (_file)
    {
        long tel = 0;
        long cur = ftell(_file);
        fseek(_file, 0L, SEEK_END);
        tel = ftell(_file);
        fseek(_file, cur, SEEK_SET);
        return tel;
    }

    return -1;
}

inline void
File::clean(int index, int len)
{
#if !defined(__APPLE__) && !defined(WIN32)
    if (_file != NULL)
    {
        int fd = fileno(_file);
        fsync(fd);
        posix_fadvise(fd, index, len, POSIX_FADV_DONTNEED);
        fsync(fd);
    }
#endif
}

inline void
File::close()
{
    if (_file != NULL)
    {
        clean(0, 0);
        fclose(_file);
        _file = NULL;
    }
}

inline void
File::write(const char * data, size_t len)
{
    if (_file && len > 0)
    {
        if (fwrite(data, 1, len, _file) != len)
        {
            close();
        }
    }
}

inline void
File::flush()
{
    if (_file)
    {
        fflush(_file);
    }
}

inline std::string
File::readLine()
{
    char buf[500] = { 0 };
    if (_file && fgets(buf, 500, _file) != NULL)
    {
        return std::string(buf);
    }
    return std::string();
}

inline bool
File::removeFile(const std::string & path)
{
    return ::remove(path.c_str()) == 0;
}

inline std::string
File::readContent()
{
    std::string content;

    if (!_file)
    {
        return content;
    }
    char buf[BUFSIZ];
    size_t ret = 0;
    do
    {
        ret = fread(buf, sizeof(char), BUFSIZ, _file);
        content.append(buf, ret);
    }
    while (ret == BUFSIZ);

    return content;
}
}
#endif
