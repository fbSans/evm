//To include the definitions before including the header define the macro SV_IMPLEMENTATION
#ifndef SV_H_
#define SV_H_

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>


#define SV_FMT "%.*s"
#define SV_ARG(sv) (int) (sv).size, (sv).data 

#define SV_NULL (Sv) {.data = NULL, .size = 0}

typedef struct {
    const char *data;
    size_t size;
} Sv;

Sv sv_from_cstr(char *src);
Sv sv_from_parts(const char *data, size_t size);
Sv sv_clone(Sv sv);

void sv_trim_left(Sv *sv);
void sv_trim_right(Sv *sv);

Sv sv_take(Sv *sv, size_t n);
Sv sv_take_while(Sv *sv, bool(*pred)(char));
Sv sv_take_until(Sv *sv, bool(*pred)(char));
Sv sv_take_until_char(Sv *sv, char c);
Sv sv_next_line(Sv *sv);
Sv sv_substr(Sv sv, size_t start, size_t end);
Sv sv_chop_left(Sv *sv);


bool sv_eq(const Sv sv1, const Sv sv2);
bool sv_starts_with(Sv haystack, Sv neddle);
bool sv_ends_with(Sv haystack, Sv needle);
bool sv_find(Sv haystack, Sv needle, size_t *fst_pos);

#ifdef SV_IMPLEMENTATION

#define MIN(a, b) (a) < (b) ? (a) : (b)
#define MAX(a, b) (a) > (b) ? (a) : (b)
#define CLAMP(v, min, max) MAX(MIN((v), (max)), min) 

static bool _isspace(char c)
{
    return isspace(c);
}

static bool _isnewline(char c)
{
    return c == '\n';
}

Sv sv_from_cstr(char *src)
{
    return (Sv) {
        .data = src,
        .size = strlen(src),
    };
}

Sv sv_from_parts(const char *data, size_t size)
{
    return (Sv) {
        .data = data,
        .size = size,
    };
}

Sv sv_clone(Sv sv)
{
    return sv;
}

void sv_trim_left(Sv *sv)
{
    sv_take_while(sv, _isspace);
}

void sv_trim_right(Sv *sv)
{
    size_t i = sv->size;
    for(; i > 0; --i){
        if(isspace(sv->data[i-1])) sv->size--;
    }
}

Sv sv_take(Sv *sv, size_t n)
{
    n = CLAMP(n, 0, sv->size);
    Sv res = sv_from_parts(sv->data, n);  
    sv->data += n;
    sv->size -= n;
    return res; 
}

Sv sv_take_while(Sv *sv, bool(*pred)(char))
{
    size_t i = 0;
    for(; i < sv->size; ++i){
        if(!pred(sv->data[i])) break;
    }
    
    Sv res = sv_from_parts(sv->data, i);
    sv->data += i;
    sv->size -= i;
    return res;
}

Sv sv_take_until(Sv *sv, bool(*pred)(char))
{
    size_t i = 0;
    for(; i < sv->size; ++i){
        if(pred(sv->data[i])) break;
    }
    
    Sv res = sv_from_parts(sv->data, i);
    sv->data += i;
    sv->size -= i;
    return res;
}

Sv sv_take_until_char(Sv *sv, char c)
{
    size_t i = 0;
    for(; i < sv->size; ++i){
        if(sv->data[i] == c) break;
    }
    
    Sv res = sv_from_parts(sv->data, i);
    sv->data += i;
    sv->size -= i;
    assert(sv->size == 0 || *sv->data == c);
    return res;
}

Sv sv_next_line(Sv *sv)
{
    Sv res = sv_take_until(sv, _isnewline);
    assert(sv->size == 0 || *sv->data == '\n');
    if(sv->size >= 0) sv_take(sv, 1);
    return res;
}

Sv sv_substr(Sv sv, size_t start, size_t end)
{
    if(start >= end) return SV_NULL;

    if(start >= sv.size) return SV_NULL;

    Sv res = sv_from_parts(sv.data + start, end - start);
    
    return res;
}

Sv sv_chop_left(Sv *sv){
    return sv_take_until(sv, _isspace);
}

bool sv_eq(const Sv sv1, const Sv sv2)
{
    if(sv1.size != sv2.size) return false;

    for(size_t i = 0; i < sv1.size; ++i){
        if(sv1.data[i] != sv2.data[i]) return false;
    }

    return true;
}

bool sv_starts_with(Sv haystack, Sv neddle) 
{
    if(haystack.size < neddle.size) return false;
    
    return sv_eq(neddle, sv_take(&haystack, neddle.size));
}

bool sv_ends_with(Sv haystack, Sv neddle) 
{
    if(haystack.size < neddle.size) return false;
    
    return sv_eq(neddle, sv_from_parts(haystack.data + haystack.size - neddle.size, neddle.size));
}

bool sv_find(Sv haystack, Sv needle, size_t *fst_pos)
{
    size_t i = 0;

    while(haystack.size > needle.size){
        if(sv_starts_with(haystack, needle)){
            if(fst_pos) *fst_pos = i;
            return true;
        }
        i++;
        sv_take(&haystack, 1);
    }

    if(haystack.size - needle.size == 0){
        if(sv_starts_with(haystack, needle)){
            if(fst_pos) *fst_pos = i;
            return true;
        }
        i++;
        sv_take(&haystack, 1);
    }

    return false;
}


#undef CLAMP
#undef MIN
#undef MAX

#endif // SV_IMPLEMENTATION

#endif //SV_H_