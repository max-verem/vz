#ifndef MEM_LEAKCHECK_H
#define MEM_LEAKCHECK_H

#ifdef _DEBUG

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#endif /* _DEBUG */

#endif /* MEM_LEAKCHECK_H */
