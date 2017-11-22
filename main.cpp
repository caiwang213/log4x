#include "log4x.h"
#include <stdio.h>

using namespace log4x;


int main(int argc, char *argv[])
{
    ilog4x *i =  ilog4x::instance();
    i->config("log.ini");
    i->start();

    for (int i = 0;  i < 10; ++i)
    {
        LOGW("i: " << i);
    }
    getchar();
    i->stop();

    return 0;
}
