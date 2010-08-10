/*
 * tinyrl.c
 */

/* make sure we can get fileno() */
#undef __STRICT_ANSI__

/* LIBC HEADERS */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* POSIX HEADERS */
#include <unistd.h>

#include "lub/string.h"

#include "private.h"

/*----------------------------------------------------------------------- */
static void
tty_set_raw_mode(tinyrl_t *this)
{
    struct termios new_termios;
    int            fd = fileno(tinyrl_vt100__get_istream(this->term));
    int            status;

    status = tcgetattr(fd,&this->default_termios);
    if(-1 != status)
    {
        status = tcgetattr(fd,&new_termios);
        assert(-1 != status);
        new_termios.c_iflag     = 0;
        new_termios.c_oflag     = OPOST | ONLCR;
        new_termios.c_lflag     = 0;
        new_termios.c_cc[VMIN]  = 1;
        new_termios.c_cc[VTIME] = 0;
        /* Do the mode switch */
        status = tcsetattr(fd,TCSAFLUSH,&new_termios); 
        assert(-1 != status);
    }
}
/*----------------------------------------------------------------------- */
static void
tty_restore_mode(const tinyrl_t *this)
{
    int fd = fileno(tinyrl_vt100__get_istream(this->term));

    /* Do the mode switch */
    (void)tcsetattr(fd,TCSAFLUSH,&this->default_termios);  
}
/*----------------------------------------------------------------------- */
/*
This is called whenever a line is edited in any way.
It signals that if we are currently viewing a history line we should transfer it
to the current buffer
*/
static void
changed_line(tinyrl_t *this)
{
    /* if the current line is not our buffer then make it so */
    if(this->line != this->buffer)
    {
        /* replace the current buffer with the new details */
        free(this->buffer);
        this->line = this->buffer = lub_string_dup(this->line);
        this->buffer_size = strlen(this->buffer);
        assert(this->line);
    }
}
/*----------------------------------------------------------------------- */
static bool_t
tinyrl_key_default(tinyrl_t *this,
                   int       key)
{
    bool_t result = BOOL_FALSE;
    if(key > 31)
    {
        char tmp[2];
        tmp[0] = (key & 0xFF),
        tmp[1] = '\0';
        /* inject this text into the buffer */
        result = tinyrl_insert_text(this,tmp);
    }
    else
    {
        char tmp[10];
        sprintf(tmp,"~%d",key);
        /* inject control characters as ~N where N is the ASCII code */
        result = tinyrl_insert_text(this,tmp);
    }
    return result;
}
/*-------------------------------------------------------- */
static bool_t
tinyrl_key_interrupt(tinyrl_t *this,
                     int       key)
{
    tinyrl_delete_text(this,0,this->end);
    this->done = BOOL_TRUE;
    /* keep the compiler happy */
    key = key;
    
    return BOOL_TRUE;
}
/*-------------------------------------------------------- */
static bool_t
tinyrl_key_start_of_line(tinyrl_t *this,
                         int       key)
{
    /* set the insertion point to the start of the line */
    this->point = 0;
    /* keep the compiler happy */
    key = key;
    return BOOL_TRUE;
}
/*-------------------------------------------------------- */
static bool_t
tinyrl_key_end_of_line(tinyrl_t *this,
                       int       key)
{
    /* set the insertion point to the end of the line */
    this->point = this->end;
    /* keep the compiler happy */
    key = key;
    return BOOL_TRUE;
}
/*-------------------------------------------------------- */
static bool_t
tinyrl_key_kill(tinyrl_t *this,
                int       key)
{
    /* release any old kill string */
    lub_string_free(this->kill_string);

    /* store the killed string */
    this->kill_string = lub_string_dup(&this->buffer[this->point]);

    /* delete the text to the end of the line */
    tinyrl_delete_text(this,this->point,this->end);
    /* keep the compiler happy */
    key = key;
    return BOOL_TRUE;
}
/*-------------------------------------------------------- */
static bool_t
tinyrl_key_yank(tinyrl_t *this,
                int       key)
{
    bool_t result = BOOL_FALSE;
    if(this->kill_string)
    {
        /* insert the kill string at the current insertion point */
        result = tinyrl_insert_text(this,this->kill_string);
    }
    /* keep the compiler happy */
    key = key;
    return result;
}
/*-------------------------------------------------------- */
static bool_t
tinyrl_key_crlf(tinyrl_t *this,
                int       key)
{
    tinyrl_crlf(this);
    this->done = BOOL_TRUE;
    /* keep the compiler happy */
    key = key;
    return BOOL_TRUE;
}
/*-------------------------------------------------------- */
static bool_t
tinyrl_key_up(tinyrl_t *this,
              int       key)
{
    bool_t                  result = BOOL_FALSE;
    tinyrl_history_entry_t *entry  = NULL;
    if(this->line == this->buffer)
    {
        /* go to the last history entry */
        entry = tinyrl_history_getlast(this->history,
                                       &this->hist_iter);
    }
    else
    {
        /* already traversing the history list so get previous */
        entry = tinyrl_history_getprevious(&this->hist_iter);
    }
    if(NULL != entry)
    {
        /* display the entry moving the insertion point
         * to the end of the line 
         */
       this->line = tinyrl_history_entry__get_line(entry);
       this->point = this->end = strlen(this->line);
       result = BOOL_TRUE;
    }
    /* keep the compiler happy */
    key = key;
    return result;
}
/*-------------------------------------------------------- */
static bool_t
tinyrl_key_down(tinyrl_t *this,
                int       key)
{
    bool_t result = BOOL_FALSE;
    if(this->line != this->buffer)
    {
        /* we are not already at the bottom */
        /* the iterator will have been set up by the key_up() function */
        tinyrl_history_entry_t *entry = tinyrl_history_getnext(&this->hist_iter);
        if(NULL == entry)
        {
            /* nothing more in the history list */
            this->line = this->buffer;
        }
        else
        {
            this->line = tinyrl_history_entry__get_line(entry);
        }
        /* display the entry moving the insertion point
         * to the end of the line 
         */        
        this->point = this->end = strlen(this->line);
        result = BOOL_TRUE;
    }
    /* keep the compiler happy */
    key = key;
    return result;
}
/*-------------------------------------------------------- */
static bool_t
tinyrl_key_left(tinyrl_t *this,
                int       key)
{
    bool_t result = BOOL_FALSE;
    if(this->point > 0)
    {
        this->point--;
        result = BOOL_TRUE;
    }
    /* keep the compiler happy */
    key = key;
    return result;
}
/*-------------------------------------------------------- */
static bool_t
tinyrl_key_right(tinyrl_t *this,
                 int       key)
{
    bool_t result = BOOL_FALSE;
    if(this->point < this->end)
    {
        this->point++;
        result = BOOL_TRUE;
    }
    /* keep the compiler happy */
    key = key;
    return result;
}
/*-------------------------------------------------------- */
static bool_t
tinyrl_key_backspace(tinyrl_t *this,
                     int       key)
{
    bool_t result = BOOL_FALSE;
    if(this->point)
    {
        this->point--;
        tinyrl_delete_text(this,this->point,this->point);
        result = BOOL_TRUE;
    }
    /* keep the compiler happy */
    key = key;
    return result;
}
/*-------------------------------------------------------- */
static bool_t
tinyrl_key_delete(tinyrl_t *this,
                  int       key)
{   
    bool_t result = BOOL_FALSE;
    if(this->point < this->end)
    {
        tinyrl_delete_text(this,this->point,this->point);
        result = BOOL_TRUE;
    }
    /* keep the compiler happy */
    key = key;
    return result;
}
/*-------------------------------------------------------- */
static bool_t
tinyrl_key_clear_screen(tinyrl_t *this,
                        int       key)
{
    tinyrl_vt100_clear_screen(this->term);
    tinyrl_vt100_cursor_home(this->term);
    tinyrl_reset_line_state(this);
    
    /* keep the compiler happy */
    key = key;
    this = this;
    return BOOL_TRUE;
}
/*-------------------------------------------------------- */
static bool_t 
tinyrl_key_erase_line(tinyrl_t *this, int key)
{
    
    tinyrl_delete_text(this,0,this->point);
    this->point = 0;

    /* keep the compiler happy */
    key = key;
    this = this;

    return BOOL_TRUE;
}/*-------------------------------------------------------- */
static bool_t
tinyrl_key_escape(tinyrl_t *this,
                  int       key)
{
    bool_t result= BOOL_FALSE;
    
    switch(tinyrl_vt100_escape_decode(this->term))
    {
        case tinyrl_vt100_CURSOR_UP:
            result = tinyrl_key_up(this,key);
            break;
        case tinyrl_vt100_CURSOR_DOWN:
            result = tinyrl_key_down(this,key);
            break;
        case tinyrl_vt100_CURSOR_LEFT:
            result = tinyrl_key_left(this,key);
            break;
        case tinyrl_vt100_CURSOR_RIGHT:
            result = tinyrl_key_right(this,key);
            break;
        case tinyrl_vt100_UNKNOWN:
            break;
    }
    return result;
}
/*-------------------------------------------------------- */
static bool_t
tinyrl_key_tab(tinyrl_t *this,
               int       key)
{
    bool_t         result = BOOL_FALSE;
    tinyrl_match_e status = tinyrl_complete_with_extensions(this);
    
    switch(status)
    {
        case TINYRL_COMPLETED_MATCH:
        case TINYRL_MATCH:
        {

            /* everything is OK with the world... */
            result = tinyrl_insert_text(this," ");
            break;
        }
        case TINYRL_NO_MATCH:
        case TINYRL_MATCH_WITH_EXTENSIONS:
        case TINYRL_AMBIGUOUS:
        case TINYRL_COMPLETED_AMBIGUOUS:
        {
            /* oops don't change the result and let the bell ring */
            break;
        }
    }
    /* keep the compiler happy */
    key = key;
    return result;
}
/*-------------------------------------------------------- */
static void
tinyrl_fini(tinyrl_t *this)
{
    /* delete the history session */
    tinyrl_history_delete(this->history);

    /* delete the terminal session */
    tinyrl_vt100_delete(this->term);
    
    /* free up any dynamic strings */
    lub_string_free(this->buffer);
    this->buffer = NULL;
    lub_string_free(this->kill_string);
    this->kill_string = NULL;
    lub_string_free(this->last_buffer);
    this->last_buffer = NULL;
}
/*-------------------------------------------------------- */
static void
tinyrl_init(tinyrl_t                 *this,
            FILE                     *instream,
            FILE                     *outstream,
            unsigned                  stifle,
            tinyrl_completion_func_t *complete_fn)
{
    int i;
    
    for(i = 0;i < NUM_HANDLERS; i++)
    {
        this->handlers[i] = tinyrl_key_default;
    }
    /* default handlers */
    this->handlers[KEY_CR]  = tinyrl_key_crlf;
    this->handlers[KEY_LF]  = tinyrl_key_crlf;
    this->handlers[KEY_ETX] = tinyrl_key_interrupt;
    this->handlers[KEY_DEL] = tinyrl_key_backspace;
    this->handlers[KEY_BS]  = tinyrl_key_backspace;
    this->handlers[KEY_EOT] = tinyrl_key_delete;
    this->handlers[KEY_ESC] = tinyrl_key_escape;
    this->handlers[KEY_FF]  = tinyrl_key_clear_screen;
    this->handlers[KEY_NAK] = tinyrl_key_erase_line;
    this->handlers[KEY_SOH] = tinyrl_key_start_of_line;
    this->handlers[KEY_ENQ] = tinyrl_key_end_of_line;
    this->handlers[KEY_VT]  = tinyrl_key_kill;
    this->handlers[KEY_EM]  = tinyrl_key_yank;
    this->handlers[KEY_HT]  = tinyrl_key_tab;

    this->line                          = NULL;
    this->max_line_length               = 0;
    this->prompt                        = NULL;
    this->prompt_size                   = 0;
    this->buffer                        = NULL;
    this->buffer_size                   = 0;
    this->done                          = BOOL_FALSE;
    this->completion_over               = BOOL_FALSE;
    this->point                         = 0;
    this->end                           = 0;
    this->attempted_completion_function = complete_fn;
    this->state                         = 0;
    this->kill_string                   = NULL;
    this->echo_char                     = '\0';
    this->echo_enabled                  = BOOL_TRUE;
    this->isatty                        = isatty(fileno(instream)) ? BOOL_TRUE : BOOL_FALSE;
    this->last_buffer                   = NULL;
    this->last_point                    = 0;
    
    /* create the vt100 terminal */
    this->term = tinyrl_vt100_new(instream,outstream);

    /* create the history */
    this->history = tinyrl_history_new(stifle);
}
/*-------------------------------------------------------- */
int
tinyrl_printf(const tinyrl_t *this,
              const char     *fmt,
              ...)
{
   va_list args;
    int len;

    va_start(args, fmt);
    len = tinyrl_vt100_vprintf(this->term, fmt, args);
    va_end(args);

    return len;
}
/*-------------------------------------------------------- */
void
tinyrl_delete(tinyrl_t *this)
{
    assert(this);
    if(this)
    {
        /* let the object tidy itself up */
        tinyrl_fini(this);

        /* release the memory associate with this instance */
        free(this);
    }
}
/*-------------------------------------------------------- */

/*#####################################
 * EXPORTED INTERFACE
 *##################################### */
/*----------------------------------------------------------------------- */
int
tinyrl_getchar(const tinyrl_t *this)
{
    return tinyrl_vt100_getchar(this->term);
}
/*----------------------------------------------------------------------- */
static void
tinyrl_internal_print(const tinyrl_t *this,
                      const char     *text)
{
    if(BOOL_TRUE == this->echo_enabled)
    {
        /* simply echo the line */
        tinyrl_vt100_printf(this->term,"%s",text);
    }
    else
    {
        /* replace the line with echo char if defined */
        if(this->echo_char)
        {
            unsigned i = strlen(text);
            while(i--)
            {
                tinyrl_vt100_printf(this->term,"%c",this->echo_char);
            }
        }
    }
}
/*----------------------------------------------------------------------- */
void
tinyrl_redisplay(tinyrl_t *this)
{   
    int          delta;
    unsigned line_len, last_line_len,count;
    line_len      = strlen(this->line);    
    last_line_len = (this->last_buffer ? strlen(this->last_buffer) : 0);
    
    do
    {
        if(this->last_buffer)
        {
            delta = (int)(line_len - last_line_len);
            if(delta > 0)
            {
                count = (unsigned)delta;
                /* is the current line simply an extension of the previous one? */
                if(0 == strncmp(this->line,this->last_buffer,last_line_len))
                {
                    /* output the line accounting for the echo behaviour */
                    tinyrl_internal_print(this,&this->line[line_len - count]);
                    break;
                }
            }
            else if(delta < 0)
            {
                /* is the current line simply a deletion of some characters from the end? */
                if(0 == strncmp(this->line,this->last_buffer,line_len))
                {
                    if(this->echo_enabled || this->echo_char)
                    {
                        int shift = (int)(this->last_point - this->point);
                        /* just get the terminal to delete the characters */
                        if(shift > 0)
                        {
                            count = (unsigned)shift;
                            /* we've moved the cursor backwards */ 
                            tinyrl_vt100_cursor_back(this->term,count);
                        }
                        else if(shift < 0)
                        {
                            count = (unsigned)-shift;
                            /* we've moved the cursor forwards */ 
                            tinyrl_vt100_cursor_forward(this->term,count);
                        }
                        /* now delete the characters */
                        count = (unsigned)-delta;
                        tinyrl_vt100_erase(this->term,count);
                    }
                    break;
                }
            }
            else
            {
                /* are the lines are the same content? */
                if(0 == strcmp(this->line,this->last_buffer))
                {
                    if(this->echo_enabled || this->echo_char)
                    {
                        delta = (int)(this->point - this->last_point);
                        if(delta > 0)
                        {
                            count = (unsigned)delta;
                            /* move the point forwards */
                            tinyrl_vt100_cursor_forward(this->term,count);
                        }
                        else if(delta < 0)
                        {
                            count = (unsigned)-delta;
                            /* move the cursor backwards */
                            tinyrl_vt100_cursor_back(this->term,count);
                        }
                    }
                    /* done for now */
                    break;
                }
            }
        }
        else
        {
            /* simply display the prompt and the line */
            tinyrl_vt100_printf(this->term,"%s",this->prompt);
            tinyrl_internal_print(this,this->line);
            if(this->point < line_len)
            {
                /* move the cursor to the insertion point */
                tinyrl_vt100_cursor_back(this->term,line_len-this->point);
            }
            break;
        }
        /* 
         * to have got this far we must have edited the middle of a line.
         */ 
        if(this->last_point)
        {
            /* move to just after the prompt of the line */
            tinyrl_vt100_cursor_back(this->term,this->last_point);
        }
        /* erase the previous line */
        tinyrl_vt100_erase(this->term,strlen(this->last_buffer));
        
        /* output the line accounting for the echo behaviour */
        tinyrl_internal_print(this,this->line);
        
        delta = (int)(line_len - this->point);
        if(delta)
        {
            /* move the cursor back to the insertion point */
            tinyrl_vt100_cursor_back(this->term,
                                     (strlen(this->line) - this->point));
        }
    } /*lint -e717 */ while(0) /*lint +e717 */;
    
    /* update the display */
    (void)tinyrl_vt100_oflush(this->term);
    
    /* set up the last line buffer */
    lub_string_free(this->last_buffer);
    this->last_buffer = lub_string_dup(this->line);
    this->last_point  = this->point;
}
/*----------------------------------------------------------------------- */
tinyrl_t *
tinyrl_new(FILE                     *instream,
           FILE                     *outstream,
           unsigned                  stifle,
           tinyrl_completion_func_t *complete_fn)
{
    tinyrl_t *this=NULL;
    
    this = malloc(sizeof(tinyrl_t));
    if(NULL != this)
    {
        tinyrl_init(this,instream,outstream,stifle,complete_fn);
    }
    
    return this;
}
/*----------------------------------------------------------------------- */
char *
tinyrl_readline(tinyrl_t   *this,
                const char *prompt,
                void       *context)
{
    FILE *istream = tinyrl_vt100__get_istream(this->term);

    /* initialise for reading a line */
    this->done             = BOOL_FALSE;
    this->point            = 0;
    this->end              = 0;
    this->buffer           = lub_string_dup("");
    this->buffer_size      = strlen(this->buffer);
    this->line             = this->buffer;
    this->prompt           = prompt;
    this->prompt_size      = strlen(prompt);
    this->context          = context;


    if(BOOL_TRUE == this->isatty)
    {        
        /* set the terminal into raw input mode */
        tty_set_raw_mode(this);

        tinyrl_reset_line_state(this);
    
        while(!this->done)
        {
            int key;
            /* update the display */
            tinyrl_redisplay(this);
        
            /* get a key */
            key = tinyrl_getchar(this);

            /* has the input stream terminated? */
            if(EOF != key)
            {
                /* call the handler for this key */
                if(BOOL_FALSE == this->handlers[key](this,key))
                {
                    /* an issue has occured */
                    tinyrl_ding(this);
                }

                if (BOOL_TRUE == this->done) 
                {
                    /*
                     * If the last character in the line (other than 
                     * the null) is a space remove it.
                     */
                     if (this->end && isspace(this->line[this->end-1]))
                     {
                         tinyrl_delete_text(this,this->end-1,this->end);
                     }
                }
            }
            else
            {
                /* time to finish the session */
                this->done = BOOL_TRUE;
                this->line = NULL;
            }
        }
        /* restores the terminal mode */
        tty_restore_mode(this);
    }
    else
    {
        /* This is a non-interactive set of commands */
        char  *s = 0, buffer[80];
        size_t len = sizeof(buffer);
        
        /* manually reset the line state without redisplaying */
        lub_string_free(this->last_buffer);
        this->last_buffer = NULL;

        while((sizeof(buffer) == len) && 
                (s = fgets(buffer,sizeof(buffer),istream)))
        {
            char *p;
            /* strip any spurious '\r' or '\n' */
            p = strchr(buffer,'\r');
            if(NULL == p)
            {
                p = strchr(buffer,'\n');
            }
            if (NULL != p)
            {
                *p = '\0';
            }
            /* skip any whitespace at the beginning of the line */
            if(0 == this->point)
            {
                while(*s && isspace(*s))
                {
                    s++;
                }
            }
            if(*s)
            {
                /* append this string to the input buffer */
                (void) tinyrl_insert_text(this,s);
                /* echo the command to the output stream */
                tinyrl_redisplay(this);
            }
            len = strlen(buffer) + 1; /* account for the '\0' */
        }

        /*
         * check against fgets returning null as either error or end of file.
         * This is a measure to stop potential task spin on encountering an
         * error from fgets.
         */
        if( s == NULL || (this->line[0] == '\0' && feof(istream)) )
        {
            /* time to finish the session */
            this->line = NULL;
        }
        else
        {
            /* call the handler for the newline key */
            if(BOOL_FALSE == this->handlers[KEY_LF](this,KEY_LF))
            {
                /* an issue has occured */
                tinyrl_ding(this);
                this->line = NULL;
            }
        }
    }
    /*
     * duplicate the string for return to the client 
     * we have to duplicate as we may be referencing a
     * history entry or our internal buffer
     */
    {
        char *result = this->line ? lub_string_dup(this->line) : NULL;

        /* free our internal buffer */
        free(this->buffer);
        this->buffer = NULL;
    
        if((NULL == result) || '\0' == *result)
        {
            /* make sure we're not left on a prompt line */
            tinyrl_crlf(this);
        }
        return result;
    }
}
/*----------------------------------------------------------------------- */
/*
 * Ensure that buffer has enough space to hold len characters,
 * possibly reallocating it if necessary. The function returns BOOL_TRUE
 * if the line is successfully extended, BOOL_FALSE if not.
 */
bool_t
tinyrl_extend_line_buffer(tinyrl_t *this,
                          unsigned  len)
{
    bool_t result = BOOL_TRUE;
    if(this->buffer_size < len)
    {
        char * new_buffer;
        size_t new_len = len;

        /* 
         * What we do depends on whether we are limited by
         * memory or a user imposed limit.
         */

        if (this->max_line_length == 0) 
        {
            if(new_len < this->buffer_size + 10)
            {
                /* make sure we don't realloc too often */
                new_len = this->buffer_size + 10;
            }
            /* leave space for terminator */
            new_buffer = realloc(this->buffer,new_len+1); 

            if(NULL == new_buffer)
            {
                tinyrl_ding(this);
                result = BOOL_FALSE;
            }
            else
            {
                this->buffer_size = new_len;
                this->line = this->buffer = new_buffer;
            }
        } 
        else
        {
            if(new_len < this->max_line_length)
            {

                /* Just reallocate once to the max size */
                new_buffer = realloc(this->buffer,this->max_line_length); 

                if(NULL == new_buffer)
                {
                    tinyrl_ding(this);
                    result = BOOL_FALSE;
                }
                else
                {
                    this->buffer_size = this->max_line_length - 1;
                    this->line = this->buffer = new_buffer;
                }
            }
            else 
            {
                tinyrl_ding(this);
                result = BOOL_FALSE;
            }
        } 
    }
    return result;
}
/*----------------------------------------------------------------------- */
/*
 * Insert text into the line at the current cursor position.
 */
bool_t
tinyrl_insert_text(tinyrl_t   *this,
                   const char *text)
{
    unsigned delta = strlen(text);

    /* 
     * If the client wants to change the line ensure that the line and buffer
     * references are in sync
     */
    changed_line(this);

    if((delta + this->end) > (this->buffer_size))
    {
        /* extend the current buffer */
        if (BOOL_FALSE == tinyrl_extend_line_buffer(this,this->end + delta))
        {
            return BOOL_FALSE;
        }
    }

    if(this->point < this->end)
    {
        /* move the current text to the right (including the terminator) */
        memmove(&this->buffer[this->point+delta],
                &this->buffer[this->point],
                (this->end - this->point) + 1);
    }
    else
    {
        /* terminate the string */
        this->buffer[this->end+delta] = '\0';
    }
                

    /* insert the new text */
    strncpy(&this->buffer[this->point],text,delta);
    
    /* now update the indexes */
    this->point += delta;
    this->end   += delta;

    return BOOL_TRUE;
}
/*----------------------------------------------------------------------- */
/* 
 * A convenience function for displaying a list of strings in columnar
 * format on Readline's output stream. matches is the list of strings,
 * in argv format, such as a list of completion matches. len is the number
 * of strings in matches, and max is the length of the longest string in matches.
 * This function uses the setting of print-completions-horizontally to select
 * how the matches are displayed
 */
void
tinyrl_display_matches(const tinyrl_t        *this,
                       char           *const *matches,
                       unsigned               len,
                       size_t                 max)
{
    unsigned    r,c;
    unsigned    width = tinyrl_vt100__get_width(this->term);
    unsigned    cols = width/(max+1); /* allow for a space between words */
    unsigned    rows = len/cols + 1;

    assert(matches);
    if(matches)
    {
        len--,matches++; /* skip the subtitution string */
        /* print out a table of completions */
        for(r=0; r<rows && len; r++)
        {
            for(c=0; c<cols && len; c++)
            {
                const char *match = *matches++;
                len--;
                tinyrl_vt100_printf(this->term,"%-*s ",max,match);
            }
            tinyrl_crlf(this);
        }
    }
}
/*----------------------------------------------------------------------- */
/*
 * Delete the text between start and end in the current line. (inclusive)
 * This adjusts the rl_point and rl_end indexes appropriately.
 */
void
tinyrl_delete_text(tinyrl_t *this,
                   unsigned  start,
                   unsigned  end)
{
    unsigned delta;

    /* 
     * If the client wants to change the line ensure that the line and buffer
     * references are in sync
     */
    changed_line(this);
    
    /* make sure we play it safe */
    if(start > end)
    {
        unsigned tmp = end;
        start        = end;
        end          = tmp;
    }
    if(end > this->end)
    {
        end = this->end;
    }
    
    delta = (end - start) + 1;    

    /* move any text which is left */
    memmove(&this->buffer[start],
            &this->buffer[start+delta],
            this->end - end);

    /* now adjust the indexs */
    if(this->point >= start)
    {
        if(this->point > end)
        {
            /* move the insertion point back appropriately */
            this->point -= delta;
        }
        else
        {
            /* move the insertion point to the start */
            this->point = start;
        }
    }
    if(this->end > end)
    {
        this->end -= delta;
    }
    else
    {
        this->end = start;
    }
    /* put a terminator at the end of the buffer */
    this->buffer[this->end] ='\0';
}
/*----------------------------------------------------------------------- */
bool_t
tinyrl_bind_key(tinyrl_t          *this,
                int                key,
                tinyrl_key_func_t *fn)
{
    bool_t result = BOOL_FALSE;
    
    if((key >= 0) && (key < 256))
    {
        /* set the key handling function */
        this->handlers[key] = fn;
        result = BOOL_TRUE;
    }
    
    return result;
}
/*-------------------------------------------------------- */
/*
 * Returns an array of strings which is a list of completions for text.
 * If there are no completions, returns NULL. The first entry in the
 * returned array is the substitution for text. The remaining entries
 * are the possible completions. The array is terminated with a NULL pointer.
 *
 * entry_func is a function of two args, and returns a char *. 
 * The first argument is text. The second is a state argument;
 * it is zero on the first call, and non-zero on subsequent calls.
 * entry_func returns a NULL pointer to the caller when there are no 
 * more matches. 
 */
char **
tinyrl_completion(tinyrl_t                *this,
                  const char              *line,
                  unsigned                 start,
                  unsigned                 end,
                  tinyrl_compentry_func_t *entry_func)
{
    unsigned     state   = 0;
    size_t       size    = 1;
    unsigned     offset  = 1; /* need at least one entry for the substitution */
    char       **matches = NULL;
    char        *match;
    /* duplicate the string upto the insertion point */
    char        *text = lub_string_dupn(line,end);

    /* now try and find possible completions */
    while( (match = entry_func(this,text,start,state++)) )
    {
        if(size == offset)
        {
            /* resize the buffer if needed - the +1 is for the NULL terminator */
            size += 10;
            matches = realloc(matches,(sizeof(char *) * (size + 1)));
        }
        if (NULL == matches)
        {
            /* not much we can do... */
            break;
        }
        matches[offset] = match;
        
        /*
         * augment the substitute string with this entry
         */
        if(1 == offset)
        {
            /* let's be optimistic */
            matches[0] = lub_string_dup(match);
        }
        else
        {
            char *p = matches[0];
            size_t match_len = strlen(p);
            /* identify the common prefix */
            while((tolower(*p) == tolower(*match)) && match_len--)
            {
                p++,match++;
            }
            /* terminate the prefix string */
            *p = '\0';
        }
        offset++;
    }
    /* be a good memory citizen */
    lub_string_free(text);

    if(matches)
    {
        matches[offset] = NULL;
    }
    return matches;
}
/*-------------------------------------------------------- */
void
tinyrl_delete_matches(char **this)
{
    char **matches = this;
    while(*matches)
    {
        /* release the memory for each contained string */
        free(*matches++);        
    }
    /* release the memory for the array */
    free(this);
}
/*-------------------------------------------------------- */
void
tinyrl_crlf(const tinyrl_t *this)
{
    tinyrl_vt100_printf(this->term,"\n");
}
/*-------------------------------------------------------- */
/*
 * Ring the terminal bell, obeying the setting of bell-style.
 */
void
tinyrl_ding(const tinyrl_t *this)
{
    tinyrl_vt100_ding(this->term);
}
/*-------------------------------------------------------- */
void
tinyrl_reset_line_state(tinyrl_t *this)
{
    /* start from scratch */
    lub_string_free(this->last_buffer);
    this->last_buffer = NULL;

    tinyrl_redisplay(this);
}
/*-------------------------------------------------------- */
void
tinyrl_replace_line(tinyrl_t   *this,
                    const char *text,
                    int         clear_undo)
{
    size_t new_len = strlen(text);
    
    /* ignored for now */
    clear_undo = clear_undo;
    
    /* ensure there is sufficient space */
    if (BOOL_TRUE == tinyrl_extend_line_buffer(this,new_len))
    {

        /* overwrite the current contents of the buffer */
        strcpy(this->buffer,text);
    
        /* set the insert point and end point */
        this->point = this->end = new_len;
    }
    tinyrl_redisplay(this);
}
/*-------------------------------------------------------- */
static tinyrl_match_e
tinyrl_do_complete(tinyrl_t *this,
                   bool_t    with_extensions)
{
    tinyrl_match_e result = TINYRL_NO_MATCH;
    char   **matches      = NULL;
    unsigned start,end;
    bool_t   completion   = BOOL_FALSE;
    bool_t   prefix       = BOOL_FALSE;
    
    /* find the start and end of the current word */
    start = end = this->point;
    while(start && !isspace(this->line[start-1]))
    {
        start--;
    }

    if(this->attempted_completion_function)
    {        
        this->completion_over       = BOOL_FALSE;
        this->completion_error_over = BOOL_FALSE;
        /* try and complete the current line buffer */
        matches = this->attempted_completion_function(this,
                                                      this->line,
                                                      start,
                                                      end);
    }
    if((NULL == matches)
       && (BOOL_FALSE == this->completion_over))
    {
            /* insert default completion call here... */
    }
    
    if(matches)
    {
        /* identify and insert a common prefix if there is one */
        if(0 != strncmp(matches[0],&this->line[start],strlen(matches[0])))
        {
            /* 
             * delete the original text not including 
             * the current insertion point character 
             */
            if(this->end != end)
            {
                end--;
            }
            tinyrl_delete_text(this,start,end);
            if (BOOL_FALSE == tinyrl_insert_text(this,matches[0]))
            {
                return TINYRL_NO_MATCH;
            }
            completion = BOOL_TRUE;
        }
        if(0 == lub_string_nocasecmp(matches[0],matches[1]))
        {
            /* this is just a prefix string */
            prefix = BOOL_TRUE;
        }
        /* is there more than one completion? */
        if(matches[2] != NULL)
        {
            char **tmp = matches;
            unsigned max,len;
            max = len = 0;
            while(*tmp)
            {
                size_t size = strlen(*tmp++);
                len++;
                if(size > max)
                {
                    max = size;
                }
            }
            if(BOOL_TRUE == completion)
            {
                result = TINYRL_COMPLETED_AMBIGUOUS;
            }
            else if(BOOL_TRUE == prefix)
            {
                result = TINYRL_MATCH_WITH_EXTENSIONS;
            }
            else
            {
                result = TINYRL_AMBIGUOUS;
            }
            if((BOOL_TRUE == with_extensions)
               || (BOOL_FALSE == prefix))
            {
                /* Either we always want to show extensions or
                 * we haven't been able to complete the current line
                 * and there is just a prefix, so let the user see the options
                 */
                tinyrl_crlf(this);
                tinyrl_display_matches(this,matches,len,max);
                tinyrl_reset_line_state(this);
            }
        }
        else
        {
            result = completion ? TINYRL_COMPLETED_MATCH : TINYRL_MATCH;
        }
        /* free the memory */
        tinyrl_delete_matches(matches);
        
        /* redisplay the line */
        tinyrl_redisplay(this);
    }
    return result;
}
/*-------------------------------------------------------- */
tinyrl_match_e
tinyrl_complete_with_extensions(tinyrl_t *this)
{
    return tinyrl_do_complete(this,BOOL_TRUE);
}
/*-------------------------------------------------------- */
tinyrl_match_e
tinyrl_complete(tinyrl_t *this)
{
    return tinyrl_do_complete(this,BOOL_FALSE);
}
/*-------------------------------------------------------- */
void *
tinyrl__get_context(const tinyrl_t *this)
{
    return this->context;
}
/*--------------------------------------------------------- */
const char *
tinyrl__get_line(const tinyrl_t *this)
{
    return this->line;
}
/*--------------------------------------------------------- */
tinyrl_history_t *
tinyrl__get_history(const tinyrl_t *this)
{
    return this->history;
}
/*--------------------------------------------------------- */
void
tinyrl_completion_over(tinyrl_t *this)
{
    this->completion_over = BOOL_TRUE;
}
/*--------------------------------------------------------- */
void
tinyrl_completion_error_over(tinyrl_t *this)
{
    this->completion_error_over = BOOL_TRUE;
}
/*--------------------------------------------------------- */
bool_t
tinyrl_is_completion_error_over(const tinyrl_t *this)
{
    return this->completion_error_over;
}
/*--------------------------------------------------------- */
void
tinyrl_done(tinyrl_t *this)
{
    this->done = BOOL_TRUE;
}
/*--------------------------------------------------------- */
void
tinyrl_enable_echo(tinyrl_t *this)
{
    this->echo_enabled = BOOL_TRUE;
}
/*--------------------------------------------------------- */
void
tinyrl_disable_echo(tinyrl_t *this,
                    char      echo_char)
{
    this->echo_enabled = BOOL_FALSE;
    this->echo_char    = echo_char;
}
/*--------------------------------------------------------- */
void
tinyrl__set_istream(tinyrl_t *this,
                    FILE     *istream)
{
    tinyrl_vt100__set_istream(this->term,istream);
    this->isatty = isatty(fileno(istream)) ? BOOL_TRUE : BOOL_FALSE;
}
/*-------------------------------------------------------- */
bool_t
tinyrl__get_isatty(const tinyrl_t *this)
{
    return this->isatty;
}
/*-------------------------------------------------------- */
FILE *
tinyrl__get_istream(const tinyrl_t *this)
{
    return tinyrl_vt100__get_istream(this->term);
}
/*-------------------------------------------------------- */
FILE *
tinyrl__get_ostream(const tinyrl_t *this)
{
    return tinyrl_vt100__get_ostream(this->term);
}
/*-------------------------------------------------------- */
const char *
tinyrl__get_prompt(const tinyrl_t *this)
{
    return this->prompt;
}
/*-------------------------------------------------------- */
bool_t 
tinyrl_is_quoting(const tinyrl_t *this)
{
    bool_t result = BOOL_FALSE;
    /* count the quotes upto the current insertion point */
    unsigned i = 0;
    while(i < this->point)
    {
        if(this->line[i++] == '"')
        {
            result = result ? BOOL_FALSE : BOOL_TRUE;
        }
    }
    return result;
}
/*--------------------------------------------------------- */
void
tinyrl_limit_line_length(tinyrl_t *this, unsigned length)
{
    this->max_line_length = length;
}
/*--------------------------------------------------------- */
