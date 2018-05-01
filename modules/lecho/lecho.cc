#include "lushan.h"
#include "lconf.h"
#include "llog.h"
#include "lutil.h"
#include "url.h"
#include "hmod.h"
#include "hdict.h"
#include <string>
#include <queue>

/*  Authors: */
/*      Tao Hui <taohui3@gmail.com> */

using namespace std;

extern "C" int hmodule_open(const char *path, void **xdata)
{
    return 0;
}

extern "C" void hmodule_close(void *xdata)
{
    return;
}

extern "C" int hmodule_handle(const char *req, char **outbuf, int *osize, int *obytes, void **xdata, hdb_t *hdb)
{
    const hrequest_t *r = (hrequest_t *)req;

    lpack_ascii(r->key, outbuf, osize, obytes, r->value_len, r->value);
    return 0;
}
