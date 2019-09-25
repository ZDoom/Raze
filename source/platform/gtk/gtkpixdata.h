#include <gdk-pixbuf/gdk-pixdata.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _EDuke32_GdkPixdata EDuke32_GdkPixdata;
struct _EDuke32_GdkPixdata
{
  guint32 magic;
  gint32  length;
  guint32 pixdata_type;
  guint32 rowstride;
  guint32 width;
  guint32 height;
  const char *pixel_data;
};

extern const EDuke32_GdkPixdata startbanner_pixdata;

#ifdef __cplusplus
}
#endif

