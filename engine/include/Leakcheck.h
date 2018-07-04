#ifndef LEAKCHECK_H
#define LEAKCHECK_H

#ifdef _WINDOWS
#include <windows.h>
#ifdef _DEBUG
//#define DBG_NEW new (_NORMAL_BLOCK, __FILE__, __LINE__)
//#define new DBG_NEW
#endif
#endif

#endif