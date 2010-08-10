/***********************************************************************
**
**	Filename	: test.c
**	Project		: Unit Test
**	Subsystem	: 
**	Module		: Basic routines for performing a Unit Test
**	Document	: 
**
*************************************************************************
**
**	Brief Description
**	=================
**
**	Functions to provide a nice, consistent Unit Test infrastructure.
**	
**
*************************************************************************
**
**  Change History
**  --------------
**
** 20-Jun-2005    Graeme McKerrell     Populated lub_test_stop_here() t ensure it is reached when a
**                                                                optimising compiler is used.
** 7-Dec-2004    Graeme McKerrell     Updated to use the "lub_test_" prefix 
**                                    rather than "unittest_"
**  4  Mar 2004  Graeme McKerrell     Ported for use in Garibaldi
**  4  Oct 2002  Graeme McKerrell     updated to use central interface
**                                    definition
**  18-Mar-2002	 Graeme McKerrell     Added unitest_stop_here() support
**  16-Mar-2002	 Graeme McKerrell     LINTed...
**  14 Nov 2000  Brett B. Bonner      created
**
*************************************************************************
**
**  Copyright (C) 3Com Corporation. All Rights Reserved.
**
\************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
/*lint -esym(534,vsprintf) */
/*lint -esym(632,va_list,__va_list) */
/*lint -esym(633,va_list,__gnuc_va_list) */
#include "lub/test.h"



/* Where to direct output (bitmasks) */
#define LUB_TEST_LOGTOFILE   0x1
#define LUB_TEST_LOGTOSTDOUT 0x2

/* Termination Mode */
typedef enum {
  ContinueOnFail,
  StopOnFail
} TerminationMode;

/* local variables */
static char unitTestName[80];
static char seqDescr[80];
static FILE *logp=NULL;
static lub_test_verbosity_t verbosity = LUB_TEST_NORMAL;
static int outputTo;
static lub_test_status_t unitTestStatus=LUB_TEST_PASS;
static int seqNum=0;
static int testNum=0;
static int failureCount=0;
static int testCount=0;
static TerminationMode termMode=ContinueOnFail;

/* local functions */
static void testLogNoIndent(lub_test_verbosity_t level, const char *format, ...);
static void printUsage(void);

/* definitions */
#define LOGGING_TO_FILE ((outputTo & LUB_TEST_LOGTOFILE) != 0)
#define LOGGING_TO_STDOUT ((outputTo & LUB_TEST_LOGTOSTDOUT) != 0)


/*
 * This is provided as a debug aid.
 */
void 
lub_test_stop_here(void)
{
  /* If any test fails, the unit test fails */
  unitTestStatus = LUB_TEST_FAIL;
  failureCount++;
  if (termMode == StopOnFail) {
    lub_test_end();
    exit(1);
  }
}

static lub_test_status_t
checkStatus(lub_test_status_t value)
{
    /* Test number gets incremented automatically... */
    testNum++;
    testCount++; /* update total number of tests performed */

    if(LUB_TEST_FAIL == value)
    {
        lub_test_stop_here();
    }
    return value;
}

/*******************************************************
* unit-test-level functions
********************************************************/
/* NAME:    unitTestLog
   PURPOSE: Log output to file and/or stdout, verbosity-filtered
   ARGS:    level - priority of this message.
                    Will be logged only if priority equals or exceeds
                    the current lub_test_verbosity_t level.
            format, args - printf-style format and parameters
   RETURN:  none
*/
static void 
unitTestLog(lub_test_verbosity_t level, const char *format, ...)
{
    va_list args;
    char string[320]; /* ought to be big enough; that's 4 lines */

    /* Turn format,args into a string */
    va_start(args, format);
    vsprintf(string, format, args);
    va_end(args);

    /* output to selected destination if lub_test_verbosity_t equals or exceeds
     current setting. */ 
    if (level <= verbosity) 
    {
        if (LOGGING_TO_FILE) 
        {
            if (NULL != logp)
            {
                fprintf(logp, "%s\n", string);
            }
            else
            {
                fprintf(stderr, "ERROR: Trying to log to file, but no logfile is open!\n");
            }
        }
        if (LOGGING_TO_STDOUT) 
        {
            fprintf(stdout, "%s\n", string);
        }
    }
}

/* NAME:    TestStartLog
   PURPOSE: Sets up logging destination(s) and opens logfile.
   ARGS:    whereToLog   - where output gets directed to
                           Bitmask of lub_test_LOGTOFILE, lub_test_LOGTOSTDOUT
            logfile      - file name to open.
                           Use NULL if not logging to file.
   RETURN:  BOOL_FALSE == failure
            BOOL_TRUE  == success
*/
static int TestStartLog(int whereToLog, const char *logFile) 
{
    bool_t status = BOOL_TRUE;
    /* Where are we logging? */
    outputTo = whereToLog;
  
    /* open log file */
    if (LOGGING_TO_FILE && (strlen(logFile) < 1)) 
    {
        status = BOOL_FALSE;
        fprintf(stderr, "ERROR: No logfile name specified.\n");  
    }
    if (LOGGING_TO_FILE && status) 
    {
        if ( (logp = fopen(logFile, "w")) == NULL ) 
        {
            status = BOOL_FALSE;
            fprintf(stderr, 
                    "ERROR: could not open log file '%s'.\n",
                    logFile);
        }
    }
    return status;
}

/* NAME:    lub_test_begin
   PURPOSE: Starts unit test.  
   ARGS:    format,args  - Specification of unit test name
   RETURN:  none
*/
void 
lub_test_begin(const char *name, ...) 
{
    va_list args;

    /* Process varargs list into unit test name string */
    va_start(args, name);
    vsprintf(unitTestName, name, args);
    va_end(args);
    unitTestLog(LUB_TEST_NORMAL, "BEGIN: Testing '%s'.", unitTestName);

    /* reset counters */
    seqNum=testNum=testCount=failureCount=0;
} 

/* NAME:    lub_test_get_status
   PURPOSE: Reports current unit test status
   ARGS:    none
   RETURN:  Status code - TESTPASS/TESTFAIL
*/
lub_test_status_t
lub_test_get_status(void)
{
    return unitTestStatus;
}

/* NAME:    lub_test_failure_count
   PURPOSE: Reports current number of test failures
   ARGS:    none
   RETURN:  Count of failures
*/
int 
lub_test_failure_count(void)
{
    return failureCount;
}

/* NAME:    lub_test_end
   PURPOSE: Ends test.  Closes log file.
   ARGS:    none
   RETURN:  none
*/
void 
lub_test_end(void)
{
    char result[40];

    if (unitTestStatus == LUB_TEST_PASS) 
    {
        sprintf(result, "PASSED (%d tests)",testCount);
    } 
    else 
    {
        if (failureCount == 1)
        {
            sprintf(result, "FAILED (%d failure, %d tests)", 
                    failureCount, testCount);
        }
        else 
        {
              sprintf(result, "FAILED (%d failures, %d tests)", 
                      failureCount, testCount);
        }
    }
    if ((termMode == ContinueOnFail) ||
        (unitTestStatus == LUB_TEST_PASS)) 
    {
        /* ran to end - either due to continue-on-fail, or because
           everything passed */ 
        unitTestLog(LUB_TEST_TERSE, "END: Test '%s' %s.\n",unitTestName, result);
    } 
    else 
    { 
        /* stopped on first failure */
        unitTestLog(LUB_TEST_TERSE, "END: Test '%s': STOPPED AT FIRST FAILURE.\n",unitTestName);    
    }
    if (LOGGING_TO_FILE) 
    {
        fclose(logp);
    }
}

/* NAME:    printUsage
   PURPOSE: Prints usage message
   ARGS:    none
   RETURN:  none
*/
static void 
printUsage(void) 
{
    printf("Usage:\n");
    printf("-terse, -normal, -verbose    : set lub_test_verbosity_t level (default: normal)\n");
    printf("-stoponfail, -continueonfail : set behavior upon failure (default: continue)\n");
    printf("-logfile [FILENAME]          : log to FILENAME (default: 'test.log')\n"); 
    printf("-nologfile                   : disable logging to file \n");
    printf("-stdout, -nostdout           : enable/disable logging to STDOUT\n");
    printf("-usage, -help                : print these options and exit\n");
    printf("\nAll arguments are optional.  Defaults are equivalent to: \n");
    printf("  -normal -continueonfail -logfile test.log -stdout\n");
}

/* NAME:    lub_test_parse_command_line
   PURPOSE: Parses command-line arguments & sets unit test options.
   ARGS:    argc, argv - as passed to main()
   RETURN:  none (will exit if there is an error)
*/
void 
lub_test_parse_command_line(int argc, const char * const *argv) 
{
    bool_t status=BOOL_TRUE;
    int logDest=0;
    static char defaultFile[] = "test.log"; 
    char *logFile;
    bool_t logFileAllocatedFromHeap=BOOL_FALSE;

    /* variables used to detect conflicting options */
    int verbset=0;
    int failset=0;
    int logfset=0;
    int stdset=0;

    /* Set logFile to defaultFile so there is always a valid filename.
       Also set logging destination to both STDOUT and FILE.
       These will be changed later if user specifies something else. */
    logFile = defaultFile;
    logDest = (LUB_TEST_LOGTOFILE | LUB_TEST_LOGTOSTDOUT);

    /* Loop through the command line arguments.  */
    while (--argc > 0) 
    {
        /* Usage/Help */
        if ((strstr(argv[argc], "-usage") != NULL) ||
            (strstr(argv[argc], "-help") != NULL)) 
        {
            printUsage();
            exit (0);
        }
        /* lub_test_verbosity_t Levels */
        else if (strstr(argv[argc], "-terse") != NULL) 
        { 
            verbosity = LUB_TEST_TERSE; verbset++;
        }
        else if (strstr(argv[argc], "-normal") != NULL) 
        {
            verbosity = LUB_TEST_NORMAL; verbset++;
        }
        else if (strstr(argv[argc], "-verbose") != NULL) 
        {
            verbosity = LUB_TEST_VERBOSE; verbset++;
        }
        /* Failure behavior */
        else if (strstr(argv[argc], "-stoponfail") != NULL) 
        {
            termMode = StopOnFail; failset++;
        }
        else if (strstr(argv[argc], "-continueonfail") != NULL) 
        {
            termMode = ContinueOnFail; failset++;
        }
        /* Log file options */
        else if (strstr(argv[argc], "-logfile") != NULL) 
        {
            logDest |= LUB_TEST_LOGTOFILE;
            /* Was a log file name specified?  
               We assume it's a file name if it doesn't start with '-' */
            if (strstr(argv[argc+1], "-") != argv[argc+1]) 
            {
                /* Yes, got a filename */
                logFile = (char *)malloc(strlen(argv[argc+1])+1);

                if (NULL == logFile) 
                {
                    status = BOOL_FALSE;
                    fprintf(stderr, "unitTestParseCL: ERROR: Memory allocation problem.\n");
                }
                else 
                {
                    /* all is well, go ahead and copy the string */
                    strcpy(logFile, argv[argc+1]);
                }
                logFileAllocatedFromHeap=BOOL_TRUE;
            }
            logfset++;
        }
        else if (strstr(argv[argc], "-nologfile") != NULL) 
        {
            logDest &= ~LUB_TEST_LOGTOFILE;
            logfset++;
        }
        /* Stdout options */
        else if (strstr(argv[argc], "-stdout") != NULL) 
        {
            logDest |= LUB_TEST_LOGTOSTDOUT;
            stdset++;
        }
        else if (strstr(argv[argc], "-nostdout") != NULL) 
        {
            logDest &= ~LUB_TEST_LOGTOSTDOUT;
            stdset++;
        }
        /* Unhandled arguments */
        else 
        {
            /* If the next argument down in the list is '-logfile', then
               this is the logfile name; don't complain. */
            if (strstr(argv[argc-1], "-logfile") == NULL) 
            {
                /* This is an unrecognized option. 
                   Don't bother setting status and forcing an exit; just ignore it. */
                fprintf(stderr,"Unrecognized argument: '%s', ignoring it...\n",argv[argc]);
            }
        }
    }
    /* See if there is a logging destination */
    if (logDest == 0) 
    {
        fprintf(stderr, "WARNING: No logging is enabled to either stdout or a logfile; expect no output.\n");
    }
    /* Make sure there were no conflicting options */
    if (verbset > 1) 
    {
        fprintf(stderr,"ERROR: conflicting lub_test_verbosity_t options specified.\n");
        fprintf(stderr,"       Specify only ONE of -terse, -normal, -verbose\n");
        status = BOOL_FALSE;
    }
    if (failset > 1) 
    {
        fprintf(stderr,"ERROR: conflicting Failure Mode options specified.\n");
        fprintf(stderr,"       Specify only ONE of -stoponfail, -continueonfail\n");
        status = BOOL_FALSE;
    }
    if (logfset > 1) 
    {
        fprintf(stderr,"ERROR: conflicting Logfile options specified.\n");
        fprintf(stderr,"       Specify only ONE of -logfile, -nologfile\n");
        status = BOOL_FALSE;
    }
    if (stdset > 1) 
    {
        fprintf(stderr,"ERROR: conflicting Stdout options specified.\n");
        fprintf(stderr,"       Specify only ONE of -stdout, -nostdout\n");
        status = BOOL_FALSE;
    }

    /* Set up log file, if things are OK so far */
    if (status && !TestStartLog(logDest, logFile))
    {
          status = BOOL_FALSE;
    }
    if (logFileAllocatedFromHeap) 
    {
        /*lint -e673 Possibly inappropriate deallocation (free) for 'static' data */
        free(logFile);
        /*lint +e673 */
    }
    if (BOOL_FALSE == status) 
    {
        fprintf(stderr, "Something bad has occurred.  Aborting...\n");
        exit(1);
    }

}

/*******************************************************
* sequence level functions
********************************************************/

/* NAME:    lub_test_seq_begin
   PURPOSE: Starts a new test sequence.
   ARGS:    num - sequence number, must be different than last one
            format, args - printf-style name for the sequence
   RETURN:  none
*/
void 
lub_test_seq_begin(int num, const char *seqName, ...)
{
    va_list args;

    /* Get sequence name */
    va_start(args, seqName);
    vsprintf(seqDescr, seqName, args);
    va_end(args);

    /* Start new sequence number */
    if (num == seqNum) 
    {
        seqNum++;
        unitTestLog(LUB_TEST_TERSE, "seq_start: duplicate sequence number.  Using next available sequence number (%d).\n", seqNum);
    } 
    else 
    {
        seqNum = num;
    }

    /* Reset test number */
    testNum = 0;

    /* Log beginning of sequence */
    lub_test_seq_log(LUB_TEST_NORMAL, "*** Sequence '%s' begins ***", seqDescr);
}

/* NAME:    lub_test_seq_end
   PURPOSE: Ends current test sequence
   ARGS:    none
   RETURN:  none
*/
void 
lub_test_seq_end(void)
{
    lub_test_seq_log(LUB_TEST_NORMAL, 
                     "----------------------------------------"
                     "--------------------");
}

/* NAME:    lub_test_seq_log
   PURPOSE: Log output to file and/or stdout, verbosity-filtered and
            formatted as a "sequence-level" message
   ARGS:    level - priority of this message.
                    Will be logged only if priority equals or exceeds
                    the current lub_test_verbosity_t level.
            format, args - printf-style format and parameters
   RETURN:  none
*/
void
lub_test_seq_log(lub_test_verbosity_t level, const char *format, ...)
{
    va_list args;
    char string[160];

    /* Turn format,args into a string */
    va_start(args, format);
    vsprintf(string, format, args);
    va_end(args);

    unitTestLog(level, "%03d     :        %s", seqNum, string);
}

/*******************************************************
* test level functions
********************************************************/

/* NAME:    lub_test_check
   PURPOSE: True/False test of an expression.  If True, test passes.
   ARGS:    expr - expression to evaluate.
            format, args - Printf-style format/parameters describing test.  
   RETURN:  Status code - PASS or FAIL
*/
lub_test_status_t
lub_test_check(bool_t expr, const char *testName, ...)
{
    va_list args;
    char testDescr[80];
    lub_test_status_t testStatus;
    char result[5];
    lub_test_verbosity_t verb;

    /* evaluate expression */
    testStatus = expr ? LUB_TEST_PASS : LUB_TEST_FAIL;

    /* extract description as a string */
    va_start(args, testName);
    vsprintf(testDescr, testName, args);
    va_end(args);

    /* log results */
    if (testStatus == LUB_TEST_PASS) 
    {
        verb=LUB_TEST_NORMAL;
        sprintf(result, "pass");
    } 
    else 
    {
        verb=LUB_TEST_TERSE;
        sprintf(result, "FAIL");
    }
    testLogNoIndent(verb, "[%s] %s", result, testDescr);

    return checkStatus(testStatus);
}

/* NAME:    lub_test_check_int
   PURPOSE: Checks whether an integer equals its expected value.
   ARGS:    expect - expected value.
            actual - value being checked.
            format, args - Printf-style format/parameters describing test.
   RETURN:  Status code - PASS or FAIL
*/
lub_test_status_t
lub_test_check_int(int expect, int actual, const char *testName, ... )
{
    va_list args;
    char testDescr[80];
    lub_test_status_t testStatus;
    char result[5];
    char eqne[3];
    lub_test_verbosity_t verb;

    /* evaluate expression */
    testStatus = (expect == actual) ? LUB_TEST_PASS : LUB_TEST_FAIL;

    /* extract description as a string */
    va_start(args, testName);
    vsprintf(testDescr, testName, args);
    va_end(args);

    /* log results */
    if (testStatus == LUB_TEST_PASS) 
    {
        sprintf(result, "pass");
        sprintf(eqne, "==");
        verb = LUB_TEST_NORMAL;
    } 
    else 
    {
        sprintf(result, "FAIL");
        sprintf(eqne, "!=");
        verb = LUB_TEST_TERSE;
    }
    testLogNoIndent(verb, "[%s] (%d%s%d) %s", 
                    result, actual, eqne, expect, testDescr);

    return checkStatus(testStatus);
}

/* NAME:    lub_test_check_float
   PURPOSE: Checks whether a float is within min/max limits.
   ARGS:    min    - minimum acceptable value.
            max    - maximum acceptable value.
            actual - value being checked.
            format, args - Printf-style format/parameters describing test.
   RETURN:  Status code - PASS or FAIL
*/
lub_test_status_t 
lub_test_check_float(double min, double max, double actual, 
                 const char *testName, ...)
{
    va_list args;
    char testDescr[80];
    lub_test_status_t testStatus;
    char result[5];
    char gteq[4], lteq[4];

    /* evaluate expression */
    testStatus = ((actual >= min) && (actual <= max)) ? LUB_TEST_PASS : LUB_TEST_FAIL;

    /* extract description as a string */
    va_start(args, testName);
    vsprintf(testDescr, testName, args);
    va_end(args);

    /* log results */
    if (testStatus == LUB_TEST_PASS) 
    {
        sprintf(result, "pass");
        sprintf(gteq, " <=");
        sprintf(lteq, " <=");
    }
    else 
    {
        sprintf(result, "FAIL");
        if (actual < min) 
        {
            sprintf(gteq, "!<=");
            sprintf(lteq, " <=");
        } 
        else 
        {
            sprintf(gteq, " <=");
            sprintf(lteq, "!<=");
        }
    }

    testLogNoIndent(LUB_TEST_NORMAL, "[%s] (%8f%s%8f%s%8f) %s", 
                    result, min, gteq, actual, lteq, max, testDescr);

    return checkStatus(testStatus);
}

/* NAME:    lub_test_log
   PURPOSE: Log output to file and/or stdout, verbosity-filtered and
            formatted as a "test-level" message
   ARGS:    level - priority of this message.
                    Will be logged only if priority equals or exceeds
                    the current lub_test_verbosity_t level.
            format, args - printf-style format and parameters
   RETURN:  none
*/
void
lub_test_log(lub_test_verbosity_t level, const char *format, ...)
{
    va_list args;
    char string[160];

    /* Turn format,args into a string */
    va_start(args, format);
    vsprintf(string, format, args);
    va_end(args);

    unitTestLog(level, "%03d-%04d:        %s", seqNum, testNum, string);
}

/* NAME:    testLogNoIndent
   PURPOSE: Log output to file and/or stdout, verbosity-filtered and
            formatted as a "test-level" message.  
            This "internal" version does not indent, so that the
            pass/fail column can be printed in the proper place.
   ARGS:    level - priority of this message.
                    Will be logged only if priority equals or exceeds
                    the current lub_test_verbosity_t level.
            format, args - printf-style format and parameters
   RETURN:  none
*/
static void testLogNoIndent(lub_test_verbosity_t level, const char *format, ...)
{
    va_list args;
    char string[160];

    /* Turn format,args into a string */
    va_start(args, format);
    vsprintf(string, format, args);
    va_end(args);

    unitTestLog(level, "%03d-%04d: %s", seqNum, testNum, string);
}
