/* Model of the corrected conversion line, not the game: a 300-column row of
 * 4-byte UTF-8 characters converted with the corrected bound into the
 * 1024-byte caller buffer used by borg_get_messages(). */
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#define ABS(a) (((a) < 0) ? (-(a)) : (a))
int main(int argc, char **argv) {
    int n = argc > 1 ? atoi(argv[1]) : 300;
    wchar_t screen_str[1024];
    char buffer[1024];
    if (!setlocale(LC_CTYPE, "C.UTF-8")) return 2;
    for (int i = 0; i < n; i++) screen_str[i] = 0x1F600; /* 4 bytes in UTF-8 */
    screen_str[n] = L'\0';
    size_t r = wcstombs(buffer, screen_str, ABS(n) * MB_CUR_MAX);
    printf("n=%d MB_CUR_MAX=%zu limit=%zu wrote=%zu buffer=1024\n", n, (size_t)MB_CUR_MAX, (size_t)(ABS(n) * MB_CUR_MAX), r);
    return 0;
}
