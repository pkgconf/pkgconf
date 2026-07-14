#include <zlib.h>
#include <stdio.h>
#include <string.h>

int
main(void)
{
    const char *input = "the quick brown fox jumps over the lazy dog";
    unsigned char compressed[128];
    unsigned char restored[128];
    uLong compressed_len = sizeof(compressed);
    uLong restored_len = sizeof(restored);

    printf("zlib version %s\n", zlibVersion());

    /* Round-trip a buffer so a broken -I/-L/-l from the macros shows up as a
     * link or runtime failure rather than passing silently. */
    if (compress(compressed, &compressed_len, (const unsigned char *) input, strlen(input) + 1) != Z_OK)
        return 1;
    if (uncompress(restored, &restored_len, compressed, compressed_len) != Z_OK)
        return 1;

    return strcmp(input, (const char *) restored) == 0 ? 0 : 1;
}
