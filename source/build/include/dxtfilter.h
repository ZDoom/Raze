
#ifndef DXTFILTER_H_
#define DXTFILTER_H_

void dxt_handle_io(int32_t fil, int32_t len, void *midbuf, char *packbuf);
int32_t dedxt_handle_io(int32_t fil, int32_t, void *midbuf, int32_t mbufsiz, char *packbuf, int32_t ispacked);

int32_t dxtfilter(int32_t fil, const texcachepicture *pict, const char *pic, void *midbuf, char *packbuf, uint32_t miplen);
int32_t dedxtfilter(int32_t fil, const texcachepicture *pict, char *pic, void *midbuf, char *packbuf, int32_t ispacked);

#endif
