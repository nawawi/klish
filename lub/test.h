/** 
 \ingroup lub
 \defgroup lub_test test
 @{
\brief This utiltiy provides a simple unittest facility.

\par  Unit Testing Philosophy

 The tests are used during development as a means of
 exercising and debugging the new code, and once developed serve as
 a means of regression-testing to ensure that changes or additions
 have not introduced defects.  Unit testing in this manner offers
 many benefits:

 \li Well designed unit tests allow for more complete exercise & test
     coverage of new code.

 \li They do not require product hardware or even a product simulator;
     they are compact, standalone executables.

 \li It is easy to instrument the unit tests with commercial tools such
     as Purify/PureCoverage.
  
 \li Use as regression tests increases code maintainability. 

 \li They can be run during builds as ``sanity-checks'' that components
     are functional.


\par What is the unit test interface?

 The Unit Test Interface provide a set of basic capabilities that
 all unit tests need, and which are trivially easy to incorporate
 into a test.  They make it easy to do things such as:
 
 \li Log output to the screen and/or to a logfile 

 \li Test values of various types and record the results
 
 \li Provide well-formatted, concise output with controllable verbosity
 
 \li Track test status. 

 \li Have a consistent set of command-line options for controlling
     the unit test behavior.  (see unitTestCL() below)


\par Verbosity

 Depending on the length of a test, why it is being run, and the
 stage of development, it is desirable to have different levels of
 detail output.  The Unit Test Utilities provide three levels of
 detail:
 
 \li \b Terse output is the minimal set.  Only test failures, serious
     errors, and final results are logged.

 \li \b Normal output is the default.  All test results, sequence headers,
     etc.  are logged.
 
 \li \b Verbose output includes all output messages.  This level is not
     used by the Utilities themselves but is useful for providing
     additional, detailed messages within unit tests.

 Most Utility functions handle verbosity issues automatically; for
 example, Tests automatically log failures as ``terse'' and pass
 results as ``normal''.  The only time a user needs to specify
 verbosity is with Log messages.

 All output has an associated Verbosity level, and will only be
 output if the current Verbosity setting is equal to or greater
 than that level.


\par Sequences

 Simple numbering of tests is adequate only for very small unit
 tests.  More commonly, there are groups of related tests covering
 a particular function or set of functionality.  This gives rise to
 the concept of Sequences.  A Sequence assigns a name and number to
 a group of tests; within the sequence, tests are numbered
 automatically.  While their use is not strictly required, the use
 of Sequences is encouraged as a means of organizing test output
 for easy understanding.

 Sequences are begun with a function call specifying their sequence
 number and a printf-style format specification of their name.
 This outputs a sequence header (at 'normal' verbosity) and
 resets the test counter.  Another call ends the sequence.


\par Tests

 Tests are the heart of the unit test.  Every test has
 a pass/fail result, which is returned as well as affecting the
 overall pass/fail status of the unit test.  Every test also
 accepts a ``name'' argument (in printf style), and generates
 appropriate log output.  Although null strings are accepted as
 names, naming each test is encouraged as it makes the output much
 more readable.

 At the Terse verbosity level, only failing tests generate log output.

 Test functions are provided for testing:

 \li \b Expressions: the expression's true/false value is evaluated.  This
     is the most basic test, similar to an assert().  Virtually
     anything can be tested using this, but the log output is minimal;
     only a Pass/Fail result and the test name are logged.

 \li \b Integer \b Comparison: the test function is passed an expected and
     actual value.  The test passes if the values are the same.  The
     log includes Pass/Fail, expected & actual values, and the test
     name; because the values appear in the log, failures may be more
     easily investigated.

 \li \b Float \b Comparison: the test function is passed minimum, maximum,
     and actual values.  The test passes if the actual value is less
     than or equal to maximum, and greater than or equal to minimum.
     Like the integer comparison, the min, max, and actual values are
     all logged in addition to the Pass/Fail result and test name.

 The overall unit test pass/fail result can be requested at any
 time via a function call.


\par Logging

 Three different logging functions are provided.  All are similar
 but each is appropriate for different purposes.  Each function
 takes as its first argument a verbosity level, specifying the
 minimum verbosity at which the message should be logged.  Each
 also takes a printf-style specification of format and arguments,
 which is used to generate the output.  In this way anything can be
 output: strings, variables, formatted floats, whatever the unit
 test needs to record.

 Note that it is not required to use any logging function; the
 Utilities will generate a reasonable log based on the sequence and
 test calls.  However, additional logging may add significant value
 and human readability to the test results.


\author  Graeme McKerrell
\author  Brett B. Bonner
\date    Created On Thu Feb 28 11:25:44 2002
\date    Last Revised 03-Oct-2002 (BBB)
\version UNTESTED
*/

/**************************************************************
 HISTORY
 7-Dec-2004		Graeme McKerrell	
    Updated to use the "lub_test_" prefix rather than "unittest_"
 18-Mar-2002		Graeme McKerrell	
  added unittest_stop_here() prototype
 16-Mar-2002		Graeme McKerrell	
  LINTed...
 4-Mar-2002		Graeme McKerrell	
  Ported across for use in Garibaldi
 28-Feb-2002		Graeme McKerrell	
  Created based on implentation header file.
  
---------------------------------------------------------------
 Copyright (c) 3Com Corporation. All Rights Reserved
***************************************************************/
#ifndef _lub_test_h
#define _lub_test_h

#include <stdarg.h>
#include "lub/types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
 
/** Status codes */
typedef enum {
  LUB_TEST_PASS, /*!< Indicates a test has passed */
  LUB_TEST_FAIL  /*!< Indicates a test has failed */
} lub_test_status_t;

/** Message priority/verbosity levels */
typedef enum {
  LUB_TEST_TERSE=0,/*!< Only output test failures, errors and final result */
  LUB_TEST_NORMAL, /*!< Output all test results, sequence headers, errors and results */
  LUB_TEST_VERBOSE /*!< Output everything */
} lub_test_verbosity_t;

/*
 ************************************************************
 UNIT-TEST LEVEL FUNCTIONS
 ************************************************************
 */

/**  
 This function is responsible for parsing all the command line
 options, setting run time variables, and opening a log file if
 necessary.

 \par Supported Command Line Switches:
 <em> Set the Verbosity level </em>
   \li -terse   
   \li -normal  (default)
   \li -verbose

 <em> Set behaviour upon failure </em>
   \li -stoponfail
   \li -continueonfail (default)

 <em> Logging:</em>
 
  Enable and disable logging to stdout
   \li -stdout   (default)
   \li -nostdout

  Log to file named FILENAME (if no filename is specified
  "test.log" is used), or disable logging to a file
   \li -logfile [FILENAME] (default)
   \li -nologfile

 <em> Prints usage message and exit </em>
   -usage
   -help

 \pre
 Must be the first unitTest function called.

 \returns
 none

 \post
 If there are conflicting command line options it will exit(1)

 If the log file cannot be opened it will exit(1)

*/
void lub_test_parse_command_line(/** The number of command line arguments */
                     		int argc, 
                     		/** An array of command line arguments */
                     		const char * const *argv);
  
/**
 This starts and specifies the name for the unit-test.

 \pre
 - lub_test_CL() must have been called.

 \returns
 none

 \post
 - The begining of the test is reported in the logfile and/or stdout
 - The overall pass/fail status is reset.

*/
void lub_test_begin(/** a printf style format string to specify the name
                       of the test */
                   const char *fmt, 
                   /** any further arguments required by the format
                       string */
                   ...);
  
/**
 This is the most generic of the logging functions. No addition
 formating is performed.

 \pre
 - lub_test_begin() must have been called.
 - lub_test_end() must not have been called.
 
 \returns
 none

 \post
 - If the specified verbosity is higher than that specified
   on the command line then the message will not be logged
   to the logfile and/or stdout.
 - If lub_test_begin() has not been called then behaviour is undefined

*/
void 
		lub_test_log(/**  the verbosity level for this message */
                	     lub_test_verbosity_t level, 
                             /**  a printf style format string to specify the
                      	          message to log */
		             const char *fmt, 
                	     /** any further arguments required by the format string */
                 	     ...);

/**
 This function provide the current overall status for the
 unit-test. If any tests have failed then the unit-test is deemed
 to have failed.

 \pre
 - lub_test_begin() must have been called.
 - lub_test_end() must not have been called.

 \returns
The current overall status. (TESTPASS/TESTFAIL)

 \post
 - If lub_test_begin() has not been called then behaviour is undefined
 - If lub_test_end() has been called the behaviour is undefined

*/
lub_test_status_t 
		lub_test_get_status(void);

/**
 
 The test utilities maintain internal counts for all the tests
 that have been performed since the unittest_begin() call.
 This function lets a client get the number of failures so far.

 \pre
 - lub_test_begin() must have been called.
 - lub_test_end() must not have been called.

 \returns
The current number of failed tests.

 \post
 - If lub_test_begin() has not been called then behaviour is undefined
 - If lub_test_end() has been called the behaviour is undefined

*/
int 
		lub_test_failure_count(void);

/**
 
 This function ends the unit-test and closes the log file.

 \pre
 - lub_test_begin() must have been called.
 - lub_test_end() must not have been called.

 \returns
 none

 \post
 - The overall pass/fail status is reported to the log file and/or
   stdout.
 - The log file is closed.
 - If lub_test_begin() has not been called then behaviour is undefined
 - If lub_test_end() has been called the behaviour is undefined

*/
void
		lub_test_end(void);

/**
 
 This function is provided as a DEBUG aid. A breakpoint set on this
 function will be reached everytime a test fails, with the failure case in context.
 
 \pre
 none
 
 \returns
 none

 \post
 none
 
*/
void 
		lub_test_stop_here(void);

/*
 ************************************************************
 SEQUENCE LEVEL FUNCTIONS
 ************************************************************
 */

/**
 
 This function starts a new test sequence.

 \pre
 - lub_test_begin() must have been called.
 - lub_test_end() must not have been called.

 \returns
 none

 \post
 - The test count is reset to zero.
 - The current sequence number and description is updated.
 - The begining of the sequence is reported in the logfile and/or stdout.
 - If lub_test_begin() has not been called the behaviour is undefined
 - If lub_test_end() has been called the behaviour is undefined

*/
void
		lub_test_seq_begin(/** a user specified sequence number */
              			   int seq, 
                                   /** a printf style format string to specify the sequence
                                       name*/
                                   const char *fmt, 
                                   /** any further arguments required by the format string */
                                   ...);

/**
 
 This is a good general purpose logging function. It parameters
 are the same as unittest_log, but this will log the provided
 information with the current sequence number, properly indented
 to provide nicely formatted output within a test.

 \pre
 - lub_test_seq_begin() must have been called.

 \returns
 none

 \post
 - The formatted message is reported in the logfile and/or stdout.
 - If lub_test_seq_begin() has not been called the behaviour is undefined

*/
void
		lub_test_seq_log(/** The verbosity level of the message */
                                 lub_test_verbosity_t level, 
                                 /** a printf style format string to specify the message */
                                 const char *fmt, 
                                 /** any further arguments required by the format string */
                                 ...);

/** 
 This function marks the end of the current sequence.

 \pre
 - lub_test_seq_begin() must have been called.

 \returns
 none

 \post
 - An end of sequence message is reported to the log file and/or
   stdout.
 - The current sequence number and description remains unchanged
   until the next call to unittest_seq_begin()
 - If lub_test_seq_begin() has not been called then behaviour is undefined

*/
void 
		lub_test_seq_end(void);

/*
 ************************************************************
 TEST LEVEL FUNCTIONS
 ************************************************************
 */

/*lint -esym(534,lub_test_check,lub_test_check_int,lub_test_check_float)
 Make LINT not moan about people ignoring the return values for
 certain files, the value return is for utility purpose rather than
 a value that ought to be used.

************************************************************/
/**
 The most basic test function, this simply evaluates an expression
 for BOOL_TRUE/BOOL_FALSE and passes/fails as a result. Like all test functions
 it accepts a printf-style format and parameters to describe the test.

 \pre
 - lub_test_begin() must have been called
 
 \returns
 A status code (LUB_TEST_PASS/LUB_TEST_FAIL)

 \post
 - The formatted scenario and result is reported in the logfile
   and/or stdout.
 - The test count is incremented by one.
 -  If the tests fails then the overall unit-test status is changed
    to LUB_TEST_FAIL
 -  If lub_test_begin() has not been called the behaviour is undefined

*/
lub_test_status_t
	lub_test_check(/** a boolean expression to evaluate */
                       bool_t expr, 
                       /** a printf style format string to specify the test
                           scenario */
                       const char *fmt, 
                       /** any further arguments required by the format string */
                       ...);

/** 
 This function is almost identical to test() except it accepts an
 exepcted and actual value to compares them. Equal values cause the
 test to pass, unequal cause the test to fail. Both values are
 recorded in the log output making it easier to understand failures.

 \pre
 - lub_test_begin() must have been called
 
 \returns
 A status code (LUB_TEST_PASS/LUB_TEST_FAIL)

 \post
 - The formatted scenario and result is reported in the logfile
   and/or stdout.
 - The test count is incremented by one.
 - If the tests fails then the overall unit-test status is changed
   to LUB_TEST_FAIL
 - If lub_test_begin() has not been called the behaviour is undefined

*/
lub_test_status_t
		lub_test_check_int(/** the expected integer value */
              		           int expect,
                                   /** the actual integer value*/
                                   int actual,
                                   /** a printf style format string to specify the test scenario */
                                   const char *fmt, 
                                   /** any further arguments required by the format string */
                                   ... );

/** 
 This function is designed to ensure that a floating point value
 is within acceptible limits. It takes a minimum, maximum and
 actual value. The test passes if the actual value is greater or
 equal than the minimum, and less than or equal to the maximum.
 All three values are recorded in the log output.

 \pre
 - lub_test_begin() must have been called
 
 \returns
 A status code (LUB_TEST_PASS/LUB_TEST_FAIL)

 \post
 - The formatted scenario and result is reported in the logfile
   and/or stdout.
 - The test count is incremented by one.
 -  If the tests fails then the overall unit-test status is changed
    to LUB_TEST_FAIL
 -  If lub_test_begin() has not been called the behaviour is undefined

*/
lub_test_status_t
		lag_test_test_float(/** the minimum acceptible value */
                		    double      min,
                                    /** the maximum acceptible value */
                                    double      max,
                                    /** the actual value */
                                    double      actual,
                                    /** a printf style format string to specify the test scenario */
                                    const char *fmt, 
                                    /** any further arguments required by the format string */
                                    ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _lub_test_h */
/** @} test */

