#undef __STRICT_ANSI__ /* we need to use fileno() */
#include <stdlib.h>
# include <unistd.h>
# include <fcntl.h>

#include "private.h"

typedef struct
{
    const char            terminator;
    tinyrl_vt100_escape_t code;
} vt100_decode_t;

/* This table maps the vt100 escape codes to an enumeration */
static vt100_decode_t cmds[] =
{
    {'A',    tinyrl_vt100_CURSOR_UP},
    {'B',    tinyrl_vt100_CURSOR_DOWN},
    {'C',    tinyrl_vt100_CURSOR_RIGHT},
    {'D',    tinyrl_vt100_CURSOR_LEFT},
};
/*--------------------------------------------------------- */
static void
_tinyrl_vt100_setInputNonBlocking(const tinyrl_vt100_t *this)
{
#if defined(STDIN_FILENO)
    int flags = (fcntl(STDIN_FILENO,F_GETFL,0) | O_NONBLOCK);
    fcntl(STDIN_FILENO,F_SETFL,flags);
#endif /* STDIN_FILENO */
}
/*--------------------------------------------------------- */
static void
_tinyrl_vt100_setInputBlocking(const tinyrl_vt100_t *this)
{
#if defined(STDIN_FILENO)
    int flags = (fcntl(STDIN_FILENO,F_GETFL,0) & ~O_NONBLOCK);
    fcntl(STDIN_FILENO,F_SETFL,flags);
#endif /* STDIN_FILENO */
}
/*--------------------------------------------------------- */
tinyrl_vt100_escape_t
tinyrl_vt100_escape_decode(const tinyrl_vt100_t *this)
{
    tinyrl_vt100_escape_t result = tinyrl_vt100_UNKNOWN;
    char                  sequence[10],*p=sequence;
    int                   c;
    unsigned              i;
    /* before the while loop, set the input as non-blocking */
    _tinyrl_vt100_setInputNonBlocking(this); 

    /* dump the control sequence into our sequence buffer 
     * ANSI standard control sequences will end 
     * with a character between 64 - 126
     */
    while(1)
    {
        c = getc(this->istream);

        /* ignore no-character condition */
        if(-1 != c)
        {
            *p++ = (c & 0xFF);
            if( (c != '[') && (c > 63) ) 
            {
                /* this is an ANSI control sequence terminator code */
                result = tinyrl_vt100_CURSOR_UP; /* just a non-UNKNOWN value */
                break;
            }
        } else { 
            result = tinyrl_vt100_UNKNOWN;
            break;
        }
    }
    /* terminate the string (for debug purposes) */
    *p = '\0';
    
    /* restore the blocking status */
    _tinyrl_vt100_setInputBlocking(this);

    if(tinyrl_vt100_UNKNOWN != result)
    {
        /* now decode the sequence */
        for(i = 0;
            i < sizeof(cmds)/sizeof(vt100_decode_t);
            i++)
        {
            if(cmds[i].terminator == c)
            {
                /* found the code in the lookup table */
                result = cmds[i].code;
                break;
            }
        }
    }
    
    return result;
}
/*-------------------------------------------------------- */
int 
tinyrl_vt100_printf(const tinyrl_vt100_t *this,
                    const char           *fmt,
                    ...)
{
    va_list args;
    int len;

    va_start(args, fmt);
    len = tinyrl_vt100_vprintf(this, fmt, args);
    va_end(args);

    return len;
}

/*-------------------------------------------------------- */
int
tinyrl_vt100_vprintf(const tinyrl_vt100_t *this,
                     const char           *fmt,
                     va_list               args)
{
    return vfprintf(this->ostream, fmt, args);
}
/*-------------------------------------------------------- */
int
tinyrl_vt100_getchar(const tinyrl_vt100_t *this)
{
    return getc(this->istream);
}
/*-------------------------------------------------------- */
int
tinyrl_vt100_oflush(const tinyrl_vt100_t *this)
{
    return fflush(this->ostream);
}
/*-------------------------------------------------------- */
int
tinyrl_vt100_ierror(const tinyrl_vt100_t *this)
{
    return ferror(this->istream);
}
/*-------------------------------------------------------- */
int
tinyrl_vt100_oerror(const tinyrl_vt100_t *this)
{
    return ferror(this->ostream);
}
/*-------------------------------------------------------- */
int
tinyrl_vt100_ieof(const tinyrl_vt100_t *this)
{
    return feof(this->istream);
}
/*-------------------------------------------------------- */
int
tinyrl_vt100_eof(const tinyrl_vt100_t *this)
{
    return feof(this->istream);
}
/*-------------------------------------------------------- */
unsigned
tinyrl_vt100__get_width(const tinyrl_vt100_t *this)
{
    this=this;
    /* hard code until we suss out how to do it properly */
    return 80;
}
/*-------------------------------------------------------- */
static void
tinyrl_vt100_init(tinyrl_vt100_t     *this,
                  FILE               *istream,
                  FILE               *ostream)
{
    this->istream  = istream;
    this->ostream = ostream;
}
/*-------------------------------------------------------- */
static void
tinyrl_vt100_fini(tinyrl_vt100_t *this)
{
    /* nothing to do yet... */
    this=this;
}
/*-------------------------------------------------------- */
tinyrl_vt100_t *
tinyrl_vt100_new(FILE               *istream,
                 FILE               *ostream)
{
    tinyrl_vt100_t *this = NULL;
    
    this = malloc(sizeof(tinyrl_vt100_t));
    if(NULL != this)
    {
        tinyrl_vt100_init(this,istream,ostream);
    }
    
    return this;
}
/*-------------------------------------------------------- */
void
tinyrl_vt100_delete(tinyrl_vt100_t *this)
{
    tinyrl_vt100_fini(this);
    /* release the memory */
    free(this);
}
/*-------------------------------------------------------- */
void
tinyrl_vt100_ding(const tinyrl_vt100_t *this)
{
    tinyrl_vt100_printf(this,"%c",KEY_BEL);
    (void)tinyrl_vt100_oflush(this);
}
/*-------------------------------------------------------- */
void
tinyrl_vt100_attribute_reset(const tinyrl_vt100_t *this)
{
        tinyrl_vt100_printf(this,"%c[0m",KEY_ESC);
}
/*-------------------------------------------------------- */
void
tinyrl_vt100_attribute_bright(const tinyrl_vt100_t *this)
{
        tinyrl_vt100_printf(this,"%c[1m",KEY_ESC);
}
/*-------------------------------------------------------- */
void
tinyrl_vt100_attribute_dim(const tinyrl_vt100_t *this)
{
        tinyrl_vt100_printf(this,"%c[2m",KEY_ESC);
}
/*-------------------------------------------------------- */
void
tinyrl_vt100_attribute_underscore(const tinyrl_vt100_t *this)
{
        tinyrl_vt100_printf(this,"%c[4m",KEY_ESC);
}
/*-------------------------------------------------------- */
void
tinyrl_vt100_attribute_blink(const tinyrl_vt100_t *this)
{
        tinyrl_vt100_printf(this,"%c[5m",KEY_ESC);
}
/*-------------------------------------------------------- */
void
tinyrl_vt100_attribute_reverse(const tinyrl_vt100_t *this)
{
        tinyrl_vt100_printf(this,"%c[7m",KEY_ESC);
}
/*-------------------------------------------------------- */
void
tinyrl_vt100_attribute_hidden(const tinyrl_vt100_t *this)
{
        tinyrl_vt100_printf(this,"%c[8m",KEY_ESC);
}
/*-------------------------------------------------------- */
void
tinyrl_vt100_erase_line(const tinyrl_vt100_t *this)
{
        tinyrl_vt100_printf(this,"%c[2K",KEY_ESC);
}
/*-------------------------------------------------------- */
void
tinyrl_vt100_clear_screen(const tinyrl_vt100_t *this)
{
        tinyrl_vt100_printf(this,"%c[2J",KEY_ESC);
}
/*-------------------------------------------------------- */
void
tinyrl_vt100_cursor_save(const tinyrl_vt100_t *this)
{
        tinyrl_vt100_printf(this,"%c7",KEY_ESC);
}
/*-------------------------------------------------------- */
void
tinyrl_vt100_cursor_restore(const tinyrl_vt100_t *this)
{
        tinyrl_vt100_printf(this,"%c8",KEY_ESC);
}
/*-------------------------------------------------------- */
void
tinyrl_vt100_cursor_forward(const tinyrl_vt100_t *this,
                            unsigned              count)
{
        tinyrl_vt100_printf(this,"%c[%dC",KEY_ESC,count);
}
/*-------------------------------------------------------- */
void
tinyrl_vt100_cursor_back(const tinyrl_vt100_t *this,
                         unsigned              count)
{
        tinyrl_vt100_printf(this,"%c[%dD",KEY_ESC,count);
}
/*-------------------------------------------------------- */
void
tinyrl_vt100_cursor_up(const tinyrl_vt100_t *this,
                       unsigned              count)
{
        tinyrl_vt100_printf(this,"%c[%dA",KEY_ESC,count);
}
/*-------------------------------------------------------- */
void
tinyrl_vt100_cursor_down(const tinyrl_vt100_t *this,
                         unsigned              count)
{
        tinyrl_vt100_printf(this,"%c[%dB",KEY_ESC,count);
}
/*-------------------------------------------------------- */
void
tinyrl_vt100_cursor_home(const tinyrl_vt100_t *this)
{
        tinyrl_vt100_printf(this,"%c[H",KEY_ESC);
}
/*-------------------------------------------------------- */
void
tinyrl_vt100_erase(const tinyrl_vt100_t *this,
                   unsigned              count)
{
        tinyrl_vt100_printf(this,"%c[%dP",KEY_ESC,count);
}
/*-------------------------------------------------------- */
void
tinyrl_vt100__set_istream(tinyrl_vt100_t *this,
                          FILE           *istream)
{
        this->istream = istream;
}
/*-------------------------------------------------------- */
FILE *
tinyrl_vt100__get_istream(const tinyrl_vt100_t *this)
{
        return this->istream;
}
/*-------------------------------------------------------- */
FILE *
tinyrl_vt100__get_ostream(const tinyrl_vt100_t *this)
{
        return this->ostream;
}
/*-------------------------------------------------------- */
