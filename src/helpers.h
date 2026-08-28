

//This is a header only library: https://en.wikipedia.org/wiki/Header-only
//The ideas for this are very inspired by Tsoding Nob: https://github.com/tsoding/nob.h

//This is not tested rigoursly yet, use at your own discretion
//They had been tested as separate modules, StringView, StringBuilder and The Macros for Dynamic Arrays
//But since the integrations tests are need to really make sure that this works like intended, specially for edge cases

#ifndef HELPERS_H_
#define HELPERS_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h> 
#include <assert.h>
#include <errno.h>

#define HP_DA_INIT_CAP (1024)
 
#ifdef HELPERS_REMOVE_PREFIX
    #define da_append                 hp_da_append
    #define da_reserve                hp_da_reserve
    #define da_align                  hp_da_align
    #define da_pop                    hp_da_pop
    #define da_remove_ord             hp_da_remove_ord
    #define da_remove                 hp_da_remove
    #define da_append_array           hp_da_append_array
    #define da_foreach                hp_da_foreach

    #define StringBuilder             hp_StringBuilder
    #define sb_append_null            hp_sb_append_null
    #define sb_append_cstrs           hp_sb_append_cstrs
    #define sb_remove                 hp_sb_remove
    #define shift_args                hp_shift_args
    #define read_file_into_sb         hp_read_file_into_sb
    #define write_file_from_sb        hp_write_file_from_sb
    #define sb_reserve                hp_sb_reserve
    #define sb_append_sstr            hp_sb_append_sstr

    #define StringView                hp_StringView
    #define sv_from_cstr              hp_sv_from_cstr
    #define sv_from_parts             hp_sv_from_parts
    #define sv_clone                  hp_sv_clone
    #define sv_trim_left              hp_sv_trim_left
    #define sv_trim_right             hp_sv_trim_right
    #define sv_eq                     hp_sv_eq
    #define sv_starts_with            hp_sv_starts_with
    #define sv_ends_with              hp_sv_ends_with
    #define sv_find                   hp_sv_find
    #define sv_take                   hp_sv_take
    #define sv_take_while             hp_sv_take_while
    #define sv_take_until             hp_sv_take_until
    #define sv_next_line              hp_sv_next_line
    #define sv_substr                 hp_sv_substr
    #define sv_chop_left              hp_sv_chop_left

    #define  Arena          hp_Arena
    #define  Arena_Mark     hp_Arena_Mark

    #define arena_alloc     hp_arena_alloc
    #define arena_free      hp_arena_free
    #define arena_clean     hp_arena_clean
    #define arena_Mark      hp_arena_mark
    #define arena_restore   hp_arena_restore
    #define arena_strdup    hp_arena_strdup
    #define arena_sprintf   hp_arena_sprintf
    #define arena_usage     hp_arena_usage
#endif //HELPER_REMOVE_PREFIX



#define UNIMPLEMENTED do {                                                              \
    fprintf(stderr, "%s:%d %s: not implemented yet!\n", __FILE__, __LINE__, __func__);  \
    exit(1);                                                                            \
}while(0)

#define UNREACHABLE do {                                                               \
    fprintf(stderr, "%s:%d %s: unreachable!\n",__FILE__, __LINE__,  __func__);         \
    exit(1);                                                                           \
}while(0)

#define TODO(msg) do {                                                                 \
    fprintf(stderr, "todo: %s:%d %s: `%s`\n",__FILE__, __LINE__,  __func__, (msg));    \
    exit(1);                                                                           \
}while(0)

#define RETURN_DEFER(resvar, resval) do { \
        (resvar) = (resval);\
        goto defer;\
    } while(0)

#define ARRAY_LEN(a) sizeof((a))/sizeof((a)[0])


//Dynamic Array Macros
#define hp_da_append(da, item) do {                                                                             \
    if((da)->count >= (da)->capacity){                                                                          \
        if((da)->capacity == 0) (da)->items = NULL;                                                             \
        (da)->capacity = ((da)->capacity == 0) ? HP_DA_INIT_CAP  : (da)->capacity * 2;                          \
        (da)->items = realloc((da)->items, (da)->capacity * sizeof(*(da)->items));                              \
        memset((da)->items + (da)->count, 0, ((da)->capacity - (da)->count) * sizeof(*(da)->items));            \
    }                                                                                                           \
    (da)->items[(da)->count++] = item;                                                                          \
} while (0)

#define hp_da_reserve(da, n) do {                                                                               \
    if((da)->count + (n) + 1 > (da)->capacity){                                                                 \
        (da)->capacity = ((da)->count + (n) + 1) * 2;                                                           \
        (da)->items = realloc((da)->items, (da)->capacity * sizeof(*(da)->items));                              \
        assert((da)->items != NULL);                                                                            \
    }                                                                                                          \
} while (0)

#define hp_da_align(da, alignment) do {                                                                         \
        hp_da_reserve((da), (((da)->count + (alignment) - 1) & ~((alignment) - 1)) - (da)->count);              \
        (da)->count = ((da)->count + (alignment) - 1) & ~((alignment) - 1);                                     \
} while (0)

#define hp_da_pop(da) do {                                                                  \
    if((da)->count > 0) (da)->count = (da)->count - 1;                                      \
} while(0)                                                                                  \

#define hp_da_remove_ord(da, idx) do {                                                      \
    assert((idx) > 0 && (idx) < (da)->count);                                               \
    memcpy((da)->items + (idx), (da)->items + (idx) + 1, (da)->count - (idx) - 1);          \
    hp_da_pop((da));                                                                        \
} while(0)

#define hp_da_remove(da, idx) do {                                                         \
    assert((idx) > 0 && (idx) < (da)->count);                                            \
    (da)->items[(idx)] = (da)->items[(da)->count];                                       \
    hp_da_pop((da));                                                                         \
} while(0)                                                                              \

#define hp_da_append_array(da, arr, count) do {                                            \
    for(size_t (it) = 0; (it) < count; ++(it)){                                               \
        hp_da_append((da), (arr)[(it)]);                                                       \
    }                                                                                   \
} while(0)

#define hp_da_foreach(type, it, da) for(type (it) = (da)->items; (it) < (da)->items + (da)->count; ++(it))


typedef struct hp_StringBuilder{
    char *items;
    size_t count;
    size_t capacity;
}  hp_StringBuilder;

typedef struct {
    const char *data;
    size_t count;
} hp_StringView;

typedef struct hp_Arena hp_Arena;
typedef struct hp_Arena_Mark hp_Arena_Mark;


//String Builder Macros and functions
#define hp_sb_append_null(sb) da_append((sb), '\0')
#define hp_sb_append_cstrs(sb, ...) hp_sb_append_cstrs_impl((sb), __VA_ARGS__, NULL)
#define hp_sb_remove(sb, idx) da_remove_ord((sb), (idx))
char *hp_shift_args(int *argc, char ***argv);
void hp_sb_reserve(hp_StringBuilder *sb, size_t n);
void hp_sb_append_sstr(hp_StringBuilder *sb, const char *str, size_t count);
bool hp_read_file_into_sb(hp_StringBuilder *sb, const char *filepath);
bool hp_write_file_from_sb(const hp_StringBuilder *sb, const char *filepath, const char *mode);


//String View Macros and Functions
#define SV_FMT "%.*s"
#define SV_ARG(sv) (int) (sv).count, (sv).data 
#define SV_NULL (hp_StringView) {.data = NULL, .count = 0}
hp_StringView hp_sv_from_cstr(const char *src);
hp_StringView hp_sv_from_parts(const char *data, size_t count);
hp_StringView hp_sv_clone(hp_StringView sv);

void hp_sv_trim_left(hp_StringView *sv);
void hp_sv_trim_right(hp_StringView *sv);

bool hp_sv_eq(const hp_StringView sv1, const hp_StringView sv2);
bool hp_sv_starts_with(hp_StringView haystack, hp_StringView neddle);
bool hp_sv_ends_with(hp_StringView haystack, hp_StringView needle);
bool hp_sv_find(hp_StringView haystack, hp_StringView needle, size_t *fst_pos);

hp_StringView hp_sv_take(hp_StringView *sv, size_t n);
hp_StringView hp_sv_take_while(hp_StringView *sv, bool(*pred)(char));
hp_StringView hp_sv_take_until(hp_StringView *sv, bool(*pred)(char));
hp_StringView hp_sv_take_until_char(hp_StringView *sv, char c);
hp_StringView hp_sv_next_line(hp_StringView *sv);
hp_StringView hp_sv_substr(hp_StringView sv, size_t start, size_t end);
hp_StringView hp_sv_chop_left(hp_StringView *sv);

//Conversions
hp_StringView hp_sb_to_sv(const hp_StringBuilder *sb);



void *hp_arena_alloc(hp_Arena *a, size_t n);
/**Frees all memory that was allocated for the arean */
void hp_arena_free(hp_Arena *a);
/**Simply zeroes out sizes for memory reuse, region capacities remains the same, no memory free occurs*/
void hp_arena_clean(hp_Arena *a);
/** Marks the current allocation point. */
hp_Arena_Mark hp_arena_mark(hp_Arena *a);
/** Restores (rewinds) the hp_arena to a previous mark, discarding later allocations. */
void hp_arena_restore(hp_Arena *a, hp_Arena_Mark m);

char *hp_arena_strdup(hp_Arena *a, const char *s);
char *hp_arena_sprintf(hp_Arena *a, const char *fmt, ...);

/** Query the usage*/
void hp_arena_usage(hp_Arena *, size_t *size, size_t *capacity);

#endif //HELPERS_H_



#ifdef HELPERS_IMPLEMENTATION
#undef HELPERS_IMPLEMENTATION



// Helpers
char *hp_shift_args(int *argc, char ***argv){
    assert(*argc >= 0);
    char *res = **argv;
    *argc-=1;
    *argv+=1; 
    return res;
}

// SB

void hp_sb_append_sstr(hp_StringBuilder *sb, const char *str, size_t count){
    hp_da_append_array(sb, str, count);
};

void hp_sb_append_cstrs_impl(hp_StringBuilder *sb, ...)
{
    va_list args;
    va_start(args, sb);
    const char *str;
    
    while((str = va_arg(args, const char *)) != NULL){
        size_t count = strlen(str);
        hp_da_append_array(sb, str, count);
    }

    va_end(args);
}

//TODO: deprecate this in favor of da_reserve
void hp_sb_reserve(hp_StringBuilder *sb, size_t n){
    if(sb->count + n + 1 > sb->capacity){
        sb->capacity = (sb->count + n + 1) * 2;
        sb->items = realloc(sb->items, sb->capacity);
        assert(sb->items != NULL); 
    }
}

bool hp_read_file_into_sb(hp_StringBuilder *sb, const char *filepath)
{
    FILE *f = fopen(filepath, "r");

    if(f == NULL){
        fprintf(stderr, "Could not open file %s: %s\n", filepath, strerror(errno));
        return false;
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if(n <= 0){
        fprintf(stderr, "Could not tell file size on %s: %s\n", filepath, strerror(errno));
        return false;
    }

    //Managing the state of sb manually here
    hp_sb_reserve(sb, n);
    size_t m = fread(sb->items + sb->count, 1, n, f);
    while(m < (size_t) n){
        m += fread(sb->items + sb->count, 1, n - m, f);
    }
    sb->count += n;
    assert(sb->count < sb->capacity);
    
    fclose(f);
    return true;
}

bool hp_write_file_from_sb(const hp_StringBuilder *sb, const char *filepath, const char *mode){
    (void) sb;
    (void) filepath;
    (void) mode;
    if(!mode){
        mode = "wb+";
    }

    FILE *f = fopen(filepath, mode);

    if(f == NULL){
        fprintf(stderr, "Could not open file %s: %s\n", filepath, strerror(errno));
        return false;
    }

    size_t m = fwrite(sb->items, 1, sb->count, f); 
    while(m < sb->count){
        m += fwrite(sb->items + m, 1, sb->count - m, f);
    }
    
    fclose(f);
    return true;

    UNIMPLEMENTED;
}

// SV

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

hp_StringView hp_sv_from_cstr(const char *src)
{
    return (hp_StringView) {
        .data = src,
        .count = strlen(src),
    };
}

hp_StringView hp_sv_from_parts(const char *data, size_t count)
{
    return (hp_StringView) {
        .data = data,
        .count = count,
    };
}

hp_StringView hp_sv_clone(hp_StringView sv)
{
    return sv;
}

void hp_sv_trim_left(hp_StringView *sv)
{
    hp_sv_take_while(sv, _isspace);
}

void hp_sv_trim_right(hp_StringView *sv)
{
    size_t i = sv->count;
    for(; i > 0; --i){
        if(isspace(sv->data[i-1])) sv->count--;
    }
}

hp_StringView hp_sv_take(hp_StringView *sv, size_t n)
{
    n = CLAMP(n, 0, sv->count);
    hp_StringView res = hp_sv_from_parts(sv->data, n);  
    sv->data += n;
    sv->count -= n;
    return res; 
}

hp_StringView hp_sv_take_while(hp_StringView *sv, bool(*pred)(char))
{
    size_t i = 0;
    for(; i < sv->count; ++i){
        if(!pred(sv->data[i])) break;
    }
    
    hp_StringView res = hp_sv_from_parts(sv->data, i);
    sv->data += i;
    sv->count -= i;
    return res;
}

hp_StringView hp_sv_take_until(hp_StringView *sv, bool(*pred)(char))
{
    size_t i = 0;
    for(; i < sv->count; ++i){
        if(pred(sv->data[i])) break;
    }
    
    hp_StringView res = hp_sv_from_parts(sv->data, i);
    sv->data += i;
    sv->count -= i;
    return res;
}

hp_StringView hp_sv_take_until_char(hp_StringView *sv, char c)
{
    size_t i = 0;
    for(; i < sv->count; ++i){
        if(sv->data[i] == c) break;
    }
    
    hp_StringView res = hp_sv_from_parts(sv->data, i);
    sv->data += i;
    sv->count -= i;
    assert(sv->count == 0 || *sv->data == c);
    return res;
}

hp_StringView hp_sv_next_line(hp_StringView *sv)
{
    hp_StringView res = hp_sv_take_until(sv, _isnewline);
    assert(sv->count == 0 || *sv->data == '\n');
    if(sv->count > 0) hp_sv_take(sv, 1);
    return res;
}

hp_StringView hp_sv_substr(hp_StringView sv, size_t start, size_t end)
{
    if(start >= end) return SV_NULL;

    if(start >= sv.count) return SV_NULL;

    hp_StringView res = hp_sv_from_parts(sv.data + start, end - start);
    
    return res;
}

hp_StringView hp_sv_chop_left(hp_StringView *sv){
    return hp_sv_take_until(sv, _isspace);
}

bool hp_sv_eq(const hp_StringView sv1, const hp_StringView sv2)
{
    if(sv1.count != sv2.count) return false;

    for(size_t i = 0; i < sv1.count; ++i){
        if(sv1.data[i] != sv2.data[i]) return false;
    }

    return true;
}

bool hp_sv_starts_with(hp_StringView haystack, hp_StringView neddle) 
{
    if(haystack.count < neddle.count) return false;
    
    return hp_sv_eq(neddle, hp_sv_take(&haystack, neddle.count));
}

bool hp_sv_ends_with(hp_StringView haystack, hp_StringView neddle) 
{
    if(haystack.count < neddle.count) return false;
    
    return hp_sv_eq(neddle, hp_sv_from_parts(haystack.data + haystack.count - neddle.count, neddle.count));
}

bool hp_sv_find(hp_StringView haystack, hp_StringView needle, size_t *fst_pos)
{
    size_t i = 0;

    while(haystack.count > needle.count){
        if(hp_sv_starts_with(haystack, needle)){
            if(fst_pos) *fst_pos = i;
            return true;
        }
        i++;
        hp_sv_take(&haystack, 1);
    }

    if(haystack.count - needle.count == 0){
        if(hp_sv_starts_with(haystack, needle)){
            if(fst_pos) *fst_pos = i;
            return true;
        }
        i++;
        hp_sv_take(&haystack, 1);
    }

    return false;
}

hp_StringView hp_sb_to_sv(const hp_StringBuilder *sb){
    return hp_sv_from_parts(sb->items, sb->count);
}

// Arena Stuff
typedef struct hp_Region hp_Region;
struct hp_Arena {
    hp_Region *start;  // Points to the first region
    hp_Region *end;    // Points to the last region
};

struct hp_Region {
    void *data;
    size_t size;
    size_t capacity;
    hp_Region *next;
};

#define hp_ARENA_REGION_MIN_CAPACITY (4 * 1024)

static size_t aligned_size(size_t n, size_t alignment) 
{
    return (n + alignment - 1) & ~(alignment - 1);
}

static hp_Region *region_create(size_t n)
{
    n = aligned_size(n, sizeof(uint64_t));
    if(n < hp_ARENA_REGION_MIN_CAPACITY) n = hp_ARENA_REGION_MIN_CAPACITY;

    hp_Region *r = (hp_Region*) malloc(sizeof(*r));
    assert(r != NULL);
    r->size = 0;
    r->capacity = n;
    r->data = malloc(n);
    r->next = NULL;
    //memset(r->data, 0, n);
    return r;
}


/**
 * *r: the region where the allocation is destined to happen
 * *p: an outout pointer to the allocated region
 * return signals whether the allocation happened or not
 */
static void *region_alloc(hp_Region *r, size_t n)
{
    if(!r) return NULL;
    n = aligned_size(n, sizeof(uint64_t));
    if(r->size + n > r->capacity) return NULL;
    void * p = (char*)r->data + r->size;
    r->size = r->size + n;
    return p;
}


/**
 * frees all allocated memory that is allocated by the region
 */
static void region_free(hp_Region *r)
{
    if(!r) return;
    free(r->data);
    free(r);
}

/**
 * Simply zeroes out sizes for memory reuse, capacity remains the same, no memory free occurs
 */
static void region_clean(hp_Region *r)
{
    if(r) r->size = 0;
}


void *hp_arena_alloc(hp_Arena *a, size_t n)
{
    if(!a) return NULL;
    n = aligned_size(n, sizeof(uint64_t));

    void *p = NULL;

    //hp_Arena is empty
    if(a->start == NULL){
       assert(a->end == NULL);
       a->start = region_create(n);
       a->end = a->start;
       p = region_alloc(a->start, n);
       assert(p); //The size was newly created to at least acommodate n bytes
       return p;
    } 

    
    
    assert(a->end != NULL);

    //Try in the last region first
    p = region_alloc(a->end, n);
    if(p) return p;

    //Could not alloc in the last region, create a new one at the end
    //This semantic is used because of 'mark' and 'restore'
    a->end->next = region_create(n);
    a->end = a->end->next;

    p = region_alloc(a->end, n);
    assert(p); //The size was newly created to at least acommodate n bytes
    return p;
}


void hp_arena_free(hp_Arena *a)
{
    if(!a) return;
    hp_Region *current = a->start; 

    while(current != NULL){
        hp_Region *next = current->next;
        region_free(current);
        current = next;
    }
    a->start = NULL;
    a->end = NULL;
}


void hp_arena_clean(hp_Arena *a)
{
    if(!a) return;
    hp_Region *current = a->start;

    while(current != NULL){
        region_clean(current);
        current = current->next;
    }
}

void hp_arena_usage(hp_Arena *a, size_t *size, size_t *capacity) 
{
    if(!size && !capacity) return;
    if(size) *size = 0;
    if(capacity) *capacity = 0;

    if(!a) return;
    
    if(a->start == NULL){
       assert(a->end == NULL);
       return;
    }

    hp_Region *r = a->start;
    while(r){
        if(size) *size += r->size;
        if(capacity) *capacity += r->capacity;
        r = r->next;
    }
}

struct hp_Arena_Mark {
    void *region;   
    size_t offset;  
};

hp_Arena_Mark hp_arena_mark(hp_Arena *a)
{
    hp_Arena_Mark m = {0};
    if(!a || !a->end) return m;
    m.region = a->end;
    m.offset = a->end->size;
    return m;
}

/** Restores (rewinds) the hp_arena to a previous mark, discarding later allocations. */
void hp_arena_restore(hp_Arena *a, hp_Arena_Mark m)
{
    if(!a) return;

    if(!m.region){
        hp_arena_free(a);
        return;
    }


    bool found = false;
    for (hp_Region *r = a->start; r; r = r->next) {
        if (r == m.region) { found = true; break; }
    }
    assert(found && "hp_arena_restore: mark does not belong to hp_arena");


    hp_Region *region_mark = (hp_Region *) m.region;
    hp_Region *r = region_mark->next;

    while (r)
    {
        hp_Region *next = r->next;
        region_free(r);
        r = next;
    }

    region_mark->next = NULL;
    a->end = region_mark;
    a->end->size = m.offset;
}

char *hp_arena_strdup(hp_Arena *a, const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = (char *) hp_arena_alloc(a, n);
    memcpy(p, s, n);
    return p;
}

char *hp_arena_sprintf(hp_Arena *a, const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);
    
    va_start(args, fmt);
    char *p = (char *) hp_arena_alloc(a, needed + 1);
    vsnprintf(p, needed+1, fmt, args);
    
    va_end(args);
    return p;
}

#undef CLAMP
#undef MIN
#undef MAX

#endif