#ifndef vzImageLoad_h
#define vzImageLoad_h

#define fio_get_buffer(f, buf, len) fread(buf, len, 1, f)
#define fio_skip(f, bytes) fseek(f, bytes, SEEK_CUR)
#define fio_seek(f, bytes) fseek(f, bytes, SEEK_SET)
#define fio_get_be16(f) (fio_get_byte(f) << 8 | fio_get_byte(f))
#define fio_get_be32(f) (fio_get_byte(f) << 24 | fio_get_byte(f) << 16 | fio_get_byte(f) << 8 | fio_get_byte(f))
#define fio_get_le16(f) (fio_get_byte(f) << 0 | fio_get_byte(f) << 8)
#define fio_get_le32(f) (fio_get_byte(f) << 0 | fio_get_byte(f) << 8 | fio_get_byte(f) << 16 | fio_get_byte(f) << 24)

inline unsigned int fio_get_byte(FILE* f)
{
    unsigned int b = 0;
    fio_get_buffer(f, &b, 1);
    return b;
};

#endif /* vzImageLoad_h */
