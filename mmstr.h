#ifndef MMSTR_H
#define MMSTR_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    void *mem;
    char *data;
    size_t size;
    size_t capacity;
} mmstr_t;

mmstr_t mmstr_new();
mmstr_t mmstr_from(const char *cstr);
mmstr_t mmstr_from_fmt(const char *fmt, ...);
mmstr_t mmstr_duplicate(const mmstr_t *mmstr);

void mmstr_delete(mmstr_t *mmstr);

void mmstr_append(mmstr_t *mmstr, char ch);
void mmstr_append_str(mmstr_t *mmstr, const char *cstr);
void mmstr_append_str_n(mmstr_t *mmstr, const char *cstr, size_t n);
void mmstr_append_fmt(mmstr_t *mmstr, const char *fmt, ...);
void mmstr_trim_left(mmstr_t *mmstr);
void mmstr_trim_right(mmstr_t *mmstr);
void mmstr_trim(mmstr_t *mmstr);

int mmstr_find(const mmstr_t *mmstr, char ch);
bool mmstr_is_empty(const mmstr_t *mmstr);
mmstr_t mmstr_split_once(mmstr_t *src, const char *delim);

#endif

#ifdef MMSTR_IMPLEMENTATION

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static size_t get_required_capacity(size_t size) {
    size_t new_capacity = 8;

    while(new_capacity < size) {
        new_capacity = new_capacity * 2;
    }

    return new_capacity;
}

static void mmstr_expand(mmstr_t *mmstr, size_t new_capacity) {
    if(mmstr->capacity >= new_capacity) 
        return;

    size_t index = (void *)mmstr->data - mmstr->mem; 

    mmstr->mem = realloc(mmstr->mem, new_capacity);
    mmstr->data = mmstr->mem + index;
    mmstr->capacity = new_capacity;

}

static void mmstr_append_va(mmstr_t *mmstr, const char *fmt, va_list *args) {
    va_list args_copy;

    va_copy(args_copy, *args);
    size_t total_len = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    char *fmt_str = malloc(total_len + 1);

    va_copy(args_copy, *args);
    vsnprintf(fmt_str, total_len + 1, fmt, args_copy);
    va_end(args_copy);

    mmstr_append_str(mmstr, fmt_str);
    free(fmt_str);
}

static void _mmstr_chop_left(mmstr_t *mmstr) {
    mmstr->data++;
    mmstr->size--;
}

static void _mmstr_chop_right(mmstr_t *mmstr) {
    mmstr->size--;
}

static void _mmstr_chop_left_n(mmstr_t *mmstr, int n) {
    for(int i = 0; i < n; i++) {
        _mmstr_chop_left(mmstr);
    }
}

static void _mmstr_chop_right_n(mmstr_t *mmstr, int n) {
    for(int i = 0; i < n; i++) {
        _mmstr_chop_right(mmstr);
    }
}

mmstr_t mmstr_new() {
    return (mmstr_t) {0};
}

mmstr_t mmstr_from(const char *cstr) {
    mmstr_t temp = mmstr_new();
    mmstr_append_str(&temp, cstr);
    return temp;
}

mmstr_t mmstr_from_fmt(const char *fmt, ...) {
    mmstr_t temp = mmstr_new();

    va_list args;
    va_start(args, fmt);

    mmstr_append_va(&temp, fmt, &args);

    va_end(args);

    return temp;
}

mmstr_t mmstr_duplicate(const mmstr_t *mmstr) {
    size_t index = (void *)mmstr->data - mmstr->mem;
    size_t capacity = get_required_capacity(mmstr->size);
    void *mem = malloc(capacity);
    memcpy(mem, mmstr->data, mmstr->size);

    return (mmstr_t) {
        .mem = mem,
        .data = mem,
        .size = mmstr->size,
        .capacity = capacity
    };

}

void mmstr_delete(mmstr_t *mmstr) {
    free(mmstr->mem);

    mmstr->mem = NULL;
    mmstr->data = NULL;
    mmstr->size = 0;
    mmstr->capacity = 0;
}

void mmstr_append(mmstr_t *mmstr, char ch) {
    mmstr_expand(mmstr, get_required_capacity(mmstr->size + 1));

    mmstr->data[mmstr->size++] = ch; 
}

void mmstr_append_str(mmstr_t *mmstr, const char *cstr) {
    size_t cstr_len = strlen(cstr);

    for(int i = 0; i < cstr_len; i++) {
        mmstr_append(mmstr, cstr[i]);
    }
}

void mmstr_append_str_n(mmstr_t *mmstr, const char *cstr, size_t n) {
    size_t cstr_len = strlen(cstr);

    if(n > cstr_len) 
        n = cstr_len;

    for(int i = 0; i < n; i++) {
        mmstr_append(mmstr, cstr[i]);
    }
}

void mmstr_append_fmt(mmstr_t *mmstr, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    mmstr_append_va(mmstr, fmt, &args);

    va_end(args);
}

void mmstr_trim_left(mmstr_t *mmstr) {
    while(mmstr->data[0] == ' ') 
        _mmstr_chop_left(mmstr);
}

void mmstr_trim_right(mmstr_t *mmstr) {
    while(mmstr->data[mmstr->size - 1] == ' ') 
        _mmstr_chop_right(mmstr);
}

void mmstr_trim(mmstr_t *mmstr) {
    mmstr_trim_left(mmstr);
    mmstr_trim_right(mmstr);
}

int mmstr_find(const mmstr_t *mmstr, char ch) {
    for(char *current = mmstr->data; current < mmstr->data + mmstr->size; current++) {
        if(*current == ch)
            return current - mmstr->data;
    }

    return -1;
}

bool mmstr_is_empty(const mmstr_t *mmstr) {
    if(mmstr->size == 0) 
        return true;

    return false;
}

mmstr_t mmstr_split_once(mmstr_t *mmstr, const char *delim) {
    int delimiter_size = strlen(delim);
    size_t split_index = -1;
    for(char *current = mmstr->data; current < mmstr->data + mmstr->size; current++) {
        if(!strncmp(current, delim, delimiter_size)) {
            split_index = current - mmstr->data;
            break;
        }
    }

    if(split_index == -1) {
        split_index = mmstr->size;
    }

    mmstr_t out;
    out.mem = NULL;
    out.data = mmstr->data;
    out.size = split_index;
    out.capacity = 0;

    if(split_index + delimiter_size > mmstr->size)
        _mmstr_chop_left_n(mmstr, split_index);
    else
        _mmstr_chop_left_n(mmstr, split_index + delimiter_size);

    return out;
}

#endif