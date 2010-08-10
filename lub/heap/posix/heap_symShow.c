/* 
 * heap_symShow.c - convert addresses to line number and function name
 */   
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../private.h"
#include "../context.h"

#undef lub_heap_init /* defined for cygwin builds */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */


extern void sysheap_suppress_leak_detection(void);
extern void sysheap_restore_leak_detection(void);


#if defined(__CYGWIN__)
/*---------------------------------------------------------
 * under CYGWIN we have the nasty behaviour that
 * argv[0] is the program name without the .exe extension
 * which is needed to load the symbol table; hence this
 * crufty hack *8-)
 *--------------------------------------------------------- */
void
cygwin_lub_heap_init(const char *file_name)
{
    char buffer[80];
    sprintf(buffer,"%s.exe",file_name);
    lub_heap_init(buffer);
}
/*--------------------------------------------------------- */
#endif /* __CYGWIN__ */

#ifndef HAVE_LIBBFD
/*--------------------------------------------------------- */
/*
 * Simple vanilla dump of symbols address
 * This is automatically used unless GPL support has been
 * configured.
 */
void 
lub_heap_symShow(unsigned address)
{
    printf(" 0x%08x",address);
}
/*--------------------------------------------------------- */
bool_t
lub_heap_symMatch(unsigned    address,
                  const char *substring)
{
    return BOOL_TRUE;
}
/*--------------------------------------------------------- */
/*
 * dummy initialisation call
 */
void
lub_heap_init(const char *file_name)
{
    file_name = file_name;
    lub_heap__set_framecount(MAX_BACKTRACE);
}
/*--------------------------------------------------------- */
#else /* HAVE_LIBBFD */
/*
   This file is part of the CLISH project http://clish.sourceforge.net/

   The code in this file is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2
   
   This code is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Derived from addr2line.c in the GNU binutils package by Ulrich.Lauther@mchp.siemens.de 
*/
#include "bfd.h"

/*--------------------------------------------------------- */
/* These global variables are used to pass information between
   translate_addresses and find_address_in_section.  */
/*--------------------------------------------------------- */
typedef struct
{
    bfd_vma      pc;
    asymbol    **syms;
    const char  *file_name;
    const char  *fn_name;
    unsigned int line_num;
    bfd_boolean  found;
} info_t;
/*--------------------------------------------------------- */
typedef struct
{
    bfd      *abfd;
    asymbol **syms;  /* Symbol table.  */
} lub_heap_posix_t;
/*--------------------------------------------------------- */
static lub_heap_posix_t my_instance; /* instance of this class */

/*--------------------------------------------------------- */
/* Look for an address in a section.  This is called via
   bfd_map_over_sections.  */
/*--------------------------------------------------------- */
static void
find_address_in_section (bfd     *abfd,
                         asection *section,
                         void     *data)
{
    info_t       *info = data;
    bfd_vma       vma;
    bfd_size_type size;

    if (info->found)
        return;

    if ((bfd_get_section_flags (abfd, section) & SEC_ALLOC) == 0)
        return;

    vma = bfd_get_section_vma (abfd, section);
    if (info->pc < vma)
        return;

    size = bfd_get_section_size (section);
    if (info->pc >= vma + size)
        return;

    info->found = bfd_find_nearest_line (abfd, 
                                         section, 
                                         info->syms,
                                         info->pc - vma,
                                         &info->file_name, 
                                         &info->fn_name,
                                         &info->line_num);
    if(!info->line_num || !info->file_name)
    {
        info->found = 0;
    }
    if(info->found)
    {
        const char *last_file_name;
        const char *last_fn_name;
        unsigned    last_line_num;
        
        bfd_find_nearest_line (abfd,
                               section, 
                               info->syms,
                               vma + size - 1,
                               &last_file_name, 
                               &last_fn_name,
                               &last_line_num);
        if(   (last_file_name == info->file_name)
           && (last_fn_name   == info->fn_name  )
           && (last_line_num  == info->line_num))
        {
            info->found = FALSE;
        }
    }
    if(info->found)
    {
        /* strip to the basename */
        char *p;

        p = strrchr (info->file_name, '/');
        if (NULL != p)
        {
            info->file_name = p + 1;
        }
    }
}
/*--------------------------------------------------------- */
static void
lub_heap_posix_fini(void)
{
    lub_heap_posix_t *this = &my_instance;
    if (this->syms != NULL)
    {
      free (this->syms);
      this->syms = NULL;
    }

    if(this->abfd)
    {
        bfd_close (this->abfd);
    }
}
/*--------------------------------------------------------- */
void
lub_heap_init(const char *file_name)
{
    lub_heap_posix_t *this = &my_instance;

    /* remember to cleanup */
    atexit(lub_heap_posix_fini);
    
    /* switch on leak detection */
    lub_heap__set_framecount(MAX_BACKTRACE);

    sysheap_suppress_leak_detection();

    bfd_init ();
    
    this->syms = NULL;
    this->abfd = bfd_openr (file_name, "pe-i386");
    if (this->abfd)
    {
        if(bfd_check_format_matches (this->abfd, bfd_object,NULL))
        {
            unsigned int size;
            long         symcount = 0;
            
            if (!(bfd_get_file_flags (this->abfd) & HAS_SYMS) == 0)
            {
                /* Read in the symbol table.  */
                symcount = bfd_read_minisymbols (this->abfd, 
                                                 FALSE, 
                                                 (void*)&this->syms, 
                                                 &size);
                if(0 == symcount)
                {
                    /* try reading dynamic symbols */
                    symcount = bfd_read_minisymbols (this->abfd, 
                                                     TRUE /* dynamic */, 
                                                     (void*)&this->syms,
                                                     &size);
                }
            }
            if(0 == symcount)
            {
                fprintf(stderr,"*** no symbols found in '%s'\n",bfd_get_filename(this->abfd));
            }
        }
    }
    sysheap_restore_leak_detection();
}
/*--------------------------------------------------------- */
static void
_lub_heap_symGetInfo(lub_heap_posix_t *this,
                     unsigned          address,
                     info_t           *info)
{
    info->pc        = address;
    info->syms      = this->syms;
    info->fn_name   = NULL;
    info->file_name = NULL;
    info->line_num  = 0;
    info->found     = FALSE;
    
    sysheap_suppress_leak_detection();
    if(NULL != this->abfd)
    {

        bfd_map_over_sections (this->abfd, 
                               find_address_in_section, 
                               info);
    }
    sysheap_restore_leak_detection();
}
/*--------------------------------------------------------- */
void
lub_heap_symShow(unsigned address)
{
    lub_heap_posix_t *this = &my_instance;
    info_t            info;

    _lub_heap_symGetInfo(this,address,&info);

    if(!info.found)
    {
        if(this->abfd && this->syms)
        {
            /* a line with the function names */
            printf("%26s","");
        }
        printf(" 0x%08x",address);
        if(!this->abfd)
        {
            /* aline with the function names */
            printf(" - lub_heap_init() not called.");
        }
    }
    else
    {
        printf(" %20s:%-4d",info.file_name,info.line_num);
        printf(" %s()",info.fn_name);
    }
}
/*--------------------------------------------------------- */
bool_t
lub_heap_symMatch(unsigned    address,
                  const char *substring)
{
    lub_heap_posix_t *this = &my_instance;
    info_t            info;
    bool_t            result = BOOL_FALSE;
    _lub_heap_symGetInfo(this,address,&info);

    if(info.found)
    {
        if(strstr(info.fn_name,substring))
        {
            result = TRUE;
        }
    }
    return result;
}
/*--------------------------------------------------------- */
#endif /* HAVE_LIBBFD */
