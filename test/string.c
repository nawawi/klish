#include "lub/test.h"
#include "lub/string.h"

/*************************************************************
 * TEST CODE
 ************************************************************* */

static int testseq;

/*--------------------------------------------------------------- */
/* This is the main entry point for this executable
 */
int main(int argc, const char *argv[])
{
    int comp;
 	int status;

	lub_test_parse_command_line(argc,argv);
	lub_test_begin("lub_string");

    lub_test_seq_begin(++testseq,"lub_string_nocasecmp()");
    
    comp = lub_string_nocasecmp("HeLlO","hello");
    lub_test_check((0 == comp),
                        "Check 'HeLlO' == 'hello'");

    comp = lub_string_nocasecmp("HeLlO","hell");
    lub_test_check((comp > 0),
                        "Check 'HeLlO' > 'hell'");

    comp = lub_string_nocasecmp("hell","HeLlO");
    lub_test_check((comp < 0),
                        "Check 'hell'  < 'HeLlO'");

    comp = lub_string_nocasecmp("HeLlO","hellp");
    lub_test_check((comp < 0),
                        "Check 'HeLlO' < 'hellp'");

    comp = lub_string_nocasecmp("hellp","HeLlO");
    lub_test_check((comp > 0),
                        "Check 'hellp' > 'HeLlO'");
     
    lub_test_seq_end();
        
    /* tidy up */
    status = lub_test_get_status();
    lub_test_end();
       
    return status;
}
