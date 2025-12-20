#ifndef _STRINGBUF_H_
#define _STRINGBUF_H_
/* Simple String buffer for the lexer */
extern char *str_buffer;

void str_reset();
void str_append(const char *str, int len);
void chr_append(const char c);
void str_free();

#endif // _STRINGBUF_H_
