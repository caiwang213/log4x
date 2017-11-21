#include "log4x.h"
#include <stdio.h>

using namespace log4x;


int main(int argc, char *argv[])
{
    ilog4x *i =  ilog4x::instance();
    i->config("log.ini");
    i->start();

    getchar();
    i->stop();

    return 0;
}
