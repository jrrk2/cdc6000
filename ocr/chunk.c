/* Output from p2c 2.02.1, the Pascal-to-C translator */
/* From input file "../ocr/chunk.pas" */


/*DECK PASCSYM*/
/*****************************************************************
 *                                                               *
 *                      PASCAL COMPILER                          *
 *                                                               *
 *                         20/06/72                              *
 *                                                               *
 *   AUTHORS:  U. AMMANN,  R. SCHILD                             *
 *                                                               *
 *           FACHGRUPPE COMPUTERWISSENSCHAFTEN                   *
 *           EIDG. TECHNISCHE HOCHSCHULE                         *
 *           CH-8006 ZUERICH                                     *
 *                                                               *
 *****************************************************************/

/*$T-,O+,R+   COMPILE WITH STACK OVERFLOW TEST AND
              ROUNDING INSTRUCTIONS*/


#include <p2c/p2c.h>


#define NOP             046000
/* p2c: ../ocr/chunk.pas, line 19: 
 * Warning: Expected a semicolon, found a comma [227] */


/* DONT INCREASE WITHOUT INCREASING UPPER BOUND OF RG3 TOO */

/*FOR PMDFILE*/

/*ADDRESSES OF RESIDENT STANDARD ROUTINES*/

/*RUNTIME ERROR EXITS*/

/* ADDRESSES OF STANDARD FILES INPUT AND OUTPUT */
/* p2c: ../ocr/chunk.pas, line 61: 
 * Warning: Unsatisfied forward reference to type CONTEXTTABLE [138] */

typedef Char CWORD[10];
typedef unsigned short ADDRESS;

typedef long SHRTINT;

typedef char BITRANGE;

typedef char RG3;

typedef enum {
  VARBL, SVAL, LVAL, LCOND
} ATTRKIND;

typedef struct ATTR {
  char *TYPTR;
  ATTRKIND KIND;
  union {
    struct {
      enum {
	DRCT, INDRCT, INXD
      } ACCESS;
      RG3 BREG;
      SHRTINT DPLMT;
      boolean PCKD;
      union {
	int empty_union;   /* Pascal variant record was empty */
      } UU;
    } U0;
/* p2c: ../ocr/chunk.pas, line 51:
 * Warning: Expected a '(', found a semicolon [227] */
    SHRTINT VAL;
    SHRTINT CTERM;
    struct {
      char JMP;
      boolean ARITH;
    } U3;
  } UU;
} ATTR;

/*DESCRIBES EXPRESSIONS TO BE COMPILED.   EXAMPLES:
   VARBL:      DRCT:                I                    **/
/* p2c: ../ocr/chunk.pas, line 59:
 * Warning: Symbol 'INDRCT' was already defined [220] */
typedef long INDRCT_;

/* p2c: ../ocr/chunk.pas, line 59:
 * Warning: Expected an '=', found a colon [227] */
/* p2c: ../ocr/chunk.pas, line 59:
 * Warning: Unmatched '*)' in input file [245] */
/* p2c: ../ocr/chunk.pas, line 60:
 * Warning: Unmatched '*)' in input file [245] */


main(argc, argv)
int argc;
Char *argv[];
{
  PASCAL_MAIN(argc, argv);
/* p2c: ../ocr/chunk.pas, line 61:
 * Warning: Expected BEGIN, found a ')' [227] */
  exit(EXIT_SUCCESS);
}
/* p2c: ../ocr/chunk.pas, line 61: 
 * Warning: Expected a '.', found a ')' [227] */
/* p2c: ../ocr/chunk.pas, line 61: 
 * Warning: Junk at end of input file ignored [277] */



/* End. */
