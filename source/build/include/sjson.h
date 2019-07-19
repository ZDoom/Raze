//
// Copyright 2018 Sepehr Taghdisian (septag@github). All rights reserved.
// License: https://github.com/septag/sjson#license-bsd-2-clause
//
// Original code by Joseph A. Adams (joeyadams3.14159@gmail.com)
// 
// sjson.h - v1.1.1 - Fast single header json encoder/decoder
//		This is actually a fork of Joseph's awesome Json encoder/decoder code from his repo:
//		https://github.com/rustyrussell/ccan/tree/master/ccan/json
//		The encoder/decoder code is almost the same. What I did was adding object pools and string pages (sjson_context)
//		that eliminates many micro memory allocations, which improves encode/decode speed and data access performance
//		I also added malloc/free and libc API overrides, and made the library single header
//
//	FEATURES:
//		- Single header C-file
//		- UTF8 support
//		- Fast with minimal allocations (Object pool, String pool, ..)
//		- Overriable malloc/free/memcpy/.. 
//	    - Have both Json Decoder/Encoder
//		- Encoder supports pretify
//		- No dependencies
//		- Simple and easy to use C api
//
//	USAGE:
//	Data types:
//		sjson_tag		Json object tag type, string, number, object, etc..
//		sjson_node		Data structure that represents json DOM node
//		sjson_context	Encoder/decoder context, almost all API functions need a context to allocate and manage memory
//						It's not thread-safe, so don't use the same context in multiple threads					
//	API:
//	--- CREATION
//		sjson_create_context	creates a json context:
//								@PARAM pool_size 		number of initial items in object pool (default=512)
//														this is actually the number of DOM nodes in json and can be grown
//														for large files with lots of elements, you can set this paramter to higher values
//								@PARAM str_buffer_size 	String buffer size in bytes (default=4096)
//														Strings in json decoder, encoder uses a special allocator that allocates string from a pool
//														String pool will be grown from the initial size to store more json string data
//														Depending on the size of the json data, you can set this value higher or lower
//								@PARAM alloc_user		A user-defined pointer that is passed to allocations, in case you override memory alloc functions
//	   sjson_destroy_context	Destroys a json context and frees all memory
//								after the context destroys, all created json nodes will become invalid
//	   
//	   sjson_reset_context		Resets the context. No memory will be freed, just resets the buffers so they can be reused
//								but the DOM nodes and strings will be invalidated next time you parse or decode a json
//
//  --- DECODE/ENCODE/VALIDATE
//	   sjson_decode				Decodes the json text and returns DOM document root node
//	   sjson_encode				Encodes the json root node to json string, does not prettify
//							    Generated string can be freed by calling 'sjson_free_string' on the returned pointer
//	   sjson_stringify			Encodes the json root node to pretty json string
//								@PARAM space sets number of spaces for tab, example two-space for a tab is "  "
//	   sjson_free_string		String pointer returned by stringify and encode should be freed by calling this function		
//	   sjson_validate			Validates the json string, returns false if json string is invalid	
//
// --- LOOKUP/TRAVERSE
//	   sjson_find_element		Finds an element by index inside array 		
//	   sjson_find_member		Finds an element by name inside object				
//	   sjson_find_member_nocase	Finds an element by name inside object, ignores case			
//	   sjson_first_child		Gets first child of Object or Array types		
//	   sjson_foreach			Iterates through an Object or array child elements, first parameter should be a pre-defined json_node*
//
// --- HIGHER LEVEL LOOKUP
//	   sjson_get_int			Gets an integer value from a child of the specified parent node, sets to 'default_val' if node is not found
//	   sjson_get_float			Gets a float value from a child of the specified parent node, sets to 'default_val' if node is not found
//	   sjson_get_double			Gets a double value from a child of the specified parent node, sets to 'default_val' if node is not found
//	   sjson_get_string			Gets a string value from a child of the specified parent node, sets to 'default_val' if node is not found
// 	   sjson_get_bool			Gets a boolean value from a child of the specified parent node, sets to 'default_val' if node is not found
//	   sjson_get_floats			Returns float array from a child of the specified parent node, returns false if node is not found or does not have enough elements to fill the variable
//	   sjson_get_ints			Returns integer array from a child of the specified parent node, returns false if node is not found or does not have enough elements to fill the variable
//
// --- CONSTRUCTION
//	   sjson_mknull				Creates a NULL type json node
//	   sjson_mknumber			Creates and sets a number type json node	
//	   sjson_mkstring			Creates and sets a string type json node	
//	   sjson_mkobject			Creates an object type json node		
//	   sjson_mkarray			Creates an array type json node 	
//	   sjson_mkbool				Creates and sets boolean type json node
//	   sjson_append_element		Appends the node to the end of an array 
//	   sjson_prepend_element	Prepends the node to the beginning of an array
//	   sjson_append_member		Appends the node to the members of an object, should provide the key name to the element
//	   sjson_prepend_member		Prepends the node to the members of an object, should provide the key name to the element
//	   sjson_remove_from_parent	Removes the node from it's parrent, if there is any
//     sjson_delete_node		Deletes the node and all of it's children (object/array)
//
// --- HIGHER LEVEL CONSTRUCTION these functions use a combination of above to facilitate some operations
//     sjson_put_obj            Add an object with a key(name) to the child of the parent node
//     sjson_put_array          Add an array with a key(name) to the child of the parent node
//	   sjson_put_int			Add an integer value with a key(name) to the child of the specified parent node
//	   sjson_put_float			Add float value with a key(name) to the child of the specified parent node
// 	   sjson_put_double			Add double value with a key(name) to the child of the specified parent node
//	   sjson_put_bool			Add boolean value with a key(name) to the child of the specified parent node
//	   sjson_put_string			Add string value with a key(name) to the child of the specified parent node
//	   sjson_put_floats			Add float array with a key(name) to the child of the specified parent node
//	   sjson_put_ints			Add integer array with a key(name) to the child of the specified parent node
//	   sjson_put_string			Add string array with a key(name) to the child of the specified parent node
//     
// --- DEBUG
//	   sjson_check				Checks and validates the json DOM node recursively
//								Returns false and writes a report to 'errmsg' parameter if DOM document has any encoding problems
//
// IMPLEMENTATION:
//	
//	   To include and implement sjson in your project, you should define SJSON_IMPLEMENTATION before including sjson.h
//	   You can also override memory allocation or any libc functions that the library uses to your own
//	   Here's a list of stuff that can be overriden:
//		 ALLOCATIONS
//			- sjson_malloc(user, size)		'user' is the pointer that is passed in sjson_create_context
//			- sjson_free(user, ptr)
//			- sjson_realloc(user, ptr, size)
//		 DEBUG/ASSERT
//			- sjson_assert
//		 STRINGS
//			- sjson_stricmp
//			- sjson_strcpy
//			- sjson_strcmp
//          - sjson_snprintf
//		 MEMORY 
//			- sjson_memcpy
//			- sjson_memset
//			- sjson_out_of_memory			happens when sjson cannot allocate memory internally
//											Default behaviour is that it asserts and exits the program
//	   Example:
//			#define SJSON_IMPLEMENTATION
//			#define sjson_malloc(user, size)			MyMalloc(user, size)
//			#define sjson_free(user, ptr)				MyFree(user, ptr)
//			#define sjson_realloc(user, ptr, size)		MyRealloc(user, ptr, size)
//	   		#include "sjson.h"	
//			...
//
// NOTE: on sjson_reset_context
//			what reset_context does is that is resets the internal buffers without freeing them
//			this makes context re-usable for the next json document, so don't have to regrow buffers or create another context
//	   Example:
//			sjson_context* ctx = sjson_create_context(0, 0, NULL);	// initial creation
//			sjson_decode(ctx, json1);		// decode json1
//			...								// do some work on data
//			sjson_reset_context(ctx);		// reset the buffers, make sure you don't need json1 data
//			sjson_decode(ctx, json2);		// decode another json
//			...
//
#pragma once

#ifndef SJSON_H_
#define SJSON_H_

#ifdef _MSC_VER
#   ifndef __cplusplus

#   define false   0
#   define true    1
#   define bool    _Bool

/* For compilers that don't have the builtin _Bool type. */
#       if ((defined(_MSC_VER) && _MSC_VER < 1800) || \
        (defined __GNUC__&& __STDC_VERSION__ < 199901L && __GNUC__ < 3)) && !defined(_lint)
typedef unsigned char _Bool;
#       endif

#   endif /* !__cplusplus */

#define __bool_true_false_are_defined   1
#else
#   include <stdbool.h>
#endif
#include <stddef.h>

// Json DOM object type
typedef enum sjson_tag {
    SJSON_NULL,
    SJSON_BOOL,
    SJSON_STRING,
    SJSON_NUMBER,
    SJSON_ARRAY,
    SJSON_OBJECT,
} sjson_tag;

// Json DOM node struct
typedef struct sjson_node
{
    struct sjson_node* parent; // only if parent is an object or array (NULL otherwise)
    struct sjson_node* prev;
    struct sjson_node* next;
    
    char* key;                 // only if parent is an object (NULL otherwise). Must be valid UTF-8. 
    
    sjson_tag tag;
    union {
        bool bool_;     // SJSON_BOOL
        char* string_;  // SJSON_STRING: Must be valid UTF-8. 
        double number_; // SJSON_NUMBER
        struct {
            struct sjson_node* head;
            struct sjson_node* tail;
        } children;     // SJSON_ARRAY, SJSON_OBJECT
    };
} sjson_node;

// Json context, handles memory, pools, etc.
// Almost every API needs a valid context to work
// Not multi-threaded. Do not use the same context in multiple threads
typedef struct sjson_context sjson_context;

#ifdef __cplusplus
extern "C" {
#endif

sjson_context* sjson_create_context(int pool_size, int str_buffer_size, void* alloc_user);
void 		   sjson_destroy_context(sjson_context* ctx);
void 		   sjson_reset_context(sjson_context* ctx);

// Encoding, decoding, and validation 
sjson_node* sjson_decode(sjson_context* ctx, const char* json);
char*	    sjson_encode(sjson_context* ctx, const sjson_node* node);
char*	    sjson_encode_string(sjson_context* ctx, const char* str);
char*	    sjson_stringify(sjson_context* ctx, const sjson_node* node, const char* space);
void	    sjson_free_string(sjson_context* ctx, char* str);

bool	    sjson_validate(sjson_context* ctx, const char* json);

// Lookup and traversal
sjson_node* sjson_find_element(sjson_node* array, int index);
sjson_node* sjson_find_member(sjson_node* object, const char* key);
sjson_node* sjson_find_member_nocase(sjson_node* object, const char *name);
sjson_node* sjson_first_child(const sjson_node* node);
int         sjson_child_count(const sjson_node* node);

// Higher level lookup/get functions
int 		sjson_get_int(sjson_node* parent, const char* key, int default_val);
float 		sjson_get_float(sjson_node* parent, const char* key, float default_val);
double 		sjson_get_double(sjson_node* parent, const char* key, double default_val);
const char* sjson_get_string(sjson_node* parent, const char* key, const char* default_val);
bool 		sjson_get_bool(sjson_node* parent, const char* key, bool default_val);
bool		sjson_get_floats(float* out, int count, sjson_node* parent, const char* key);
bool		sjson_get_ints(int* out, int count, sjson_node* parent, const char* key);
bool        sjson_get_uints(uint32_t* out, int count, sjson_node* parent, const char* key);
bool        sjson_get_int16s(int16_t* out, int count, sjson_node* parent, const char* key);
bool        sjson_get_uint16s(uint16_t* out, int count, sjson_node* parent, const char* key);

#define sjson_foreach(i, object_or_array)            \
    for ((i) = sjson_first_child(object_or_array);   \
         (i) != NULL;                                \
         (i) = (i)->next)

// Construction and manipulation 
sjson_node* sjson_mknull(sjson_context* ctx);
sjson_node* sjson_mkbool(sjson_context* ctx, bool b);
sjson_node* sjson_mkstring(sjson_context* ctx, const char *s);
sjson_node* sjson_mknumber(sjson_context* ctx, double n);
sjson_node* sjson_mkarray(sjson_context* ctx);
sjson_node* sjson_mkobject(sjson_context* ctx);

void sjson_append_element(sjson_node* array, sjson_node* element);
void sjson_prepend_element(sjson_node* array, sjson_node* element);
void sjson_append_member(sjson_context* ctx, sjson_node* object, const char* key, sjson_node* value);
void sjson_prepend_member(sjson_context* ctx, sjson_node* object, const char* key, sjson_node* value);

void sjson_remove_from_parent(sjson_node* node);

void sjson_delete_node(sjson_context* ctx, sjson_node* node);

// Higher level construction
sjson_node* sjson_put_obj(sjson_context* ctx, sjson_node* parent, const char* key);
sjson_node* sjson_put_array(sjson_context* ctx, sjson_node* parent, const char* key);
sjson_node* sjson_put_int(sjson_context* ctx, sjson_node* parent, const char* key, int val);
sjson_node* sjson_put_float(sjson_context* ctx, sjson_node* parent, const char* key, float val);
sjson_node* sjson_put_double(sjson_context* ctx, sjson_node* parent, const char* key, double val);
sjson_node* sjson_put_bool(sjson_context* ctx, sjson_node* parent, const char* key, bool val);
sjson_node* sjson_put_string(sjson_context* ctx, sjson_node* parent, const char* key, const char* val);
sjson_node* sjson_put_floats(sjson_context* ctx, sjson_node* parent, const char* key, const float* vals, int count);
sjson_node* sjson_put_ints(sjson_context* ctx, sjson_node* parent, const char* key, const int* vals, int count);
sjson_node* sjson_put_strings(sjson_context* ctx, sjson_node* parent, const char* key, const char** vals, int count);
sjson_node* sjson_put_uints(sjson_context* ctx, sjson_node* parent, const char* key, const uint32_t* vals, int count);
sjson_node* sjson_put_int16s(sjson_context* ctx, sjson_node* parent, const char* key, const int16_t* vals, int count);
sjson_node* sjson_put_uint16s(sjson_context* ctx, sjson_node* parent, const char* key, const uint16_t* vals, int count);

// Debugging

/*
 * Look for structure and encoding problems in a sjson_node or its descendents.
 *
 * If a problem is detected, return false, writing a description of the problem
 * to errmsg (unless errmsg is NULL).
 */
bool sjson_check(const sjson_node* node, char errmsg[256]);

#ifdef __cplusplus
}
#endif

#endif // SJSON_H_

#if defined(SJSON_IMPLEMENT)

/*
  Copyright (C) 2011 Joseph A. Adams (joeyadams3.14159@gmail.com)
  All rights reserved.

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef sjson_strcpy
#   define __STDC_WANT_LIB_EXT1__ 1
#endif

#include <stdint.h>

#ifndef sjson_malloc
#	include <stdlib.h>
#	include <string.h>
#	define sjson_malloc(user, size)		  malloc(size)
#	define sjson_free(user, ptr)		  free(ptr)
#	define sjson_realloc(user, ptr, size) realloc(ptr, size)
#endif

#ifndef sjson_assert
#	include <assert.h>
#	define sjson_assert(_e)				  assert(_e)
#endif

#ifndef sjson_out_of_memory
#	define sjson_out_of_memory()		  do { sjson_assert(0 && "Out of memory"); exit(EXIT_FAILURE); } while(0)
#endif

#ifndef sjson_snprintf
#   include <stdio.h>
#   define sjson_snprintf                snprintf
#endif

#ifndef sjson_strcmp 
#	include <string.h>
#	define sjson_strcmp(_a, _b) 		 strcmp(_a, _b)
#endif

#ifndef sjson_memset
#	include <string.h>
#	define sjson_memset(_p, _b, _n)		 memset(_p, _b, _n)
#endif

#ifndef sjson_memcpy
#	include <string.h>
#	define sjson_memcpy(_a, _b, _n)		 memcpy(_a, _b, _n)
#endif

#ifndef sjson_stricmp
#	ifdef _WIN32
#		include <string.h>
#		define sjson_stricmp(_a, _b) 	 _stricmp(_a, _b)
#	else                           
#		include <string.h>         
#		define sjson_stricmp(_a, _b) 	 strcasecmp(_a, _b)
#	endif	
#endif

#ifndef sjson_strlen
#	include <string.h>
#	define sjson_strlen(_str)			 strlen(_str)
#endif

#ifndef sjson_strcpy
#	include <string.h>
#   ifdef _MSC_VER
#       define sjson_strcpy(_a, _s, _b)      strcpy_s(_a, _s, _b)
#   else
#       define sjson_strcpy(_a, _s, _b)      strcpy(_a, _b) 
#   endif
#endif

#define sjson__align_mask(_value, _mask) ( ( (_value)+(_mask) ) & ( (~0)&(~(_mask) ) ) )

typedef struct sjson__str_page
{
    int					    offset;
    int					    size;
    struct sjson__str_page* next;
    struct sjson__str_page* prev;
} sjson__str_page;

typedef struct sjson__node_page
{
    int 				     iter;
    int 				     capacity;
    sjson_node** 		     ptrs;
    sjson_node* 			 buff;
    struct sjson__node_page* next;
    struct sjson__node_page* prev;
} sjson__node_page;

typedef struct sjson_context 
{
    void*			  alloc_user;
    int				  pool_size;
    int 		      str_buffer_size;
    sjson__node_page* node_pages;	
    sjson__str_page*  str_pages;
    sjson__str_page*  cur_str_page;
    char*			  cur_str;
} sjson_context;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Growable string buffer
typedef struct sjson__sb
{
    char *cur;
    char *end;
    char *start;
} sjson__sb;

static inline void sjson__sb_init(sjson_context* ctx, sjson__sb* sb, int reserve_sz)
{
    int ssize = reserve_sz >= ctx->str_buffer_size ? reserve_sz : ctx->str_buffer_size;
    sb->start = (char*) sjson_malloc(ctx->alloc_user, ssize);
    if (sb->start == NULL)
        sjson_out_of_memory();
    sb->cur = sb->start;
    sb->end = sb->start + ssize - 1;
}

/* sb and need may be evaluated multiple times. */
#define sjson__sb_need(ctx, sb, need) do {       \
        if ((sb)->end - (sb)->cur < (need))     \
            sjson__sb_grow(ctx, sb, need);             \
    } while (0)

static inline void sjson__sb_grow(sjson_context* ctx, sjson__sb* sb, int need)
{
    size_t length = sb->cur - sb->start;
    size_t alloc = sb->end - sb->start + 1;
    
    do {
        alloc <<= 1;
    } while (alloc <= length + need);
    
    sb->start = (char*)sjson_realloc(ctx->alloc_user, sb->start, alloc);
    if (sb->start == NULL) {
        sjson_out_of_memory();
    }
    sb->cur = sb->start + length;
    sb->end = sb->start + alloc - 1;
}

static inline void sjson__sb_put(sjson_context* ctx, sjson__sb* sb, const char *bytes, int count)
{
    sjson__sb_need(ctx, sb, count);
    sjson_memcpy(sb->cur, bytes, count);
    sb->cur += count;
}

#define sjson__sb_putc(ctx, sb, c) do {         \
        if ((sb)->cur >= (sb)->end) \
            sjson__sb_grow(ctx, sb, 1);         \
        *(sb)->cur++ = (c);         \
    } while (0)

static inline void sjson__sb_puts(sjson_context* ctx, sjson__sb* sb, const char *str)
{
    sjson__sb_put(ctx, sb, str, (int)sjson_strlen(str));
}

static inline char* sjson__sb_finish(sjson__sb* sb)
{
    *sb->cur = 0;
    sjson_assert(sb->start <= sb->cur && sjson_strlen(sb->start) == (int)(intptr_t)(sb->cur - sb->start));
    return sb->start;
}

static inline void sjson__sb_free(sjson_context* ctx, sjson__sb* sb)
{
    sjson_free(ctx->alloc_user, sb->start);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Node page
static inline sjson__node_page* sjson__page_create(void* alloc_user, int capacity)
{
    capacity = sjson__align_mask(capacity, 15);
    uint8_t* buff = (uint8_t*)sjson_malloc(alloc_user, 
                sizeof(sjson__node_page) + (sizeof(sjson_node) + sizeof(sjson_node*))*capacity);
    if (!buff) {
        sjson_out_of_memory();
        return NULL;
    }

    sjson__node_page* page = (sjson__node_page*)buff;
    page->iter = capacity;
    page->capacity = capacity;
    page->next = page->prev = NULL;

    buff += sizeof(sjson__node_page);
    page->ptrs = (sjson_node**)buff;
    buff += sizeof(sjson_node*)*capacity;
    page->buff = (sjson_node*)buff;

    for (int i = 0; i < capacity; i++)
        page->ptrs[capacity - i - 1] = &page->buff[i];

    return page;
}

static inline void sjson__page_destroy(sjson__node_page* page, void* alloc_user)
{
    sjson_assert(page);
    page->capacity = page->iter = 0;
    sjson_free(alloc_user, page);
}

static inline sjson_node* sjson__page_new(sjson__node_page* page)
{
    if (page->iter > 0) {
        return page->ptrs[--page->iter];
    } else {
        sjson_assert(0 && "Node page capacity is full");
        return NULL;
    }
}

static inline bool sjson__page_full(const sjson__node_page* page)
{
    return page->iter == 0;
}

static inline bool sjson__page_valid(const sjson__node_page* page, sjson_node* ptr)
{
    uintptr_t uptr = (uintptr_t)ptr;
    bool inbuf = uptr >= (uintptr_t)page->buff && 
                 uptr < (uintptr_t)(page->buff + page->capacity*sizeof(sjson_node));
    bool valid = (uintptr_t)((uint8_t*)ptr - (uint8_t*)page->buff) % sizeof(sjson_node) == 0;
    return inbuf & valid;
}

static inline void sjson__page_del(sjson__node_page* page, sjson_node* ptr)
{
    sjson_assert(page->iter != page->capacity && "Cannot delete more objects");
    sjson_assert(sjson__page_valid(page, ptr) && "Pointer does not belong to page");
    page->ptrs[page->iter++] = ptr;
}

static inline void sjson__page_add_list(sjson__node_page** pfirst, sjson__node_page* node)
{
    if (*pfirst) {
        (*pfirst)->prev = node;
        node->next = *pfirst;
    }
    
    *pfirst = node;
}

static inline void sjson__page_remove_list(sjson__node_page** pfirst, sjson__node_page* node)
{
    if (node->prev)
        node->prev->next = node->next;
    if (node->next)
        node->next->prev = node->prev;
    if (*pfirst == node)
        *pfirst = node->next;
    node->prev = node->next = NULL;	
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// json string buffer page
static inline sjson__str_page* sjson__str_page_create(void* alloc_user, int size)
{
    size = sjson__align_mask(size, 7);

    // allocate the page and reserve memory for string-buffer after the struct
    sjson__str_page* page = (sjson__str_page*)sjson_malloc(alloc_user, size + sizeof(sjson__str_page));
    if (!page) {
        sjson_out_of_memory();
        return NULL;
    }

    page->offset = 0;
    page->size = size;
    page->next = page->prev = NULL;

    return page;
}

static inline void sjson__str_page_destroy(sjson__str_page* page, void* alloc_user)
{
    sjson_assert(page);
    sjson_free(alloc_user, page);
}

static inline char* sjson__str_page_ptr(sjson__str_page* page) 
{
    char* buff = (char*)(page + 1);
    return buff + page->offset;
}

static inline char* sjson__str_page_startptr(sjson__str_page* page) 
{
    return (char*)(page + 1);
}

static inline bool sjson__str_page_grow(sjson__str_page* page, int size)
{
    if (page->offset + size <= page->size) {
        page->offset += size;
        return true;
    } else {
        return false;
    }
}

static inline void sjson__str_page_add_list(sjson__str_page** pfirst, sjson__str_page* node)
{
    if (*pfirst) {
        (*pfirst)->prev = node;
        node->next = *pfirst;
    }	
    *pfirst = node;
}

static inline void sjson__str_page_remove_list(sjson__str_page** pfirst, sjson__str_page* node)
{
    if (node->prev)
        node->prev->next = node->next;
    if (node->next)
        node->next->prev = node->prev;
    if (*pfirst == node)
        *pfirst = node->next;
    node->prev = node->next = NULL;	
}

sjson_context* sjson_create_context(int pool_size, int str_buffer_size, void* alloc_user)
{
    sjson_context* ctx = (sjson_context*)sjson_malloc(alloc_user, sizeof(sjson_context));
    if (!ctx) {
        sjson_out_of_memory();
        return NULL;
    }
    sjson_memset(ctx, 0x00, sizeof(sjson_context));

    if (pool_size <= 0)
        pool_size = 512;
    if (str_buffer_size <= 0)
        str_buffer_size = 4096;

    ctx->alloc_user = alloc_user;
    ctx->pool_size = pool_size;
    ctx->str_buffer_size = str_buffer_size;

    // Create first pool page
    ctx->node_pages = sjson__page_create(alloc_user, pool_size);
    if (!ctx->node_pages)
        return NULL;

    ctx->str_pages = NULL;
    ctx->cur_str_page = NULL;
    return ctx;
}

void sjson_destroy_context(sjson_context* ctx)
{
    sjson_assert(ctx);

    // destroy node pages
    sjson__node_page* npage = ctx->node_pages;
    while (npage) {
        sjson__node_page* next = npage->next;
        sjson__page_destroy(npage, ctx->alloc_user);
        npage = next;
    }

    sjson__str_page* spage = ctx->str_pages;
    while (spage) {
        sjson__str_page* next = spage->next;
        sjson__str_page_destroy(spage, ctx->alloc_user);
        spage = next;
    }

    sjson_free(ctx->alloc_user, ctx);
}

void sjson_reset_context(sjson_context* ctx)
{
    // 
    for (sjson__node_page* npage = ctx->node_pages; npage; npage = npage->next) {
        int capacity = npage->capacity;
        npage->iter = capacity;
        for (int i = 0; i < capacity; i++)
            npage->ptrs[capacity - i - 1] = &npage->buff[i];
    }

    // 
    for (sjson__str_page* spage = ctx->str_pages; spage; spage = spage->next) {
        spage->offset = 0;
    }

    // TODO: maybe we can reverse the linked-lists to get better cache coherency
}

static inline sjson_node* sjson__new_node(sjson_context* ctx, sjson_tag tag)
{
    sjson__node_page* npage = ctx->node_pages;
    while (npage && sjson__page_full(npage)) 
        npage = npage->next;
    
    if (npage == NULL) {
        npage = sjson__page_create(ctx->alloc_user, ctx->pool_size);
        sjson_assert(npage);
        sjson__page_add_list(&ctx->node_pages, npage);
    }

    sjson_node* node = sjson__page_new(npage);
    sjson_memset(node, 0x0, sizeof(sjson_node));
    node->tag = tag;
    return node;
}

static inline void sjson__del_node(sjson_context* ctx, sjson_node* node) 
{
    sjson__node_page* npage = ctx->node_pages;
    while (npage && !sjson__page_valid(npage, node)) 
        npage = npage->next;
    
    sjson_assert(npage && "sjson_node doesn't belong to any page - check the pointer");
    sjson__page_del(npage, node);
}

// Usage:
static inline char* sjson__str_begin(sjson_context* ctx, int init_sz)
{
    sjson_assert (ctx->cur_str_page == NULL && "Should call sjson__str_end before begin");

    // find a page that can grow to requested size
    sjson__str_page* spage = ctx->str_pages;
    while (spage && (spage->offset + init_sz) <= spage->size)
        spage = spage->next;

    // create a new string page
    if (spage == NULL) {
        int page_sz = ctx->str_buffer_size;
        if (init_sz > (page_sz >> 1)) {
            page_sz = page_sz << 1;
            page_sz = sjson__align_mask(init_sz, page_sz-1);
        }
        spage = sjson__str_page_create(ctx->alloc_user, page_sz);
        sjson_assert(spage);
        sjson__str_page_add_list(&ctx->str_pages, spage);
    }

    sjson_assert(spage->offset + init_sz <= spage->size);
    char* ptr = sjson__str_page_ptr(spage);
    spage->offset += init_sz;

    ctx->cur_str_page = spage;
    ctx->cur_str = ptr;

    return ptr;
}

// Returns the pointer to the begining of the string, pointer may be reallocated and changed from sjson__str_begin
static inline char* sjson__str_grow(sjson_context* ctx, int grow_sz)
{
    sjson_assert (ctx->cur_str_page && "Should call sjson__str_begin before grow");

    sjson__str_page* spage = ctx->cur_str_page;
    if (sjson__str_page_grow(spage, grow_sz)) {
        return ctx->cur_str;
    } else {
        int page_sz = spage->size;
        int cur_str_sz = (int)(uintptr_t)(sjson__str_page_ptr(spage) - ctx->cur_str);
        int total_sz = cur_str_sz + grow_sz;
        if (total_sz > (page_sz >> 1)) {
            page_sz <<= 1;
            page_sz = total_sz > page_sz ? total_sz : page_sz;
        }

        sjson__str_page* newspage = sjson__str_page_create(ctx->alloc_user, page_sz);
        sjson_assert(newspage);
        sjson_assert(total_sz <= newspage->size);
        char* ptr = sjson__str_page_startptr(newspage);
        newspage->offset += total_sz;

        // copy previous buffer into the new one
        if (cur_str_sz > 0)
            sjson_memcpy(ptr, ctx->cur_str, cur_str_sz);

        // check and see if we can remove the old page completely
        if (ctx->cur_str == sjson__str_page_startptr(spage)) {
            sjson__str_page_remove_list(&ctx->str_pages, spage);
            sjson__str_page_destroy(spage, ctx->alloc_user);
        }
            
        ctx->cur_str_page = newspage;
        ctx->cur_str = ptr;
        return ptr;
    }
}

static inline char* sjson__str_end(sjson_context* ctx)
{
    sjson_assert (ctx->cur_str_page && "Should call sjson__str_begin before end");
    char* ptr = ctx->cur_str;
    ctx->cur_str_page = NULL;
    ctx->cur_str = NULL;
    return ptr;
}

static char* sjson__strdup(sjson_context* ctx, const char *str)
{
    int len = (int)sjson_strlen(str);
    char* ret = sjson__str_begin(ctx, len + 1);
    sjson_memcpy(ret, str, len);
    ret[len] = '\0';
    sjson__str_end(ctx);
    return ret;
}

/*
 * Unicode helper functions
 *
 * These are taken from the ccan/charset module and customized a bit.
 * Putting them here means the compiler can (choose to) inline them,
 * and it keeps ccan/json from having a dependency.
 */

/*
 * Type for Unicode codepoints.
 * We need our own because wchar_t might be 16 bits.
 */
typedef uint32_t json__uchar_t;

/*
 * Validate a single UTF-8 character starting at @s.
 * The string must be null-terminated.
 *
 * If it's valid, return its length (1 thru 4).
 * If it's invalid or clipped, return 0.
 *
 * This function implements the syntax given in RFC3629, which is
 * the same as that given in The Unicode Standard, Version 6.0.
 *
 * It has the following properties:
 *
 *  * All codepoints U+0000..U+10FFFF may be encoded,
 *    except for U+D800..U+DFFF, which are reserved
 *    for UTF-16 surrogate pair encoding.
 *  * UTF-8 byte sequences longer than 4 bytes are not permitted,
 *    as they exceed the range of Unicode.
 *  * The sixty-six Unicode "non-characters" are permitted
 *    (namely, U+FDD0..U+FDEF, U+xxFFFE, and U+xxFFFF).
 */
static int sjson__utf8_validate_cz(const char *s)
{
    unsigned char c = *s++;
    
    if (c <= 0x7F) {        /* 00..7F */
        return 1;
    } else if (c <= 0xC1) { /* 80..C1 */
        /* Disallow overlong 2-byte sequence. */
        return 0;
    } else if (c <= 0xDF) { /* C2..DF */
        /* Make sure subsequent byte is in the range 0x80..0xBF. */
        if (((unsigned char)*s++ & 0xC0) != 0x80)
            return 0;
        
        return 2;
    } else if (c <= 0xEF) { /* E0..EF */
        /* Disallow overlong 3-byte sequence. */
        if (c == 0xE0 && (unsigned char)*s < 0xA0)
            return 0;
        
        /* Disallow U+D800..U+DFFF. */
        if (c == 0xED && (unsigned char)*s > 0x9F)
            return 0;
        
        /* Make sure subsequent bytes are in the range 0x80..0xBF. */
        if (((unsigned char)*s++ & 0xC0) != 0x80)
            return 0;
        if (((unsigned char)*s++ & 0xC0) != 0x80)
            return 0;
        
        return 3;
    } else if (c <= 0xF4) { /* F0..F4 */
        /* Disallow overlong 4-byte sequence. */
        if (c == 0xF0 && (unsigned char)*s < 0x90)
            return 0;
        
        /* Disallow codepoints beyond U+10FFFF. */
        if (c == 0xF4 && (unsigned char)*s > 0x8F)
            return 0;
        
        /* Make sure subsequent bytes are in the range 0x80..0xBF. */
        if (((unsigned char)*s++ & 0xC0) != 0x80)
            return 0;
        if (((unsigned char)*s++ & 0xC0) != 0x80)
            return 0;
        if (((unsigned char)*s++ & 0xC0) != 0x80)
            return 0;
        
        return 4;
    } else {                /* F5..FF */
        return 0;
    }
}

/* Validate a null-terminated UTF-8 string. */
static bool sjson__utf8_validate(const char *s)
{
    int len;
    
    for (; *s != 0; s += len) {
        len = sjson__utf8_validate_cz(s);
        if (len == 0)
            return false;
    }
    
    return true;
}

/*
 * Read a single UTF-8 character starting at @s,
 * returning the length, in bytes, of the character read.
 *
 * This function assumes input is valid UTF-8,
 * and that there are enough characters in front of @s.
 */
static int sjson__utf8_read_char(const char *s, json__uchar_t *out)
{
    const unsigned char *c = (const unsigned char*) s;
    
    sjson_assert(sjson__utf8_validate_cz(s));

    if (c[0] <= 0x7F) {
        /* 00..7F */
        *out = c[0];
        return 1;
    } else if (c[0] <= 0xDF) {
        /* C2..DF (unless input is invalid) */
        *out = ((json__uchar_t)c[0] & 0x1F) << 6 |
               ((json__uchar_t)c[1] & 0x3F);
        return 2;
    } else if (c[0] <= 0xEF) {
        /* E0..EF */
        *out = ((json__uchar_t)c[0] &  0xF) << 12 |
               ((json__uchar_t)c[1] & 0x3F) << 6  |
               ((json__uchar_t)c[2] & 0x3F);
        return 3;
    } else {
        /* F0..F4 (unless input is invalid) */
        *out = ((json__uchar_t)c[0] &  0x7) << 18 |
               ((json__uchar_t)c[1] & 0x3F) << 12 |
               ((json__uchar_t)c[2] & 0x3F) << 6  |
               ((json__uchar_t)c[3] & 0x3F);
        return 4;
    }
}

/*
 * Write a single UTF-8 character to @s,
 * returning the length, in bytes, of the character written.
 *
 * @unicode must be U+0000..U+10FFFF, but not U+D800..U+DFFF.
 *
 * This function will write up to 4 bytes to @out.
 */
static int sjson__utf8_write_char(json__uchar_t unicode, char *out)
{
    unsigned char *o = (unsigned char*) out;
    
    sjson_assert(unicode <= 0x10FFFF && !(unicode >= 0xD800 && unicode <= 0xDFFF));

    if (unicode <= 0x7F) {
        /* U+0000..U+007F */
        *o++ = unicode;
        return 1;
    } else if (unicode <= 0x7FF) {
        /* U+0080..U+07FF */
        *o++ = 0xC0 | unicode >> 6;
        *o++ = 0x80 | (unicode & 0x3F);
        return 2;
    } else if (unicode <= 0xFFFF) {
        /* U+0800..U+FFFF */
        *o++ = 0xE0 | unicode >> 12;
        *o++ = 0x80 | (unicode >> 6 & 0x3F);
        *o++ = 0x80 | (unicode & 0x3F);
        return 3;
    } else {
        /* U+10000..U+10FFFF */
        *o++ = 0xF0 | unicode >> 18;
        *o++ = 0x80 | (unicode >> 12 & 0x3F);
        *o++ = 0x80 | (unicode >> 6 & 0x3F);
        *o++ = 0x80 | (unicode & 0x3F);
        return 4;
    }
}

/*
 * Compute the Unicode codepoint of a UTF-16 surrogate pair.
 *
 * @uc should be 0xD800..0xDBFF, and @lc should be 0xDC00..0xDFFF.
 * If they aren't, this function returns false.
 */
static bool sjson__from_surrogate_pair(uint16_t uc, uint16_t lc, json__uchar_t *unicode)
{
    if (uc >= 0xD800 && uc <= 0xDBFF && lc >= 0xDC00 && lc <= 0xDFFF) {
        *unicode = 0x10000 + ((((json__uchar_t)uc & 0x3FF) << 10) | (lc & 0x3FF));
        return true;
    } else {
        return false;
    }
}

/*
 * Construct a UTF-16 surrogate pair given a Unicode codepoint.
 *
 * @unicode must be U+10000..U+10FFFF.
 */
static void sjson__to_surrogate_pair(json__uchar_t unicode, uint16_t *uc, uint16_t *lc)
{
    json__uchar_t n;
    
    sjson_assert(unicode >= 0x10000 && unicode <= 0x10FFFF);
    
    n = unicode - 0x10000;
    *uc = ((n >> 10) & 0x3FF) | 0xD800;
    *lc = (n & 0x3FF) | 0xDC00;
}

#define sjson__is_space(c) ((c) == '\t' || (c) == '\n' || (c) == '\r' || (c) == ' ')
#define sjson__is_digit(c) ((c) >= '0' && (c) <= '9')

static bool sjson__parse_value(sjson_context* ctx, const char **sp, sjson_node **out);
static bool sjson__parse_string(sjson_context* ctx, const char **sp, char **out);
static bool sjson__parse_number(const char **sp, double *out);
static bool sjson__parse_array(sjson_context* ctx, const char **sp, sjson_node **out);
static bool sjson__parse_object(sjson_context* ctx, const char **sp, sjson_node **out);
static bool sjson__parse_hex16(const char **sp, uint16_t         *out);

static bool sjson__expect_literal(const char **sp, const char *str);
static void sjson__skip_space(const char **sp);

static void sjson__emit_value(sjson_context* ctx, sjson__sb* out, const sjson_node *node);
static void sjson__emit_value_indented(sjson_context* ctx, sjson__sb* out, const sjson_node *node, const char *space, int indent_level);
static void sjson__emit_string(sjson_context* ctx, sjson__sb* out, const char *str);
static void sjson__emit_number(sjson_context* ctx, sjson__sb* out, double num);
static void sjson__emit_array(sjson_context* ctx, sjson__sb* out, const sjson_node* array);
static void sjson__emit_array_indented(sjson_context* ctx, sjson__sb* out, const sjson_node* array, const char *space, int indent_level);
static void sjson__emit_object(sjson_context* ctx, sjson__sb* out, const sjson_node *object);
static void sjson__emit_object_indented(sjson_context* ctx, sjson__sb* out, const sjson_node *object, const char *space, int indent_level);

static int sjson__write_hex16(char *out, uint16_t val);

static void sjson__append_node(sjson_node *parent, sjson_node *child);
static void sjson__prepend_node(sjson_node *parent, sjson_node *child);
static void sjson__append_member(sjson_node *object, char *key, sjson_node *value);

/* Assertion-friendly validity checks */
static bool sjson__tag_is_valid(unsigned int tag);
static bool sjson__number_is_valid(const char *num);

sjson_node *sjson_decode(sjson_context* ctx, const char *json)
{
    const char *s = json;
    sjson_node *ret;
    
    sjson__skip_space(&s);
    if (!sjson__parse_value(ctx, &s, &ret))
        return NULL;
    
    sjson__skip_space(&s);
    if (*s != 0) {
        sjson_delete_node(ctx, ret);
        return NULL;
    }
    
    return ret;
}

char* sjson_encode(sjson_context* ctx, const sjson_node *node)
{
    return sjson_stringify(ctx, node, NULL);
}

char* sjson_encode_string(sjson_context* ctx, const char *str)
{
    sjson__sb sb;
    sjson__sb_init(ctx, &sb, ctx->str_buffer_size);
    
    sjson__emit_string(ctx, &sb, str);
    
    return sjson__sb_finish(&sb);
}

char* sjson_stringify(sjson_context* ctx, const sjson_node *node, const char *space)
{
    sjson__sb sb;
    sjson__sb_init(ctx, &sb, ctx->str_buffer_size);
    
    if (space != NULL)
        sjson__emit_value_indented(ctx, &sb, node, space, 0);
    else
        sjson__emit_value(ctx, &sb, node);
    
    return sjson__sb_finish(&sb);
}

void sjson_free_string(sjson_context* ctx, char* str)
{
    sjson_free(ctx->alloc_user, str);
}

void sjson_delete_node(sjson_context* ctx, sjson_node *node)
{
    if (node != NULL) {
        sjson_remove_from_parent(node);
        
        switch (node->tag) {
            case SJSON_ARRAY:
            case SJSON_OBJECT:
            {
                sjson_node *child, *next;
                for (child = node->children.head; child != NULL; child = next) {
                    next = child->next;
                    sjson_delete_node(ctx, child);
                }
                break;
            }
            default:;
        }
        
        sjson__del_node(ctx, node);
    }
}

bool sjson_validate(sjson_context* ctx, const char *json)
{
    const char *s = json;
    
    sjson__skip_space(&s);
    if (!sjson__parse_value(ctx, &s, NULL))
        return false;
    
    sjson__skip_space(&s);
    if (*s != 0)
        return false;
    
    return true;
}

sjson_node *sjson_find_element(sjson_node *array, int index)
{
    sjson_node *element;
    int i = 0;
    
    if (array == NULL || array->tag != SJSON_ARRAY)
        return NULL;
    
    sjson_foreach(element, array) {
        if (i == index)
            return element;
        i++;
    }
    
    return NULL;
}

sjson_node *sjson_find_member(sjson_node *object, const char *name)
{
    sjson_node *member;
    
    if (object == NULL || object->tag != SJSON_OBJECT)
        return NULL;
    
    sjson_foreach(member, object)
        if (sjson_strcmp(member->key, name) == 0)
            return member;
    
    return NULL;
}

sjson_node* sjson_find_member_nocase(sjson_node* object, const char *name)
{
    sjson_node *member;
    
    if (object == NULL || object->tag != SJSON_OBJECT)
        return NULL;
    
    sjson_foreach(member, object)
        if (sjson_stricmp(member->key, name) == 0)
            return member;
    
    return NULL;	
}

sjson_node *sjson_first_child(const sjson_node *node)
{
    if (node != NULL && (node->tag == SJSON_ARRAY || node->tag == SJSON_OBJECT))
        return node->children.head;
    return NULL;
}

int sjson_child_count(const sjson_node* node)
{
    int count = 0;
    sjson_node* item;
    sjson_foreach(item, node) ++count;
    return count;
}


int sjson_get_int(sjson_node* parent, const char* key, int default_val)
{
    sjson_node* p = sjson_find_member(parent, key);
    if (p) {
        sjson_assert(p->tag == SJSON_NUMBER);
        return (int)p->number_;
    } else {
        return default_val;
    }
}

float sjson_get_float(sjson_node* parent, const char* key, float default_val)
{
    sjson_node* p = sjson_find_member(parent, key);
    if (p) {
        sjson_assert(p->tag == SJSON_NUMBER);
        return (float)p->number_;
    } else {
        return default_val;
    }	
}

double sjson_get_double(sjson_node* parent, const char* key, double default_val)
{
    sjson_node* p = sjson_find_member(parent, key);
    if (p) {
        sjson_assert(p->tag == SJSON_NUMBER);
        return p->number_;
    } else {
        return default_val;
    }
}

const char* sjson_get_string(sjson_node* parent, const char* key, const char* default_val)
{
    sjson_node* p = sjson_find_member(parent, key);
    if (p) {
        sjson_assert(p->tag == SJSON_STRING);
        return p->string_;
    } else {
        return default_val;
    }
}

bool sjson_get_bool(sjson_node* parent, const char* key, bool default_val)
{
    sjson_node* p = sjson_find_member(parent, key);
    if (p) {
        sjson_assert(p->tag == SJSON_BOOL);
        return p->bool_;
    } else {
        return default_val;
    }
}

bool sjson_get_floats(float* out, int count, sjson_node* parent, const char* key)
{
    sjson_node* p = key ? sjson_find_member(parent, key) : parent;
    if (p) {
        sjson_assert(p->tag == SJSON_ARRAY);
        int index = 0;
        for (sjson_node* elem = p->children.head; elem && index < count; elem = elem->next) {
            sjson_assert(elem->tag == SJSON_NUMBER);
            out[index++] = (float)elem->number_;
        }
        return index == count;
    } else {
        return false;
    }
}

bool sjson_get_ints(int* out, int count, sjson_node* parent, const char* key)
{
    sjson_node* p = key ? sjson_find_member(parent, key) : parent;
    if (p) {
        sjson_assert(p->tag == SJSON_ARRAY);
        int index = 0;
        for (sjson_node* elem = p->children.head; elem && index < count; elem = elem->next) {
            sjson_assert(elem->tag == SJSON_NUMBER);
            out[index++] = (int)elem->number_;
        }
        return index == count;
    } else {
        return false;
    }
}

bool sjson_get_uints(uint32_t* out, int count, sjson_node* parent, const char* key) 
{
    sjson_node* p = key ? sjson_find_member(parent, key) : parent;
    if (p) {
        sjson_assert(p->tag == SJSON_ARRAY);
        int index = 0;
        for (sjson_node* elem = p->children.head; elem && index < count; elem = elem->next) {
            sjson_assert(elem->tag == SJSON_NUMBER);
            out[index++] = (uint32_t)elem->number_;
        }
        return index == count;
    } else {
        return false;
    }    
}

bool sjson_get_int16s(int16_t* out, int count, sjson_node* parent, const char* key) 
{
    sjson_node* p = key ? sjson_find_member(parent, key) : parent;
    if (p) {
        sjson_assert(p->tag == SJSON_ARRAY);
        int index = 0;
        for (sjson_node* elem = p->children.head; elem && index < count; elem = elem->next) {
            sjson_assert(elem->tag == SJSON_NUMBER);
            out[index++] = (int16_t)elem->number_;
        }
        return index == count;
    } else {
        return false;
    }
}

bool sjson_get_uint16s(uint16_t* out, int count, sjson_node* parent, const char* key) 
{
    sjson_node* p = key ? sjson_find_member(parent, key) : parent;
    if (p) {
        sjson_assert(p->tag == SJSON_ARRAY);
        int index = 0;
        for (sjson_node* elem = p->children.head; elem && index < count; elem = elem->next) {
            sjson_assert(elem->tag == SJSON_NUMBER);
            out[index++] = (uint16_t)elem->number_;
        }
        return index == count;
    } else {
        return false;
    }
}

sjson_node *sjson_mknull(sjson_context* ctx)
{
    return sjson__new_node(ctx, SJSON_NULL);
}

sjson_node *sjson_mkbool(sjson_context* ctx, bool b)
{
    sjson_node *ret = sjson__new_node(ctx, SJSON_BOOL);
    ret->bool_ = b;
    return ret;
}

static sjson_node *sjson__mkstring(sjson_context* ctx, char *s)
{
    sjson_node *ret = sjson__new_node(ctx, SJSON_STRING);
    ret->string_ = s;
    return ret;
}

sjson_node *sjson_mkstring(sjson_context* ctx, const char *s)
{
    return sjson__mkstring(ctx, sjson__strdup(ctx, s));
}

sjson_node *sjson_mknumber(sjson_context* ctx, double n)
{
    sjson_node *node = sjson__new_node(ctx, SJSON_NUMBER);
    node->number_ = n;
    return node;
}

sjson_node *sjson_mkarray(sjson_context* ctx)
{
    return sjson__new_node(ctx, SJSON_ARRAY);
}

sjson_node *sjson_mkobject(sjson_context* ctx)
{
    return sjson__new_node(ctx, SJSON_OBJECT);
}

static void sjson__append_node(sjson_node *parent, sjson_node *child)
{
    child->parent = parent;
    child->prev = parent->children.tail;
    child->next = NULL;
    
    if (parent->children.tail != NULL)
        parent->children.tail->next = child;
    else
        parent->children.head = child;
    parent->children.tail = child;
}

static void sjson__prepend_node(sjson_node *parent, sjson_node *child)
{
    child->parent = parent;
    child->prev = NULL;
    child->next = parent->children.head;
    
    if (parent->children.head != NULL)
        parent->children.head->prev = child;
    else
        parent->children.tail = child;
    parent->children.head = child;
}

static void sjson__append_member(sjson_node *object, char *key, sjson_node *value)
{
    value->key = key;
    sjson__append_node(object, value);
}

void sjson_append_element(sjson_node *array, sjson_node *element)
{
    sjson_assert(array->tag == SJSON_ARRAY);
    sjson_assert(element->parent == NULL);
    
    sjson__append_node(array, element);
}

void sjson_prepend_element(sjson_node *array, sjson_node *element)
{
    sjson_assert(array->tag == SJSON_ARRAY);
    sjson_assert(element->parent == NULL);
    
    sjson__prepend_node(array, element);
}

void sjson_append_member(sjson_context* ctx, sjson_node *object, const char *key, sjson_node *value)
{
    sjson_assert(object->tag == SJSON_OBJECT);
    sjson_assert(value->parent == NULL);
    
    sjson__append_member(object, sjson__strdup(ctx, key), value);
}

void sjson_prepend_member(sjson_context* ctx, sjson_node *object, const char *key, sjson_node *value)
{
    sjson_assert(object->tag == SJSON_OBJECT);
    sjson_assert(value->parent == NULL);
    
    value->key = sjson__strdup(ctx, key);
    sjson__prepend_node(object, value);
}

void sjson_remove_from_parent(sjson_node *node)
{
    sjson_node *parent = node->parent;
    
    if (parent != NULL) {
        if (node->prev != NULL)
            node->prev->next = node->next;
        else
            parent->children.head = node->next;
        if (node->next != NULL)
            node->next->prev = node->prev;
        else
            parent->children.tail = node->prev;
        
        node->parent = NULL;
        node->prev = node->next = NULL;
        node->key = NULL;
    }
}

sjson_node* sjson_put_obj(sjson_context* ctx, sjson_node* parent, const char* key)
{
    sjson_node* n = sjson_mkobject(ctx);
    sjson_append_member(ctx, parent, key, n);
    return n;
}

sjson_node* sjson_put_array(sjson_context* ctx, sjson_node* parent, const char* key)
{
    sjson_node* n = sjson_mkarray(ctx);
    sjson_append_member(ctx, parent, key, n);
    return n;    
}

sjson_node* sjson_put_int(sjson_context* ctx, sjson_node* parent, const char* key, int val)
{
    sjson_node* n = sjson_mknumber(ctx, (double)val);
    sjson_assert(n);
    sjson_append_member(ctx, parent, key, n);
    return n;
}

sjson_node* sjson_put_float(sjson_context* ctx, sjson_node* parent, const char* key, float val)
{
    sjson_node* n = sjson_mknumber(ctx, (double)val);
    sjson_assert(n);
    sjson_append_member(ctx, parent, key, n);
    return n;
}

sjson_node* sjson_put_double(sjson_context* ctx, sjson_node* parent, const char* key, double val)
{
    sjson_node* n = sjson_mknumber(ctx, val);
    sjson_assert(n);
    sjson_append_member(ctx, parent, key, n);
    return n;
}

sjson_node* sjson_put_bool(sjson_context* ctx, sjson_node* parent, const char* key, bool val)
{
    sjson_node* n = sjson_mkbool(ctx, val);
    sjson_assert(n);
    sjson_append_member(ctx, parent, key, n);
    return n;
}

sjson_node* sjson_put_string(sjson_context* ctx, sjson_node* parent, const char* key, const char* val)
{
    sjson_node* n = sjson_mkstring(ctx, val);
    sjson_assert(n);
    sjson_append_member(ctx, parent, key, n);
    return n;
}

sjson_node* sjson_put_floats(sjson_context* ctx, sjson_node* parent, const char* key, const float* vals, int count)
{
    sjson_node* a = sjson_mkarray(ctx);
    sjson_assert(a);
    for (int i = 0; i < count; i++) {
        sjson_node* n = sjson_mknumber(ctx, (double)vals[i]);
        sjson_assert(n);
        sjson_append_element(a, n);
    }
    sjson_append_member(ctx, parent, key, a);
    return a;
}

sjson_node* sjson_put_ints(sjson_context* ctx, sjson_node* parent, const char* key, const int* vals, int count)
{
    sjson_node* a = sjson_mkarray(ctx);
    sjson_assert(a);
    for (int i = 0; i < count; i++) {
        sjson_node* n = sjson_mknumber(ctx, (double)vals[i]);
        sjson_assert(n);
        sjson_append_element(a, n);
    }
    sjson_append_member(ctx, parent, key, a);
    return a;
}

sjson_node* sjson_put_uints(sjson_context* ctx, sjson_node* parent, const char* key, const uint32_t* vals, int count)
{
    sjson_node* a = sjson_mkarray(ctx);
    sjson_assert(a);
    for (int i = 0; i < count; i++) {
        sjson_node* n = sjson_mknumber(ctx, (double)vals[i]);
        sjson_assert(n);
        sjson_append_element(a, n);
    }
    sjson_append_member(ctx, parent, key, a);
    return a;
}

sjson_node* sjson_put_int16s(sjson_context* ctx, sjson_node* parent, const char* key, const int16_t* vals, int count)
{
    sjson_node* a = sjson_mkarray(ctx);
    sjson_assert(a);
    for (int i = 0; i < count; i++) {
        sjson_node* n = sjson_mknumber(ctx, (double)vals[i]);
        sjson_assert(n);
        sjson_append_element(a, n);
    }
    sjson_append_member(ctx, parent, key, a);
    return a;
}

sjson_node* sjson_put_uint16s(sjson_context* ctx, sjson_node* parent, const char* key, const uint16_t* vals, int count)
{
    sjson_node* a = sjson_mkarray(ctx);
    sjson_assert(a);
    for (int i = 0; i < count; i++) {
        sjson_node* n = sjson_mknumber(ctx, (double)vals[i]);
        sjson_assert(n);
        sjson_append_element(a, n);
    }
    sjson_append_member(ctx, parent, key, a);
    return a;
}

sjson_node* sjson_put_strings(sjson_context* ctx, sjson_node* parent, const char* key, const char** vals, int count)
{
    sjson_node* a = sjson_mkarray(ctx);
    sjson_assert(a);
    for (int i = 0; i < count; i++) {
        sjson_node* n = sjson_mkstring(ctx, vals[i]);
        sjson_assert(n);
        sjson_append_element(a, n);
    }
    sjson_append_member(ctx, parent, key, a);
    return a;
}

static bool sjson__parse_value(sjson_context* ctx, const char **sp, sjson_node **out)
{
    const char *s = *sp;
    
    switch (*s) {
        case 'n':
            if (sjson__expect_literal(&s, "null")) {
                if (out)
                    *out = sjson_mknull(ctx);
                *sp = s;
                return true;
            }
            return false;
        
        case 'f':
            if (sjson__expect_literal(&s, "false")) {
                if (out)
                    *out = sjson_mkbool(ctx, false);
                *sp = s;
                return true;
            }
            return false;
        
        case 't':
            if (sjson__expect_literal(&s, "true")) {
                if (out)
                    *out = sjson_mkbool(ctx, true);
                *sp = s;
                return true;
            }
            return false;
        
        case '"': {
            char *str;
            if (sjson__parse_string(ctx, &s, out ? &str : NULL)) {
                if (out)
                    *out = sjson__mkstring(ctx, str);
                *sp = s;
                return true;
            }
            return false;
        }
        
        case '[':
            if (sjson__parse_array(ctx, &s, out)) {
                *sp = s;
                return true;
            }
            return false;
        
        case '{':
            if (sjson__parse_object(ctx, &s, out)) {
                *sp = s;
                return true;
            }
            return false;
        
        default: {
            double num;
            if (sjson__parse_number(&s, out ? &num : NULL)) {
                if (out)
                    *out = sjson_mknumber(ctx, num);
                *sp = s;
                return true;
            }
            return false;
        }
    }
}

static bool sjson__parse_array(sjson_context* ctx, const char **sp, sjson_node **out)
{
    const char *s = *sp;
    sjson_node *ret = out ? sjson_mkarray(ctx) : NULL;
    sjson_node *element;
    
    if (*s++ != '[')
        goto failure;
    sjson__skip_space(&s);
    
    if (*s == ']') {
        s++;
        goto success;
    }
    
    for (;;) {
        if (!sjson__parse_value(ctx, &s, out ? &element : NULL))
            goto failure;
        sjson__skip_space(&s);
        
        if (out)
            sjson_append_element(ret, element);
        
        if (*s == ']') {
            s++;
            goto success;
        }
        
        if (*s++ != ',')
            goto failure;
        sjson__skip_space(&s);
    }
    
success:
    *sp = s;
    if (out)
        *out = ret;
    return true;

failure:
    sjson_delete_node(ctx, ret);
    return false;
}

static bool sjson__parse_object(sjson_context* ctx, const char **sp, sjson_node **out)
{
    const char *s = *sp;
    sjson_node *ret = out ? sjson_mkobject(ctx) : NULL;
    char *key;
    sjson_node *value;
    
    if (*s++ != '{')
        goto failure;
    sjson__skip_space(&s);
    
    if (*s == '}') {
        s++;
        goto success;
    }
    
    for (;;) {
        if (!sjson__parse_string(ctx, &s, out ? &key : NULL))
            goto failure;
        sjson__skip_space(&s);
        
        if (*s++ != ':')
            goto failure_free_key;
        sjson__skip_space(&s);
        
        if (!sjson__parse_value(ctx, &s, out ? &value : NULL))
            goto failure_free_key;
        sjson__skip_space(&s);
        
        if (out)
            sjson__append_member(ret, key, value);
        
        if (*s == '}') {
            s++;
            goto success;
        }
        
        if (*s++ != ',')
            goto failure;
        sjson__skip_space(&s);
    }
    
success:
    *sp = s;
    if (out)
        *out = ret;
    return true;

failure_free_key:
failure:
    sjson_delete_node(ctx, ret);
    return false;
}

static bool sjson__parse_string(sjson_context* ctx, const char **sp, char **out)
{
    const char* s = *sp;
    char throwaway_buffer[4]; /* enough space for a UTF-8 character */
    char* b;
    char* src = NULL;

    if (*s++ != '"')
        return false;
    
    if (out) {
        b = sjson__str_begin(ctx, 4);
        src = b;
    } else {
        b = throwaway_buffer;
    }
    
    while (*s != '"') {
        unsigned char c = *s++;
        
        /* Parse next character, and write it to b. */
        if (c == '\\') {
            c = *s++;
            switch (c) {
                case '"':
                case '\\':
                case '/':
                    *b++ = c;
                    break;
                case 'b':
                    *b++ = '\b';
                    break;
                case 'f':
                    *b++ = '\f';
                    break;
                case 'n':
                    *b++ = '\n';
                    break;
                case 'r':
                    *b++ = '\r';
                    break;
                case 't':
                    *b++ = '\t';
                    break;
                case 'u':
                {
                    uint16_t uc, lc;
                    json__uchar_t unicode;
                    
                    if (!sjson__parse_hex16(&s, &uc))
                        goto failed;
                    
                    if (uc >= 0xD800 && uc <= 0xDFFF) {
                        /* Handle UTF-16 surrogate pair. */
                        if (*s++ != '\\' || *s++ != 'u' || !sjson__parse_hex16(&s, &lc))
                            goto failed; /* Incomplete surrogate pair. */
                        if (!sjson__from_surrogate_pair(uc, lc, &unicode))
                            goto failed; /* Invalid surrogate pair. */
                    } else if (uc == 0) {
                        /* Disallow "\u0000". */
                        goto failed;
                    } else {
                        unicode = uc;
                    }
                    
                    b += sjson__utf8_write_char(unicode, b);
                    break;
                }
                default:
                    /* Invalid escape */
                    goto failed;
            }
        } else if (c <= 0x1F) {
            /* Control characters are not allowed in string literals. */
            goto failed;
        } else {
            /* Validate and echo a UTF-8 character. */
            int len;
            
            s--;
            len = sjson__utf8_validate_cz(s);
            if (len == 0)
                goto failed; /* Invalid UTF-8 character. */

            while (len--)
                *b++ = *s++;
        }
        
        /*
         * Update sb to know about the new bytes,
         * and set up b to write another character.
         */
        if (out) {
            int offset = (int)(uintptr_t)(b - src);
            src = sjson__str_grow(ctx, 4);
            b = src + offset;
        } else {
            b = throwaway_buffer;
        }
    }
    s++;
    
    if (out) {
        *b = '\0';
        *out = sjson__str_end(ctx);
    }
    *sp = s;
    return true;

failed:
    if (out)
        sjson__str_end(ctx);
    return false;
}

/*
 * The JSON spec says that a number shall follow this precise pattern
 * (spaces and quotes added for readability):
 *	 '-'? (0 | [1-9][0-9]*) ('.' [0-9]+)? ([Ee] [+-]? [0-9]+)?
 *
 * However, some JSON parsers are more liberal.  For instance, PHP accepts
 * '.5' and '1.'.  JSON.parse accepts '+3'.
 *
 * This function takes the strict approach.
 */
bool sjson__parse_number(const char **sp, double *out)
{
    const char *s = *sp;

    /* '-'? */
    if (*s == '-')
        s++;

    /* (0 | [1-9][0-9]*) */
    if (*s == '0') {
        s++;
    } else {
        if (!sjson__is_digit(*s))
            return false;
        do {
            s++;
        } while (sjson__is_digit(*s));
    }

    /* ('.' [0-9]+)? */
    if (*s == '.') {
        s++;
        if (!sjson__is_digit(*s))
            return false;
        do {
            s++;
        } while (sjson__is_digit(*s));
    }

    /* ([Ee] [+-]? [0-9]+)? */
    if (*s == 'E' || *s == 'e') {
        s++;
        if (*s == '+' || *s == '-')
            s++;
        if (!sjson__is_digit(*s))
            return false;
        do {
            s++;
        } while (sjson__is_digit(*s));
    }

    if (out)
        *out = strtod(*sp, NULL);

    *sp = s;
    return true;
}

static void sjson__skip_space(const char **sp)
{
    const char *s = *sp;
    while (sjson__is_space(*s))
        s++;
    *sp = s;
}

static void sjson__emit_value(sjson_context* ctx, sjson__sb* out, const sjson_node *node)
{
    sjson_assert(sjson__tag_is_valid(node->tag));
    switch (node->tag) {
        case SJSON_NULL:
            sjson__sb_puts(ctx, out, "null");
            break;
        case SJSON_BOOL:
            sjson__sb_puts(ctx, out, node->bool_ ? "true" : "false");
            break;
        case SJSON_STRING:
            sjson__emit_string(ctx, out, node->string_);
            break;
        case SJSON_NUMBER:
            sjson__emit_number(ctx, out, node->number_);
            break;
        case SJSON_ARRAY:
            sjson__emit_array(ctx, out, node);
            break;
        case SJSON_OBJECT:
            sjson__emit_object(ctx, out, node);
            break;
        default:
            sjson_assert(false);
    }
}

void sjson__emit_value_indented(sjson_context* ctx, sjson__sb* out, const sjson_node *node, const char *space, int indent_level)
{
    sjson_assert(sjson__tag_is_valid(node->tag));
    switch (node->tag) {
        case SJSON_BOOL:
            sjson__sb_puts(ctx, out, node->bool_ ? "true" : "false");
            break;
        case SJSON_STRING:
            sjson__emit_string(ctx, out, node->string_);
            break;
        case SJSON_NUMBER:
            sjson__emit_number(ctx, out, node->number_);
            break;
        case SJSON_ARRAY:
            sjson__emit_array_indented(ctx, out, node, space, indent_level);
            break;
        case SJSON_OBJECT:
            sjson__emit_object_indented(ctx, out, node, space, indent_level);
            break;
        case SJSON_NULL:
            sjson__sb_puts(ctx, out, "null");
            break;
        default:
            sjson_assert(false);
    }
}

static void sjson__emit_array(sjson_context* ctx, sjson__sb* out, const sjson_node *array)
{
    const sjson_node *element;
    
    sjson__sb_putc(ctx, out, '[');
    sjson_foreach(element, array) {
        sjson__emit_value(ctx, out, element);
        if (element->next != NULL)
            sjson__sb_putc(ctx, out, ',');
    }
    sjson__sb_putc(ctx, out, ']');
}

static void sjson__emit_array_indented(sjson_context* ctx, sjson__sb* out, const sjson_node *array, const char *space, int indent_level)
{
    const sjson_node *element = array->children.head;
    int i;
    
    if (element == NULL) {
        sjson__sb_puts(ctx, out, "[]");
        return;
    }
    
    sjson__sb_puts(ctx, out, "[\n");
    while (element != NULL) {
        for (i = 0; i < indent_level + 1; i++)
            sjson__sb_puts(ctx, out, space);
        sjson__emit_value_indented(ctx, out, element, space, indent_level + 1);
        
        element = element->next;
        sjson__sb_puts(ctx, out, element != NULL ? ",\n" : "\n");
    }
    for (i = 0; i < indent_level; i++)
        sjson__sb_puts(ctx, out, space);
    sjson__sb_putc(ctx, out, ']');
}

static void sjson__emit_object(sjson_context* ctx, sjson__sb* out, const sjson_node *object)
{
    const sjson_node *member;
    
    sjson__sb_putc(ctx, out, '{');
    sjson_foreach(member, object) {
        sjson__emit_string(ctx, out, member->key);
        sjson__sb_putc(ctx, out, ':');
        sjson__emit_value(ctx, out, member);
        if (member->next != NULL)
            sjson__sb_putc(ctx, out, ',');
    }
    sjson__sb_putc(ctx, out, '}');
}

static void sjson__emit_object_indented(sjson_context* ctx, sjson__sb* out, const sjson_node *object, const char *space, int indent_level)
{
    const sjson_node *member = object->children.head;
    int i;
    
    if (member == NULL) {
        sjson__sb_puts(ctx, out, "{}");
        return;
    }
    
    sjson__sb_puts(ctx, out, "{\n");
    while (member != NULL) {
        for (i = 0; i < indent_level + 1; i++)
            sjson__sb_puts(ctx, out, space);
        sjson__emit_string(ctx, out, member->key);
        sjson__sb_puts(ctx, out, ": ");
        sjson__emit_value_indented(ctx, out, member, space, indent_level + 1);
        
        member = member->next;
        sjson__sb_puts(ctx, out, member != NULL ? ",\n" : "\n");
    }
    for (i = 0; i < indent_level; i++)
        sjson__sb_puts(ctx, out, space);
    sjson__sb_putc(ctx, out, '}');
}

void sjson__emit_string(sjson_context* ctx, sjson__sb* out, const char *str)
{
    bool escape_unicode = false;
    const char *s = str;
    char *b;
    
    sjson_assert(sjson__utf8_validate(str));
    
    /*
     * 14 bytes is enough space to write up to two
     * \uXXXX escapes and two quotation marks.
     */
    sjson__sb_need(ctx, out, 14);
    b = out->cur;
    
    *b++ = '"';
    while (*s != 0) {
        unsigned char c = *s++;
        
        /* Encode the next character, and write it to b. */
        switch (c) {
            case '"':
                *b++ = '\\';
                *b++ = '"';
                break;
            case '\\':
                *b++ = '\\';
                *b++ = '\\';
                break;
            case '\b':
                *b++ = '\\';
                *b++ = 'b';
                break;
            case '\f':
                *b++ = '\\';
                *b++ = 'f';
                break;
            case '\n':
                *b++ = '\\';
                *b++ = 'n';
                break;
            case '\r':
                *b++ = '\\';
                *b++ = 'r';
                break;
            case '\t':
                *b++ = '\\';
                *b++ = 't';
                break;
            default: {
                int len;
                
                s--;
                len = sjson__utf8_validate_cz(s);
                
                if (len == 0) {
                    /*
                     * Handle invalid UTF-8 character gracefully in production
                     * by writing a replacement character (U+FFFD)
                     * and skipping a single byte.
                     *
                     * This should never happen when assertions are enabled
                     * due to the assertion at the beginning of this function.
                     */
                    sjson_assert(false);
                    if (escape_unicode) {
                        sjson_strcpy(b, 7, "\\uFFFD");
                        b += 6;
                    } else {
                        *b++ = (char)0xEF;
                        *b++ = (char)0xBF;
                        *b++ = (char)0xBD;
                    }
                    s++;
                } else if (c < 0x1F || (c >= 0x80 && escape_unicode)) {
                    /* Encode using \u.... */
                    uint32_t unicode;
                    
                    s += sjson__utf8_read_char(s, &unicode);
                    
                    if (unicode <= 0xFFFF) {
                        *b++ = '\\';
                        *b++ = 'u';
                        b += sjson__write_hex16(b, unicode);
                    } else {
                        /* Produce a surrogate pair. */
                        uint16_t uc, lc;
                        sjson_assert(unicode <= 0x10FFFF);
                        sjson__to_surrogate_pair(unicode, &uc, &lc);
                        *b++ = '\\';
                        *b++ = 'u';
                        b += sjson__write_hex16(b, uc);
                        *b++ = '\\';
                        *b++ = 'u';
                        b += sjson__write_hex16(b, lc);
                    }
                } else {
                    /* Write the character directly. */
                    while (len--)
                        *b++ = *s++;
                }
                
                break;
            }
        }
    
        /*
         * Update *out to know about the new bytes,
         * and set up b to write another encoded character.
         */
        out->cur = b;
        sjson__sb_need(ctx, out, 14);
        b = out->cur;
    }
    *b++ = '"';
    
    out->cur = b;
}

static void sjson__emit_number(sjson_context* ctx, sjson__sb* out, double num)
{
    /*
     * This isn't exactly how JavaScript renders numbers,
     * but it should produce valid JSON for reasonable numbers
     * preserve precision well enough, and avoid some oddities
     * like 0.3 -> 0.299999999999999988898 .
     */
    char buf[64];
    sjson_snprintf(buf, sizeof(buf), "%.16g", num);
    
    if (sjson__number_is_valid(buf))
        sjson__sb_puts(ctx, out, buf);
    else
        sjson__sb_puts(ctx, out, "null");
}

static bool sjson__tag_is_valid(unsigned int tag)
{
    return (/* tag >= SJSON_NULL && */ tag <= SJSON_OBJECT);
}

static bool sjson__number_is_valid(const char *num)
{
    return (sjson__parse_number(&num, NULL) && *num == '\0');
}

static bool sjson__expect_literal(const char **sp, const char *str)
{
    const char *s = *sp;
    
    while (*str != '\0')
        if (*s++ != *str++)
            return false;
    
    *sp = s;
    return true;
}

/*
 * Parses exactly 4 hex characters (capital or lowercase).
 * Fails if any input chars are not [0-9A-Fa-f].
 */
static bool sjson__parse_hex16(const char **sp, uint16_t *out)
{
    const char *s = *sp;
    uint16_t ret = 0;
    uint16_t i;
    uint16_t tmp;
    char c;

    for (i = 0; i < 4; i++) {
        c = *s++;
        if (c >= '0' && c <= '9')
            tmp = c - '0';
        else if (c >= 'A' && c <= 'F')
            tmp = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f')
            tmp = c - 'a' + 10;
        else
            return false;

        ret <<= 4;
        ret += tmp;
    }
    
    if (out)
        *out = ret;
    *sp = s;
    return true;
}

/*
 * Encodes a 16-bit number into hexadecimal,
 * writing exactly 4 hex chars.
 */
static int sjson__write_hex16(char *out, uint16_t val)
{
    const char *hex = "0123456789ABCDEF";
    
    *out++ = hex[(val >> 12) & 0xF];
    *out++ = hex[(val >> 8)  & 0xF];
    *out++ = hex[(val >> 4)  & 0xF];
    *out++ = hex[ val        & 0xF];
    
    return 4;
}

bool sjson_check(const sjson_node* node, char errmsg[256])
{
    #define problem(...) do { \
            if (errmsg != NULL) \
                sjson_snprintf(errmsg, 256, __VA_ARGS__); \
            return false; \
        } while (0)
    
    if (node->key != NULL && !sjson__utf8_validate(node->key))
        problem("key contains invalid UTF-8");
    
    if (!sjson__tag_is_valid(node->tag))
        problem("tag is invalid (%u)", node->tag);
    
    if (node->tag == SJSON_BOOL) {
        if (node->bool_ != false && node->bool_ != true)
            problem("bool_ is neither false (%d) nor true (%d)", (int)false, (int)true);
    } else if (node->tag == SJSON_STRING) {
        if (node->string_ == NULL)
            problem("string_ is NULL");
        if (!sjson__utf8_validate(node->string_))
            problem("string_ contains invalid UTF-8");
    } else if (node->tag == SJSON_ARRAY || node->tag == SJSON_OBJECT) {
        sjson_node *head = node->children.head;
        sjson_node *tail = node->children.tail;
        
        if (head == NULL || tail == NULL) {
            if (head != NULL)
                problem("tail is NULL, but head is not");
            if (tail != NULL)
                problem("head is NULL, but tail is not");
        } else {
            sjson_node *child;
            sjson_node *last = NULL;
            
            if (head->prev != NULL)
                problem("First child's prev pointer is not NULL");
            
            for (child = head; child != NULL; last = child, child = child->next) {
                if (child == node)
                    problem("node is its own child");
                if (child->next == child)
                    problem("child->next == child (cycle)");
                if (child->next == head)
                    problem("child->next == head (cycle)");
                
                if (child->parent != node)
                    problem("child does not point back to parent");
                if (child->next != NULL && child->next->prev != child)
                    problem("child->next does not point back to child");
                
                if (node->tag == SJSON_ARRAY && child->key != NULL)
                    problem("Array element's key is not NULL");
                if (node->tag == SJSON_OBJECT && child->key == NULL)
                    problem("Object member's key is NULL");
                
                if (!sjson_check(child, errmsg))
                    return false;
            }
            
            if (last != tail)
                problem("tail does not match pointer found by starting at head and following next links");
        }
    }
    
    return true;
    
    #undef problem
}

#endif  // SJSON_IMPLEMENT

//
// Version History:
//			1.0.0			Initial release
//			1.1.0			Added higher level json get/put functions
//							Implemented sjson_reset_context
//          1.1.1           Fixed some macro defines (sjson_strcpy, sjson_snprintf)
//
