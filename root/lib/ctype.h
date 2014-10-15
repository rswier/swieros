// ctype.h

int islower(int c) { return c >= 'a' && c <= 'z'; }
int isupper(int c) { return c >= 'A' && c <= 'Z'; }
int isdigit(int c) { return c >= '0' && c <= '9'; }
int isalpha(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
int isalnum(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'); }
int isspace(int c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
int iscntrl(int c) { return c < ' ' && c != 0; }
int isblank(int c) { return c == ' ' || c == '\t'; }
int isprint(int c) { return c >= ' ' && c <= '~'; }
int isgraph(int c) { return c > ' ' && c <= '~'; }
int ispunct(int c) { return c > ' ' && c <= '~' && !isalnum(c); }
int isxdigit(int c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
int tolower(int c) { return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c; }
int toupper(int c) { return (c >= 'a' && c <= 'z') ? c + ('A' - 'a') : c; }
