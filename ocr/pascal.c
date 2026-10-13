/* Output from p2c 2.02.1, the Pascal-to-C translator */
/* From input file "pascal_p2c.pas" */


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

/* $T-,O+,R+   COMPILE WITH STACK OVERFLOW TEST AND
              ROUNDING INSTRUCTIONS*/


#include <p2c/p2c.h>


#define NOP             19456

#define NILVAL          65536L

#define JMPCST          897623701267152896.0
#define EQJMP           72057594675481600.0
#define MAX10           57646075230342347.0

#define MASK            (-262143L)

#define ALFALENG        10
#define MAXLABS         20
#define MAXEXLABS       10
#define JMPMAX          49
#define CSTMAX          40
#define CODMAX          1000
#define UNDMAX          70
#define DISPLIMIT       20
#define PTLIMIT         10
#define FILLIMIT        10
#define MAXLEVEL        7
/* DONT INCREASE WITHOUT INCREASING UPPER BOUND OF RG3 TOO */
#define WORDLENGTH      60

#define PROCDR          (-1)
#define OBJ             1   /*FOR PMDFILE*/

#define GETC            65
#define PUTC            67
/*ADDRESSES OF RESIDENT STANDARD ROUTINES*/

#define ASSERR          120
#define INXERR          121
#define DIVERR          122
#define STOFLERR        124
/*RUNTIME ERROR EXITS*/

#define INPT            138
#define OUTPT           155


/* ADDRESSES OF STANDARD FILES INPUT AND OUTPUT */

typedef Char CWORD[10];
typedef unsigned short ADDRESS;

typedef long SHRTINT;

typedef char BITRANGE;

typedef char RG3;

typedef enum {
  VARBL, SVAL, LVAL, LCOND
} ATTRKIND;

typedef struct ATTR {
  struct CONTEXTTABLE *TYPTR;
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
	struct {
	  BITRANGE BITADR, BITSZ;
	} U1;
      } UU;
    } U0;
    SHRTINT VAL;
    SHRTINT CTERM;
    struct {
      char JMP;
      boolean ARITH;
    } U3;
  } UU;
} ATTR;

/*DESCRIBES EXPRESSIONS TO BE COMPILED.   EXAMPLES:
   VARBL:      DRCT:                I                    **
               INDRCT:              INPUT^                *
               INXD:                A[K]                  *
   SVAL:                            4                    **
   LVAL:                            2*I - 1               *
   LCOND:      ARITH:               X >= 4.1              *
               NOT ARITH:           B < TRUE              *

   *  RESULT OF CODEGENERATION IN REGISTER X-RP
   ** NO CODE GENERATION UNTIL NOW*/


typedef enum {
  NOOPT, PUREP, POSP, NEGP
} OPTPWR;
typedef enum {
  TYPES, KONST, PROC, VARS, FIELD, TAGFIELD, DUMMYCLASS
} IDCLASS;
typedef enum {
  NUMERIC, SYMBOLIC, POINTER, POWER, ARRAYS, RECORDS, CLASSS, FILES
} TYPFORM;
typedef enum {
  ACTUAL, FORMAL
} IDKINDS;
typedef enum {
  BLOCK, CWITH, VWITH
} WHERE;
typedef short PKINDS;
   /*FOR PMDFILE*/

typedef struct CONTEXTTABLE {
  alfa NAME;
  struct CONTEXTTABLE *NXTEL;
/* p2c: pascal_p2c.pas, line 81:
 * Note: Field width for KLASS assumes enum IDCLASS has 7 elements [105] */
  unsigned KLASS : 3;
  union {
    struct {
      Signed int SIZE : 18;
/* p2c: pascal_p2c.pas, line 83:
 * Note: Field width for FORM assumes enum TYPFORM has 8 elements [105] */
      unsigned FORM : 3;
      union {
	struct {
	  unsigned BITS : 6, MIN : 31, MAX : 31;
	} U0;
	struct {
	  struct CONTEXTTABLE *FCONST, *PWSET;
	  unsigned BITSIZE : 6;
	} U1;
	struct {
	  struct CONTEXTTABLE *DOMAIN, *ELTYPE;
	} U2;
	struct {
	  struct CONTEXTTABLE *ELSET;
	  unsigned PWBITS : 6;
	} U3;
	struct {
	  struct CONTEXTTABLE *AELTYPE, *INXTYPE;
	  Signed int LO : 18, HI : 18;
	  unsigned OPTTYP : 2;
/* p2c: pascal_p2c.pas, line 89:
 * Note: Field width for OPTTYP assumes enum OPTPWR has 4 elements [105] */
	  unsigned EXP1 : 6, EXP2 : 6;
	} U4;
	struct {
	  struct CONTEXTTABLE *FSTFLD, *RECVAR;
	} U5;
	struct CONTEXTTABLE *PELTYPE;
	struct CONTEXTTABLE *FELTYPE;
      } UU;
    } U0;
    struct {
      struct CONTEXTTABLE *CONTYPE;
/* p2c: pascal_p2c.pas, line 94: Note:
 * Field width for CONKIND assumes enum IDKINDS has 2 elements [105] */
      unsigned CONKIND : 1;
      union {
	struct {
	  struct CONTEXTTABLE *SUCC;
	  long VALUES;
	} U0;
	struct {
	  ADDRESS CADDR;
	  unsigned CLEVEL : 3;
	} U1;
      } UU;
    } U1;
    struct {
      struct CONTEXTTABLE *PROCTYPE, *FORMALS;
      unsigned PROCKIND : 1;
/* p2c: pascal_p2c.pas, line 98: Note:
 * Field width for PROCKIND assumes enum IDKINDS has 2 elements [105] */
      unsigned PROCADDR : 16, PROCLEVEL : 3;
      Signed int SEGSIZE : 18;
    } U2;
    struct {
      struct CONTEXTTABLE *VTYPE;
      unsigned VKIND : 1;
/* p2c: pascal_p2c.pas, line 101:
 * Note: Field width for VKIND assumes enum IDKINDS has 2 elements [105] */
      unsigned VADDR : 16, VLEVEL : 3;
    } U3;
    struct {
      struct CONTEXTTABLE *FLDTYPE;
      ADDRESS FLDADDR;
      unsigned BITDISPL : 6, BITWIDTH : 6;
    } U4;
    struct {
      Signed int CASESIZE : 18;
      struct CONTEXTTABLE *VARIANTS;
      unsigned TAGVAL : 1;
      union {
	struct CONTEXTTABLE *CASETYPE;
	long CASEVAL;
      } UU;
    } U5;
  } UU;
} CONTEXTTABLE;


typedef struct _REC_CSTTB {
  long VALU;
  SHRTINT INX;
} _REC_CSTTB;

typedef struct _REC_EXTAB {
  Signed int EXVAL : 18, JMPTABIX : 18;
} _REC_EXTAB;

typedef struct _REC_LABTAB {
  Signed int LABVAL : 18, FLD2 : 18, FLD3 : 18;
} _REC_LABTAB;

typedef struct _REC_UNDLAB {
  Signed int SUCC : 18, PLACE : 18;
  unsigned LFTSH : 6;
} _REC_UNDLAB;

typedef struct _REC_PTLIST {
  alfa HNAME;
  CONTEXTTABLE *PPTR;
} _REC_PTLIST;

typedef struct _REC_ERRLIST {
  Signed int POS : 18, NMR : 18;
} _REC_ERRLIST;

typedef struct _REC_DISPLAY {
  CONTEXTTABLE *FNAME;
/* p2c: pascal_p2c.pas, line 190:
 * Note: Field width for OCCUR assumes enum WHERE has 3 elements [105] */
  unsigned OCCUR : 2;
  union {
    struct {
      Signed int CDSPL : 18;
      unsigned CLEV : 3;
    } U1;
    Signed int VDSPL;
  } UU;
} _REC_DISPLAY;

typedef struct _REC_PMDFILE {
  alfa PNAME;
  Signed int PKIND : 2;
  union {
    struct {
      Signed int ACCESS : 18, TYP : 18, RLA : 18;
    } U1;
    struct {
      Signed int LINK_ : 18, SA : 18, COUNT : 18;
    } UM1;
  } UU;
} _REC_PMDFILE;


Static struct {
  ADDRESS ICO;
  boolean ERRFLAG;
  long ERRNRS[4];
  ADDRESS LOADPT, LIMCODE, LPJMPTAB;
  long EXTFLAGS;
  ADDRESS ENTRYPT;
} PARMLIST;
Static long JMPTAB[JMPMAX + 1];
Static SHRTINT JMPIX;
/*TRANSFER VECTOR FOR CALLS OF FORWARD DECLARED PROCEDURES AND
 GOTO STATEMENTS LEADING OUT OF PROCEDURES*/

Static _REC_CSTTB CSTTB[CSTMAX];
Static SHRTINT LCX;
/*CONTAINS ALL CONSTANTS C, ABS(INTBITS(C)) >= 2**17, OCCURRING IN THE
 PROCEDURE ACTUALLY BEING COMPILED TOGETHER WITH AN INDEX INTO
 UNDLAB WHERE THEIR OCCURRENCES IN THE CODE OF THIS PROCEDURE
 ARE CHAINED*/

Static SHRTINT PILEV[MAXLEVEL + 1], FILEV[MAXLEVEL + 1];
Static ADDRESS PFL[FILLIMIT + 1];
Static CONTEXTTABLE *FILPTS[FILLIMIT + 1];
Static SHRTINT PFTOP, FILTOP;
/*PFL CONTAINS THE ADDRESSES OF LOCAL CLASSES,
   PILEV CONTAINS POINTERS INTO PFL, PFTOP = TOP OF PFL.
 FILPTS CONTAINS POINTERS TO ENTRIES FOR LOCAL FILES,
   FILEV CONTAINS POINTERS INTO FILPTS, FILTOP = TOP OF FILPTS*/

Static _REC_EXTAB EXTAB[MAXEXLABS];
Static SHRTINT CEXTABIX;
/*CONTAINS THE EXPLICITLY DECLARED LABELS OF ALL PROCEDURES
 NOT YET CLOSED, TOGETHER WITH THEIR CORRESPONDING INDEX
 INTO JMPTAB*/

Static _REC_LABTAB LABTAB[MAXLABS];
Static SHRTINT CLABIX;
/*CONTAINS ALL LABELS MET SO FAR IN THE BODY OF THE PROCEDURE
 ACTUALLY BEING COMPILED, TOGETHER WITH INFORMATION WHETHER
 LABEL DEFINITION (<LABEL>:) ALREADY FOUND OR NOT (FLD2).
 IN THE FORMER CASE FLD3 CONTAINS THE CORRESPONDING ADDRESS,
 WHERE IN THE LATTER CASE FLD3 CONTAINS AN INDEX INTO UNDLAB
 WHERE THE OCCURRENCES ARE CHAINED*/

Static _REC_UNDLAB UNDLAB[UNDMAX];
Static SHRTINT CHNIX;
/*ACTS AS LISTSTRUCTURE, CHAINING OCCURRENCES OF CONSTANTS AND
 JUMPS TO NOT YET REACHED LABELS IN THE CODE OF THE PROCEDURE
 ACTUALLY BEING COMPILED.
 CHNIX = HEAD OF FREE LIST*/

Static _REC_PTLIST PTLIST[PTLIMIT + 1];
Static SHRTINT PTX;
/*PTLIST CONTAINS NAMES OF YET UNDECLARED CLASSES AND
   POINTERS TO THE CORRESPONDING POINTER-TYPE ENTRIES
 PTX = TOP OF PTLIST */

Static _REC_ERRLIST ERRLIST[10];
Static SHRTINT ERRINX, CHCNT;
Static boolean EOLFLAG, ERR;
/*ERRLIST CONTAINS THE POSITIONS AND NUMBERS OF THE ERRORS
   ON ONE LINE
   ERRINX = TOP OF ERRLIST
   CHCNT = POSITION OF LAST READ CHARACTER
 ERRLIST,ERRINX,CHCNT,EOLFLAG ARE USED BY NEXTCH
   AND ERROR.
 ERR IS SET WHENEVER ERROR HAS BEEN CALLED AND IS TESTED
   (AND RESET) BY SEVERAL PROCEDURES */

Static _REC_DISPLAY DISPLAY[DISPLIMIT + 1];
Static SHRTINT TOP, DISX;
/*TOP = TOP OF DISPLAY, DISX IS RETURNED BY SEARCH*/

Static long CODE[CODMAX + 1];
Static SHRTINT CA, CP;
Static long BUF;
Static FILE *PASCLGO;
/*CODE IS WRITTEN ONTO PASCLGO.
 BUF IS THE CURRENT ELEMENT OF CODE.
 CA = INDEX TO CODE, CP = NUMBER OF QUARTER-WORDS USED */
Static SHRTINT LASTOP, LASTI;   /*TO AVOID B XI XJ - INSTRUCTIONS*/

Static FILE *PMDFILE;
Static SHRTINT PMCTR, LASTLINK;

Static ADDRESS LC, IC;
/*LOCATION COUNTER AND INSTRUCTION COUNTER*/

Static RG3 LEVEL, RP, RP1;
/*RP,RP1 = REGISTER-POINTERS (STACK OF X-REGS)*/
Static boolean DP;
/*DP = TRUE : DECLARATION PART (USED BY NEXTCH)*/

Static boolean PRCODE, ASSCHECK, INXCHECK, DIVCHECK, STOFLCHECK, ROUNDING;
    /*COMPILER OPTIONS*/

Static ATTR GATTR;   /*GLOBAL ATTRIBUTE RECORD */



Static CONTEXTTABLE *INTPTR, *REALPTR, *ALFAPTR, *CHARPTR, *BOOLPTR, *PREDEFP,
		    *NILPTR, *UNDECPTR, *PNUMPTR, *EXTPTR, *LAMPTR;
    /*CONSTANT POINTERS*/

Static CONTEXTTABLE *NEXT, *CTPTR, *MAXCTP;
/*VARIABLES POINTING INTO CONTEXTTABLE*/

Static Char CH;
Static alfa AVAL;
Static long IVAL;
Static CWORD A;
Static SHRTINT NO, CL;
/*CH IS OUTPUT BY NEXTCH AND USED BY INSYMBOL.
   AVAL/IVAL, NO, CL  ARE OUTPUT BY INSYMBOL*/

Static SHRTINT TCT, TMAX, B6DPL, KK;
Static CONTEXTTABLE *PT;
Static long IT, IT1;
/*AUXILIARY VARIABLES, USED IN MAIN PGM AND SEVERAL PROCEDURES*/

Static long DIGITS[9];
/*0: SIN/COS, 1: EXP, 2: LN, 3: SQRT, 4: ARCTAN*/

/*CONSTANTS AND CONSTANT TABLES: */
Static long TWOTO17;
Static SHRTINT SPLITSTAT[49];
Static enum {
  IRRELSY, BEGSY, ENDSY
} ERRCL[49], TERRCL[49];
Static alfa BLANK;
Static alfa WD[33];
Static SHRTINT WNO[33], WCL[33];
Static SHRTINT WL[12];
/* WD = WORD DELIMITERS
 WNO,WCL = CORRESPONDING VALUES OF  NO  AND  CL
 WORD DELIMITERS OF LENGTH  I  START IN WD[WL[I]]  */
Static SHRTINT SYMNO[';' + 1 - '+'], SYMCL[';' + 1 - '+'];
/*NO AND CL OF EACH CHARACTER*/
Static Char JB1[8], JB2[8], JX1[8], JX2[8], REL[8], FS[8], R1[8];
Static Char SH[12];
/*ARRAYS JB1 ... SH ARE USED BY PRTCOMP*/
Static alfa INITNAM[39];


/*CONTAINS THE PREDEFINED IDENTIFIERS,
   USED FOR INITIALIZING THE CONTEXT TABLE*/

/* ------------------------------------------------------------------ */

/* VARIABLE INITIALIZATIONS */
/* VALUE section (CDC extension) omitted for p2c */

/*TWO STANDARD PROCEDURES  INSERT  AND  APPEND  ARE PROVIDED FOR
   CODE GENERATION.
 APPEND(A,B,C) SHIFTS THE CONTENTS OF A LEFT B BITS AND
   'OR'S C INTO IT. RESULT IN A. B,C UNCHANGED.
 INSERT(A,B,C) SHIFTS THE CONTENTS OF A LEFT B BITS AND
   'OR'S THEM INTO C. RESULT IN C. A,B UNCHANGED
 A,B,C ARE OF TYPE INTEGER  */


extern long INTBITS PP((Char *A));

extern Void APPEND PP((long *A, long B, long C));

extern Void INSERT PP((long A, long B, long *C));


Static Void PRTERR()
{
  SHRTINT LASTPOS = 0, FREEPOS = 1;
  SHRTINT CURRPOS, CURRNMR, F, K, FORLIM;
  _REC_ERRLIST *WITH;

  printf("%7s", " ****  ");
  FORLIM = ERRINX;
  for (K = 0; K <= FORLIM - 1; K++) {
    WITH = &ERRLIST[K];
    CURRPOS = SEXT(WITH->POS, 18);
    CURRNMR = SEXT(WITH->NMR, 18);
    if (CURRPOS == LASTPOS)
      putchar(',');
    else {
      while (FREEPOS < CURRPOS) {
	putchar(' ');
	FREEPOS++;
      }
      putchar('^');
      LASTPOS = CURRPOS;
    }
    if (CURRNMR < 10)
      F = 1;
    else {
      if (CURRNMR < 100)
	F = 2;
      else
	F = 3;
    }
    printf("%*ld", (int)F, CURRNMR);
    FREEPOS += F + 1;
  }
/* p2c: pascal_p2c.pas, line 305:
 * Note: Null character at end of sprintf control string [148] */
  printf("");
  ERRINX = 0;
}  /*PRTERR*/


Static Void NEXTCH()
{
  /*READS NEXT CHAR OF FILE INPUT,
   CONVERTS CHR(0) TO BLANK,
   PRINTS ERROR SUMMARY AFTER EACH LINE (IF ERRORS PRESENT)
   AND PRINTS ADDRESS AT BEGINNING OF LINE */
  if (EOLFLAG) {
    if (ERRINX > 0)   /*PRINT ERRORS*/
      PRTERR();
    EOLFLAG = false;
    CHCNT = 0;
    putc(' ', stdout);
    if (DP)
      printf("%6o", LC);
    else
      printf("%6o", IC);
    putc(' ', stdout);
  }  /*EOLFLAG*/
  getc(stdin);
  CH = P_peek(stdin);
  putc(CH, stdout);
  CHCNT++;
  if (CH == '\0') {
    CH = ' ';
    EOLFLAG = true;
  }
}  /*NEXTCH*/


Static Void ERROR(I)
SHRTINT I;
{
  ERR = true;
  PARMLIST.ERRFLAG = true;
  PARMLIST.ERRNRS[I / 32] = ((long)PARMLIST.ERRNRS[I / 32]) |
			    (1L << ((int)(I & 31)));
  if (ERRINX >= 9) {
    ERRLIST[9].NMR = 104;
    ERRINX = 10;
    PARMLIST.ERRNRS[3] |= 0x100;
  } else {
    ERRINX++;
    ERRLIST[ERRINX-1].NMR = I;
  }
  ERRLIST[ERRINX-1].POS = CHCNT;
}  /*ERROR*/


/* ------------------------------------------------------------------ */

/* I N S Y M B O L          READS ONE PASCAL-SYMBOL  */
/* LIST OF SYMBOL NUMBERS
 NO   CL   SYMBOL                  NO   CL   SYMBOL

  1        ID                      20        :=
  2    1   INTEGER CONST           21        BEGIN
  2    2   REAL    CONST           22        END
  2    3   ALFA    CONST           23        IF
  2    4   CHAR    CONST           24        THEN
                                   25        ELSE
                                   26        CASE
  5    1   NOT                     27        OF
  6    1   *                       28        REPEAT
  6    2   /                       29        UNTIL
  6    3   ^                       30        WHILE
  6    4   DIV                     31        DO
  6    5   MOD                     32        FOR
  7    1   +                       33   1    TO
  7    2   -                       33   2    DOWNTO
  7    3   V                       35        GOTO
  8    1   <                       36        NIL
  8    2   <=                      37        TYPE
  8    3   >=                      38   1    ARRAY
  8    4   >                       38   2    RECORD
  8    5   <>                      38   3    FILE
  8    6   =                       38   4    CLASS
  8    7   IN                      38   5    POWERSET
  9        (                       40        LABEL
 10        )                       41        CONST
 11        [                       42        PACKED
 12        ]                       43        VAR
 15        ,                       44        FUNCTION
 16        ;                       45        PROCEDURE
 17        .                       47        VALUE
 18        ^                       48        WITH
 19        :

 NOT PASCAL SYMBOLS : BLANK  '  (*  $  */

Static Void INSYMBOL()
{
  SHRTINT SCALE, EXP;
  double R, FAC, RVAL;
  SHRTINT DIGIT[20];
  /*FOR WORDLENGTH DIV 3 = 20 DIGITS*/
  SHRTINT I, K;
  boolean BT1, SIGN;
  alfa AA;
  Char CH1;
  SHRTINT FORLIM;

_L1:
  while (CH == ' ')
    NEXTCH();
  if (CH <= 'Z')   /*LETTER*/
  {  /* READ IDENTIFIER OR WORD DELIMITER */
    K = 0;
    do {
      if (K < ALFALENG) {
	K++;
	A[K-1] = CH;
      }
      NEXTCH();
    } while (CH <= '9');
    if (K >= KK)
      KK = K;
    else {
      do {
	A[KK-1] = ' ';
	KK--;
      } while (KK != K);
    }
    memcpy(AA, A, sizeof(alfa));
    FORLIM = WL[K+1];
    /* TEST FOR WORD DELIMITER */
    for (I = WL[K]; I <= FORLIM - 1; I++) {
      if (!strncmp(AA, WD[I], sizeof(alfa))) {
	NO = WNO[I];
	CL = WCL[I];
	IVAL = 0;
	goto _L2;
      }
    }
    /* IDENTIFIER */
    NO = 1;
    CL = K;
    memcpy(AVAL, AA, sizeof(alfa));
_L2:
    return;
  } else {
    if (CH <= '9') {   /*NUMBER*/
      NO = 2;
      CL = 1;
      I = 0;
      while (P_inset(CH, DIGITS)) {
	if (I < 20)   /*20 = WORDLENGTH DIV 3*/
	  DIGIT[I] = CH - '0';
	I++;
	NEXTCH();
      }
      IVAL = 0;
      if (CH == 'B') {   /*OCTAL*/
	if (I > 20)
	  ERROR(2L);
	else {
	  for (K = 0; K <= I - 1; K++)
	    IVAL = IVAL * 8 + DIGIT[K];
	}
	NEXTCH();
	/* SPECIAL CHARACTER */
	/* SPECIAL CHARACTER */
	return;
      }
      /*OCTAL*/
      if (I > 17)
	ERROR(2L);
      else {
	for (K = 0; K <= I - 1; K++) {
	  if (IVAL > MAX10) {
	    ERROR(2L);
	    IVAL = 0;
	    goto _L40;
	  }
	  IVAL = IVAL * 10 + DIGIT[K];
	}
      }
_L40:
      SCALE = 0;
      if (CH == '.') {
	NEXTCH();
	if (CH == '.')
	  CH = ':';
	else {
	  RVAL = IVAL;
	  CL = 2;
	  if (!P_inset(CH, DIGITS))
	    ERROR(3L);
	  else {
	    do {
	      R = CH - 48.0;
	      RVAL = 10.0 * RVAL + R;
	      SCALE--;
	      NEXTCH();
	    } while (P_inset(CH, DIGITS));
	  }
	}
      }
      if (CH == 'E') {
	if (SCALE == 0) {
	  RVAL = IVAL;
	  CL = 2;
	}
	SIGN = false;
	NEXTCH();
	if (CH == '+')
	  NEXTCH();
	else {
	  if (CH == '-') {
	    NEXTCH();
	    SIGN = true;
	  }
	}
	EXP = 0;
	while (P_inset(CH, DIGITS)) {
	  EXP = EXP * 10 + CH - '0';
	  NEXTCH();
	}
	if (SIGN)
	  EXP = -EXP;
	SCALE += EXP;
      }
      if (SCALE != 0) {
	R = 1.0;
	if (SCALE < 0) {
	  FAC = 0.1;
	  SCALE = -SCALE;
	} else
	  FAC = 10.0;
	do {
	  if (SCALE & 1)
	    R *= FAC;
	  FAC *= FAC;
	  SCALE /= 2;
	} while (SCALE != 0);   /*R = 10 ** SCALE*/
	RVAL *= R;
      }
      if (CL == 2)
	IVAL = INTBITS(RVAL);
      return;
    } else {
      /*NOT OCTAL*/
      if (CH == '\'') {   /* ALFA OR CHAR CONSTANT */
	NO = 2;
	K = 0;
	BT1 = false;
	do {
	  NEXTCH();
	  if (CH == '\'') {
	    NEXTCH();
	    BT1 = (CH != '\'');
	  }
	  if (!BT1) {
	    if (K == ALFALENG) {
	      ERROR(3L);
	      BT1 = true;
	    } else {
	      K++;
	      A[K-1] = CH;
	    }
	  }
	} while (!BT1);
	if (K == 1) {   /* CHAR CONST */
	  CL = 4;
	  IVAL = A[0];
	  return;
	}
	if (K >= KK)
	  KK = K;
	else {
	  do {
	    A[KK-1] = ' ';
	    KK--;
	  } while (KK != K);
	}
	memcpy(AA, A, sizeof(alfa));
	CL = 3;
	IVAL = INTBITS(AA);
	return;
      }
      /* ' */
      else {
	/* ALFA CONST */
	NO = SYMNO[CH - '+'];
	CL = SYMCL[CH - '+'];
	IVAL = 0;
	/* TEST FOR TWO-CHARACTER SYMBOL */
	if (CH == ':') {
	  NEXTCH();
	  if (CH == '=') {
	    NO = 20;
	    NEXTCH();
	  }
	  return;
	} else {
	  if (CH == '.') {
	    NEXTCH();
	    if (CH == '.') {   /* : */
	      NO = 19;
	      NEXTCH();
	    }
	    return;
	  } else {
	    if (CH == '^') {   /* SKIP COMMENT */
	      NEXTCH();
	      if (CH == '$') {
		do {
		  NEXTCH();
		  if (CH != '~') {
		    CH1 = CH;
		    NEXTCH();
		    if (CH1 == 'A')
		      ASSCHECK = (CH == '+');
		    else {
		      if (CH1 == 'C')
			PRCODE = (CH == '+');
		      else {
			if (CH1 == 'D')
			  DIVCHECK = (CH == '+');
			else {
			  if (CH1 == 'O')
			    STOFLCHECK = (CH == '+');
			  else {
			    if (CH1 == 'R')
			      ROUNDING = (CH == '+');
			    else {
			      if (CH1 == 'T') {
				ASSCHECK = (CH == '+');
				DIVCHECK = (CH == '+');
				STOFLCHECK = (CH == '+');
				ROUNDING = (CH == '+');
				INXCHECK = (CH == '+');
			      } else {
				if (CH1 == 'X')
				  INXCHECK = (CH == '+');
			      }
			    }
			  }
			}
		      }
		    }
		    if (CH != '~')
		      NEXTCH();
		  }
		} while (CH == ',');
	      }
	      while (CH != '~')
		NEXTCH();
	      NEXTCH();
	      goto _L1;
	    } else {
	      NEXTCH();
	      return;
	    }
	  }
	}
      }
    }
  }
}  /* INSYMBOL */


/* ------------------------------------------------------------------ */

Static Void SRCHREC(P)
CONTEXTTABLE *P;
{
  /* SEARCHES ONE BLOCK, RETURNS CTPTR */
  CTPTR = P;
  while (CTPTR != NULL) {
    if (!strncmp(CTPTR->NAME, AVAL, sizeof(alfa)))
      goto _L1;
    CTPTR = CTPTR->NXTEL;
  }
_L1: ;
}  /* SRCHREC */


Static Void SEARCH()
{
  /* SEARCHES CONTEXTTABLE, RETURNS CTPTR AND DISX = INDEX TO
     DISPLAY */
  SHRTINT I;

  for (I = TOP; I >= 0; I--) {
    CTPTR = DISPLAY[I].FNAME;
    while (CTPTR != NULL) {
      if (!strncmp(CTPTR->NAME, AVAL, sizeof(alfa)))
	goto _L1;
      CTPTR = CTPTR->NXTEL;
    }
  }
_L1:
  DISX = I;
}  /* SEARCH */


Static Void INCONST(V, P, NXT)
long *V;
CONTEXTTABLE *P, *NXT;
{
  /* INPUT PARAMETER : NXT. SEARCHING IS TO BEGIN AT NXT
     OUTPUT PARAMETER : V = VALUE
                        P = TYPE POINTER , P = NIL IF ERROR */
  boolean SIGN = false;
  CONTEXTTABLE *PT, *WITH;

/* p2c: pascal_p2c.pas:
 * Note: Eliminated unused assignment statement [338] */
  if (NO == 7) {
    SIGN = (CL == 2);
    if (CL <= 2) {
      INSYMBOL();
      if (NO == 1)
	ERROR(3L);
    }
  }
  if (NO == 2) {
    switch (CL) {

    case 1:
      /* blank case */
      break;

/* p2c: pascal_p2c.pas:
 * Note: Eliminated unused assignment statement [338] */
    case 2:
      break;

/* p2c: pascal_p2c.pas:
 * Note: Eliminated unused assignment statement [338] */
    case 3:
      break;

/* p2c: pascal_p2c.pas:
 * Note: Eliminated unused assignment statement [338] */
    case 4:
      break;
    }
/* p2c: pascal_p2c.pas:
 * Note: Eliminated unused assignment statement [338] */
    if (SIGN)
      *V = -IVAL;
    else
      *V = IVAL;
    INSYMBOL();
    return;
  }
  if (NO != 1) {
    ERROR(3L);
    return;
  }
  PT = CTPTR;
  SRCHREC(NXT);
  if (CTPTR == NULL)
    SEARCH();
  if (CTPTR == NULL)
    ERROR(12L);
  else {
    WITH = CTPTR;
    if ((IDCLASS)WITH->KLASS != KONST ||
	(IDKINDS)WITH->UU.U1.CONKIND == FORMAL)
      ERROR(63L);
    else {
/* p2c: pascal_p2c.pas:
 * Note: Eliminated unused assignment statement [338] */
      *V = WITH->UU.U1.UU.U0.VALUES;
    }
  }
  CTPTR = PT;
  INSYMBOL();
}  /* INCONST */


/* ------------------------------------------------------------------ */

/* CODE GENERATING ROUTINES */

Static Void NOOP()
{
  while (CP < 4) {
    APPEND(&BUF, 15L, (long)NOP);
    CP++;
  }
}  /* NOOP */


Static Void GEN15(OP, I, J, K)
SHRTINT OP, I, J, K;
{
  LASTOP = OP;
  LASTI = I;
  if (CP != 4) {
    CP++;
    APPEND(&BUF, 6L, OP);
  } else {
    CODE[CA] = BUF;
    BUF = OP;
    CP = 1;
    CA++;
    IC++;
    if (CA > CODMAX) {
      ERROR(90L);
      CA = 1;
    }
  }
  BUF = ((BUF * 8 + I) * 8 + J) * 8 + K;
}  /* GEN15 */


Static Void GEN30(OP, I, J, K)
SHRTINT OP, I, J, K;
{
  LASTOP = OP;
  LASTI = I;
  if (CP == 3) {   /*NOOP*/
    APPEND(&BUF, 15L, (long)NOP);
    CP = 4;
  }
  if (CP < 4) {
    CP += 2;
    APPEND(&BUF, 6L, OP);
  } else {
    CODE[CA] = BUF;
    BUF = OP;
    CP = 2;
    CA++;
    IC++;
    if (CA > CODMAX) {
      ERROR(90L);
      CA = 1;
    }
  }
  BUF = (BUF * 8 + I) * 8 + J;
  if (K >= 0)
    APPEND(&BUF, 18L, K);
  else
    APPEND(&BUF, 18L, K + 262143L);
}  /* GEN30 */


Static Void BXIXJ(I, J)
SHRTINT I, J;
{
  /*TO AVOID B XI XJ INSTRUCTIONS WHENEVER POSSIBLE*/
  if (J != LASTI || LASTOP < 8 || LASTOP > 37 || LASTOP == 16 ||
      LASTOP == 17 || LASTOP == 35) {
    GEN15(8L, I, J, 0L);
    return;
  }
  if (BUF >= 0)
    BUF += I * 64 - LASTI * 64;
  else {
    BUF = -BUF;
    BUF += (LASTI - 7) * 64 - (I - 7) * 64;
    BUF = -BUF;
  }
  LASTI = I;
}  /*BXIXJ*/


Static Void INS(FADR, FCP, FCA)
ADDRESS FADR;
SHRTINT FCP, FCA;
{
  if (CA != FCA)
    INSERT((long)FADR, (4 - FCP) * 15, &CODE[FCA]);
  else
    INSERT((long)FADR, (CP - FCP) * 15, &BUF);
}


Local Void OUTCH(C)
Char C;
{
  putchar(C);
}

Local Void SPACE()
{
  printf("%3c", ' ');
}


/* ------------------------------------------------------------------ */

/* ROUTINES TO PRINT CODE AND JUMPTABLE AND TO OUTPUT CODE */

Static Void PRTCOMP()
{
  /* GLOBAL ARRAYS : JB1,JB2,JX1,JX2,REL,FS,R1,SH */
  Char II, JJ, KK;
  SHRTINT C, CC, S, IT3, OP, I, J, K;
  SHRTINT M = -63, IT4 = -7;
  SHRTINT FORLIM;

  IC += -CA - 1;
  FORLIM = CA;
  for (IT3 = 1; IT3 <= FORLIM; IT3++) {
    BUF = -CODE[IT3];
    OUTCH('\0');
    OUTCH(' ');
    OUTCH(' ');
    printf("%6lo", IC + IT3);
    S = 0;
    do {
      IT = BUF;
      APPEND(&IT, S + 6, M);
      OP = -IT;
      C = OP & 7;
      IT = BUF;
      APPEND(&IT, S + 9, IT4);
      I = -IT;
      IT = BUF;
      APPEND(&IT, S + 12, IT4);
      J = -IT;
      IT = BUF;
      if (OP < 8 || OP >= 40 && C <= 2) {
	S += 30;
	APPEND(&IT, S, MASK);
      } else {
	S += 15;
	APPEND(&IT, S, IT4);
      }
      K = -IT;

      CC = OP / 8;
      II = (Char)(I + '0');
      JJ = (Char)(J + '0');
      KK = (Char)(K + '0');
      OUTCH('\0');
      printf("%.10s", BLANK);
      if (CC < 5) {
	switch (CC) {   /* CASE */

	case 0:
	  if (OP == 3) {
	    OUTCH(JX1[I]);
	    OUTCH(JX2[I]);
	    OUTCH(' ');
	    SPACE();
	    OUTCH('X');
	    OUTCH(JJ);
	    OUTCH(',');
	    printf("%6lo", K);
	  } else {
	    OUTCH(JB1[OP]);
	    OUTCH(JB2[OP]);
	    OUTCH(' ');
	    SPACE();
	    if (OP > 3) {
	      OUTCH('B');
	      OUTCH(II);
	      OUTCH(REL[OP]);
	      OUTCH('B');
	      OUTCH(JJ);
	      OUTCH(',');
	      printf("%6lo", K);
	    } else {
	      if (OP == 2) {
		OUTCH('B');
		OUTCH(II);
		OUTCH('+');
	      }
	      printf("%6lo", K);
	    }
	  }
	  break;

	/* 0 */
	case 1:
	  OUTCH('B');
	  OUTCH('X');
	  OUTCH(II);
	  SPACE();
	  if (OP > 11) {
	    OUTCH('!');
	    II = JJ;
	    JJ = KK;
	    KK = II;
	  }
	  OUTCH('X');
	  OUTCH(JJ);
	  C = OP & 3;
	  if (C != 0) {
	    OUTCH(REL[C]);
	    OUTCH('X');
	    OUTCH(KK);
	  }
	  break;
	  /* 1 */

	case 2:
	  OUTCH(SH[OP-12]);
	  OUTCH('X');
	  OUTCH(II);
	  SPACE();
	  if (OP < 18) {
	    OUTCH(JJ);
	    OUTCH(KK);
	  } else {
	    OUTCH('B');
	    OUTCH(JJ);
	    OUTCH(',');
	    OUTCH('X');
	    OUTCH(KK);
	  }
	  break;
	  /* 2 */

	case 3:
	  OUTCH(SH[OP / 2 - 12]);
	  OUTCH('X');
	  OUTCH(II);
	  SPACE();
	  OUTCH('X');
	  OUTCH(JJ);
	  if (OP & 1)
	    OUTCH('-');
	  else
	    OUTCH('+');
	  OUTCH('X');
	  OUTCH(KK);
	  break;
	  /* 3 */

	case 4:
	  C = OP & 3;
	  if (OP == 38) {
	    OUTCH('N');
	    OUTCH('O');
	  } else {
	    if (C == 3) {
	      if (OP == 35)
		OUTCH('M');
	      else
		OUTCH('C');
	      OUTCH('X');
	      OUTCH(II);
	      SPACE();
	      if (OP == 35)
		OUTCH(JJ);
	      else
		OUTCH('X');
	      OUTCH(KK);
	    } else {
	      OUTCH(FS[C]);
	      OUTCH('X');
	      OUTCH(II);
	      SPACE();
	      OUTCH('X');
	      OUTCH(JJ);
	      if (OP < 36)
		OUTCH('*');
	      else
		OUTCH('/');
	      OUTCH('X');
	      OUTCH(KK);
	    }
	  }
	  break;
	  /* 4 */
	}
      } else {
	OUTCH('S');
	OUTCH(FS[CC]);
	OUTCH(II);
	SPACE();
	OUTCH(R1[C]);
	OUTCH(JJ);
	if ((unsigned long)C < 32 && ((1L << C) & 0xa0) != 0)
	  OUTCH('-');
	else
	  OUTCH('+');
	if (C <= 2)
	  printf("%6lo", K);
	else {
	  OUTCH('B');
	  OUTCH(KK);
	}
      }
    } while (S < 60);
  }

  IC += CA + 1;
  OUTCH('\0');
}  /* PRTCOMP */


Static Void PRJMPTAB()
{
  /* PRINTS OUT JMPTAB.
    LOADPOINT OF JMPTAB IN PARMLIST.LPJMPTAB */
  SHRTINT I, FORLIM;

  if (JMPIX > 0)
    putchar('1');
  FORLIM = JMPIX;
/* p2c: pascal_p2c.pas, line 757:
 * Note: Null character at end of sprintf control string [148] */
/* p2c: pascal_p2c.pas, line 757:
 * Note: Null character at end of sprintf control string [148] */
  for (I = 0; I <= FORLIM - 1; I++)
    printf("%2c%6lo %20lo", ' ', PARMLIST.LPJMPTAB + I, JMPTAB[I]);
/* p2c: pascal_p2c.pas, line 759:
 * Note: Null character at end of sprintf control string [148] */
}  /*PRJMPTAB*/


Static Void WRITOUT()
{
  /*CALLED AT PROCEDURE END, UPDATING ITS CODE (INSERTING ADDRESSES
    OF CONSTANTS) AND WRITING OUT CODE TOGETHER WITH THE CONSTANTS
    USED IN THIS PROCEDURE*/
  SHRTINT IT2;
  long FORLIM;
  _REC_CSTTB *WITH;
  _REC_UNDLAB *WITH1;

  NOOP();
  CODE[CA] = BUF;
  FORLIM = LCX;
  for (IT = 1; IT <= FORLIM; IT++) {
    WITH = &CSTTB[IT-1];
    CA++;
    if (CA > CODMAX) {
      ERROR(90L);
      CA = 1;
    } else {
      CODE[CA] = WITH->VALU;
      IT1 = WITH->INX;
      do {
	WITH1 = &UNDLAB[IT1-1];
	INS(IC, (long)WITH1->LFTSH, (long)SEXT(WITH1->PLACE, 18));
	IT2 = IT1;
	IT1 = SEXT(WITH1->SUCC, 18);
      } while (IT1 != 0);
      UNDLAB[IT2-1].SUCC = CHNIX;
      CHNIX = WITH->INX;
      IC++;
    }
  }
  FORLIM = CA;
  for (IT = 1; IT <= FORLIM; IT++) {
    fwrite(&CODE[IT], sizeof(long), 1, PASCLGO);
/* p2c: pascal_p2c.pas, line 786:
 * Note: Discovered too late that PASCLGO should be buffered [143] */
  }
  if (!PRCODE)
    return;
  IC -= LCX;
  CA -= LCX;
  PRTCOMP();
  FORLIM = LCX;
  for (IT = 1; IT <= FORLIM; IT++) {
    CA++;
/* p2c: pascal_p2c.pas, line 792:
 * Note: Null character at end of sprintf control string [148] */
    printf("%2c%6o %20lo", ' ', IC, CODE[CA]);
    IC++;
  }
  if (!EOLFLAG)
    printf("%*c", (int)(CHCNT + 8), ' ');
}  /* WRITOUT */


/* ------------------------------------------------------------------ */

Static Void MULOPT(VAL1, EXP1, EXP2, OPT)
SHRTINT VAL1, *EXP1, *EXP2;
OPTPWR OPT;
{
  /*MULOPT DECIDES IF VAL = 2**EXP1                    : OPT = PUREP
                          = 2**EXP1*(2**EXP2+1)        : OPT = POSP
                          = 2**EXP1*(2**EXP2-1)        : OPT = NEGP
                          OTHER                        : OPT = NOOPT */
  SHRTINT E1 = 0, E2 = 1;
  SHRTINT VAL;

  *EXP1 = 0;
  *EXP2 = 0;
  if (VAL1 <= 0) {
    OPT = NOOPT;
    return;
  }
  VAL = VAL1;
  while (!(VAL & 1)) {
    VAL /= 2;
    E1++;
  }
  if (VAL == 1) {
    OPT = PUREP;
    *EXP1 = E1;
    return;
  }
  VAL /= 2;
  if (VAL & 1) {
    do {
      VAL /= 2;
      E2++;
    } while (VAL & 1);
    if (VAL > 0) {
      OPT = NOOPT;
      return;
    }
    OPT = NEGP;
    *EXP2 = E2;
    *EXP1 = E1;
    return;
  }
  do {
    VAL /= 2;
    E2++;
  } while (!(VAL & 1));
  if (VAL > 1) {
    OPT = NOOPT;
    return;
  }
  OPT = POSP;
  *EXP2 = E2;
  *EXP1 = E1;
}  /*MULOPT*/


Static SHRTINT LOG2(VAL1)
long VAL1;
{
  SHRTINT E = 1;
  long VAL = VAL1;

  while (VAL > 0) {
    VAL /= 2;
    E++;
  }
  return E;
}  /*LOG2*/


Static Void SKIP(FNO)
SHRTINT FNO;
{
  while (ERRCL[NO] == IRRELSY && FNO != NO) {
    if (NO != 38 || CL != 2) {   /*RECORD*/
      INSYMBOL();
      continue;
    }
    do {
      INSYMBOL();
      SKIP(49L);
    } while ((unsigned long)NO < 32 && ((1L << NO) & 0x4010000L) != 0);
    if (NO == 22)
      INSYMBOL();
  }
}  /* SKIP */


/* ------------------------------------------------------------------ */

Static Void PACKANDNORM(FRP)
RG3 FRP;
{
  GEN15(23L, (long)FRP, 0L, (long)FRP);
  GEN15(20L, (long)FRP, 0L, (long)FRP);
}


Static Void JUMPTO(FADR)
ADDRESS FADR;
{
  NOOP();
  GEN30(57L, 7L, 0L, IC + 1L);
  GEN30(4L, 0L, 0L, (long)FADR);
}


Static Void LDCST(FVAL)
long FVAL;
{
  /* ENTERS CONSTANT WITH VALUE FVAL INTO CONSTANTTABLE CSTTB IFF
     NOT YET PRESENT, ELSE CHAINS OCCURRENCES OF FVAL IN CODE
     (THROUGH UNDLAB)  */
  SHRTINT IT, FORLIM;
  _REC_CSTTB *WITH;
  _REC_UNDLAB *WITH1;

  RP++;
  if (RP == 6)
    ERROR(33L);
  GEN30(41L, (long)RP, 0L, 0L);
  FORLIM = LCX;
  for (IT = 0; IT <= FORLIM - 1; IT++) {
    WITH = &CSTTB[IT];
    if (WITH->VALU == FVAL) {
      if (CHNIX == 0)
	ERROR(75L);
      else {
	WITH1 = &UNDLAB[CHNIX-1];
	IT1 = SEXT(WITH1->SUCC, 18);
	WITH1->SUCC = WITH->INX;
	WITH->INX = CHNIX;
	WITH1->PLACE = CA;
	WITH1->LFTSH = CP;
	CHNIX = IT1;
      }
      goto _L10;
    }
  }
  if (LCX == CSTMAX) {
    ERROR(84L);
    goto _L10;
  }
  if (CHNIX == 0) {
    ERROR(75L);
    goto _L10;
  }
  LCX++;
  WITH = &CSTTB[LCX-1];
  WITH->VALU = FVAL;
  WITH->INX = CHNIX;
  WITH1 = &UNDLAB[CHNIX-1];
  IT1 = SEXT(WITH1->SUCC, 18);
  WITH1->SUCC = 0;
  WITH1->PLACE = CA;
  WITH1->LFTSH = CP;
  CHNIX = IT1;
_L10: ;
}  /* LDCST */


Static Void MULTCODE(FRP)
RG3 FRP;
{
  /*GENERATES CODE FOR REAL MULTIPLICATION, ASSUMING FIRST OPERAND
   IN X-RP, THE OTHER IN X-FRP; RESULT IN X-RP*/
  if (ROUNDING)
    GEN15(33L, (long)RP, (long)RP, (long)FRP);
  else
    GEN15(32L, (long)RP, (long)RP, (long)FRP);
}  /*MULTCODE*/


Static Void CHECKBNDS(FRP, FMIN, FMAX, FADR)
RG3 FRP;
long FMIN, FMAX;
ADDRESS FADR;
{
  /*DONT USE WITH FRP IN [0,7]*/
  GEN30(57L, 7L, 0L, (long)IC);
  if (FMIN != 0) {
    if (labs(FMIN) >= TWOTO17)
      LDCST(FMIN);
    else {
      RP++;
      GEN30(57L, (long)RP, 0L, FMIN);
    }
    GEN15(31L, 0L, (long)FRP, (long)RP);
    RP--;
  }
  if (labs(FMAX) >= TWOTO17)
    LDCST(FMAX);
  else {
    RP++;
    GEN30(57L, (long)RP, 0L, FMAX);
  }
  GEN15(31L, (long)RP, (long)RP, (long)FRP);
  if (FMIN != 0)
    GEN15(10L, 0L, (long)RP, 0L);
  else
    GEN15(10L, 0L, (long)RP, (long)FRP);
  GEN30(3L, 3L, 0L, (long)FADR);
  RP--;
}  /*CHECKBNDS*/


Static Void LOADBASE(FRP, FLEVEL)
RG3 FRP, FLEVEL;
{
  SHRTINT I, FORLIM;

  GEN15(46L, (long)FRP, 5L, 0L);
  FORLIM = LEVEL - FLEVEL;
  for (I = 1; I <= FORLIM - 1; I++)
    GEN15(43L, (long)FRP, (long)FRP, 0L);
}  /*LOADBASE*/


Static Void TRANSFER(FATTR, FRP)
ATTR FATTR;
SHRTINT FRP;
{
  /*DEPENDING ON THE VALUE OF FRP, TRANSFER GENERATES CODE FOR EITHER
   LOADING THE EXPRESSION DESCRIBED BY FATTR INTO REGISTER X-FRP
   OR FOR STORING X-FRP AT LOCATION DESCRIBED BY FATTR (TAKING INTO
   ACCOUNT CDC 6000 HARDWARE FEATURES)*/
  SHRTINT LRP, REG;
  ADDRESS AT;
  boolean BT;

  if (FATTR.TYPTR == NULL)
    return;
  switch (FATTR.KIND) {

  case VARBL:
    if (FRP > 5 && FATTR.UU.U0.PCKD) {
      FATTR.UU.U0.PCKD = false;
      TRANSFER(FATTR, 1L);
      IT = FATTR.UU.U0.UU.U1.BITADR + FATTR.UU.U0.UU.U1.BITSZ;
      BT = (IT != WORDLENGTH);
      if (BT)
	GEN15(16L, 1L, 0L, IT);
      GEN15(35L, 0L, 0L, (long)(WORDLENGTH - FATTR.UU.U0.UU.U1.BITSZ));
      GEN15(9L, 1L, 0L, 1L);
      GEN15(13L, FRP, FRP, 0L);
      GEN15(10L, FRP, FRP, 1L);
      if (BT)
	GEN15(16L, FRP, 0L, WORDLENGTH - IT);
      GEN15(44L, FRP, 1L, 0L);
    } else {
      LRP = FRP;
      if (FATTR.UU.U0.ACCESS == DRCT) {
	if (FRP < 5) {
	  RP++;
	  LRP = RP;
	} else {
	  if (FRP == 5)
	    ERROR(33L);
	}
	if (FATTR.UU.U0.BREG == 0 || FATTR.UU.U0.BREG == LEVEL) {
	  if (FATTR.UU.U0.BREG != 0)
	    REG = 5;
	  else
	    REG = 0;
	  GEN30(41L, LRP, REG, FATTR.UU.U0.DPLMT);
	} else {
	  LOADBASE(5, FATTR.UU.U0.BREG);
	  GEN30(42L, LRP, 5L, FATTR.UU.U0.DPLMT);
	}
      } else {
	if (FATTR.UU.U0.ACCESS == INXD) {
	  if (FATTR.UU.U0.BREG == 0 || FATTR.UU.U0.BREG == LEVEL) {
	    if (FATTR.UU.U0.BREG != 0)
	      REG = 5;
	    else
	      REG = 0;
	    if (FATTR.UU.U0.DPLMT == 0)
	      GEN15(43L, LRP, (long)RP, REG);
	    else {
	      if (REG == 0)
		GEN30(42L, LRP, (long)RP, FATTR.UU.U0.DPLMT);
	      else {
		GEN30(50L, 7L, (long)RP, FATTR.UU.U0.DPLMT);
		GEN15(46L, LRP, 7L, 5L);
	      }
	    }
	  } else {
	    GEN30(50L, 7L, (long)RP, FATTR.UU.U0.DPLMT);
	    LOADBASE(5, FATTR.UU.U0.BREG);
	    GEN15(43L, LRP, 5L, 7L);
	  }
	} else {
	  if (FATTR.UU.U0.DPLMT == 0)
	    GEN15(43L, LRP, (long)RP, 0L);
	  else
	    GEN30(42L, LRP, (long)RP, FATTR.UU.U0.DPLMT);
	}
      }
      if (FATTR.UU.U0.PCKD) {
	if (FATTR.UU.U0.UU.U1.BITADR != 0)
	  GEN15(16L, LRP, 0L, (long)FATTR.UU.U0.UU.U1.BITADR);
	GEN15(17L, LRP, 0L, (long)(WORDLENGTH - FATTR.UU.U0.UU.U1.BITSZ));
	FATTR.UU.U0.PCKD = false;
      }
    }
    break;

  case SVAL:
    if (FRP < 5) {
      RP++;
      LRP = RP;
    } else {
      if (FRP > 5)
	LRP = FRP;
      else
	ERROR(33L);
    }
    if (FATTR.UU.VAL == 0)
      GEN15(11L, LRP, 0L, 0L);
    else
      GEN30(57L, LRP, 0L, FATTR.UU.VAL);
    break;

  case LVAL:
    if (FATTR.UU.CTERM != 0) {
      GEN30(57L, 0L, 0L, FATTR.UU.CTERM);
      GEN15(30L, FRP, (long)RP, 0L);
    } else {
      if (RP != FRP)
	BXIXJ(FRP, (long)RP);
    }
    break;

  case LCOND:
    if (FATTR.UU.U3.ARITH) {
      if (FATTR.UU.U3.JMP <= 1) {
	GEN15(35L, FRP, 0L, 0L);
	AT = IC + 1;
	GEN30(3L, (long)FATTR.UU.U3.JMP, 0L, (long)AT);
	GEN30(57L, FRP, 0L, 1L);
	NOOP();
      } else {
	GEN15(35L, 0L, 0L, 1L);
	if (FATTR.UU.U3.JMP == 2)
	  GEN15(9L, FRP, 0L, (long)RP);
	else
	  GEN15(13L, FRP, 0L, (long)RP);
	GEN15(16L, FRP, 0L, 1L);
      }
    } else {
      if (FATTR.UU.U3.JMP != 0) {
	GEN15(35L, 0L, 0L, 59L);
	GEN15(15L, FRP, 0L, (long)RP);
      } else {
	if (RP != FRP)
	  BXIXJ(FRP, (long)RP);
      }
    }
    break;
  }/*CASE KIND*/
}  /*TRANSFER*/


Static Void LOADADR(FATTR, FRP)
ATTR FATTR;
SHRTINT FRP;
{
  /*GENERATES CODE FOR LOADING THE ADDRESS OF THE VARIABLE DESCRIBED
   BY FATTR INTO X-FRP*/
  SHRTINT REG;

  if (FATTR.TYPTR == NULL)
    return;
  if (FATTR.UU.U0.PCKD) {
    ERROR(87L);
    return;
  }
  switch (FATTR.UU.U0.ACCESS) {

  case DRCT:
    if (FATTR.UU.U0.BREG == 0 || FATTR.UU.U0.BREG == LEVEL) {
      if (FATTR.UU.U0.BREG != 0)
	REG = 5;
      else
	REG = 0;
      GEN30(57L, FRP, REG, FATTR.UU.U0.DPLMT);
    } else {
      LOADBASE(5, FATTR.UU.U0.BREG);
      GEN30(58L, FRP, 5L, FATTR.UU.U0.DPLMT);
    }
    break;

  case INDRCT:
    if (FATTR.UU.U0.DPLMT != 0)
      GEN30(58L, FRP, (long)RP, FATTR.UU.U0.DPLMT);
    else {
      if (RP != FRP)
	BXIXJ(FRP, (long)RP);
    }
    break;

  case INXD:
    if (FATTR.UU.U0.BREG == 0 || FATTR.UU.U0.BREG == LEVEL) {
      if (FATTR.UU.U0.BREG != 0)
	REG = 5;
      else
	REG = 0;
      if (FATTR.UU.U0.DPLMT == 0)
	GEN15(59L, FRP, (long)RP, REG);
      else {
	if (REG == 0)
	  GEN30(58L, FRP, (long)RP, FATTR.UU.U0.DPLMT);
	else {
	  GEN30(50L, 7L, (long)RP, FATTR.UU.U0.DPLMT);
	  GEN15(62L, FRP, 7L, 5L);
	}
      }
    } else {
      GEN30(50L, 7L, (long)RP, FATTR.UU.U0.DPLMT);
      LOADBASE(5, FATTR.UU.U0.BREG);
      GEN15(59L, FRP, 5L, 7L);
    }
    break;
  }
}  /*LOADADR*/


Static Void GENJP(FJPADDR)
ADDRESS FJPADDR;
{
  /* ACCORDING TO THE GLOBAL ATTRIBUTE RECORD GATTR (DEFINING AN
     EXPRESSION), THIS PROCEDURE PERFORMS CODE GENERATION FOR A
     CONDITIONAL JUMP (ADDRESS = FJPADDR), DEPENDING ON THE TRUTH
     VALUE OF THE (NECESSARILY) BOOLEAN EXPRESSION */
  if (GATTR.TYPTR != BOOLPTR) {
    ERROR(57L);
    return;
  }
  if ((long)GATTR.KIND <= (long)SVAL)
    TRANSFER(GATTR, (long)RP);
  if (GATTR.KIND != LCOND) {
    GEN30(3L, 0L, (long)RP, (long)FJPADDR);
    return;
  }
  if (GATTR.UU.U3.JMP <= 1 && GATTR.UU.U3.ARITH)
    GEN30(3L, (long)GATTR.UU.U3.JMP, 0L, (long)FJPADDR);
  else
    GEN30(3L, (long)GATTR.UU.U3.JMP, 1L, (long)FJPADDR);
}  /* GENJP */


Static Void ADDRESSVAR(FCTP, FATTR)
CONTEXTTABLE *FCTP;
ATTR *FATTR;
{
  /*GENERATES CODE TO ADDRESS THE QUANTITY WITH CTPTR = FCTP AND
   BUILDS UP ITS ATTRIBUTES IN FATTR*/
  _REC_DISPLAY *WITH2;

  /*WITH FCTP^, FATTR*/
  FATTR->KIND = VARBL;
  if ((IDCLASS)FCTP->KLASS == VARS) {
    FATTR->TYPTR = FCTP->UU.U3.VTYPE;
    FATTR->UU.U0.PCKD = false;
    if ((IDKINDS)FCTP->UU.U3.VKIND == ACTUAL) {
      FATTR->UU.U0.ACCESS = DRCT;
      FATTR->UU.U0.DPLMT = FCTP->UU.U3.VADDR;
      FATTR->UU.U0.BREG = FCTP->UU.U3.VLEVEL;
      return;
    }
    RP++;
    if (RP == 6)
      ERROR(33L);
    if (FCTP->UU.U3.VLEVEL == LEVEL)
      GEN30(41L, (long)RP, 5L, (long)FCTP->UU.U3.VADDR);
    else {
      LOADBASE(RP, FCTP->UU.U3.VLEVEL);
      GEN30(42L, (long)RP, (long)RP, (long)FCTP->UU.U3.VADDR);
    }
    FATTR->UU.U0.ACCESS = INDRCT;
    FATTR->UU.U0.DPLMT = 0;
    return;
  }
  if ((IDCLASS)FCTP->KLASS == FIELD) {
    FATTR->TYPTR = FCTP->UU.U4.FLDTYPE;
    WITH2 = &DISPLAY[DISX];
    if ((WHERE)WITH2->OCCUR == CWITH) {
      FATTR->UU.U0.ACCESS = DRCT;
      FATTR->UU.U0.BREG = WITH2->UU.U1.CLEV;
      FATTR->UU.U0.DPLMT = FCTP->UU.U4.FLDADDR + SEXT(WITH2->UU.U1.CDSPL, 18);
    } else {
      RP++;
      if (RP == 6)
	ERROR(33L);
      GEN30(41L, (long)RP, 5L, (long)WITH2->UU.VDSPL);
      FATTR->UU.U0.ACCESS = INDRCT;
      FATTR->UU.U0.DPLMT = FCTP->UU.U4.FLDADDR;
    }
    if (FCTP->UU.U4.BITWIDTH == 0) {
      FATTR->UU.U0.PCKD = false;
      return;
    }
    FATTR->UU.U0.PCKD = true;
    FATTR->UU.U0.UU.U1.BITADR = FCTP->UU.U4.BITDISPL;
    FATTR->UU.U0.UU.U1.BITSZ = FCTP->UU.U4.BITWIDTH;
    return;
  }
  FATTR->TYPTR = FCTP->UU.U2.PROCTYPE;
  FATTR->UU.U0.ACCESS = DRCT;
  FATTR->UU.U0.BREG = FCTP->UU.U2.PROCLEVEL + 1;
  FATTR->UU.U0.DPLMT = 2;
  FATTR->UU.U0.PCKD = false;
  if ((IDKINDS)FCTP->UU.U2.PROCKIND == FORMAL)
    ERROR(81L);
  else {
    if (LEVEL != FATTR->UU.U0.BREG)
      ERROR(85L);
  }
}  /*ADDRESSVAR*/


/* ------------------------------------------------------------ */

Static Void STATEMENT PV();

Static Void EXPRESSION PV();

Static Void PASSPARAMS PV();


Static Void VARIABLE()
{
  ATTR LATTR;
  CONTEXTTABLE *WITH;

  ADDRESSVAR(CTPTR, &LATTR);
  INSYMBOL();
  while ((unsigned long)NO < 32 && ((1L << NO) & 0x60800L) != 0) {   /*[,.,^*/
    if (NO == 11) {   /*[*/
      do {
	if (LATTR.TYPTR != NULL) {
	  if ((TYPFORM)LATTR.TYPTR->UU.U0.FORM != ARRAYS) {
	    ERROR(34L);
	    LATTR.TYPTR = NULL;
	  }
	}
	INSYMBOL();
	EXPRESSION();
	if (GATTR.TYPTR != NULL) {
	  if (GATTR.TYPTR->UU.U0.FORM > (long)SYMBOLIC)
	    ERROR(35L);
	  if (LATTR.TYPTR != NULL) {
	    PT = LATTR.TYPTR->UU.U0.UU.U4.INXTYPE;
	    if (((TYPFORM)GATTR.TYPTR->UU.U0.FORM == SYMBOLIC ||
		 (TYPFORM)PT->UU.U0.FORM == SYMBOLIC) && GATTR.TYPTR != PT)
	      ERROR(36L);
	    if (GATTR.KIND == SVAL) {
	      if (SEXT(LATTR.TYPTR->UU.U0.UU.U4.LO, 18) > GATTR.UU.VAL ||
		  SEXT(LATTR.TYPTR->UU.U0.UU.U4.HI, 18) < GATTR.UU.VAL)
		ERROR(99L);
	      IT = GATTR.UU.VAL;
	    } else {
	      if (GATTR.KIND == LVAL)
		IT = GATTR.UU.CTERM;
	      else {
		TRANSFER(GATTR, (long)RP);
		IT = 0;
	      }
	    }
	    WITH = LATTR.TYPTR;
	    LATTR.UU.U0.DPLMT += (IT - SEXT(WITH->UU.U0.UU.U4.LO, 18)) * SEXT(
				   WITH->UU.U0.UU.U4.AELTYPE->UU.U0.SIZE, 18);
	    if (GATTR.KIND != SVAL) {
	      if (INXCHECK)
		CHECKBNDS(RP, SEXT(WITH->UU.U0.UU.U4.LO, 18) - IT,
			  SEXT(WITH->UU.U0.UU.U4.HI, 18) - IT, INXERR);
	      if ((OPTPWR)WITH->UU.U0.UU.U4.OPTTYP == NOOPT) {
		GEN30(57L, 6L, 0L,
		      (long)SEXT(WITH->UU.U0.UU.U4.AELTYPE->UU.U0.SIZE, 18));
		GEN15(34L, (long)RP, (long)RP, 6L);
	      } else {
		if (WITH->UU.U0.UU.U4.EXP1 != 0)
		  GEN15(16L, (long)RP, 0L, (long)WITH->UU.U0.UU.U4.EXP1);
		if ((OPTPWR)WITH->UU.U0.UU.U4.OPTTYP != PUREP) {
		  GEN15(8L, 0L, (long)RP, 0L);
		  GEN15(16L, (long)RP, 0L, (long)WITH->UU.U0.UU.U4.EXP2);
		  if ((OPTPWR)WITH->UU.U0.UU.U4.OPTTYP == POSP)
		    GEN15(30L, (long)RP, (long)RP, 0L);
		  else
		    GEN15(31L, (long)RP, (long)RP, 0L);
		}
	      }
	      if (LATTR.UU.U0.ACCESS == DRCT)
		LATTR.UU.U0.ACCESS = INXD;
	      else {
		RP--;
		GEN15(30L, (long)RP, (long)RP, RP + 1L);
	      }
	    }
	  }  /*IF LATTR.TYPTR <> NIL*/
	}  /*IF GATTR.TYPTR <> NIL*/
	if (LATTR.TYPTR != NULL)   /*,*/
	  LATTR.TYPTR = LATTR.TYPTR->UU.U0.UU.U4.AELTYPE;
      } while (NO == 15);
      if (NO != 12)   /*]*/
	ERROR(37L);
      else
	INSYMBOL();
      continue;
    }
    /*IF NO = 11*/
    if (NO == 17) {   /*.*/
      INSYMBOL();
      if (NO != 1) {   /*ID*/
	ERROR(41L);
	LATTR.TYPTR = NULL;
	continue;
      }
      if (LATTR.TYPTR != NULL) {
	if ((TYPFORM)LATTR.TYPTR->UU.U0.FORM == RECORDS) {
	  SRCHREC(LATTR.TYPTR->UU.U0.UU.U5.FSTFLD);
	  if (CTPTR == NULL) {
	    ERROR(39L);
	    CTPTR = UNDECPTR;
	  }
	  WITH = CTPTR;
	  LATTR.TYPTR = WITH->UU.U4.FLDTYPE;
	  LATTR.UU.U0.DPLMT += WITH->UU.U4.FLDADDR;
	  if (WITH->UU.U4.BITWIDTH != 0) {
	    LATTR.UU.U0.UU.U1.BITADR = WITH->UU.U4.BITDISPL;
	    LATTR.UU.U0.UU.U1.BITSZ = WITH->UU.U4.BITWIDTH;
	    LATTR.UU.U0.PCKD = true;
	  } else
	    LATTR.UU.U0.PCKD = false;
	} else {
	  ERROR(38L);
	  LATTR.TYPTR = NULL;
	}
      }
      INSYMBOL();
      continue;
    }
    /*IF NO = 17*/
    if (LATTR.TYPTR != NULL) {
      TRANSFER(LATTR, (long)RP);
      LATTR.UU.U0.ACCESS = INDRCT;
      LATTR.UU.U0.DPLMT = 0;
      if ((TYPFORM)LATTR.TYPTR->UU.U0.FORM == FILES)
	LATTR.TYPTR = LATTR.TYPTR->UU.U0.UU.FELTYPE;
      else {
	if ((TYPFORM)LATTR.TYPTR->UU.U0.FORM == POINTER)
	  LATTR.TYPTR = LATTR.TYPTR->UU.U0.UU.U2.ELTYPE;
	else {
	  ERROR(40L);
	  LATTR.TYPTR = NULL;
	}
      }
    }
    INSYMBOL();
    /*^*/
    /*^*/
  }
  GATTR = LATTR;
}  /*VARIABLE*/


/* Local variables for FACTOR: */
struct LOC_FACTOR {
  ATTR LATTR;
  long LPSVAL;
} ;

Local Void ELEMENT(LINK)
struct LOC_FACTOR *LINK;
{
  /*WORKS UP THE NEXT ELEMENT IN THE SET BEEING ANALYSED*/
  EXPRESSION();
  if (GATTR.TYPTR == NULL)
    return;
  if (GATTR.TYPTR->UU.U0.FORM > (long)SYMBOLIC) {
    ERROR(35L);
    return;
  }
  if (GATTR.TYPTR == REALPTR || GATTR.TYPTR == ALFAPTR) {
    ERROR(62L);
    return;
  }
  if (LINK->LATTR.TYPTR != NULL) {
    if (((TYPFORM)LINK->LATTR.TYPTR->UU.U0.FORM != NUMERIC ||
	 (TYPFORM)GATTR.TYPTR->UU.U0.FORM != NUMERIC) &&
	LINK->LATTR.TYPTR != GATTR.TYPTR)
      ERROR(73L);
  }
  LINK->LATTR.TYPTR = GATTR.TYPTR;
  if (GATTR.KIND == SVAL) {
    INSERT(1L, GATTR.UU.VAL, &LINK->LPSVAL);
    return;
  }
  TRANSFER(GATTR, (long)RP);
  GEN15(51L, 7L, (long)RP, 0L);
  if (LINK->LATTR.KIND == SVAL) {
    GEN30(57L, (long)RP, 0L, 1L);
    GEN15(18L, (long)RP, 7L, (long)RP);
    LINK->LATTR.KIND = LVAL;
    LINK->LATTR.UU.CTERM = 0;
    return;
  }
  GEN30(57L, 0L, 0L, 1L);
  GEN15(18L, 0L, 7L, 0L);
  RP--;
  GEN15(10L, (long)RP, (long)RP, 0L);
}  /*ELEMENT*/


Static Void FACTOR()
{
  struct LOC_FACTOR V;
  CONTEXTTABLE *LPTR;
  SHRTINT LRP, LB6;
  long IT2;
  CONTEXTTABLE *WITH;
  long FORLIM;

  if (NO == 1) {   /*IDENTIFIER*/
    SEARCH();
    if (CTPTR == NULL) {
      ERROR(31L);
      CTPTR = UNDECPTR;
    }
    switch ((IDCLASS)CTPTR->KLASS) {

    case TYPES:
      ERROR(45L);
      GATTR.TYPTR = NULL;
      INSYMBOL();
      break;

    case KONST:
      WITH = CTPTR;
      GATTR.TYPTR = WITH->UU.U1.CONTYPE;
      if ((IDKINDS)WITH->UU.U1.CONKIND == ACTUAL) {
	if (labs(WITH->UU.U1.UU.U0.VALUES) >= TWOTO17) {
	  LDCST(WITH->UU.U1.UU.U0.VALUES);
	  GATTR.KIND = LVAL;
	  GATTR.UU.CTERM = 0;
	} else {
	  GATTR.KIND = SVAL;
	  GATTR.UU.VAL = WITH->UU.U1.UU.U0.VALUES;
	}
      } else {
	GATTR.KIND = VARBL;
	GATTR.UU.U0.ACCESS = DRCT;
	GATTR.UU.U0.BREG = WITH->UU.U1.UU.U1.CLEVEL;
	GATTR.UU.U0.DPLMT = WITH->UU.U1.UU.U1.CADDR;
	GATTR.UU.U0.PCKD = false;
      }
      /*FORMAL*/
      INSYMBOL();
      break;

    case PROC:
      INSYMBOL();
      if (CTPTR->UU.U2.PROCTYPE == NULL || CTPTR->UU.U2.PROCTYPE == CTPTR) {
	ERROR(46L);
	GATTR.TYPTR = NULL;
      } else {
	if (NO != 9) {   /*(*/
	  ERROR(79L);
	  GATTR.TYPTR = NULL;
	} else {
	  if (CTPTR <= PREDEFP) {   /***INLINE FCT***/
	    LPTR = CTPTR;
	    INSYMBOL();
	    EXPRESSION();
	    if (GATTR.TYPTR != NULL) {
	      switch (SEXT(LPTR->UU.U2.SEGSIZE, 18)) {

	      /*ODD*/
	      case 1:
		if ((TYPFORM)GATTR.TYPTR->UU.U0.FORM != NUMERIC)
		  ERROR(47L);
		TRANSFER(GATTR, (long)RP);
		if (GATTR.TYPTR->UU.U0.UU.U0.MIN >= 0)
		  GEN15(16L, (long)RP, 0L, 59L);
		else {
		  GEN15(8L, 0L, (long)RP, 0L);
		  GEN15(16L, 0L, 0L, 59L);
		  GEN15(11L, (long)RP, (long)RP, 0L);
		}
		GATTR.TYPTR = BOOLPTR;
		GATTR.KIND = LCOND;
		GATTR.UU.U3.JMP = 2;
		GATTR.UU.U3.ARITH = true;
		break;

	      /*INT*/
	      case 2:
		if (GATTR.TYPTR->UU.U0.FORM > (long)POWER)
		  ERROR(44L);
		GATTR.TYPTR = INTPTR;
		break;

	      /*CHR*/
	      case 3:
		if ((TYPFORM)GATTR.TYPTR->UU.U0.FORM != NUMERIC)
		  ERROR(47L);
		if (ASSCHECK) {
		  TRANSFER(GATTR, (long)RP);
		  GATTR.KIND = LVAL;
		  GATTR.UU.CTERM = 0;
		  CHECKBNDS(RP, 0L, 63L, ASSERR);
		}
		GATTR.TYPTR = CHARPTR;
		break;

	      /*EOF*/
	      case 4:
		if ((TYPFORM)GATTR.TYPTR->UU.U0.FORM != FILES)
		  ERROR(44L);
		GATTR.UU.U0.DPLMT++;
		TRANSFER(GATTR, (long)RP);
		GATTR.TYPTR = BOOLPTR;
		GATTR.KIND = LCOND;
		GATTR.UU.U3.JMP = 2;
		GATTR.UU.U3.ARITH = true;
		break;

	      /*ABS*/
	      case 5:
		if (GATTR.TYPTR != REALPTR &&
		    (TYPFORM)GATTR.TYPTR->UU.U0.FORM != NUMERIC)
		  ERROR(44L);
		TRANSFER(GATTR, (long)RP);
		GEN15(8L, 0L, (long)RP, 0L);
		GEN15(17L, 0L, 0L, 59L);
		GEN15(11L, (long)RP, 0L, (long)RP);
		if (GATTR.TYPTR != REALPTR)
		  GATTR.TYPTR = INTPTR;
		GATTR.KIND = LVAL;
		GATTR.UU.CTERM = 0;
		break;

	      /*SQR*/
	      case 6:
		if (GATTR.TYPTR != REALPTR &&
		    (TYPFORM)GATTR.TYPTR->UU.U0.FORM != NUMERIC)
		  ERROR(44L);
		TRANSFER(GATTR, (long)RP);
		if (GATTR.TYPTR == REALPTR)
		  MULTCODE(RP);
		else {
		  GEN15(34L, (long)RP, (long)RP, (long)RP);
		  GATTR.TYPTR = INTPTR;
		}
		GATTR.KIND = LVAL;
		GATTR.UU.CTERM = 0;
		break;

	      /*TRC*/
	      case 7:
		if (GATTR.TYPTR != REALPTR)
		  ERROR(44L);
		TRANSFER(GATTR, (long)RP);
		GEN15(22L, (long)RP, 7L, (long)RP);
		GEN15(18L, (long)RP, 7L, (long)RP);
		GEN15(11L, 0L, 0L, 0L);
		GEN15(30L, (long)RP, (long)RP, 0L);
		GATTR.TYPTR = INTPTR;
		GATTR.KIND = LVAL;
		GATTR.UU.CTERM = 0;
		break;

	      /*PRE*/
	      case 8:
	      case 9:
		/*SUC*/
		if (GATTR.TYPTR->UU.U0.FORM > (long)SYMBOLIC ||
		    GATTR.TYPTR == REALPTR || GATTR.TYPTR == ALFAPTR) {
		  ERROR(44L);
		  GATTR.TYPTR = NULL;
		} else {
		  TRANSFER(GATTR, (long)RP);
		  WITH = GATTR.TYPTR;
		  if ((TYPFORM)WITH->UU.U0.FORM == NUMERIC) {
		    IT1 = WITH->UU.U0.UU.U0.MIN;
		    IT2 = WITH->UU.U0.UU.U0.MAX;
		  } else {
		    IT1 = 0;
		    IT2 = WITH->UU.U0.UU.U1.FCONST->UU.U1.UU.U0.VALUES;
		  }
		  IT = SEXT(LPTR->UU.U2.SEGSIZE, 18);
		  if (labs(IT1) >= TWOTO17 || labs(IT2) >= TWOTO17) {
		    GEN30(57L, 0L, 0L, 1L);
		    GEN15(39 - IT, (long)RP, (long)RP, 0L);   /*30 OR 31*/
		  } else
		    GEN30(58L, (long)RP, (long)RP, IT * 2 - 17);
		  /*+1 OR -1*/
		  if (ASSCHECK && GATTR.TYPTR != INTPTR)
		    CHECKBNDS(RP, IT1, IT2, ASSERR);
		  GATTR.KIND = LVAL;
		  GATTR.UU.CTERM = 0;
		}
		break;
	      }/*CASE LPTR^.SEGSIZE OF*/
	    }
	    if (NO != 10) {   /*)*/
	      ERROR(48L);
	      GATTR.TYPTR = NULL;
	    } else
	      INSYMBOL();
	  }
	  /*IF CTPTR <= PREDEFP*/
	  else {
	    FORLIM = RP;
	    for (IT = 1; IT <= FORLIM; IT++) {   /***USERDEF. FCT***/
	      BXIXJ(6L, IT);
	      GEN30(41L, 6L, 5L, LC + TCT);
	      TCT++;
	      if (TCT > TMAX)
		TMAX = TCT;
	    }
	    if (CTPTR <= EXTPTR)   /* SET BIT TO INFORM MONITOR */
	      PARMLIST.EXTFLAGS = ((long)PARMLIST.EXTFLAGS) |
		  (1L << ((int)SEXT(CTPTR->UU.U2.SEGSIZE, 18)));
	    LRP = RP;
	    LB6 = B6DPL;
	    LPTR = CTPTR->UU.U2.PROCTYPE;
	    if (LB6 != 3)
	      GEN30(49L, 6L, 6L, LB6);
	    RP = 0;
	    PASSPARAMS();
	    if (LB6 != 3)
	      GEN30(49L, 6L, 6L, -LB6);
	    B6DPL = LB6;
	    for (IT = LRP; IT >= 1; IT--) {
	      TCT--;
	      GEN30(41L, IT, 5L, LC + TCT);
	    }
	    if (LRP == 5)
	      ERROR(33L);
	    else {
	      RP = LRP + 1;
	      if (LB6 == 3)
		IT = 2;
	      else
		IT = LB6 + 2;
	      GEN30(41L, (long)RP, 6L, IT);
	    }
	    GATTR.TYPTR = LPTR;
	    GATTR.KIND = LVAL;
	    GATTR.UU.CTERM = 0;
	    /***EXTERNAL FCT***/
	  }
	}
      }
      break;

    case VARS:
    case FIELD:
      VARIABLE();
      break;
    }/*CASE CTPTR^.KLASS OF*/
    return;
  }
  /*IF NO = 1*/
  if (NO == 2) {   /*CONSTANT*/
    if (labs(IVAL) >= TWOTO17) {
      LDCST(IVAL);
      GATTR.KIND = LVAL;
      GATTR.UU.CTERM = 0;
    } else {
      GATTR.KIND = SVAL;
      GATTR.UU.VAL = IVAL;
    }
    switch (CL) {

    case 1:
      GATTR.TYPTR = INTPTR;
      break;

    case 2:
      GATTR.TYPTR = REALPTR;
      break;

    case 3:
      GATTR.TYPTR = ALFAPTR;
      break;

    case 4:
      GATTR.TYPTR = CHARPTR;
      break;
    }
    INSYMBOL();
    return;
  }
  /*IF NO = 2*/
  if (NO == 36) {   /*NIL*/
    GATTR.TYPTR = NILPTR;
    GATTR.KIND = SVAL;
    GATTR.UU.VAL = NILVAL;
    INSYMBOL();
    return;
  }
  if (NO == 5) {   /*NOT*/
    INSYMBOL();
    FACTOR();
    if (GATTR.TYPTR == NULL)
      return;
    if (GATTR.TYPTR != BOOLPTR) {
      ERROR(50L);
      return;
    }
    if (GATTR.KIND == LCOND) {
      if (GATTR.UU.U3.JMP <= 1)
	GATTR.UU.U3.JMP = 1 - GATTR.UU.U3.JMP;
      else
	GATTR.UU.U3.JMP = 5 - GATTR.UU.U3.JMP;
      return;
    }
    TRANSFER(GATTR, (long)RP);
    GEN15(35L, 0L, 0L, 59L);
    GEN15(15L, (long)RP, 0L, (long)RP);
    GATTR.KIND = LVAL;
    GATTR.UU.CTERM = 0;
    return;
  }
  /*IF NO = 5*/
  if (NO == 9) {   /*(*/
    INSYMBOL();
    EXPRESSION();
    if (NO != 10) {   /*)*/
      ERROR(48L);
      GATTR.TYPTR = NULL;
    } else
      INSYMBOL();
    return;
  }
  if (NO != 11) {   /*[*/
    ERROR(42L);
    GATTR.TYPTR = NULL;
    return;
  }
  /*IF NO = 11*/
  INSYMBOL();
  if (NO == 12) {   /*]*/
    GATTR.TYPTR = LAMPTR;
    GATTR.KIND = SVAL;
    GATTR.UU.VAL = 0;
    INSYMBOL();
    return;
  }
  V.LATTR.TYPTR = NULL;
  V.LATTR.KIND = SVAL;
  V.LPSVAL = 0;
  ELEMENT(&V);
  while (NO == 15) {   /*,*/
    INSYMBOL();
    ELEMENT(&V);
  }
  if (V.LPSVAL != 0) {
    if (V.LPSVAL >= TWOTO17) {
      LDCST(V.LPSVAL);
      if (V.LATTR.KIND == LVAL) {
	RP--;
	GEN15(10L, (long)RP, (long)RP, RP + 1L);
      } else
	V.LATTR.KIND = LVAL;
      V.LATTR.UU.CTERM = 0;
    } else {
      if (V.LATTR.KIND == SVAL)
	V.LATTR.UU.VAL = V.LPSVAL;
      else {
	GEN30(57L, 0L, 0L, V.LPSVAL);
	GEN15(10L, (long)RP, (long)RP, 0L);
      }
    }
  }
  if (V.LATTR.TYPTR != NULL) {
    if ((TYPFORM)V.LATTR.TYPTR->UU.U0.FORM == NUMERIC)
      V.LATTR.TYPTR = PNUMPTR;
    else
      V.LATTR.TYPTR = V.LATTR.TYPTR->UU.U0.UU.U1.PWSET;
  }
  if (NO != 12) {   /*]*/
    ERROR(37L);
    V.LATTR.TYPTR = NULL;
  } else
    INSYMBOL();
  GATTR = V.LATTR;
}  /*FACTOR*/


Local Void CHECKDIV(FRP)
RG3 FRP;
{
  /*DONOT USE WITH FRP = 7*/
  if (GATTR.KIND == SVAL) {
    if (GATTR.UU.VAL == 0)
      ERROR(102L);
    return;
  }
  if (DIVCHECK) {
    GEN30(57L, 7L, 0L, (long)IC);
    GEN30(3L, 0L, (long)FRP, (long)DIVERR);
  }
}  /*DIVCHECK*/


Static Void TERM()
{
  ATTR LATTR;
  boolean BT1, BT2, BT3, BT4;
  SHRTINT LEXP1, LEXP2, LMOPCL;
  OPTPWR LOPT = NOOPT;

  FACTOR();
  if (NO != 6) {   /*MULOP*/
    return;
  }  /*IF NO = 6*/
  if (GATTR.KIND == SVAL && CL == 1)
    MULOPT(GATTR.UU.VAL, &LEXP1, &LEXP2, LOPT);
  if (LOPT == NOOPT) {
    TRANSFER(GATTR, (long)RP);
    LATTR.TYPTR = GATTR.TYPTR;
    LATTR.KIND = LVAL;
    LATTR.UU.CTERM = 0;
  } else
    LATTR = GATTR;
  do {
    LMOPCL = CL;
    INSYMBOL();
    FACTOR();
    if (LATTR.TYPTR != NULL && GATTR.TYPTR != NULL) {
      switch (LMOPCL) {

      /* * */
      case 1:
	BT1 = ((TYPFORM)LATTR.TYPTR->UU.U0.FORM == NUMERIC);
	BT2 = ((TYPFORM)GATTR.TYPTR->UU.U0.FORM == NUMERIC);
	BT3 = (LATTR.TYPTR == REALPTR);
	BT4 = (GATTR.TYPTR == REALPTR);
	if (LOPT != NOOPT) {
	  TRANSFER(GATTR, (long)RP);
	  if (BT4) {
	    TRANSFER(LATTR, (long)RP);
	    PACKANDNORM(RP);
	    LATTR.TYPTR = REALPTR;
	    LOPT = NOOPT;
	    BT3 = true;
	  }
	  LATTR.KIND = LVAL;
	  LATTR.UU.CTERM = 0;
	} else {
	  if (GATTR.KIND == SVAL && !BT3)
	    MULOPT(GATTR.UU.VAL, &LEXP1, &LEXP2, NOOPT);
	  if (true)
	    TRANSFER(GATTR, (long)RP);
	}
	if (LOPT == NOOPT) {
	  RP1 = RP;
	  RP--;
	}
	if (BT3 && BT4)
	  MULTCODE(RP1);
	else {
	  if (BT1 && BT2) {
	    if (LOPT == NOOPT) {
	      GEN15(34L, (long)RP, (long)RP, (long)RP1);
	      GEN15(11L, 0L, 0L, 0L);
	      GEN15(30L, (long)RP, (long)RP, 0L);
	    } else {
	      if (LEXP1 != 0)
		GEN15(16L, (long)RP, 0L, LEXP1);
	      if (LOPT != PUREP) {
		GEN15(8L, 0L, (long)RP, 0L);
		GEN15(16L, (long)RP, 0L, LEXP2);
		if (LOPT == POSP)
		  GEN15(30L, (long)RP, (long)RP, 0L);
		else
		  GEN15(31L, (long)RP, (long)RP, 0L);
	      }
	    }
	    LATTR.TYPTR = INTPTR;
	  } else {
	    if (BT1 && BT4 || BT2 && BT3) {
	      if (BT1)
		PACKANDNORM(RP);
	      else
		PACKANDNORM(RP1);
	      MULTCODE(RP1);
	      LATTR.TYPTR = REALPTR;
	    } else
	      ERROR(50L);
	  }
	}
	break;

      /* / */
      case 2:
	TRANSFER(GATTR, (long)RP);
	if ((TYPFORM)GATTR.TYPTR->UU.U0.FORM == NUMERIC) {
	  PACKANDNORM(RP);
	  GATTR.TYPTR = REALPTR;
	}
	RP--;
	if ((TYPFORM)LATTR.TYPTR->UU.U0.FORM == NUMERIC) {
	  PACKANDNORM(RP);
	  LATTR.TYPTR = REALPTR;
	}
	if (GATTR.TYPTR == REALPTR && LATTR.TYPTR == REALPTR) {
	  CHECKDIV(RP + 1);
	  if (ROUNDING)
	    GEN15(37L, (long)RP, (long)RP, RP + 1L);
	  else
	    GEN15(36L, (long)RP, (long)RP, RP + 1L);
	} else
	  ERROR(50L);
	break;

      /* AND */
      case 3:
	TRANSFER(GATTR, (long)RP);
	RP--;
	if (LATTR.TYPTR == GATTR.TYPTR &&
	    ((TYPFORM)LATTR.TYPTR->UU.U0.FORM == POWER ||
	     LATTR.TYPTR == BOOLPTR))
	  GEN15(9L, (long)RP, (long)RP, RP + 1L);
	else
	  ERROR(50L);
	break;

      /*DIV*/
      case 4:
	if (GATTR.KIND == SVAL)
	  MULOPT(GATTR.UU.VAL, &LEXP1, &LEXP2, LOPT);
	if ((TYPFORM)LATTR.TYPTR->UU.U0.FORM == NUMERIC &&
	    (TYPFORM)GATTR.TYPTR->UU.U0.FORM == NUMERIC) {
	  if (LOPT == PUREP) {
	    if (LEXP1 != 0)
	      GEN15(17L, (long)RP, 0L, LEXP1);
	    if (LATTR.TYPTR->UU.U0.UU.U0.MIN < 0) {
	      GEN15(11L, 0L, 0L, 0L);
	      GEN15(30L, (long)RP, (long)RP, 0L);
	    }
	  } else {
	    TRANSFER(GATTR, (long)RP);
	    CHECKDIV(RP);
	    PACKANDNORM(RP);
	    RP--;
	    GEN15(23L, (long)RP, 0L, (long)RP);
	    GEN15(36L, (long)RP, (long)RP, RP + 1L);
	    GEN15(22L, (long)RP, 7L, (long)RP);
	    GEN15(18L, (long)RP, 7L, (long)RP);
	    GEN15(11L, 0L, 0L, 0L);
	    GEN15(30L, (long)RP, (long)RP, 0L);
	  }
	  LATTR.TYPTR = INTPTR;
	} else
	  ERROR(50L);
	break;

      /*MOD*/
      case 5:
	if (GATTR.KIND == SVAL)
	  MULOPT(GATTR.UU.VAL, &LEXP1, &LEXP2, LOPT);
	if ((TYPFORM)LATTR.TYPTR->UU.U0.FORM == NUMERIC &&
	    (TYPFORM)GATTR.TYPTR->UU.U0.FORM == NUMERIC) {
	  if (LOPT == PUREP) {
	    GEN15(8L, 0L, (long)RP, 0L);
	    GEN15(17L, 0L, 0L, LEXP1);
	    GEN15(16L, 0L, 0L, LEXP1);
	    GEN15(31L, (long)RP, (long)RP, 0L);
	  } else {
	    TRANSFER(GATTR, (long)RP);
	    CHECKDIV(RP);
	    GEN15(23L, 6L, 0L, (long)RP);
	    GEN15(20L, 6L, 0L, 6L);
	    RP1 = RP;
	    RP--;
	    GEN15(23L, 0L, 0L, (long)RP);
	    GEN15(36L, 6L, 0L, 6L);
	    GEN15(22L, 6L, 7L, 6L);
	    GEN15(18L, 6L, 7L, 6L);
	    GEN15(34L, 6L, (long)RP1, 6L);
	    GEN15(31L, (long)RP, (long)RP, 6L);
	  }
	  LATTR.TYPTR = INTPTR;
	} else
	  ERROR(50L);
	break;
      }/*CASE LMOPCL OF*/
    }
    LOPT = NOOPT;
  } while (NO == 6);   /*MULOP*/
  GATTR = LATTR;
}  /*TERM*/


Static Void SIMPLEEXP()
{
  ATTR LATTR;
  SHRTINT LADOPCL;
  boolean LFG = false;
  boolean BT1, BT2;

  if (NO == 7) {   /*ADDOP*/
    if (CL == 2)
      LFG = true;
    else if (CL == 3)
      ERROR(51L);
    INSYMBOL();
  }
  TERM();
  if (!(LFG || NO == 7)) {
    return;
  }  /*IF LFG OR (NO = 7)*/
  LATTR.TYPTR = GATTR.TYPTR;
  LATTR.KIND = LVAL;
  LATTR.UU.CTERM = 0;
  if (LATTR.TYPTR != NULL) {
    TRANSFER(GATTR, (long)RP);
    if (LFG) {
      GEN15(11L, 0L, 0L, 0L);
      if ((TYPFORM)LATTR.TYPTR->UU.U0.FORM == NUMERIC ||
	  LATTR.TYPTR == REALPTR)
	GEN15(31L, (long)RP, 0L, (long)RP);
      else
	ERROR(50L);
    }
  }
  while (NO == 7) {
    LADOPCL = CL;
    INSYMBOL();
    TERM();
    if (LATTR.TYPTR == NULL || GATTR.TYPTR == NULL)
      continue;
    BT1 = ((TYPFORM)LATTR.TYPTR->UU.U0.FORM == NUMERIC);
    BT2 = ((TYPFORM)GATTR.TYPTR->UU.U0.FORM == NUMERIC);
    if (BT1 && BT2 && GATTR.KIND == SVAL) {
      LATTR.TYPTR = INTPTR;
      switch (LADOPCL) {

      case 1:
	LATTR.UU.CTERM += GATTR.UU.VAL;
	break;

      case 2:
	LATTR.UU.CTERM -= GATTR.UU.VAL;
	break;

      case 3:
	ERROR(50L);
	break;
      }
      continue;
    }
    TRANSFER(GATTR, (long)RP);
    RP1 = RP;
    RP--;
    if (BT1 && BT2) {
      LATTR.TYPTR = INTPTR;
      switch (LADOPCL) {

      case 1:
	GEN15(30L, (long)RP, (long)RP, (long)RP1);
	break;

      case 2:
	GEN15(31L, (long)RP, (long)RP, (long)RP1);
	break;

      case 3:
	ERROR(50L);
	break;
      }
      continue;
    }
    if (BT1) {
      PACKANDNORM(RP);
      LATTR.TYPTR = REALPTR;
    } else {
      if (BT2) {
	PACKANDNORM(RP1);
	GATTR.TYPTR = REALPTR;
      }
    }
    if (LATTR.TYPTR == REALPTR && GATTR.TYPTR == REALPTR) {
      if (LADOPCL <= 2) {
	if (ROUNDING)
	  GEN15(LADOPCL + 27, (long)RP, (long)RP, (long)RP1);
	else
	  GEN15(LADOPCL + 23, (long)RP, (long)RP, (long)RP1);
	GEN15(20L, (long)RP, 0L, (long)RP);
      } else
	ERROR(50L);
      continue;
    }
    if (LATTR.TYPTR != GATTR.TYPTR) {
      ERROR(50L);
      continue;
    }
    if (LATTR.TYPTR == BOOLPTR && LADOPCL == 3) {
      GEN15(10L, (long)RP, (long)RP, (long)RP1);
      continue;
    }
    if ((TYPFORM)LATTR.TYPTR->UU.U0.FORM != POWER) {
      ERROR(50L);
      continue;
    }
    switch (LADOPCL) {

    case 1:   /*NON-STANDARD PASCAL*/
      GEN15(11L, (long)RP, (long)RP, (long)RP1);
      break;

    case 2:
      GEN15(13L, (long)RP, (long)RP, (long)RP1);
      break;

    case 3:
      GEN15(10L, (long)RP, (long)RP, (long)RP1);
      break;
    }
  }  /*WHILE NO = 7*/
  GATTR = LATTR;
}  /*SIMPLEEXP*/


Static Void EXPRESSION()
{
  ATTR LATTR;
  SHRTINT LRELOPCL, LCST;
  boolean LOF = true;
  boolean BT1, BT2;
  ADDRESS AT;

  SIMPLEEXP();
  if (NO != 8) {   /*RELOP*/
    return;
  }  /*IF NO = 8*/
  if (GATTR.KIND == SVAL &&
      (CL == 7 || (GATTR.UU.VAL == 0 && GATTR.TYPTR != BOOLPTR &&
		   ((unsigned long)CL >= 32 || ((1L << CL) & 0xa) == 0)))) {
    LOF = false;
    LCST = GATTR.UU.VAL;
  } else {
    if (GATTR.TYPTR != NULL) {
      if (((1L << GATTR.TYPTR->UU.U0.FORM) &
	   ((1L << ((long)ARRAYS)) | (1L << ((long)RECORDS)))) != 0) {
	if (GATTR.UU.U0.ACCESS == DRCT) {
	  RP++;
	  if (RP == 6)
	    ERROR(33L);
	}
	GATTR.UU.U0.DPLMT--;
	LOADADR(GATTR, (long)RP);
      } else
	TRANSFER(GATTR, (long)RP);
    }
  }
  LATTR.TYPTR = GATTR.TYPTR;
  LATTR.KIND = LCOND;
  LRELOPCL = CL;
  INSYMBOL();
  SIMPLEEXP();
  if (LATTR.TYPTR != NULL && GATTR.TYPTR != NULL) {
    if (GATTR.KIND == SVAL && GATTR.UU.VAL == 0 && GATTR.TYPTR != BOOLPTR &&
	((unsigned long)LRELOPCL >= 32 || ((1L << LRELOPCL) & 0x14) == 0) &&
	LOF)
      LOF = false;
    else {
      if (((1L << GATTR.TYPTR->UU.U0.FORM) &
	   ((1L << ((long)RECORDS)) | (1L << ((long)ARRAYS)))) != 0) {
	GATTR.UU.U0.DPLMT--;
	LOADADR(GATTR, 6L);
	if (GATTR.UU.U0.ACCESS == DRCT)
	  RP++;
      } else
	TRANSFER(GATTR, (long)RP);
    }
    if (LOF) {
      RP1 = RP;
      RP--;
    }
    if (LRELOPCL == 7) {   /*IN*/
      if (((TYPFORM)GATTR.TYPTR->UU.U0.FORM == POWER &&
	   GATTR.TYPTR->UU.U0.UU.U3.ELSET == LATTR.TYPTR) ||
	  (GATTR.TYPTR == PNUMPTR &&
	   (TYPFORM)LATTR.TYPTR->UU.U0.FORM == NUMERIC)) {
	if (LOF) {
	  GEN15(51L, 7L, (long)RP, 0L);
	  GEN15(19L, (long)RP, 7L, (long)RP1);
	  GEN15(16L, (long)RP, 0L, 59L);
	} else
	  GEN15(16L, (long)RP, 0L, 59 - LCST);
	LATTR.UU.U3.JMP = 2;
	LATTR.UU.U3.ARITH = true;
      } else
	ERROR(50L);
    }
    /*IF LRELOPCL = 7*/
    else {
      if (LATTR.TYPTR == BOOLPTR && GATTR.TYPTR == BOOLPTR) {   /*IF*/
	switch (LRELOPCL) {

	case 1:
	  GEN15(13L, (long)RP, (long)RP1, (long)RP);
	  LATTR.UU.U3.JMP = 0;
	  break;

	case 2:
	  GEN15(13L, (long)RP, (long)RP, (long)RP1);
	  LATTR.UU.U3.JMP = 1;
	  break;

	case 3:
	  GEN15(13L, (long)RP, (long)RP1, (long)RP);
	  LATTR.UU.U3.JMP = 1;
	  break;

	case 4:
	  GEN15(13L, (long)RP, (long)RP, (long)RP1);
	  LATTR.UU.U3.JMP = 0;
	  break;

	case 5:
	  GEN15(11L, (long)RP, (long)RP, (long)RP1);
	  LATTR.UU.U3.JMP = 0;
	  break;

	case 6:
	  GEN15(11L, (long)RP, (long)RP, (long)RP1);
	  LATTR.UU.U3.JMP = 1;
	  break;
	}
	LATTR.UU.U3.ARITH = false;
      } else {
	BT1 = ((TYPFORM)LATTR.TYPTR->UU.U0.FORM == NUMERIC);
	BT2 = ((TYPFORM)GATTR.TYPTR->UU.U0.FORM == NUMERIC);
	if (BT1 && GATTR.TYPTR == REALPTR) {
	  if (LOF)
	    PACKANDNORM(RP);
	  LATTR.TYPTR = REALPTR;
	}
	if (BT2 && LATTR.TYPTR == REALPTR) {
	  if (LOF)
	    PACKANDNORM(RP1);
	  GATTR.TYPTR = REALPTR;
	}
	if (BT1 && BT2 ||
	    (LATTR.TYPTR == GATTR.TYPTR &&
	     LATTR.TYPTR->UU.U0.FORM <= (long)POINTER) ||
	    ((TYPFORM)LATTR.TYPTR->UU.U0.FORM == POINTER &&
	     GATTR.TYPTR == NILPTR) ||
	    ((TYPFORM)GATTR.TYPTR->UU.U0.FORM == POINTER &&
	     LATTR.TYPTR == NILPTR))
	{   /*IF BT1 AND BT2 OR (...*/
	  if (LATTR.TYPTR == REALPTR)
	    IT = 25;
	  else
	    IT = 31;
	  switch (LRELOPCL) {

	  case 1:
	    if (LOF)
	      GEN15(IT, (long)RP, (long)RP, (long)RP1);
	    LATTR.UU.U3.JMP = 2;
	    break;

	  case 2:
	    if (LOF)
	      GEN15(IT, (long)RP, (long)RP1, (long)RP);
	    LATTR.UU.U3.JMP = 3;
	    break;

	  case 3:
	    if (LOF)
	      GEN15(IT, (long)RP, (long)RP, (long)RP1);
	    LATTR.UU.U3.JMP = 3;
	    break;

	  case 4:
	    if (LOF)
	      GEN15(IT, (long)RP, (long)RP1, (long)RP);
	    LATTR.UU.U3.JMP = 2;
	    break;

	  case 5:
	    if (LOF)
	      GEN15(31L, 0L, (long)RP, (long)RP1);
	    else
	      BXIXJ(0L, (long)RP);
	    LATTR.UU.U3.JMP = 0;
	    break;

	  case 6:
	    if (LOF)
	      GEN15(31L, 0L, (long)RP, (long)RP1);
	    else
	      BXIXJ(0L, (long)RP);
	    LATTR.UU.U3.JMP = 1;
	    break;
	  }
	  LATTR.UU.U3.ARITH = true;
	} else {
	  if ((TYPFORM)LATTR.TYPTR->UU.U0.FORM == POWER &&
	      (TYPFORM)GATTR.TYPTR->UU.U0.FORM == POWER)
	  {   /*IF (LATTR...*/
	    if (LATTR.TYPTR == GATTR.TYPTR || LATTR.TYPTR == LAMPTR ||
		GATTR.TYPTR == LAMPTR) {
	      switch (LRELOPCL) {

	      case 1:
	      case 4:
		ERROR(88L);
		break;

	      case 2:
		if (LOF)
		  GEN15(13L, 0L, (long)RP, (long)RP1);
		else
		  ERROR(89L);
		LATTR.UU.U3.JMP = 1;
		break;

	      case 3:
		if (LOF)
		  GEN15(13L, 0L, (long)RP1, (long)RP);
		else
		  ERROR(89L);
		LATTR.UU.U3.JMP = 1;
		break;

	      case 5:
	      case 6:
		if (LOF)
		  GEN15(11L, 0L, (long)RP, (long)RP1);
		else
		  BXIXJ(0L, (long)RP);
		LATTR.UU.U3.JMP = LRELOPCL - 5;
		break;
	      }
	      LATTR.UU.U3.ARITH = true;
	    } else
	      ERROR(50L);
	  } else {
	    if (LATTR.TYPTR == GATTR.TYPTR &&
		((TYPFORM)LATTR.TYPTR->UU.U0.FORM == RECORDS ||
		 (TYPFORM)LATTR.TYPTR->UU.U0.FORM == ARRAYS)) {
	      if (LRELOPCL <= 4)
		ERROR(94L);
	      else {
		if (RP >= 4)
		  ERROR(33L);
		GEN30(49L, 7L, 0L, (long)SEXT(GATTR.TYPTR->UU.U0.SIZE, 18));
		NOOP();
		AT = IC;
		GEN15(43L, (long)RP1, 6L, 7L);
		GEN15(43L, RP1 + 1L, (long)RP, 7L);
		GEN15(31L, 0L, RP1 + 1L, (long)RP1);
		GEN30(3L, 1L, 0L, IC + 2L);
		GEN30(49L, 7L, 7L, -1L);
		GEN30(5L, 7L, 0L, (long)AT);
		NOOP();
		LATTR.KIND = LCOND;
		LATTR.UU.U3.ARITH = true;
		LATTR.UU.U3.JMP = LRELOPCL - 5;
	      }
	    } else
	      ERROR(50L);
	  }
	}
      }
    }
  }  /*IF TYPTRS <> NIL*/
  /*IF <> BOOLPTRS*/
  /*IF LRELOPCL <= 6*/
  LATTR.TYPTR = BOOLPTR;
  GATTR = LATTR;
}  /*EXPRESSION*/


/* ------------------------------------------------------------ */

Static Void PASSPARAMS()
{
  /*PERFORMS CODE GENERATION FOR PASSING PARAMETERS TO CALLED
   PROCEDURE AND JUMPING TO ITS ENTRY*/
  SHRTINT LPL;
  SHRTINT LPC = 0, LDSP = 3;
  SHRTINT IT2;
  ADDRESS LPA, AT;
  boolean LPK, BT;
  CONTEXTTABLE *LFP, *WITH;

  WITH = CTPTR;
  LPA = WITH->UU.U2.PROCADDR;
  LPL = WITH->UU.U2.PROCLEVEL;
  LFP = WITH->UU.U2.FORMALS;
  LPK = ((IDKINDS)WITH->UU.U2.PROCKIND == ACTUAL);
  if (NO == 9) {   /*(*/
    do {
      if (LFP == NULL) {
	if (LPK) {
	  ERROR(72L);
	  SKIP(49L);
	  goto _L1;
	}
	BT = false;
      } else
	BT = ((IDCLASS)LFP->KLASS == PROC);
      INSYMBOL();
      if (BT) {   /*PROC/FCT TO BE PASSED*/
	if (NO != 1)   /*ID*/
	  ERROR(11L);
	else {
	  SEARCH();
	  if (CTPTR == NULL) {
	    ERROR(31L);
	    CTPTR = UNDECPTR;
	  }
	  if ((IDCLASS)CTPTR->KLASS != PROC)
	    ERROR(60L);
	  else {
	    WITH = CTPTR;
	    /*WITH CTPTR^*/
	    PT = LFP->UU.U2.PROCTYPE;
	    if (WITH->UU.U2.PROCTYPE != CTPTR) {   /*PARAM. IS FCT*/
	      if (PT == LFP ||
		  (WITH->UU.U2.PROCTYPE != PT &&
		   ((TYPFORM)WITH->UU.U2.PROCTYPE->UU.U0.FORM != NUMERIC ||
		    (TYPFORM)PT->UU.U0.FORM != NUMERIC)))
		ERROR(60L);
	    } else {
	      if (PT != LFP)
		ERROR(60L);
	    }
	    /*PARAM. IS PROC*/
	    if (CTPTR <= PREDEFP) {
	      ERROR(83L);
	      SKIP(49L);
	      goto _L1;
	    }
	    if (CTPTR <= EXTPTR)   /* SET BIT TO INFORM MONITOR */
	      PARMLIST.EXTFLAGS = ((long)PARMLIST.EXTFLAGS) |
		  (1L << ((int)SEXT(CTPTR->UU.U2.SEGSIZE, 18)));
	    if (WITH->UU.U2.PROCTYPE != NULL) {
	      if ((IDKINDS)WITH->UU.U2.PROCKIND == ACTUAL) {
		PT = WITH->UU.U2.FORMALS;
		IT1 = 0;
		IT2 = 0;
		while (PT != NULL) {
		  if ((IDCLASS)PT->KLASS == VARS &&
		      (IDKINDS)PT->UU.U3.VKIND == FORMAL)
		    APPEND(&IT1, 1L, 1L);
		  else
		    APPEND(&IT1, 1L, 0L);
		  IT2++;
		  PT = PT->NXTEL;
		}
		APPEND(&IT1, 17 - IT2, 0L);
		if (IT2 > 17)
		  ERROR(103L);
		GEN30(57L, 6L, 0L, (long)WITH->UU.U2.PROCADDR);
		if (IT1 != 0) {
		  GEN30(57L, 0L, 0L, IT1);
		  GEN15(16L, 0L, 0L, 43L);
		  GEN15(10L, 6L, 6L, 0L);
		}
		if (WITH->UU.U2.PROCLEVEL != 0) {
		  if (WITH->UU.U2.PROCLEVEL == LEVEL)
		    GEN15(62L, 1L, 5L, 0L);
		  else
		    LOADBASE(1, WITH->UU.U2.PROCLEVEL);
		  GEN15(16L, 1L, 0L, 18L);
		  GEN15(10L, 6L, 6L, 1L);
		}
	      } else {
		if (WITH->UU.U2.PROCLEVEL == LEVEL)
		  GEN30(41L, 1L, 5L, (long)WITH->UU.U2.PROCADDR);
		else {
		  LOADBASE(1, WITH->UU.U2.PROCLEVEL);
		  GEN30(42L, 1L, 1L, (long)WITH->UU.U2.PROCADDR);
		}
		GEN15(8L, 6L, 1L, 0L);
	      }
	    }
	  }
	  /*IF CTPTR^.KLASS = PROC*/
	  INSYMBOL();
	}
      } else {
	/*NO = 1*/
	B6DPL = LDSP;
	EXPRESSION();
	if (GATTR.TYPTR != NULL) {
	  if (LPK) {   /*IF LPK*/
	    if ((IDCLASS)LFP->KLASS == KONST ||
		(IDKINDS)LFP->UU.U3.VKIND == ACTUAL) {
	      if (GATTR.KIND == VARBL) {
		TRANSFER(GATTR, (long)RP);
		GEN15(8L, 6L, (long)RP, 0L);
	      } else
		TRANSFER(GATTR, 6L);
	      if ((IDCLASS)LFP->KLASS == KONST)
		PT = LFP->UU.U1.CONTYPE;
	      else
		PT = LFP->UU.U3.VTYPE;
	      if ((TYPFORM)GATTR.TYPTR->UU.U0.FORM == NUMERIC && PT == REALPTR) {
		PACKANDNORM(6);
		GATTR.TYPTR = REALPTR;
	      }
	    } else {
	      if (GATTR.KIND == VARBL)
		LOADADR(GATTR, 6L);
	      else
		ERROR(86L);
	      PT = LFP->UU.U3.VTYPE;
	    }
	    if (PT != NULL) {
	      if (GATTR.TYPTR != PT) {
		if ((TYPFORM)GATTR.TYPTR->UU.U0.FORM != NUMERIC ||
		    (TYPFORM)PT->UU.U0.FORM != NUMERIC) {
		  if (GATTR.TYPTR != NILPTR ||
		      (TYPFORM)PT->UU.U0.FORM != POINTER) {
		    if (GATTR.TYPTR != LAMPTR ||
			(TYPFORM)PT->UU.U0.FORM != POWER)
		      ERROR(60L);
		  }
		}
	      }
	    }
	  } else {
	    if (GATTR.KIND != VARBL)
	      TRANSFER(GATTR, 6L);
	    else {
	      LOADADR(GATTR, 1L);
	      if (LPL == LEVEL)
		GEN30(41L, 2L, 5L, (long)LPA);
	      else {
		LOADBASE(2, (int)LPL);
		GEN30(42L, 2L, 2L, (long)LPA);
	      }
	      if (LPC != 0)
		GEN15(16L, 2L, 0L, LPC);
	      if (CP == 1)
		AT = IC;
	      else
		AT = IC + 1;
	      GEN30(3L, 3L, 2L, (long)AT);
	      GEN15(43L, 1L, 1L, 0L);
	      NOOP();
	      GEN15(8L, 6L, 1L, 0L);
	    }
	  }
	}  /*IF GATTR.TYPTR <> NIL*/
      }
      /*EXPRESSION TO BE PASSED*/
      /*EXP*/
      GEN30(41L, 6L, 6L, LDSP);
      LDSP++;
      LPC++;
      RP = 0;
      if (LPK)
	LFP = LFP->NXTEL;
    } while (NO == 15);   /*,*/
    if (NO != 10) {   /*)*/
      ERROR(48L);
      SKIP(49L);
    } else
      INSYMBOL();
  }  /*IF NO = 9*/
  if (LFP != NULL)
    ERROR(72L);
  if (LPK) {
    if (LPL != 0) {
      if (LPL == LEVEL)
	GEN15(62L, 6L, 5L, 0L);
      else {
	LOADBASE(1, (int)LPL);
	GEN15(8L, 6L, 1L, 0L);
      }
    }
    GEN30(57L, 7L, 0L, IC + 1L);
    GEN30(4L, 0L, 0L, (long)LPA);
  } else {
    if (LPL == LEVEL)
      GEN30(41L, 1L, 5L, (long)LPA);
    else {
      LOADBASE(1, (int)LPL);
      GEN30(42L, 1L, 1L, (long)LPA);
    }
    GEN15(51L, 7L, 1L, 0L);
    GEN15(16L, 1L, 0L, 42L);
    GEN15(59L, 6L, 1L, 0L);
    GEN30(57L, 7L, 0L, IC + 1L);
    GEN30(2L, 7L, 0L, 0L);
  }
  NOOP();
_L1:
  B6DPL = 3;
}  /*PASSPARAMS*/


Static Void ASSIGN()
{
  ATTR LATTR;
  ADDRESS AT;
  CONTEXTTABLE *WITH;

  VARIABLE();
  LATTR = GATTR;
  if (NO != 20) {   /*:=*/
    if (GATTR.TYPTR != NULL)
      ERROR(52L);
    SKIP(20L);
    if (NO != 20) {
      if (GATTR.TYPTR == NULL)
	ERROR(52L);
      goto _L1;
    }
  }
  INSYMBOL();
  EXPRESSION();
  if (GATTR.TYPTR == NULL)
    SKIP(49L);
  else {
    if (LATTR.TYPTR != NULL) {
      if (GATTR.TYPTR->UU.U0.FORM >= (long)ARRAYS) {
	GATTR.UU.U0.DPLMT--;
	LOADADR(GATTR, 2L);
	RP = 2;
      } else {
	if (GATTR.KIND == LVAL && GATTR.UU.CTERM != 0) {
	  WITH = LATTR.TYPTR;
	  if ((TYPFORM)WITH->UU.U0.FORM == NUMERIC &&
	      ((WITH->UU.U0.UU.U0.MIN < 0) ? -WITH->UU.U0.UU.U0.MIN : WITH->
		 UU.U0.UU.U0.MIN) < TWOTO17 &&
	      ((WITH->UU.U0.UU.U0.MAX < 0) ? -WITH->UU.U0.UU.U0.MAX : WITH->
		 UU.U0.UU.U0.MAX) < TWOTO17)
	    GEN30(58L, 6L, (long)RP, GATTR.UU.CTERM);
	  else
	    TRANSFER(GATTR, 6L);
	} else {
	  if (GATTR.KIND == VARBL) {
	    TRANSFER(GATTR, (long)RP);
	    GEN15(8L, 6L, (long)RP, 0L);
	  } else
	    TRANSFER(GATTR, 6L);
	}
      }
      if (GATTR.KIND != SVAL)
	RP--;
      if (LATTR.TYPTR == REALPTR &&
	  (TYPFORM)GATTR.TYPTR->UU.U0.FORM == NUMERIC)
	PACKANDNORM(6);
      else {
	if (LATTR.TYPTR != GATTR.TYPTR) {
	  if ((TYPFORM)LATTR.TYPTR->UU.U0.FORM != NUMERIC ||
	      (TYPFORM)GATTR.TYPTR->UU.U0.FORM != NUMERIC) {
	    if ((TYPFORM)LATTR.TYPTR->UU.U0.FORM != POINTER ||
		GATTR.TYPTR != NILPTR) {
	      if ((TYPFORM)LATTR.TYPTR->UU.U0.FORM != POWER ||
		  GATTR.TYPTR != LAMPTR)
		ERROR(53L);
	    }
	  }
	}
      }
      WITH = LATTR.TYPTR;
      if (WITH->UU.U0.FORM <= (long)POWER) {
	if ((TYPFORM)WITH->UU.U0.FORM == NUMERIC && LATTR.TYPTR != INTPTR) {
	  if (GATTR.KIND == SVAL) {
	    if (WITH->UU.U0.UU.U0.MIN > GATTR.UU.VAL ||
		WITH->UU.U0.UU.U0.MAX < GATTR.UU.VAL)
	      ERROR(101L);
	  } else {
	    if (ASSCHECK)
	      CHECKBNDS(6, WITH->UU.U0.UU.U0.MIN, WITH->UU.U0.UU.U0.MAX,
			ASSERR);
	  }
	} else {
	  if ((TYPFORM)WITH->UU.U0.FORM == SYMBOLIC &&
	      LATTR.TYPTR != REALPTR &&
	      LATTR.TYPTR != ALFAPTR && GATTR.KIND != SVAL && ASSCHECK)
	    CHECKBNDS(6, 0L, WITH->UU.U0.UU.U1.FCONST->UU.U1.UU.U0.VALUES,
		      ASSERR);
	}
	TRANSFER(LATTR, 6L);
      } else {
	if (WITH->UU.U0.FORM >= (long)CLASSS)
	  ERROR(53L);
	else {
	  LATTR.UU.U0.DPLMT--;
	  LOADADR(LATTR, 1L);
	  GEN30(49L, 7L, 0L, (long)SEXT(WITH->UU.U0.SIZE, 18));
	  NOOP();
	  AT = IC;
	  GEN15(43L, 3L, 2L, 7L);
	  GEN15(8L, 6L, 3L, 0L);
	  GEN15(43L, 6L, 1L, 7L);
	  GEN30(49L, 7L, 7L, -1L);
	  GEN30(5L, 0L, 7L, (long)AT);
	}
      }
    }  /*IF LATTR.TYPTR = NIL*/
  }
_L1: ;
}  /*ASSIGN*/


Static Void VARIAB()
{
  if (NO != 1) {
    ERROR(49L);
    GATTR.TYPTR = NULL;
    return;
  }
  SEARCH();
  if (CTPTR == NULL) {
    ERROR(31L);
    CTPTR = UNDECPTR;
  }
  if (CTPTR->KLASS > (long)PROC) {
    VARIABLE();
    return;
  }
  ERROR(32L);
  INSYMBOL();
  GATTR.TYPTR = NULL;
}  /*VARIAB*/


#define TXT             74


Static Void TITLE()
{
  SHRTINT I = 0;
  SHRTINT LCA, LCP;
  alfa AWORD;
  Char A[ALFALENG];

  while (CH == ' ')
    NEXTCH();
  if (CH != '\'') {
    INSYMBOL();
    ERROR(44L);
    SKIP(49L);
    goto _L10;
  }
  GEN30(57L, 7L, 0L, 0L);
  LCA = CA;
  LCP = CP;
  GEN30(57L, 1L, 0L, IC + 1L);
  GEN30(4L, 0L, 0L, (long)TXT);
  NOOP();
  CODE[CA] = BUF;
  NEXTCH();
  while (CH != '\'' && !EOLFLAG) {
    I++;
    A[I-1] = CH;
    if (I == ALFALENG) {
      memcpy(AWORD, A, sizeof(alfa));
      I = 0;
      CA++;
      IC++;
      if (CA > CODMAX) {
	ERROR(90L);
	CA = 1;
      }
      CODE[CA] = INTBITS(AWORD);
    }
    NEXTCH();
  }
  if (EOLFLAG) {
    ERROR(44L);
    INSYMBOL();
    SKIP(49L);
    goto _L10;
  }
  for (; I <= ALFALENG - 1; I++)
    A[I] = '\0';
  memcpy(AWORD, A, sizeof(alfa));
  BUF = INTBITS(AWORD);
  CA++;
  IC++;
  INS(IC, LCP, LCA);
  if (CA > CODMAX) {
    ERROR(90L);
    CA = 1;
  }
  NEXTCH();
  INSYMBOL();
  if (NO != 10) {
    ERROR(48L);
    SKIP(49L);
  } else
    INSYMBOL();
_L10: ;
}  /*TITLE*/

#undef TXT


#define EOR             71
#define GETB            75
#define PUTB            66
#define REST            70


Static Void GETPUT(LPSW)
SHRTINT LPSW;
{
  if (NO != 9) {   /* SY <> ( */
    ERROR(79L);
    SKIP(49L);
    goto _L10;
  }
  INSYMBOL();
  VARIAB();
  if (GATTR.TYPTR != NULL) {
    if ((TYPFORM)GATTR.TYPTR->UU.U0.FORM == FILES) {
      GATTR.UU.U0.DPLMT++;
      if (GATTR.UU.U0.ACCESS == DRCT) {
	if (GATTR.UU.U0.BREG == 0 || GATTR.UU.U0.BREG == LEVEL) {
	  if (GATTR.UU.U0.BREG != 0)
	    GATTR.UU.U0.BREG = 5;
	  GEN30(49L, 7L, (long)GATTR.UU.U0.BREG, GATTR.UU.U0.DPLMT);
	} else {
	  LOADBASE(5, GATTR.UU.U0.BREG);
	  GEN30(50L, 7L, 5L, GATTR.UU.U0.DPLMT);
	}
      } else
	GEN30(50L, 7L, (long)RP, GATTR.UU.U0.DPLMT);
      switch (LPSW) {

      /*EOR*/
      case 1:
	JUMPTO(EOR);
	break;

      /*GET*/
      case 2:
	if (GATTR.TYPTR->UU.U0.UU.FELTYPE == CHARPTR)
	  JUMPTO(GETC);
	else
	  JUMPTO(GETB);
	break;

      /*PUT*/
      case 3:
	if (GATTR.TYPTR->UU.U0.UU.FELTYPE == CHARPTR)
	  JUMPTO(PUTC);
	else
	  JUMPTO(PUTB);
	break;

      /*RST*/
      case 4:
	JUMPTO(REST);
	break;
      }
    } else {
      if ((TYPFORM)GATTR.TYPTR->UU.U0.FORM == POINTER && LPSW == 4) {
	TRANSFER(GATTR, (long)RP);
	GEN15(8L, 6L, (long)RP, 0L);
	RP--;
	ADDRESSVAR(GATTR.TYPTR->UU.U0.UU.U2.DOMAIN, &GATTR);
	TRANSFER(GATTR, 6L);
      } else
	ERROR(44L);
    }
  }
  if (NO != 10) {   /* SY <> ) */
    ERROR(48L);
    SKIP(49L);
  } else
    INSYMBOL();
_L10: ;
}  /* GETPUT */

#undef EOR
#undef GETB
#undef PUTB
#undef REST


Static Void ALLC()
{
  boolean BT1;
  ATTR LATTR;
  CONTEXTTABLE *WITH;

  if (NO != 9) {   /* SY <> ( */
    ERROR(79L);
    SKIP(49L);
    goto _L10;
  }
  INSYMBOL();
  VARIAB();
  PT = NULL;
  if (GATTR.TYPTR != NULL) {
    WITH = GATTR.TYPTR;
    if ((TYPFORM)WITH->UU.U0.FORM != POINTER)
      ERROR(44L);
    else {
      ADDRESSVAR(WITH->UU.U0.UU.U2.DOMAIN, &LATTR);
      TRANSFER(LATTR, (long)RP);
      IT1 = SEXT(WITH->UU.U0.UU.U2.DOMAIN->UU.U3.VTYPE->UU.U0.SIZE, 18);
      PT = WITH->UU.U0.UU.U2.ELTYPE;
    }
  }
  if (PT == NULL) {
    SKIP(49L);
    goto _L10;
  }
  if (NO == 15) {   /* SY = , */
    PT = PT->UU.U0.UU.U5.RECVAR;
    do {
      if (PT == NULL) {
	ERROR(66L);
	SKIP(49L);
	goto _L10;
      }
      INSYMBOL();
      if (NO == 1) {
	SEARCH();
	if (CTPTR == NULL) {
	  ERROR(31L);
	  SKIP(49L);
	  goto _L10;
	}
	if ((IDCLASS)CTPTR->KLASS != KONST) {
	  ERROR(63L);
	  SKIP(49L);
	  goto _L10;
	}
	WITH = PT;
	if (((TYPFORM)WITH->UU.U5.UU.CASETYPE->UU.U0.FORM == SYMBOLIC &&
	     CTPTR->UU.U1.CONTYPE != WITH->UU.U5.UU.CASETYPE) ||
	    ((TYPFORM)WITH->UU.U5.UU.CASETYPE->UU.U0.FORM == NUMERIC &&
	     CTPTR->UU.U1.CONTYPE != INTPTR)) {
	  ERROR(73L);
	  SKIP(49L);
	  goto _L10;
	}
	IVAL = CTPTR->UU.U1.UU.U0.VALUES;
      } else {
	if (NO != 2 || (unsigned long)CL < 32 && ((1L << CL) & 0xc) != 0) {
	  ERROR(63L);
	  SKIP(49L);
	  goto _L10;
	}
	WITH = PT;
	if (CL == 1 && (TYPFORM)WITH->UU.U5.UU.CASETYPE->UU.U0.FORM != NUMERIC ||
	    CL == 4 && WITH->UU.U5.UU.CASETYPE != CHARPTR) {
	  ERROR(73L);
	  SKIP(49L);
	  goto _L10;
	}
      }
      CTPTR = PT->UU.U5.VARIANTS;
      BT1 = false;
      while (CTPTR != NULL && !BT1) {
	WITH = CTPTR;
	if (WITH->UU.U5.UU.CASEVAL == IVAL)
	  BT1 = true;
	else
	  CTPTR = WITH->NXTEL;
      }
      if (CTPTR == NULL) {
	IT = SEXT(PT->UU.U5.CASESIZE, 18);
	PT = NULL;
      } else {
	IT = SEXT(CTPTR->UU.U5.CASESIZE, 18);
	PT = CTPTR->UU.U5.VARIANTS;
      }
      INSYMBOL();
    } while (NO == 15);
  } else
    IT = SEXT(PT->UU.U0.SIZE, 18);
  GEN30(58L, 6L, (long)RP, IT);
  GEN15(44L, 6L, (long)RP, 0L);
  GEN30(56L, 0L, (long)RP, IT1);
  GEN15(31L, 0L, 0L, 6L);
  GEN15(8L, 6L, (long)RP, 0L);
  GEN30(3L, 2L, 0L, IC + (CP + 1) / 2);
  GEN15(44L, 6L, (long)RP, 0L);
  GEN30(57L, 6L, 0L, NILVAL);
  NOOP();
  RP--;
  TRANSFER(GATTR, 6L);
  if (NO != 10) {   /* SY <> ) */
    ERROR(48L);
    SKIP(49L);
  } else
    INSYMBOL();
_L10: ;
}  /* ALLC */


Static Void PCK()
{
  CONTEXTTABLE *LPT;
  SHRTINT LLO, LHI;
  CONTEXTTABLE *WITH;

  if (NO != 9) {   /* SY <> ( */
    ERROR(79L);
    SKIP(49L);
    goto _L10;
  }
  INSYMBOL();
  VARIAB();
  if (GATTR.TYPTR != NULL) {
    WITH = GATTR.TYPTR;
    if ((TYPFORM)WITH->UU.U0.FORM != ARRAYS ||
	WITH->UU.U0.UU.U4.AELTYPE != CHARPTR)
      ERROR(44L);
    else {
      LPT = WITH->UU.U0.UU.U4.INXTYPE;
      GATTR.UU.U0.DPLMT += ALFALENG - SEXT(WITH->UU.U0.UU.U4.LO, 18) - 1;
      LLO = SEXT(WITH->UU.U0.UU.U4.LO, 18);
      LHI = SEXT(WITH->UU.U0.UU.U4.HI, 18) - ALFALENG + 1;
    }
    LOADADR(GATTR, 1L);
    RP = 1;
  } else
    LPT = INTPTR;
  if (NO != 15) {
    ERROR(80L);
    SKIP(49L);
    goto _L10;
  }
  INSYMBOL();
  EXPRESSION();
  if (GATTR.TYPTR != NULL) {
    if ((TYPFORM)GATTR.TYPTR->UU.U0.FORM != NUMERIC ||
	(TYPFORM)LPT->UU.U0.FORM != NUMERIC) {
      if (GATTR.TYPTR != LPT)
	ERROR(44L);
    } else {
      TRANSFER(GATTR, 2L);
      RP = 2;
      if (INXCHECK)
	CHECKBNDS(2, LLO, LHI, INXERR);
      GEN15(30L, 0L, 1L, 2L);
      GEN30(49L, 7L, 0L, (long)(-ALFALENG));
      GEN15(11L, 6L, 0L, 0L);
      NOOP();
      IT = IC;
      GEN30(49L, 7L, 7L, 1L);
      GEN15(43L, 1L, 0L, 7L);
      GEN15(16L, 6L, 0L, 6L);
      GEN15(10L, 6L, 1L, 6L);
      GEN30(5L, 7L, 0L, IT);
      RP = 0;
    }
  }
  if (NO != 15) {
    ERROR(80L);
    SKIP(49L);
    goto _L10;
  }
  INSYMBOL();
  VARIAB();
  if (GATTR.TYPTR != NULL) {
    if (GATTR.TYPTR != ALFAPTR)
      ERROR(44L);
    else
      TRANSFER(GATTR, 6L);
  }
  if (NO != 10) {   /* SY <> ) */
    ERROR(48L);
    SKIP(49L);
  } else
    INSYMBOL();
_L10: ;
}  /* PCK */


Static Void UNPCK()
{
  CONTEXTTABLE *LPT;
  SHRTINT LLO, LHI;
  CONTEXTTABLE *WITH;

  if (NO != 9) {   /* SY <> ( */
    ERROR(79L);
    SKIP(49L);
    goto _L10;
  }
  INSYMBOL();
  EXPRESSION();
  if (GATTR.TYPTR != NULL) {
    if (GATTR.TYPTR != ALFAPTR)
      ERROR(44L);
    else
      TRANSFER(GATTR, 1L);
  }
  if (NO != 15) {
    ERROR(80L);
    SKIP(49L);
    goto _L10;
  }
  INSYMBOL();
  VARIAB();
  if (GATTR.TYPTR != NULL) {
    WITH = GATTR.TYPTR;
    if ((TYPFORM)WITH->UU.U0.FORM != ARRAYS ||
	WITH->UU.U0.UU.U4.AELTYPE != CHARPTR)
      ERROR(44L);
    else {
      LPT = WITH->UU.U0.UU.U4.INXTYPE;
      GATTR.UU.U0.DPLMT += ALFALENG - SEXT(WITH->UU.U0.UU.U4.LO, 18) - 1;
      LLO = SEXT(WITH->UU.U0.UU.U4.LO, 18);
      LHI = SEXT(WITH->UU.U0.UU.U4.HI, 18) - ALFALENG + 1;
    }
    LOADADR(GATTR, 2L);
    RP = 2;
  } else
    LPT = INTPTR;
  if (NO != 15) {
    ERROR(80L);
    SKIP(49L);
    goto _L10;
  }
  INSYMBOL();
  EXPRESSION();
  if (GATTR.TYPTR != NULL) {
    if ((TYPFORM)GATTR.TYPTR->UU.U0.FORM != NUMERIC ||
	(TYPFORM)LPT->UU.U0.FORM != NUMERIC) {
      if (GATTR.TYPTR != LPT)
	ERROR(44L);
    } else {
      TRANSFER(GATTR, 3L);
      RP = 3;
      if (INXCHECK)
	CHECKBNDS(3, LLO, LHI, INXERR);
      GEN15(30L, 2L, 2L, 3L);
      GEN30(49L, 7L, 0L, (long)(-ALFALENG));
      GEN15(35L, 0L, 0L, 54L);
      NOOP();
      IT = IC;
      GEN30(49L, 7L, 7L, 1L);
      GEN15(16L, 1L, 0L, 6L);
      GEN15(13L, 6L, 1L, 0L);
      GEN15(43L, 6L, 2L, 7L);
      GEN30(5L, 7L, 0L, IT);
    }
  }
  if (NO != 10) {   /* SY <> ) */
    ERROR(48L);
    SKIP(49L);
  } else
    INSYMBOL();
_L10: ;
}  /* UNPCK */


Static Void INSAPP(LPSW)
SHRTINT LPSW;
{
  SHRTINT LVALUE;
  boolean LOF = false;

  if (NO != 9) {   /* SY <> ( */
    ERROR(79L);
    SKIP(49L);
    goto _L10;
  }
  INSYMBOL();
  if (LPSW == 8)
    EXPRESSION();
  else
    VARIAB();
  if (GATTR.TYPTR != NULL) {
    if ((TYPFORM)GATTR.TYPTR->UU.U0.FORM != NUMERIC)
      ERROR(44L);
    else
      TRANSFER(GATTR, 1L);
  }
  if (NO != 15) {
    ERROR(80L);
    SKIP(49L);
    goto _L10;
  }
  INSYMBOL();
  EXPRESSION();
  if (GATTR.TYPTR != NULL) {
    if ((TYPFORM)GATTR.TYPTR->UU.U0.FORM != NUMERIC)
      ERROR(44L);
    else {
      if (GATTR.KIND == SVAL) {
	LOF = true;
	LVALUE = GATTR.UU.VAL;
	RP = 2;
      } else
	TRANSFER(GATTR, 2L);
    }
  }
  if (NO != 15) {
    ERROR(80L);
    SKIP(49L);
    goto _L10;
  }
  INSYMBOL();
  if (LPSW == 8)
    VARIAB();
  else
    EXPRESSION();
  if (GATTR.TYPTR != NULL) {
    if ((TYPFORM)GATTR.TYPTR->UU.U0.FORM != NUMERIC)
      ERROR(44L);
    else
      TRANSFER(GATTR, 3L);
  }
  if (LOF) {
    if (LVALUE != 0)
      GEN15(16L, 1L, 0L, LVALUE);
  } else {
    GEN15(51L, 7L, 2L, 0L);
    GEN15(18L, 1L, 7L, 1L);
  }
  GEN15(10L, 6L, 1L, 3L);
  if (LPSW == 8)
    GEN15(44L, 6L, 3L, 0L);
  else
    GEN15(44L, 6L, 1L, 0L);
  if (NO != 10) {   /* SY <> ) */
    ERROR(48L);
    SKIP(49L);
  } else
    INSYMBOL();
_L10: ;
}  /* INSAPP */


#define RD              72


Static Void READIR()
{
  if (NO != 9) {   /* ( */
    ERROR(79L);
    SKIP(49L);
    goto _L10;
  }
  do {
    INSYMBOL();
    VARIAB();
    if (GATTR.TYPTR != NULL) {
      if (GATTR.TYPTR == CHARPTR) {  /* INLINE CODE */
	if (GATTR.UU.U0.ACCESS != DRCT) {
	  GEN15(8L, 6L, 1L, 0L);
	  IT = LC + TCT;
	  if (TCT == TMAX)
	    TMAX++;
	  GEN30(41L, 6L, 5L, IT);
	}
	GEN30(49L, 7L, 0L, INPT + 1L);
	JUMPTO(GETC);
	GEN30(41L, 1L, 0L, (long)INPT);
	GEN15(43L, 1L, 1L, 0L);
	GEN15(8L, 6L, 1L, 0L);
	if (GATTR.UU.U0.ACCESS != DRCT)
	  GEN30(41L, 1L, 5L, IT);
	TRANSFER(GATTR, 6L);
      } else {
	LOADADR(GATTR, 1L);
	if ((TYPFORM)GATTR.TYPTR->UU.U0.FORM == NUMERIC)
	  GEN15(54L, 7L, 0L, 0L);
	else {
	  if (GATTR.TYPTR == REALPTR)
	    GEN30(49L, 7L, 0L, 1L);
	  else
	    ERROR(44L);
	}
	JUMPTO(RD);
      }
    }
    RP = 0;   /* , */
  } while (NO == 15);
  if (NO != 10) {   /* ) */
    ERROR(48L);
    SKIP(49L);
  } else
    INSYMBOL();
_L10: ;
}  /*READIR*/

#undef RD


#define WRTI            76
#define WRTE            77
#define WRTF            78
#define WRTA            79
#define WRTC            80
#define WRTB            81
#define WRTO            73


/* Local variables for WRITEIR: */
struct LOC_WRITEIR {
  boolean LX2;
} ;

Local Void SETX2ANDJMP(FDEFVAL, FADR, LINK)
long FDEFVAL;
ADDRESS FADR;
struct LOC_WRITEIR *LINK;
{
  if (LINK->LX2)
    GEN30(57L, 2L, 0L, FDEFVAL);
  JUMPTO(FADR);
}

Local Void LOADEXP(LINK)
struct LOC_WRITEIR *LINK;
{
  INSYMBOL();
  EXPRESSION();
  if (GATTR.TYPTR != NULL) {
    if ((TYPFORM)GATTR.TYPTR->UU.U0.FORM != NUMERIC)
      ERROR(62L);
  }
  TRANSFER(GATTR, (long)RP);
}


Static Void WRITEIR()
{
  struct LOC_WRITEIR V;
  CONTEXTTABLE *LPTR;

  if (NO != 9) {   /*(*/
    ERROR(79L);
    SKIP(49L);
    goto _L10;
  }
  do {
    INSYMBOL();
    EXPRESSION();
    TRANSFER(GATTR, (long)RP);
    LPTR = GATTR.TYPTR;
    if (NO == 19) {   /*:*/
      LOADEXP(&V);
      V.LX2 = false;
    } else
      V.LX2 = true;
    /*X2 NOT YET SET*/
    if (LPTR != NULL) {
      if (NO == 1 && !strncmp(AVAL, "OCT       ", sizeof(alfa)) &&
	  LPTR->UU.U0.FORM <= (long)POWER)
      {   /*O-FORMAT*/
	SETX2ANDJMP(20L, WRTO, &V);
	INSYMBOL();
      } else {
	if ((TYPFORM)LPTR->UU.U0.FORM == NUMERIC)
	  SETX2ANDJMP(10L, WRTI, &V);
	else {
	  if (LPTR == REALPTR) {
	    if (NO == 19) {   /*:*/
	      LOADEXP(&V);
	      SETX2ANDJMP(20L, WRTF, &V);
	    } else
	      SETX2ANDJMP(20L, WRTE, &V);
	  } else {
	    if (LPTR == ALFAPTR)
	      SETX2ANDJMP(10L, WRTA, &V);
	    else {
	      if (LPTR == BOOLPTR)
		SETX2ANDJMP(10L, WRTB, &V);
	      else {
		if (LPTR == CHARPTR) {
		  if (V.LX2) {   /*CALL PUTC ONLY*/
		    BXIXJ(6L, 1L);
		    GEN30(41L, 1L, 0L, (long)OUTPT);
		    GEN15(43L, 6L, 1L, 0L);
		    GEN30(49L, 7L, 0L, OUTPT + 1L);
		    JUMPTO(PUTC);
		  } else
		    JUMPTO(WRTC);
		} else
		  ERROR(44L);
	      }
	    }
	  }
	}
      }
    }
    RP = 0;
  } while (NO == 15);   /*,*/
  if (NO != 10) {   /*)*/
    ERROR(48L);
    SKIP(49L);
  } else
    INSYMBOL();
_L10: ;
}  /*WRITEIR*/

#undef WRTI
#undef WRTE
#undef WRTF
#undef WRTA
#undef WRTC
#undef WRTB
#undef WRTO


Static Void IFSTAT()
{
  SHRTINT LCA1, LCA2, LCP1, LCP2;

  INSYMBOL();
  EXPRESSION();
  if (GATTR.TYPTR != NULL)
    GENJP(0);
  LCA1 = CA;
  LCP1 = CP;
  if (NO != 24) {   /* SY <> THEN */
    if (GATTR.TYPTR != NULL)
      ERROR(56L);
    SKIP(24L);
    if (NO != 24) {
      if (GATTR.TYPTR == NULL)
	ERROR(56L);
      if (ERRCL[NO] == ENDSY)
	goto _L30;
      goto _L20;
    }
  }
  INSYMBOL();
_L20:
  STATEMENT();
_L30:
  if (NO != 25) {   /* SY <> ELSE */
    NOOP();
    INS(IC, LCP1, LCA1);
    return;
  }
  GEN30(4L, 0L, 0L, 0L);
  LCA2 = CA;
  LCP2 = CP;
  NOOP();
  INS(IC, LCP1, LCA1);
  INSYMBOL();
  STATEMENT();
  NOOP();
  INS(IC, LCP2, LCA2);
}  /* IFSTAT */


typedef struct LCSLABS {
  struct LCSLABS *NEXT;
  Signed int CSLAB : 18;
  unsigned short ADDR;
} LCSLABS;


/* Local variables for CASESTAT: */
struct LOC_CASESTAT {
  SHRTINT LMIN, LMAX;
  LCSLABS *LPTR;
  CONTEXTTABLE *LCTP;
} ;

Local Void ACASE(LINK)
struct LOC_CASESTAT *LINK;
{
  SHRTINT LCA, LCP;
  boolean LERRFG = true;
  LCSLABS *PT1, *PT2, *PT3;
  CONTEXTTABLE *WITH;

  NOOP();
  do {
    if ((unsigned long)NO < 32 && ((1L << NO) & 0x8008000L) != 0)   /*, OF*/
      INSYMBOL();
    if (NO == 1) {   /*ID*/
      SEARCH();
      if (CTPTR == NULL) {
	ERROR(31L);
	CTPTR = UNDECPTR;
      }
      WITH = CTPTR;
      if ((IDCLASS)WITH->KLASS != KONST) {
	if (LERRFG)
	  ERROR(61L);
	goto _L1;
      }
      if (WITH->UU.U1.CONTYPE != NULL) {
	if ((IDKINDS)WITH->UU.U1.CONKIND == FORMAL) {
	  ERROR(61L);
	  goto _L1;
	}
	if (LINK->LCTP == NULL)
	  LINK->LCTP = WITH->UU.U1.CONTYPE;
	else {
	  if (LINK->LCTP != WITH->UU.U1.CONTYPE)
	    ERROR(73L);
	}
	IT = WITH->UU.U1.UU.U0.VALUES;
      }
    }
    /*IF NO = 1*/
    else {
      if (NO != 2) {   /*CONST*/
	if (LERRFG)
	  ERROR(61L);
	goto _L1;
      }
      /*IF NO = 2*/
      switch (CL) {

      case 1:
	PT = INTPTR;
	break;

      case 2:
	PT = REALPTR;
	break;

      case 3:
	PT = ALFAPTR;
	break;

      case 4:
	PT = CHARPTR;
	break;
      }
      if (LINK->LCTP == NULL)
	LINK->LCTP = PT;
      else {
	if (LINK->LCTP != PT)
	  ERROR(73L);
      }
      IT = IVAL;
    }
    if (IT >= TWOTO17)
      ERROR(100L);
    LERRFG = false;
    PT1 = LINK->LPTR;
    PT2 = NULL;
    if (!ERR) {
      while (PT1 != NULL) {
	if (SEXT(PT1->CSLAB, 18) >= IT) {
	  if (SEXT(PT1->CSLAB, 18) == IT)
	    ERROR(77L);
	  goto _L3;
	}
	PT2 = PT1;
	PT1 = PT1->NEXT;
      }
    }
    LINK->LMAX = IT;
_L3:
    ALLOC(PT3);
/* p2c: pascal_p2c.pas, line 2373:
 * Warning: Symbol 'ALLOC' is not defined [221] */
    if (PT3 != NULL) {
      PT3->NEXT = PT1;
      PT3->CSLAB = IT;
      PT3->ADDR = IC;
      if (PT2 == NULL) {
	LINK->LPTR = PT3;
	LINK->LMIN = IT;
      } else
	PT2->NEXT = PT3;
    } else
      ERROR(71L);
    INSYMBOL();
  } while (NO == 15);   /*,*/
  if (NO != 19)   /*:*/
    ERROR(64L);
  else
    INSYMBOL();
_L1:
  STATEMENT();
  GEN30(4L, 0L, 0L, 0L);
  LCA = CA;
  LCP = CP;
  if (NO == 16) {   /*;*/
    INSYMBOL();
    if (NO != 22)
      ACASE(LINK);
  } else {
    if (ERRCL[NO] == BEGSY) {
      ERROR(58L);
      goto _L1;
    }
    if (NO != 22)
      ERROR(68L);
  }
  INS((int)(IC + LINK->LMAX - LINK->LMIN + 1), LCP, LCA);
}  /*ACASE*/


Static Void CASESTAT()
{
  struct LOC_CASESTAT V;
  SHRTINT LCA, LCP, LCA1, LCP1, LCA2, LCP2;
  LCSLABS *PT1;
  boolean LERR;

  V.LMIN = 0;
  V.LMAX = 0;
  V.LPTR = NULL;
  LERR = ERR;
  ERR = false;
  INSYMBOL();
  EXPRESSION();
  V.LCTP = GATTR.TYPTR;
  if (V.LCTP != NULL) {
    if ((TYPFORM)V.LCTP->UU.U0.FORM == NUMERIC)
      V.LCTP = INTPTR;
    else {
      if (V.LCTP->UU.U0.FORM > (long)SYMBOLIC || V.LCTP == REALPTR ||
	  V.LCTP == ALFAPTR) {
	ERROR(62L);
	V.LCTP = NULL;
      }
    }
    TRANSFER(GATTR, (long)RP);
  }
  if (INXCHECK) {
    GEN30(57L, 7L, 0L, (long)IC);
    GEN30(57L, 2L, 0L, 0L);
    LCA1 = CA;
    LCP1 = CP;
    GEN15(31L, 0L, 1L, 2L);
    GEN30(57L, 2L, 0L, 0L);
    LCA2 = CA;
    LCP2 = CP;
    GEN15(31L, 2L, 2L, 1L);
    GEN15(10L, 2L, 2L, 0L);
    GEN30(3L, 3L, 2L, (long)INXERR);
  }
  GEN15(51L, 7L, 1L, 0L);
  GEN30(2L, 7L, 0L, 0L);
  LCA = CA;
  LCP = CP;
  if (NO != 27) {   /*OF*/
    if (V.LCTP != NULL)
      ERROR(65L);
    SKIP(27L);
    if (NO != 27) {
      if (V.LCTP == NULL)
	ERROR(65L);
    }
  }
  ACASE(&V);
  INS(IC - V.LMIN, LCP, LCA);
  PT1 = V.LPTR;
  if (INXCHECK) {
    INS((int)V.LMIN, LCP1, LCA1);
    INS((int)V.LMAX, LCP2, LCA2);
  }
  IT = V.LMIN;
  IT1 = IC + V.LMAX - V.LMIN + 1;
  NOOP();
  if (!ERR) {
    while (PT1 != NULL) {
      while (SEXT(PT1->CSLAB, 18) != IT) {
	if (INXCHECK) {
	  GEN30(57L, 7L, 0L, (long)IC);
	  GEN30(4L, 0L, 0L, (long)INXERR);
	} else {
	  GEN30(4L, 0L, 0L, IT1);
	  NOOP();
	}
	IT++;
      }
      GEN30(4L, 0L, 0L, (long)PT1->ADDR);
      NOOP();
      PT1 = PT1->NEXT;
      IT++;
    }
  }
  if (NO == 22)   /*END*/
    INSYMBOL();
  ERR = (ERR || LERR);
}  /*CASESTAT*/


Static Void REPEATSTAT()
{
  ADDRESS LJPADDR;

  NOOP();
  LJPADDR = IC;
  do {
    INSYMBOL();
_L20:
    STATEMENT();
    if (ERRCL[NO] == BEGSY) {
      ERROR(58L);
      goto _L20;
    }
    if (NO == 25) {   /*ELSE*/
      ERROR(54L);
      INSYMBOL();
      goto _L20;
    }
  } while (NO == 16);   /* SY <> ; */
  if (NO != 29) {   /* SY <> UNTIL */
    ERROR(67L);
    return;
  }
  INSYMBOL();
  EXPRESSION();
  if (GATTR.TYPTR != NULL)
    GENJP(LJPADDR);
  else
    SKIP(49L);
}  /* REPEATSTAT */


Static Void WHILESTAT()
{
  ADDRESS LJPADDR;
  SHRTINT LCA, LCP;

  NOOP();
  LJPADDR = IC;
  INSYMBOL();
  EXPRESSION();
  if (GATTR.TYPTR != NULL)
    GENJP(0);
  LCA = CA;
  LCP = CP;
  if (NO != 31) {   /* SY <> DO */
    if (GATTR.TYPTR != NULL)
      ERROR(59L);
    SKIP(31L);
    if (NO != 31) {
      if (GATTR.TYPTR == NULL)
	ERROR(59L);
      if (ERRCL[NO] == BEGSY)
	goto _L20;
      goto _L10;
    }
  }
  INSYMBOL();
_L20:
  STATEMENT();
  GEN30(4L, 0L, 0L, (long)LJPADDR);
_L10:
  NOOP();
  INS(IC, LCP, LCA);
}  /* WHILESTAT */


/* Local variables for FORSTAT: */
struct LOC_FORSTAT {
  ATTR LATTR;
} ;

Local Void CHTYPE(LINK)
struct LOC_FORSTAT *LINK;
{
  if (GATTR.TYPTR == NULL)
    return;
  if (GATTR.TYPTR->UU.U0.FORM > (long)SYMBOLIC || GATTR.TYPTR == REALPTR ||
      GATTR.TYPTR == ALFAPTR) {
    ERROR(62L);
    GATTR.TYPTR = NULL;
  }
}

Local Void CHTYPES(LINK)
struct LOC_FORSTAT *LINK;
{
  if (GATTR.TYPTR == NULL || LINK->LATTR.TYPTR == NULL)
    return;
  if (((TYPFORM)GATTR.TYPTR->UU.U0.FORM == SYMBOLIC ||
       (TYPFORM)LINK->LATTR.TYPTR->UU.U0.FORM == SYMBOLIC) &&
      GATTR.TYPTR != LINK->LATTR.TYPTR) {
    ERROR(73L);
    GATTR.TYPTR = NULL;
  }
}


Static Void FORSTAT()
{
  struct LOC_FORSTAT V;
  SHRTINT LCLASS;
  SHRTINT LCA = 0;
  SHRTINT LCP;
  ADDRESS LJPADDR;
  boolean LOF;
  SHRTINT LUPBND;
  CONTEXTTABLE *WITH;

  INSYMBOL();
  if (NO != 1) {
    ERROR(49L);
    GATTR.TYPTR = NULL;
  } else {
    SEARCH();
    if (CTPTR == NULL) {
      ERROR(31L);
      CTPTR = UNDECPTR;
    }
    if (CTPTR->KLASS <= (long)PROC) {
      ERROR(32L);
      INSYMBOL();
    } else
      VARIABLE();
  }
  CHTYPE(&V);
  if (GATTR.TYPTR != NULL) {
    if (GATTR.UU.U0.ACCESS != DRCT) {
      ERROR(69L);
      GATTR.TYPTR = NULL;
    } else {
      if (GATTR.UU.U0.BREG != 0 && GATTR.UU.U0.BREG != LEVEL)
	ERROR(69L);
    }
  }
  V.LATTR = GATTR;
  if (NO != 20) {   /* SY <> := */
    if (GATTR.TYPTR != NULL)
      ERROR(52L);
    SKIP(20L);
    if (NO != 20) {
      if (GATTR.TYPTR == NULL)
	ERROR(52L);
      if (ERRCL[NO] == BEGSY)
	goto _L20;
      goto _L10;
    }
  }
  INSYMBOL();
  EXPRESSION();
  CHTYPE(&V);
  CHTYPES(&V);
  TRANSFER(GATTR, (long)RP);
  if (NO != 33) {   /* SY <> TO/DOWNTO */
    if (GATTR.TYPTR != NULL)
      ERROR(70L);
    SKIP(33L);
    if (NO != 33) {
      if (GATTR.TYPTR == NULL)
	ERROR(70L);
      if (ERRCL[NO] == BEGSY)
	goto _L20;
      goto _L10;
    }
  }
  LCLASS = CL;
  INSYMBOL();
  EXPRESSION();
  CHTYPE(&V);
  CHTYPES(&V);
  if (GATTR.TYPTR != NULL && V.LATTR.TYPTR != NULL) {
    TRANSFER(GATTR, (long)RP);
    if (GATTR.KIND == SVAL) {
      LOF = true;
      LUPBND = GATTR.UU.VAL;
    } else {
      GEN15(8L, 7L, (long)RP, 0L);
      GEN30(41L, 7L, 5L, LC + TCT);
      TCT++;
      if (TCT > TMAX)
	TMAX = TCT;
      LOF = false;
    }
    GEN15(8L, 6L, 1L, 0L);
    NOOP();
    LJPADDR = IC;
    if (LCLASS == 1)   /*STEP +1*/
      GEN15(31L, 1L, 2L, 6L);
    else
      GEN15(31L, 1L, 6L, 2L);
    /*STEP -1*/
    GEN30(3L, 3L, 1L, 0L);
    LCA = CA;
    LCP = CP;
    RP = 0;
    if (ASSCHECK) {
      WITH = V.LATTR.TYPTR;
      if ((TYPFORM)WITH->UU.U0.FORM == NUMERIC) {
	if (V.LATTR.TYPTR != INTPTR)
	  CHECKBNDS(6, WITH->UU.U0.UU.U0.MIN, WITH->UU.U0.UU.U0.MAX, ASSERR);
      } else
	CHECKBNDS(6, 0L, WITH->UU.U0.UU.U1.FCONST->UU.U1.UU.U0.VALUES, ASSERR);
    }
    TRANSFER(V.LATTR, 6L);
  }
  if (NO != 31) {   /* SY <> DO */
    if (GATTR.TYPTR != NULL)
      ERROR(59L);
    SKIP(31L);
    if (NO != 31) {
      if (GATTR.TYPTR == NULL)
	ERROR(59L);
      if (ERRCL[NO] == BEGSY)
	goto _L20;
      goto _L10;
    }
  }
  INSYMBOL();
_L20:
  STATEMENT();
  if (V.LATTR.TYPTR != NULL) {
    TRANSFER(V.LATTR, (long)RP);
    WITH = V.LATTR.TYPTR;
    if ((TYPFORM)WITH->UU.U0.FORM == NUMERIC &&
	(((WITH->UU.U0.UU.U0.MIN < 0) ? -WITH->UU.U0.UU.U0.MIN : WITH->
	    UU.U0.UU.U0.MIN) >= TWOTO17 ||
	 ((WITH->UU.U0.UU.U0.MAX < 0) ? -WITH->UU.U0.UU.U0.MAX : WITH->
	    UU.U0.UU.U0.MAX) >= TWOTO17)) {
/* p2c: pascal_p2c.pas, line 2587: 
 * Note: Line breaker spent 0.0 seconds, 5000 tries on line 4576 [251] */
      GEN30(57L, 0L, 0L, 1L);
      GEN15(LCLASS + 29, 6L, 1L, 0L);   /* +1 OR -1*/
    } else
      GEN30(58L, 6L, 1L, 3 - LCLASS * 2);
    if (LOF)
      GEN30(57L, 2L, 0L, LUPBND);
    else {
      TCT--;
      GEN30(41L, 2L, 5L, LC + TCT);
    }
    GEN30(4L, 0L, 0L, (long)LJPADDR);
    NOOP();
    INS(IC, LCP, LCA);
  }
_L10: ;
}  /* FORSTAT */


Static Void GOTOSTAT()
{
  long FORLIM;
  _REC_LABTAB *WITH;
  _REC_UNDLAB *WITH1;

  INSYMBOL();
  if (NO == 1 && !strncmp(AVAL, "EXIT      ", sizeof(alfa))) {
    INSYMBOL();
    if (NO != 2 || CL != 1) {
      ERROR(61L);
      SKIP(49L);
      return;
    }
    for (IT = CEXTABIX; IT >= 1; IT--) {
      if (SEXT(EXTAB[IT-1].EXVAL, 18) == IVAL) {
	IT1 = SEXT(EXTAB[IT-1].JMPTABIX, 18);
	if (JMPTAB[IT1] != 0) {
	  if (JMPTAB[IT1] != LEVEL) {
	    LOADBASE(1, (int)JMPTAB[IT1]);
	    GEN15(51L, 5L, 1L, 0L);
	  }
	} else {
	  GEN30(41L, 1L, 0L, (long)PARMLIST.ICO);
	  GEN15(51L, 5L, 1L, 0L);
	}
	GEN30(4L, 0L, 0L, PARMLIST.LPJMPTAB + IT1);
	goto _L1;
      }
    }
    ERROR(43L);
_L1:
    INSYMBOL();
    return;
  }
  if (NO != 2 || CL != 1) {
    ERROR(61L);
    SKIP(49L);
    return;
  }
  if (IVAL >= TWOTO17) {
    ERROR(100L);
    goto _L20;
  }
  FORLIM = CLABIX;
  /* SEARCH THROUGH LABELTABLE OF CURRENT BLOCK */
  for (IT = 1; IT <= FORLIM; IT++) {
    WITH = &LABTAB[IT-1];
    if (SEXT(WITH->LABVAL, 18) == IVAL) {   /* LABEL ALREADY OCCURED */
      /* IF DECL. OCC. GENERATE CODE ELSE CHAIN OCC. */
      if (SEXT(WITH->FLD2, 18) == 0)
	GEN30(4L, 0L, 0L, (long)SEXT(WITH->FLD3, 18));
      else {
	GEN30(4L, 0L, 0L, 0L);
	if (CHNIX == 0) {
	  ERROR(75L);
	  goto _L20;
	}
	WITH1 = &UNDLAB[CHNIX-1];
	IT1 = SEXT(WITH1->SUCC, 18);
	WITH1->SUCC = SEXT(WITH->FLD3, 18);
	WITH->FLD3 = CHNIX;
	WITH1->PLACE = CA;
	WITH1->LFTSH = CP;
	CHNIX = IT1;
      }
      goto _L20;
    }
  }
  /* LABEL NOT YET MET, ENTER IT INTO LABELTABLE */
  GEN30(4L, 0L, 0L, 0L);
  if (CLABIX == MAXLABS) {
    ERROR(74L);
    goto _L20;
  }
  if (CHNIX == 0) {
    ERROR(75L);
    goto _L20;
  }
  CLABIX++;
  WITH = &LABTAB[CLABIX-1];
  WITH1 = &UNDLAB[CHNIX-1];
  IT1 = SEXT(WITH1->SUCC, 18);
  WITH->LABVAL = IVAL;
  WITH1->SUCC = 0;
  WITH->FLD2 = CHNIX;
  WITH->FLD3 = CHNIX;
  WITH1->PLACE = CA;
  WITH1->LFTSH = CP;
  CHNIX = IT1;
_L20:
  INSYMBOL();
}  /* GOTOSTAT */


Static Void WITHSTAT()
{
  SHRTINT LTPCT = 0, LB7CT = 0;
  _REC_DISPLAY *WITH;

  do {
    INSYMBOL();
    VARIAB();
    if (GATTR.TYPTR != NULL) {
      if ((TYPFORM)GATTR.TYPTR->UU.U0.FORM != RECORDS)
	ERROR(38L);
      else {
	TOP++;
	LTPCT++;
	if (TOP > DISPLIMIT)
	  ERROR(82L);
	else {
	  WITH = &DISPLAY[TOP];
	  WITH->FNAME = GATTR.TYPTR->UU.U0.UU.U5.FSTFLD;
	  if (GATTR.UU.U0.ACCESS == DRCT &&
	      (GATTR.UU.U0.BREG == 0 || GATTR.UU.U0.BREG == LEVEL)) {
	    WITH->OCCUR = (unsigned)CWITH;
	    WITH->UU.U1.CDSPL = GATTR.UU.U0.DPLMT;
	    WITH->UU.U1.CLEV = GATTR.UU.U0.BREG;
	  } else {
	    LOADADR(GATTR, 6L);
	    IT = LC + TCT;
	    GEN30(41L, 6L, 5L, IT);
	    WITH->OCCUR = (unsigned)VWITH;
	    WITH->UU.VDSPL = IT;
	    TCT++;
	    LB7CT++;
	    if (TCT > TMAX)
	      TMAX = TCT;
	  }
	}
      }
    }
  } while (NO == 15);   /* SY <> , */
  if (NO != 31) {
    if (GATTR.TYPTR != NULL)
      ERROR(59L);
    SKIP(31L);
    if (NO != 31) {
      if (GATTR.TYPTR == NULL)
	ERROR(59L);
      if (ERRCL[NO] == BEGSY)
	goto _L20;
      goto _L10;
    }
  }
  INSYMBOL();
_L20:
  STATEMENT();
_L10:
  TOP -= LTPCT;
  TCT -= LB7CT;
}  /* WITHSTAT */


/* ------------------------------------------------------------ */

Static Void COMPSTAT()
{
  do {
    INSYMBOL();
_L1:
    STATEMENT();
    if (ERRCL[NO] == BEGSY) {
      ERROR(58L);
      goto _L1;
    }
    if (NO == 25) {   /*ELSE*/
      ERROR(54L);
      INSYMBOL();
      goto _L1;
    }
    /*;*/
  } while (NO == 16);
  if (NO != 22)   /*END*/
    ERROR(68L);
  else
    INSYMBOL();
}  /*COMPSTAT*/


Static Void STATEMENT()
{
  SHRTINT LPSW;
  long FORLIM;
  _REC_LABTAB *WITH;
  _REC_UNDLAB *WITH1;
  CONTEXTTABLE *WITH2;

  if (NO == 2 && CL == 1) {   /*LABEL*/
    NOOP();
    if (IVAL >= TWOTO17) {
      ERROR(100L);
      goto _L1;
    }
    FORLIM = CLABIX;
    for (IT = 1; IT <= FORLIM; IT++) {
      WITH = &LABTAB[IT-1];
      if (SEXT(WITH->LABVAL, 18) == IVAL) {   /*FOUND*/
	if (SEXT(WITH->FLD2, 18) == 0)   /*MULTIDEF.*/
	  ERROR(77L);
	else {
	  IT1 = SEXT(WITH->FLD3, 18);
	  do {
	    WITH1 = &UNDLAB[IT1-1];
	    INS(IC, (long)WITH1->LFTSH, (long)SEXT(WITH1->PLACE, 18));
	    IT1 = SEXT(WITH1->SUCC, 18);
	  } while (IT1 != 0);
	  IT1 = SEXT(WITH->FLD2, 18);
	  UNDLAB[IT1-1].SUCC = CHNIX;
	  CHNIX = SEXT(WITH->FLD3, 18);
	  WITH->FLD2 = 0;
	  WITH->FLD3 = IC;
	}
	/*FIXUP*/
	goto _L1;
      }  /*IF, WITH, FOR*/
    }
    /*NEW LABEL*/
    if (CLABIX == MAXLABS)
      ERROR(74L);
    else {
      CLABIX++;
      WITH = &LABTAB[CLABIX-1];
      WITH->LABVAL = IVAL;
      WITH->FLD2 = 0;
      WITH->FLD3 = IC;
    }
_L1:
    INSYMBOL();
    if (NO != 19) {   /*:*/
      ERROR(64L);
      SKIP(49L);
    } else
      INSYMBOL();
  }  /*IF (NO=2) AND (CL=1)*/
  RP = 0;
  switch (SPLITSTAT[NO]) {

  /*PASS  */
  case 1:
    break;

  /*ENDSY OR IRRELSY*/
  /*IDENT */
  case 2:
    SEARCH();
    if (CTPTR == NULL) {
      ERROR(31L);
      CTPTR = UNDECPTR;
    }
    WITH2 = CTPTR;
    if (WITH2->KLASS <= (long)KONST)
      ERROR(55L);
    else {
      if ((IDCLASS)WITH2->KLASS == PROC &&
	  (WITH2->UU.U2.PROCTYPE == CTPTR || WITH2->UU.U2.PROCTYPE == NULL))
	  /*PROCCALL*/
	  {  /* PROCCALL */
	if (WITH2->UU.U2.PROCTYPE == CTPTR) {
	  INSYMBOL();
	  if (CTPTR <= PREDEFP) {
	    LPSW = SEXT(CTPTR->UU.U2.SEGSIZE, 18);
	    switch (LPSW) {

	    case 0:
	      TITLE();
	      break;

	    case 1:
	    case 2:
	    case 3:
	    case 4:
	      GETPUT(LPSW);
	      break;

	    case 5:
	      ALLC();
	      break;

	    case 6:
	      PCK();
	      break;

	    case 7:
	      UNPCK();
	      break;

	    case 8:
	    case 9:
	      INSAPP(LPSW);
	      break;

	    case 10:
	      READIR();
	      break;

	    case 11:
	      WRITEIR();
	      break;
	    }
	  } else
	    PASSPARAMS();
	} else
	  SKIP(49L);
      } else
	ASSIGN();
    }
    break;

  /*BEGIN */
  case 3:
    COMPSTAT();
    break;

  /*IF    */
  case 4:
    IFSTAT();
    break;

  /*CASE  */
  case 5:
    CASESTAT();
    break;

  /*REPEAT*/
  case 6:
    REPEATSTAT();
    break;

  /*WHILE */
  case 7:
    WHILESTAT();
    break;

  /*FOR   */
  case 8:
    FORSTAT();
    break;

  /*GOTO  */
  case 9:
    GOTOSTAT();
    break;

  /*WITH  */
  case 10:
    WITHSTAT();
    break;
  }
  if (ERRCL[NO] == IRRELSY) {
    ERROR(54L);
    SKIP(49L);
  }
  RP = 0;
}  /*STATEMENT*/


Static Void TYPEDECL PP((SHRTINT *TL, CONTEXTTABLE *P1));

/* Local variables for TYPEDECL: */
struct LOC_TYPEDECL {
  SHRTINT *TL;
  CONTEXTTABLE *P1;
  SHRTINT I, J, BDISPL;
  ADDRESS DISPL;
  boolean PACKFLAG;
  CONTEXTTABLE *LASTFLD, *PP_, *P;
} ;

Local Void SKIPT(FNO, LINK)
SHRTINT FNO;
struct LOC_TYPEDECL *LINK;
{
  while (TERRCL[NO] == IRRELSY && FNO != NO)
    INSYMBOL();
}  /*SKIPT*/

Local Void TYPERR(I, LINK)
SHRTINT I;
struct LOC_TYPEDECL *LINK;
{
  *LINK->TL = 0;
  LINK->P1 = NULL;
  ERROR(I);
  SKIPT(49L, LINK);
}  /*TYPERR*/

Local Void SUBRANGE(VAL1, VAL2, N1, P, LINK)
long *VAL1, *VAL2;
CONTEXTTABLE *N1, *P;
struct LOC_TYPEDECL *LINK;
{
  /*THE FIRST SYMBOL OF SUBRANGE HAS BEEN READ.
     THE PROCEDURE RETURNS THE TWO BOUND-VALUES IN VAL1,VAL2,
     AND RETURNS  N1 = POINTER TO TYPE OF CONSTANTS.
     ERRORS : TYPES DO NOT AGREE, TYPE IS NOT INTEGER,CHAR,
        OR SYMBOLIC,  VAL1 > VAL2.
     P INDICATES BEGINNING OF SEARCH FOR FIRST SYMBOL IF IT IS
     AN ID.  */
  CONTEXTTABLE *N2;

  INCONST(VAL1, N1, P);
  if (N1 == NULL || N1 == REALPTR || N1 == ALFAPTR) {
    ERR = true;
    return;
  }
  if (NO != 19)
    ERROR(10L);
  else
    INSYMBOL();
  INCONST(VAL2, N2, NEXT);
  if (N1 != N2)
    ERR = true;
  else {
    if (*VAL1 > *VAL2)
      ERROR(25L);
  }
}  /*SUBRANGE*/

Local Void SUBTYPE(I, J, P, LINK)
SHRTINT *I, *J;
CONTEXTTABLE *P;
struct LOC_TYPEDECL *LINK;
{
  /*EITHER A TYPE-ID FOR A SUBRANGE OR AN EXPLICIT SUBRANGE
     ARE PROCESSED.
     RETURNS I = LOWBOUND, J = HIGHBOUND,
        P = POINTER TO TYPE OF CONSTANTS */
  if (NO == 1) {   /*ID*/
    SRCHREC(NEXT);
    if (CTPTR == NULL)
      SEARCH();
    if (CTPTR == NULL) {
      ERROR(12L);
      return;
    }
    if ((IDCLASS)CTPTR->KLASS == TYPES) {   /*TYPES*/
      if (CTPTR->UU.U0.FORM > (long)SYMBOLIC) {
	ERROR(13L);
	return;
      }
      switch ((TYPFORM)CTPTR->UU.U0.FORM) {

      case NUMERIC:
	*I = CTPTR->UU.U0.UU.U0.MIN;
	*J = CTPTR->UU.U0.UU.U0.MAX;
	break;

/* p2c: pascal_p2c.pas:
 * Note: Eliminated unused assignment statement [338] */
      case SYMBOLIC:
	if (CTPTR == REALPTR || CTPTR == ALFAPTR)
	  ERROR(6L);
	else {
	  *I = 0;
	  *J = CTPTR->UU.U0.UU.U1.FCONST->UU.U1.UU.U0.VALUES;
/* p2c: pascal_p2c.pas:
 * Note: Eliminated unused assignment statement [338] */
	}
	break;
      }
      INSYMBOL();
      return;
    }
    if ((IDCLASS)CTPTR->KLASS == KONST) {
/* p2c: pascal_p2c.pas, line 2857:
 * Warning: Type mismatch in VAR parameter VAL1 [295] */
/* p2c: pascal_p2c.pas, line 2857:
 * Warning: Type mismatch in VAR parameter VAL2 [295] */
      SUBRANGE(I, J, P, CTPTR, LINK);
    } else
      ERROR(63L);
    return;
  }
  if ((unsigned long)NO < 32 && ((1L << NO) & 0x84) != 0) {
/* p2c: pascal_p2c.pas, line 2860:
 * Warning: Type mismatch in VAR parameter VAL1 [295] */
/* p2c: pascal_p2c.pas, line 2860:
 * Warning: Type mismatch in VAR parameter VAL2 [295] */
    SUBRANGE(I, J, P, NULL, LINK);
  } else
    ERROR(1L);
}  /*SUBTYPE*/

Local Void SCALDECL(N, LINK)
CONTEXTTABLE *N;
struct LOC_TYPEDECL *LINK;
{
  CONTEXTTABLE *WITH;

/* p2c: pascal_p2c.pas, line 2865:
 * Warning: Type mismatch in VAR parameter VAL1 [295] */
/* p2c: pascal_p2c.pas, line 2865:
 * Warning: Type mismatch in VAR parameter VAL2 [295] */
  SUBRANGE(&LINK->I, &LINK->J, LINK->PP_, N, LINK);
  if (ERR) {
    LINK->P1 = NULL;
    return;
  }
  if ((TYPFORM)LINK->PP_->UU.U0.FORM == SYMBOLIC) {
    ERROR(28L);
    LINK->P1 = NULL;
    return;
  }
  ALLOC(LINK->P, TYPES, NUMERIC);
/* p2c: pascal_p2c.pas, line 2869:
 * Warning: Symbol 'ALLOC' is not defined [221] */
  WITH = LINK->P;
  memcpy(WITH->NAME, BLANK, sizeof(alfa));
  WITH->NXTEL = NULL;
  WITH->KLASS = (unsigned)TYPES;
  WITH->UU.U0.SIZE = 1;
  WITH->UU.U0.FORM = (unsigned)NUMERIC;
  WITH->UU.U0.UU.U0.MIN = LINK->I;
  WITH->UU.U0.UU.U0.MAX = LINK->J;
  if (labs(LINK->I) > labs(LINK->J))
    WITH->UU.U0.UU.U0.BITS = LOG2(labs(LINK->I));
  else
    WITH->UU.U0.UU.U0.BITS = LOG2(labs(LINK->J));
  *LINK->TL = 1;
  LINK->P1 = LINK->P;
}  /*SCALDECL*/

/* Local variables for FIELDLIST: */
struct LOC_FIELDLIST {
  struct LOC_TYPEDECL *LINK;
  boolean TAGFLAG;
} ;

Local Void RECERR(I, LINK)
SHRTINT I;
struct LOC_FIELDLIST *LINK;
{
  ERROR(I);
  SKIPT(49L, LINK->LINK);
}

Local Void ADJUST(LINK)
struct LOC_FIELDLIST *LINK;
{
  /* MOVE LAST FIELD TO RIGHT. IF IT IS THE ONLY FIELD,
     CHANGE TO NONPACKED.
     IF LAST FIELD IS TAGFIELD THEN DO NOT MOVE.
     INCREASE DISPL, RESET BDISPL  */
  CONTEXTTABLE *WITH;

  if (!LINK->TAGFLAG) {
    WITH = LINK->LINK->LASTFLD;
    if (WITH->UU.U4.BITDISPL == 0)   /*ONLY ONE FIELD IN WORD*/
      WITH->UU.U4.BITWIDTH = 0;
    else
      WITH->UU.U4.BITDISPL = WORDLENGTH - WITH->UU.U4.BITWIDTH;
  }
  LINK->LINK->DISPL++;
  LINK->LINK->BDISPL = 0;
}  /*ADJUST*/

Local Void FIELDLIST(MAXSIZE, VARPTR, NXTF, LINK)
SHRTINT *MAXSIZE;
CONTEXTTABLE *VARPTR, *NXTF;
struct LOC_TYPEDECL *LINK;
{
  struct LOC_FIELDLIST V;
  SHRTINT MXL, L, LL, MINSIZE, CASEBITS, I;
  CONTEXTTABLE *P, *PP_, *PP1, *PP2, *NXTC, *NXT, *CPTR, *WITH;

  V.LINK = LINK;
  V.TAGFLAG = true;
  NXT = NXTF;
  do {
    if (NO == 26)   /*CASE*/
      goto _L2;
    I = 0;
_L1:
    if (NO != 1) {
      RECERR(11L, &V);
      if (TERRCL[NO] == BEGSY)
	goto _L11;
      goto _L12;
    }
    SRCHREC(NXT);
    if (CTPTR != NULL)
      ERROR(5L);
    else {
      ALLOC(P, FIELD);
/* p2c: pascal_p2c.pas, line 2915:
 * Warning: Symbol 'ALLOC' is not defined [221] */
      I++;
      memcpy(P->NAME, AVAL, sizeof(alfa));
      P->NXTEL = NXT;
      P->KLASS = (unsigned)FIELD;
      P->UU.U4.FLDTYPE = NULL;
      NXT = P;
    }
    INSYMBOL();
    if (NO == 15) {   /*,*/
      INSYMBOL();
      goto _L1;
    }
    if (NO != 19)   /*NOT :*/
      ERROR(10L);
    else
      INSYMBOL();
_L11:
    TYPEDECL(&L, P);
    if (P != NULL) {
      if (P->UU.U0.FORM > (long)RECORDS)
	ERROR(30L);
      else {
	if (LINK->PACKFLAG) {   /*PACKFLAG*/
	  if (I > 1) {   /*REVERSE POINTERS*/
	    PP_ = NXT;
	    for (; I >= 1; I--) {
	      PP1 = PP_->NXTEL;
	      PP_->NXTEL = PP2;
	      PP2 = PP_;
	      PP_ = PP1;
	    }
	    NXT->NXTEL = PP_;
	    NXT = PP2;
	  }
	  /*REVERSE*/
	  else
	    PP_ = NXT->NXTEL;
	  PP1 = NXT;
	  if (P->UU.U0.FORM <= (long)POWER) {   /*FORM <= POINTER*/
	    switch ((TYPFORM)P->UU.U0.FORM) {

	    case NUMERIC:
	      LL = P->UU.U0.UU.U0.BITS;
	      break;

	    case SYMBOLIC:
	      LL = P->UU.U0.UU.U1.BITSIZE;
	      break;

	    case POINTER:
	      LL = 18;
	      break;

	    case POWER:
	      LL = P->UU.U0.UU.U3.PWBITS;
	      break;
	    }
	    do {
	      if (LINK->BDISPL + LL > WORDLENGTH)
		ADJUST(&V);
	      if (LL == WORDLENGTH) {
		PP1->UU.U4.BITWIDTH = 0;
		PP1->UU.U4.FLDADDR = LINK->DISPL;
		LINK->DISPL++;
	      } else {
		PP1->UU.U4.BITWIDTH = LL;
		PP1->UU.U4.BITDISPL = LINK->BDISPL;
		PP1->UU.U4.FLDADDR = LINK->DISPL;
		LINK->BDISPL += LL;
	      }
	      PP1->UU.U4.FLDTYPE = P;
	      LINK->LASTFLD = PP1;
	      V.TAGFLAG = false;
	      PP1 = PP1->NXTEL;
	    } while (PP1 != PP_);
	  } else {
	    if (LINK->BDISPL != 0)
	      ADJUST(&V);
	    V.TAGFLAG = false;
	    do {
	      PP1->UU.U4.BITWIDTH = 0;
	      PP1->UU.U4.FLDTYPE = P;
	      PP1->UU.U4.FLDADDR = LINK->DISPL;
	      LINK->DISPL += L;
	      LINK->LASTFLD = PP1;
	      PP1 = PP1->NXTEL;
	    } while (PP1 != PP_);
	  }
	} else {
	  LL = LINK->DISPL + I * L;
	  LINK->DISPL = LL;
	  PP_ = NXT;
	  for (; I >= 1; I--) {
	    PP_->UU.U4.FLDTYPE = P;
	    LL -= L;
	    PP_->UU.U4.FLDADDR = LL;
	    PP_->UU.U4.BITWIDTH = 0;
	    PP_ = PP_->NXTEL;
	  }
	}
      }
    }
    /*FORM <= RECORDS*/
_L12:
    if (NO == 16)
      INSYMBOL();
  } while (TERRCL[NO] != ENDSY || NO == 26);
  if (LINK->BDISPL != 0)
    ADJUST(&V);
  *MAXSIZE = LINK->DISPL;
  VARPTR = NULL;
  goto _L9;
_L2:   /*CASE*/
  INSYMBOL();
  if (NO != 1)
    ERROR(11L);
  else {
    SRCHREC(NXT);
    if (CTPTR != NULL)
      ERROR(5L);
    else {
      ALLOC(P, FIELD);
/* p2c: pascal_p2c.pas, line 2991:
 * Warning: Symbol 'ALLOC' is not defined [221] */
      memcpy(P->NAME, AVAL, sizeof(alfa));
      P->NXTEL = NXT;
      P->KLASS = (unsigned)FIELD;
      P->UU.U4.FLDTYPE = NULL;
      NXT = P;
    }
    INSYMBOL();
  }
  /*NO = 1*/
  if (NO != 19)
    ERROR(10L);
  else
    INSYMBOL();
  if (NO != 1)
    ERROR(11L);
  else {
    SRCHREC(NEXT);
    if (CTPTR == NULL)
      SEARCH();
    if (CTPTR == NULL)
      ERROR(12L);
    else {
      if ((IDCLASS)CTPTR->KLASS != TYPES || CTPTR->UU.U0.FORM > (long)SYMBOLIC)
	ERROR(7L);
    }
    INSYMBOL();
    if (NO != 27)
      ERROR(14L);
    CPTR = CTPTR;
    if (CPTR != NULL) {
      if (LINK->PACKFLAG) {   /*PACKFLAG*/
	switch ((TYPFORM)CPTR->UU.U0.FORM) {

	case NUMERIC:
	  LL = CPTR->UU.U0.UU.U0.BITS;
	  break;

	case SYMBOLIC:
	  LL = CPTR->UU.U0.UU.U1.BITSIZE;
	  break;
	}
	if (LINK->BDISPL + LL > WORDLENGTH)
	  ADJUST(&V);
	P->UU.U4.BITDISPL = LINK->BDISPL;
	P->UU.U4.BITWIDTH = LL;
	P->UU.U4.FLDADDR = LINK->DISPL;
	LINK->BDISPL += LL;
	LINK->LASTFLD = P;
	CASEBITS = LINK->BDISPL;
      } else {
	P->UU.U4.FLDADDR = LINK->DISPL;
	P->UU.U4.BITWIDTH = 0;
      }
    }
    P->UU.U4.FLDTYPE = CPTR;
    LINK->DISPL++;
    MINSIZE = LINK->DISPL;
    *MAXSIZE = LINK->DISPL;
    NXTC = NULL;
    INSYMBOL();
    do {
      I = 0;
      do {
	if (NO > 2 || (NO == 2 && ((unsigned long)CL >= 32 ||
				   ((1L << CL) & 0x12) == 0))) {
	  RECERR(63L, &V);
	  goto _L3;
	}
	if (CPTR != NULL) {   /*END CPTR <> NIL*/
	  if (NO == 1) {   /*NO = 1*/
	    SRCHREC(NEXT);
	    if (CTPTR == NULL)
	      SEARCH();
	    if (CTPTR == NULL)
	      ERROR(12L);
	    else {
	      WITH = CTPTR;
	      if ((IDCLASS)WITH->KLASS != KONST)
		ERROR(63L);
	      else {
		if (((TYPFORM)CPTR->UU.U0.FORM == SYMBOLIC &&
		     WITH->UU.U1.CONTYPE != CPTR) ||
		    ((TYPFORM)CPTR->UU.U0.FORM == NUMERIC &&
		     WITH->UU.U1.CONTYPE != INTPTR))
		  ERROR(73L);
		else
		  IT = WITH->UU.U1.UU.U0.VALUES;
	      }
	    }
	  } else {
	    if (CL == 1 && (TYPFORM)CPTR->UU.U0.FORM != NUMERIC ||
		CL == 4 && CPTR != CHARPTR)
	      ERROR(73L);
	    else
	      IT = IVAL;
	  }
	}
	ALLOC(P, TAGFIELD);
/* p2c: pascal_p2c.pas, line 3048:
 * Warning: Symbol 'ALLOC' is not defined [221] */
	memcpy(P->NAME, BLANK, sizeof(alfa));
	P->NXTEL = NXTC;
	P->KLASS = (unsigned)TAGFIELD;
	P->UU.U5.TAGVAL = true;
	P->UU.U5.UU.CASEVAL = IT;
	NXTC = P;
	I++;
	INSYMBOL();
_L3:
	if (NO != 19)
	  ERROR(10L);
	else
	  INSYMBOL();
      } while (NO <= 2);
      if (NO == 9) {   /* ( FIELDLIST ) */
	if (LINK->PACKFLAG) {
	  LINK->DISPL = MINSIZE - 1;
	  LINK->BDISPL = CASEBITS;
	} else
	  LINK->DISPL = MINSIZE;
	INSYMBOL();
	FIELDLIST(&MXL, PP_, NXT, LINK);
	if (NO != 10)
	  ERROR(9L);
	else
	  INSYMBOL();
      } else {
	PP_ = NULL;
	MXL = MINSIZE;
      }
      P = NXTC;
      for (; I >= 1; I--) {
	WITH = P;
	WITH->UU.U5.CASESIZE = MXL;
	WITH->UU.U5.VARIANTS = PP_;
	P = WITH->NXTEL;
      }
      if (*MAXSIZE < MXL)
	*MAXSIZE = MXL;
      if (NO == 16)
	INSYMBOL();
    } while (NO <= 2);
    ALLOC(P, TAGFIELD);
/* p2c: pascal_p2c.pas, line 3074:
 * Warning: Symbol 'ALLOC' is not defined [221] */
    memcpy(P->NAME, BLANK, sizeof(alfa));
    P->NXTEL = NULL;
    P->KLASS = (unsigned)TAGFIELD;
    P->UU.U5.CASESIZE = MINSIZE;
    P->UU.U5.VARIANTS = NXTC;
    P->UU.U5.TAGVAL = false;
    P->UU.U5.UU.CASETYPE = CPTR;
/* p2c: pascal_p2c.pas:
 * Note: Eliminated unused assignment statement [338] */
  }
  /*NO = 1*/
_L9:
  NXTF = NXT;
}  /* FIELDLIST */


/* ------------------------------------------------------------ */

Static Void TYPEDECL(TL_, P1_)
SHRTINT *TL_;
CONTEXTTABLE *P1_;
{
  struct LOC_TYPEDECL V;
  SHRTINT L, LL, CV, E1, E2;
  OPTPWR OPT;
  boolean LERR;
  CONTEXTTABLE *NXTF, *NXTC, *NXTA, *RTYP, *WITH;
  _REC_PTLIST *WITH1;

  /*TYPEDECL*/
  V.TL = TL_;
  V.P1 = P1_;
  V.PACKFLAG = false;
  if (NO == 42) {   /*PACKED*/
    V.PACKFLAG = true;
    INSYMBOL();
  }
  if (NO == 1) {   /*ID*/
    SRCHREC(NEXT);
    if (CTPTR == NULL)
      SEARCH();
    if (CTPTR == NULL) {
      ERROR(12L);
      V.P1 = NULL;
      SKIPT(16L, &V);
      return;
    }
    if ((IDCLASS)CTPTR->KLASS == TYPES) {   /*TYPE-ID*/
      *V.TL = SEXT(CTPTR->UU.U0.SIZE, 18);
      V.P1 = CTPTR;
      INSYMBOL();
      return;
    }
    if ((IDCLASS)CTPTR->KLASS == KONST)
      SCALDECL(CTPTR, &V);
    else
      TYPERR(11L, &V);
    return;
  }
  if (NO == 9) {   /* SYMBOLIC */
    CV = 0;
    LERR = ERR;
    ERR = false;
    ALLOC(V.P, TYPES, SYMBOLIC);
/* p2c: pascal_p2c.pas, line 3101:
 * Warning: Symbol 'ALLOC' is not defined [221] */
    WITH = V.P;
    memcpy(WITH->NAME, BLANK, sizeof(alfa));
    WITH->NXTEL = NULL;
    WITH->KLASS = (unsigned)TYPES;
    WITH->UU.U0.FORM = (unsigned)SYMBOLIC;
    RTYP = V.P;
    NXTC = NULL;
    do {
      INSYMBOL();
      if (NO != 1) {
	ERROR(11L);
	SKIPT(15L, &V);
	goto _L2;
      }
      SRCHREC(NEXT);
      if (CTPTR != NULL)
	ERROR(8L);
      else {
	ALLOC(V.P, KONST, ACTUAL);
/* p2c: pascal_p2c.pas, line 3113:
 * Warning: Symbol 'ALLOC' is not defined [221] */
	WITH = V.P;
	memcpy(WITH->NAME, AVAL, sizeof(alfa));
	WITH->NXTEL = NEXT;
	WITH->KLASS = (unsigned)KONST;
	WITH->UU.U1.CONTYPE = RTYP;
	WITH->UU.U1.CONKIND = (unsigned)ACTUAL;
	WITH->UU.U1.UU.U0.VALUES = CV;
	WITH->UU.U1.UU.U0.SUCC = NXTC;
	CV++;
	NEXT = V.P;
	NXTC = V.P;
      }
      INSYMBOL();
_L2: ;
    } while (NO == 15);
    ALLOC(V.P, TYPES, POWER);
/* p2c: pascal_p2c.pas, line 3122:
 * Warning: Symbol 'ALLOC' is not defined [221] */
    WITH = V.P;
    memcpy(WITH->NAME, BLANK, sizeof(alfa));
    WITH->NXTEL = NULL;
    WITH->KLASS = (unsigned)TYPES;
    WITH->UU.U0.SIZE = 1;
    WITH->UU.U0.FORM = (unsigned)POWER;
    WITH->UU.U0.UU.U3.ELSET = RTYP;
    WITH->UU.U0.UU.U3.PWBITS = CV + 1;
    RTYP->UU.U0.UU.U1.FCONST = NEXT;
    RTYP->UU.U0.SIZE = 1;
    RTYP->UU.U0.UU.U1.BITSIZE = LOG2(CV - 1);
    RTYP->UU.U0.UU.U1.PWSET = V.P;
    if (ERR)
      V.P1 = NULL;
    else {
      ERR = LERR;
      V.P1 = RTYP;
    }
    *V.TL = 1;
    if (NO != 10)
      TYPERR(9L, &V);
    else
      INSYMBOL();
    return;
  }
  /* SYMBOLIC */
  if ((unsigned long)NO < 32 && ((1L << NO) & 0x84) != 0) {   /* SUBRANGE */
    LERR = ERR;
    ERR = false;
    SCALDECL(NEXT, &V);
    if (ERR)
      ERROR(6L);
    else
      ERR = LERR;
    return;
  }
  /*SUBRANGE*/
  if (NO == 38) {   /* STRUCTURED TYPES */
    switch (CL) {   /*STRUCTURED TYPES*/

    /*ARRAY*/
    case 1:
      INSYMBOL();
      if (NO != 11) {
	TYPERR(98L, &V);
	if (TERRCL[NO] == BEGSY)
	  TYPEDECL(&V.I, CTPTR);
	goto _L19;
      }
      NXTA = NULL;
      do {
	ALLOC(V.P, TYPES, ARRAYS);
/* p2c: pascal_p2c.pas, line 3151:
 * Warning: Symbol 'ALLOC' is not defined [221] */
	WITH = V.P;
	memcpy(WITH->NAME, BLANK, sizeof(alfa));
	WITH->NXTEL = NULL;
	WITH->KLASS = (unsigned)TYPES;
	WITH->UU.U0.FORM = (unsigned)ARRAYS;
	WITH->UU.U0.UU.U4.AELTYPE = NXTA;
	/* AELTYPE TEMPORARILY LINKS SUBARRAYS */
	NXTA = V.P;
	INSYMBOL();
	LERR = ERR;
	ERR = false;
	SUBTYPE(&V.I, &V.J, V.P, &V);
	if (ERR) {
	  ERROR(6L);
	  SKIPT(15L, &V);
	  V.I = 0;
	  V.J = 0;
	  V.P = NULL;
	} else
	  ERR = LERR;
	NXTA->UU.U0.UU.U4.LO = V.I;
	NXTA->UU.U0.UU.U4.HI = V.J;
	NXTA->UU.U0.UU.U4.INXTYPE = V.P;
      } while (NO == 15);
      if (NO != 12) {
	ERROR(37L);
	SKIPT(27L, &V);
	if (TERRCL[NO] == BEGSY)
	  goto _L11;
	if (NO == 27) {
	  INSYMBOL();
	  goto _L11;
	}
	if (NO != 12) {
	  V.P1 = NULL;
	  *V.TL = 0;
	  goto _L19;
	}
      }
      INSYMBOL();
      if (NO != 27)
	ERROR(14L);
      else
	INSYMBOL();
_L11:
      TYPEDECL(V.TL, CTPTR);
      if (CTPTR != NULL) {
	if (CTPTR->UU.U0.FORM > (long)RECORDS) {
	  ERROR(30L);
	  CTPTR = NULL;
	} else {
	  do {
	    MULOPT(*V.TL, &E1, &E2, OPT);
	    NXTA->UU.U0.UU.U4.OPTTYP = (unsigned)OPT;
	    NXTA->UU.U0.UU.U4.EXP1 = E1;
	    NXTA->UU.U0.UU.U4.EXP2 = E2;
	    *V.TL *= SEXT(NXTA->UU.U0.UU.U4.HI, 18) - SEXT(NXTA->UU.U0.UU.U4.LO,
							   18) + 1;
	    NXTA->UU.U0.SIZE = *V.TL;
	    V.P = NXTA->UU.U0.UU.U4.AELTYPE;
	    NXTA->UU.U0.UU.U4.AELTYPE = CTPTR;
	    CTPTR = NXTA;
	    NXTA = V.P;
	  } while (NXTA != NULL);
	  /* NOW TL = SIZE OF ARRAY, CTPTR POINTS TO IT */
	}
      }
      V.P1 = CTPTR;
_L19: ;
      break;
      /*ARRAY*/

    /*RECORD*/
    case 2:
      ALLOC(V.P, TYPES, RECORDS);
/* p2c: pascal_p2c.pas, line 3196:
 * Warning: Symbol 'ALLOC' is not defined [221] */
      WITH = V.P;
      memcpy(WITH->NAME, BLANK, sizeof(alfa));
      WITH->NXTEL = NULL;
      WITH->KLASS = (unsigned)TYPES;
      WITH->UU.U0.FORM = (unsigned)RECORDS;
      RTYP = V.P;
      INSYMBOL();
      NXTF = NULL;
      V.DISPL = 0;
      V.BDISPL = 0;
      LERR = ERR;
      ERR = false;
      FIELDLIST(V.TL, V.P, NULL, &V);
      if (NO != 22)
	ERROR(17L);
      if (ERR)
	TYPERR(18L, &V);
      else {
	RTYP->UU.U0.SIZE = *V.TL;
	RTYP->UU.U0.UU.U5.FSTFLD = NULL;
	RTYP->UU.U0.UU.U5.RECVAR = V.P;
	V.P1 = RTYP;
	ERR = LERR;
      }
      if (NO == 22)
	INSYMBOL();
      break;
      /*RECORD*/

    /*FILE*/
    case 3:
      INSYMBOL();
      if (NO == 27) {   /*OF*/
	V.I = 8;
	INSYMBOL();
      } else {
/* p2c: pascal_p2c.pas, line 3216:
 * Warning: Type mismatch in VAR parameter V [295] */
	INCONST(&V.I, PT, NEXT);
	if (V.I <= 0 || PT != INTPTR) {
	  ERROR(95L);
	  V.I = 0;
	}
	if (NO != 27)
	  ERROR(14L);
	else
	  INSYMBOL();
      }
      TYPEDECL(V.TL, CTPTR);
      if (CTPTR != NULL) {
	if (CTPTR->UU.U0.FORM > (long)RECORDS) {
	  ERROR(30L);
	  V.P1 = NULL;
	} else {
	  L = SEXT(CTPTR->UU.U0.SIZE, 18);
	  LL = L / 64 + 1;
	  if (LL < V.I)
	    LL = V.I;
	  LL = (LL * 64 / L + 1) * L;
	  ALLOC(V.P, TYPES, FILES);
/* p2c: pascal_p2c.pas, line 3229:
 * Warning: Symbol 'ALLOC' is not defined [221] */
	  WITH = V.P;
	  memcpy(WITH->NAME, BLANK, sizeof(alfa));
	  WITH->NXTEL = NULL;
	  WITH->KLASS = (unsigned)TYPES;
	  WITH->UU.U0.FORM = (unsigned)FILES;
	  WITH->UU.U0.SIZE = LL;
	  WITH->UU.U0.UU.FELTYPE = CTPTR;
	  *V.TL = LL;
	  V.P1 = V.P;
	}
      } else
	V.P1 = NULL;
      break;
      /*FILE*/

    /*CLASS*/
    case 4:
      ALLOC(V.P, TYPES, CLASSS);
/* p2c: pascal_p2c.pas, line 3238:
 * Warning: Symbol 'ALLOC' is not defined [221] */
      WITH = V.P;
      memcpy(WITH->NAME, BLANK, sizeof(alfa));
      WITH->NXTEL = NULL;
      WITH->KLASS = (unsigned)TYPES;
      WITH->UU.U0.FORM = (unsigned)CLASSS;
      INSYMBOL();
      if (NO == 27) {
	V.I = 100;
	INSYMBOL();
      } else {
/* p2c: pascal_p2c.pas, line 3246:
 * Warning: Type mismatch in VAR parameter V [295] */
	INCONST(&V.I, PT, NEXT);
	if (V.I <= 0 || PT != INTPTR) {
	  ERROR(6L);
	  V.I = 0;
	}
	if (NO != 27)
	  ERROR(14L);
	else
	  INSYMBOL();
      }
      TYPEDECL(V.TL, CTPTR);
      if (CTPTR != NULL) {
	if (CTPTR->UU.U0.FORM > (long)RECORDS) {
	  ERROR(30L);
	  V.P1 = NULL;
	} else {
	  *V.TL = *V.TL * V.I + 1;
	  V.P->UU.U0.SIZE = *V.TL;
	  V.P->UU.U0.UU.PELTYPE = CTPTR;
	  V.P1 = V.P;
	}
      } else
	V.P1 = NULL;
      break;
      /*CLASS*/

    /*POWERSET*/
    case 5:
      INSYMBOL();
      LERR = ERR;
      ERR = false;
      SUBTYPE(&V.I, &V.J, V.P, &V);
      if (ERR)
	TYPERR(6L, &V);
      else {
	ERR = LERR;
	switch ((TYPFORM)V.P->UU.U0.FORM) {

	case NUMERIC:
	  if (V.I < 0 || V.J > WORDLENGTH - 2) {
	    ERROR(6L);
	    V.P1 = NULL;
	  } else
	    V.P1 = PNUMPTR;
	  break;

	case SYMBOLIC:
	  if (V.P == CHARPTR)
	    V.J = WORDLENGTH - 2;
	  if (V.J > WORDLENGTH - 2) {
	    ERROR(6L);
	    V.P1 = NULL;
	  } else
	    V.P1 = V.P->UU.U0.UU.U1.PWSET;
	  break;
	}
	*V.TL = 1;
      }
      break;
      /*POWERSET*/

    }
    return;
  }
  /*NOT ERR*/
  if (NO != 18) {   /*POINTER*/
    TYPERR(18L, &V);
    return;
  }
  /*POINTER*/
  INSYMBOL();
  if (NO != 1) {
    TYPERR(11L, &V);
    return;
  }
  *V.TL = 1;
  ALLOC(V.P, TYPES, POINTER);
/* p2c: pascal_p2c.pas, line 3282:
 * Warning: Symbol 'ALLOC' is not defined [221] */
  WITH = V.P;
  memcpy(WITH->NAME, BLANK, sizeof(alfa));
  WITH->NXTEL = NULL;
  WITH->KLASS = (unsigned)TYPES;
  WITH->UU.U0.FORM = (unsigned)POINTER;
  WITH->UU.U0.SIZE = 1;
  SRCHREC(NEXT);
  if (CTPTR == NULL)
    SEARCH();
  if (CTPTR != NULL) {
    if (CTPTR->UU.U3.VTYPE == NULL)
      CTPTR = NULL;
  }
  if (CTPTR == NULL) {   /*UNDECLARED CLASS*/
    if (PTX > PTLIMIT) {
      ERROR(92L);
      V.P1 = NULL;
      INSYMBOL();
      return;
    }
    WITH1 = &PTLIST[PTX];
    WITH = V.P;
    memcpy(WITH1->HNAME, AVAL, sizeof(alfa));
    WITH1->PPTR = V.P;
    WITH->UU.U0.UU.U2.DOMAIN = V.P;
    WITH->UU.U0.UU.U2.ELTYPE = V.P;
    PTX++;
    V.P1 = V.P;
    INSYMBOL();
    return;
  }
  WITH = CTPTR;
  if ((IDCLASS)WITH->KLASS != VARS ||
      (TYPFORM)WITH->UU.U3.VTYPE->UU.U0.FORM != CLASSS) {
    TYPERR(15L, &V);
    return;
  }
  V.P->UU.U0.UU.U2.DOMAIN = CTPTR;
  V.P->UU.U0.UU.U2.ELTYPE = WITH->UU.U3.VTYPE->UU.U0.UU.PELTYPE;
  V.P1 = V.P;
  INSYMBOL();
}  /*TYPEDECL*/
/* p2c: pascal_p2c.pas, line 3314: 
 * Warning: Symbol '1' was already defined [220] */
/* p2c: pascal_p2c.pas, line 3314: 
 * Warning: Symbol '_JL1' was already defined [220] */
/* p2c: pascal_p2c.pas, line 3330: 
 * Warning: Symbol '10' was already defined [220] */
/* p2c: pascal_p2c.pas, line 3330: 
 * Warning: Symbol '_JL10' was already defined [220] */


#define DIRECT          0
#define INDIRECT        1


#define OPN             68


/*CALLED AT PROCEDURE END. GOES THROUGH LABELTABLE LABTAB AND
  EXITTABLE EXTAB, UPDATING CODE, AND GENERATES PROCEDURE EXIT CODE*/

#define CLS             69


/* Local variables for BODY: */
struct LOC_BODY {
  jmp_buf _JL1;
  SHRTINT TL, LL, VLC, FSTIX, LCA, LCP;
  CONTEXTTABLE *N, *P;
  alfa TYPID;
  CWORD LCH;
} ;

Local Void FINDSEMICOLON(LINK)
struct LOC_BODY *LINK;
{
  if (NO != 16) {
    ERROR(58L);
    SKIP(16L);
    if (NO != 16)
      longjmp(LINK->_JL1, 1);
  }
  INSYMBOL();
}  /*FINDSEMICOLON*/

/* Local variables for VARDECL: */
struct LOC_VARDECL {
  struct LOC_BODY *LINK;
  jmp_buf _JL10;
} ;

Local Void FILERR(LINK)
struct LOC_VARDECL *LINK;
{
  ERROR(95L);
  SKIP(12L);
  longjmp(LINK->_JL10, 1);
}

Local Void VARDECL(LINK)
struct LOC_BODY *LINK;
{
  struct LOC_VARDECL V;
  ADDRESS AT;
  SHRTINT I, J;
  CONTEXTTABLE *WITH;
  SHRTINT FORLIM;
  ADDRESS FORLIM1;
  _REC_PTLIST *WITH1;
  long TEMP;

  V.LINK = LINK;
  if (setjmp(V._JL10))
    goto _L10;
  INSYMBOL();
  while (NO == 1) {
    I = 0;
    do {
      SRCHREC(NEXT);
      if (CTPTR != NULL)
	ERROR(8L);
      else {
	ALLOC(LINK->P, VARS);
/* p2c: pascal_p2c.pas, line 3342:
 * Warning: Symbol 'ALLOC' is not defined [221] */
	WITH = LINK->P;
	memcpy(WITH->NAME, AVAL, sizeof(alfa));
	WITH->NXTEL = NEXT;
	WITH->KLASS = (unsigned)VARS;
	WITH->UU.U3.VKIND = (unsigned)ACTUAL;
	WITH->UU.U3.VTYPE = NULL;
	WITH->UU.U3.VLEVEL = 0;
	NEXT = LINK->P;
	I++;
      }
      INSYMBOL();
      if (NO == 11) {   /* [ IN,OUT,PRINT,PUNCH  ] */
	INSYMBOL();
	if (NO == 8 && CL == 7)   /*IN*/
	  LINK->P->UU.U3.VLEVEL = 4;
	else {
	  if (NO == 1) {
	    if (!strncmp(AVAL, "OUT       ", sizeof(alfa)))
	      LINK->P->UU.U3.VLEVEL = 1;
	    else {
	      if (!strncmp(AVAL, "PRINT     ", sizeof(alfa)))
		LINK->P->UU.U3.VLEVEL = 2;
	      else {
		if (!strncmp(AVAL, "PUNCH     ", sizeof(alfa)))
		  LINK->P->UU.U3.VLEVEL = 3;
		else
		  FILERR(&V);
	      }
	    }
	  } else
	    FILERR(&V);
	}
	INSYMBOL();
	if (NO != 12)
	  FILERR(&V);
	INSYMBOL();
      }
      ERR = false;
      if (NO == 15) {
	INSYMBOL();
	if (NO != 1) {
	  ERROR(11L);
	  goto _L10;
	}
      } else if (NO != 19)
	ERROR(10L);
    } while (NO == 1);
    if (NO == 19)
      INSYMBOL();
    else if (!ERR)
      ERROR(10L);
    LINK->N = NEXT;
    ERR = false;
    TYPEDECL(&LINK->TL, CTPTR);
    if (ERR)
      goto _L10;
    LC += I * LINK->TL;
    LINK->LL = LC;
    for (; I >= 1; I--) {
      WITH = LINK->N;
      /* FOR I */
      LINK->LL -= LINK->TL;
      WITH->UU.U3.VTYPE = CTPTR;
      if (CTPTR != NULL) {
	if ((TYPFORM)CTPTR->UU.U0.FORM == CLASSS)   /* CHECK CLASS */
	{  /* CHECK FOR PREDECLARED CLASS */
	  memcpy(LINK->TYPID, WITH->NAME, sizeof(alfa));
	  LINK->P = CTPTR->UU.U0.UU.PELTYPE;
	  if (PFTOP == FILLIMIT)
	    ERROR(92L);
	  else {
	    PFTOP++;
	    PFL[PFTOP] = LINK->LL;
	  }
	  for (J = PTX - 1; J >= 0; J--) {
	    WITH1 = &PTLIST[J];
	    if (!strncmp(WITH1->HNAME, LINK->TYPID, sizeof(alfa))) {
	      WITH1->PPTR->UU.U0.UU.U2.DOMAIN = LINK->N;
	      WITH1->PPTR->UU.U0.UU.U2.ELTYPE = LINK->P;
	      PTX--;
	      memcpy(WITH1->HNAME, PTLIST[PTX].HNAME, sizeof(alfa));
	      WITH1->PPTR = PTLIST[PTX].PPTR;
	    }  /* WITH, FOR */
	  }
	  WITH->UU.U3.VLEVEL = LEVEL;
	  WITH->UU.U3.VADDR = LINK->LL;
	} else {
	  if ((TYPFORM)CTPTR->UU.U0.FORM == FILES) {
	    if (FILTOP == FILLIMIT)
	      ERROR(92L);
	    else {
	      FILTOP++;
	      FILPTS[FILTOP] = LINK->N;
	      WITH->UU.U3.VADDR = LINK->LL;
	      if (LEVEL > 0) {
		/* TAKE EFET INTO ACCOUNT FOR ADDRESS */
		if (CTPTR->UU.U0.UU.FELTYPE == CHARPTR)
		  J = ALFALENG + 7;
		else
		  J = 7;
		LC += J;
		WITH->UU.U3.VADDR += I * J - 7;
		WITH->UU.U3.VLEVEL = LEVEL;
	      }
	    }
	  } else {
	    /*FORM = FILES*/
	    WITH->UU.U3.VLEVEL = LEVEL;
	    WITH->UU.U3.VADDR = LINK->LL;
	  }
	}
      }
      LINK->N = WITH->NXTEL;
    }
_L10:
    FINDSEMICOLON(LINK);
  }  /*WHILE NO = 1*/
  if (NO != 47) {   /* VALUE */
    LINK->VLC = LC;
    return;
  }
  /*VALUE*/
  if (LEVEL != 0)
    ERROR(22L);
  INSYMBOL();
  while (NO == 1) {
    SRCHREC(NEXT);
    if (CTPTR == NULL)
      SEARCH();
    if (CTPTR == NULL) {
      ERROR(12L);
      FINDSEMICOLON(LINK);
      goto _L20;
    }
    if ((IDCLASS)CTPTR->KLASS != VARS) {
      ERROR(26L);
      FINDSEMICOLON(LINK);
      goto _L20;
    }
    WITH = CTPTR;
    IT = WITH->UU.U3.VADDR;
    if (LINK->VLC == 0) {
      LINK->VLC = IT;
      LINK->LL = IT;
      AT = IT;
    }
    if (IT < LINK->LL)
      ERROR(27L);
    else {
      while (IT > AT) {
	PUT(PASCLGO, long);
	AT++;
      }
    }
    INSYMBOL();
    if (NO != 8 || CL != 6)
      ERROR(4L);
    else
      INSYMBOL();
    if (NO == 9) {   /* LIST */
      do {
	INSYMBOL();
/* p2c: pascal_p2c.pas, line 3437:
 * Warning: Type mismatch in VAR parameter V [295] */
	INCONST(&I, PT, NEXT);
	IT1 = 1;
	if (NO == 6 && CL == 1) {   /* * */
	  IT1 = I;
	  INSYMBOL();
/* p2c: pascal_p2c.pas, line 3440:
 * Warning: Type mismatch in VAR parameter V [295] */
	  INCONST(&I, PT, NEXT);
	}
	if (!ERR) {
	  FORLIM = IT1;
	  for (J = 1; J <= FORLIM; J++) {
	    TEMP = I;
	    fwrite(&TEMP, sizeof(long), 1, PASCLGO);
	  }
	  AT += IT1;
	}  /*NOT ERR*/
	if ((unsigned long)NO >= 32 || ((1L << NO) & 0x8400L) == 0)
	{   /* NOT , NOR ) */
	  ERROR(20L);
	  FINDSEMICOLON(LINK);
	  goto _L20;
	}
      } while (NO != 10);
      INSYMBOL();
    } else {
/* p2c: pascal_p2c.pas, line 3452:
 * Warning: Type mismatch in VAR parameter V [295] */
      INCONST(&I, PT, NEXT);
      if (!ERR) {
	TEMP = I;
	fwrite(&TEMP, sizeof(long), 1, PASCLGO);
	AT++;
      }
    }
    LINK->LL = IT + SEXT(WITH->UU.U3.VTYPE->UU.U0.SIZE, 18);
    if (LINK->LL < AT)
      ERROR(20L);
    FINDSEMICOLON(LINK);
_L20: ;
  }  /*WHILE NO = 1*/
  FORLIM1 = LC;
  for (AT++; AT <= FORLIM1; AT++)
    PUT(PASCLGO, long);
}  /*VARDECL*/

/* Local variables for FORMPARM: */
struct LOC_FORMPARM {
  struct LOC_BODY *LINK;
  jmp_buf _JL3;
} ;

Local Void FORMERR(LINK)
struct LOC_FORMPARM *LINK;
{
  ERROR(11L);
  SKIP(10L);
  longjmp(LINK->_JL3, 1);
}

Local Void FORMPARM(LINK)
struct LOC_BODY *LINK;
{
  struct LOC_FORMPARM V;
  /* ( HAS BEEN VERIFIED AND NEXT SYMBOL READ */
  IDCLASS SPEC = KONST;
  boolean REP;
  CONTEXTTABLE *WITH;

  V.LINK = LINK;
  if (setjmp(V._JL3))
    goto _L3;
_L1:
  do {
    if (NO == 45) {   /*PROCEDURE*/
      do {
	INSYMBOL();
	if (NO != 1)
	  ERROR(11L);
	else {
	  SRCHREC(NEXT);
	  if (CTPTR != NULL)
	    ERROR(8L);
	  else {
	    ALLOC(LINK->P, PROC);
/* p2c: pascal_p2c.pas, line 3483:
 * Warning: Symbol 'ALLOC' is not defined [221] */
	    WITH = LINK->P;
	    memcpy(WITH->NAME, AVAL, sizeof(alfa));
	    WITH->NXTEL = NEXT;
	    WITH->KLASS = (unsigned)PROC;
	    WITH->UU.U2.PROCTYPE = LINK->P;
	    WITH->UU.U2.PROCKIND = (unsigned)FORMAL;
	    WITH->UU.U2.PROCADDR = LC;
	    WITH->UU.U2.PROCLEVEL = LEVEL;
	    WITH->UU.U2.FORMALS = NULL;
	    LC++;
	    NEXT = LINK->P;
	  }
	  /*CTPTR = NIL*/
	  INSYMBOL();
	}
      } while (NO == 15);   /*NOT PROCEDURE*/
    } else {
      if (NO != 1) {
	if (NO == 43)
	  SPEC = VARS;
	else {
	  if (NO == 41)
	    SPEC = KONST;
	  else {
	    if (NO == 44)
	      SPEC = PROC;
	    else
	      FORMERR(&V);
	  }
	}
	INSYMBOL();
      } else if (SPEC != VARS)
	SPEC = KONST;
      if (NO != 1)
	FORMERR(&V);
      do {
	SRCHREC(NEXT);
	if (CTPTR != NULL)
	  ERROR(8L);
	else {
	  switch (SPEC) {

	  case KONST:
	    ALLOC(LINK->P, KONST, FORMAL);
/* p2c: pascal_p2c.pas, line 3509:
 * Warning: Symbol 'ALLOC' is not defined [221] */
	    WITH = LINK->P;
	    WITH->UU.U1.CONTYPE = NULL;
	    WITH->UU.U1.CONKIND = (unsigned)FORMAL;
	    WITH->UU.U1.UU.U1.CADDR = LC;
	    WITH->UU.U1.UU.U1.CLEVEL = LEVEL;
	    break;

	  case PROC:
	    ALLOC(LINK->P, PROC);
/* p2c: pascal_p2c.pas, line 3515:
 * Warning: Symbol 'ALLOC' is not defined [221] */
	    WITH = LINK->P;
	    WITH->UU.U2.PROCTYPE = NULL;
	    WITH->UU.U2.PROCKIND = (unsigned)FORMAL;
	    WITH->UU.U2.PROCADDR = LC;
	    WITH->UU.U2.PROCLEVEL = LEVEL;
	    WITH->UU.U2.FORMALS = NULL;
	    break;

	  case VARS:
	    ALLOC(LINK->P, VARS);
/* p2c: pascal_p2c.pas, line 3522:
 * Warning: Symbol 'ALLOC' is not defined [221] */
	    WITH = LINK->P;
	    WITH->UU.U3.VTYPE = NULL;
	    WITH->UU.U3.VKIND = (unsigned)FORMAL;
	    WITH->UU.U3.VADDR = LC;
	    WITH->UU.U3.VLEVEL = LEVEL;
	    break;
	  }
	  memcpy(LINK->P->NAME, AVAL, sizeof(alfa));
	  LINK->P->NXTEL = NEXT;
	  LINK->P->KLASS = (unsigned)SPEC;
	  LC++;
	  NEXT = LINK->P;
	}
	/*CTPTR = NIL*/
	INSYMBOL();
	if (NO == 15) {
	  INSYMBOL();
	  if (NO == 19)
	    ERROR(11L);
	} else if (NO != 19)
	  goto _L4;
      } while (NO == 1);
_L4:
      if (NO != 19)
	ERROR(10L);
      else
	INSYMBOL();
      if (NO != 1)
	FORMERR(&V);
      SEARCH();
      if (CTPTR == NULL) {
	ERROR(12L);
	goto _L2;
      }
      if ((IDCLASS)CTPTR->KLASS != TYPES) {
	ERROR(18L);
	goto _L2;
      }
      LINK->N = NEXT;
      do {
	switch (SPEC) {

	case KONST:
	  if (LINK->N->UU.U1.CONTYPE == NULL) {
	    if (((1L << CTPTR->UU.U0.FORM) & ((1L << ((long)NUMERIC)) |
		   (1L << ((long)SYMBOLIC)) | (1L << ((long)POWER)))) == 0) {
	      LINK->N->KLASS = (unsigned)VARS;
	      if ((TYPFORM)CTPTR->UU.U0.FORM == POINTER)
		LINK->N->UU.U3.VKIND = (unsigned)ACTUAL;
	    }
	    LINK->N->UU.U1.CONTYPE = CTPTR;
	    LINK->N = LINK->N->NXTEL;
	  } else
	    LINK->N = NULL;
	  break;

	case PROC:
	  if (LINK->N->UU.U2.PROCTYPE == NULL) {
	    if (CTPTR->UU.U0.FORM > (long)POWER) {
	      ERROR(93L);
	      LINK->N = NULL;
	    } else {
	      LINK->N->UU.U2.PROCTYPE = CTPTR;
	      LINK->N = LINK->N->NXTEL;
	    }
	  } else
	    LINK->N = NULL;
	  break;

	case VARS:
	  if (LINK->N->UU.U3.VTYPE == NULL) {
	    LINK->N->UU.U3.VTYPE = CTPTR;
	    LINK->N = LINK->N->NXTEL;
	  } else
	    LINK->N = NULL;
	  break;
	}
      } while (LINK->N != NULL);
_L2:
      INSYMBOL();
    }
    /*NOT PROCEDURE*/
_L3:
    REP = (NO == 1 || NO == 45 || NO == 44 || NO == 43 || NO == 41);
    if (NO == 16) {
      INSYMBOL();
      REP = (NO != 10);
    }
  } while (REP);
  if (NO != 10) {
    ERROR(9L);
    SKIP(10L);
    if (NO == 45 || NO == 44 || NO == 43 || NO == 41)
      goto _L1;
  }
  /*REVERSE POINTERS*/
  LINK->N = NEXT;
  NEXT = NULL;
  while (LINK->N != NULL) {
    LINK->P = LINK->N;
    LINK->N = LINK->P->NXTEL;
    LINK->P->NXTEL = NEXT;
    NEXT = LINK->P;
  }
}  /*FORMPARM*/

Local Void OUTPMD(SURRPTR, LINK)
CONTEXTTABLE *SURRPTR;
struct LOC_BODY *LINK;
{
  CONTEXTTABLE *N, *P;
  SHRTINT ADR, ACC;
  boolean OUT;
  _REC_PMDFILE *WITH;
  CONTEXTTABLE *WITH1;

  WITH = &GETFBUF(PMDFILE, _REC_PMDFILE);
/* p2c: pascal_p2c.pas, line 3590:
 * Note: Discovered too late that PMDFILE should be buffered [143] */
  if (SURRPTR == NULL) {
    memcpy(WITH->PNAME, "(MAIN)    ", sizeof(alfa));
    WITH->UU.UM1.SA = IC;
  } else {
    memcpy(WITH->PNAME, SURRPTR->NAME, sizeof(alfa));
    WITH->UU.UM1.SA = SURRPTR->UU.U2.PROCADDR;
  }
  WITH->PKIND = PROCDR;
  WITH->UU.UM1.LINK_ = LASTLINK;
  WITH->UU.UM1.COUNT = 0;
  PUT(PMDFILE, _REC_PMDFILE);
  PMCTR += 2;
  LASTLINK = PMCTR;
  N = NEXT;
  while (N != NULL) {
    WITH1 = N;
    /*WITH, WHILE*/
    OUT = false;
    ACC = DIRECT;
    switch ((IDCLASS)WITH1->KLASS) {

    case KONST:
      if ((IDKINDS)WITH1->UU.U1.CONKIND == FORMAL) {
	OUT = true;
	P = WITH1->UU.U1.CONTYPE;
	ADR = WITH1->UU.U1.UU.U1.CADDR;
      }
      break;

    case VARS:
      OUT = true;
      P = WITH1->UU.U3.VTYPE;
      ADR = WITH1->UU.U3.VADDR;
      if ((IDKINDS)WITH1->UU.U3.VKIND == FORMAL)
	ACC = INDIRECT;
      break;

    case TYPES:
    case PROC:
      /* blank case */
      break;
    }
    if (OUT) {
      if (P->UU.U0.FORM < (long)POWER) {
	WITH = &GETFBUF(PMDFILE, _REC_PMDFILE);
	memcpy(WITH->PNAME, WITH1->NAME, sizeof(alfa));
	WITH->PKIND = OBJ;
	if ((TYPFORM)P->UU.U0.FORM == NUMERIC)
	  WITH->UU.U1.TYP = 1;
	else {
	  if (P == REALPTR)
	    WITH->UU.U1.TYP = 2;
	  else {
	    if (P == ALFAPTR)
	      WITH->UU.U1.TYP = 3;
	    else {
	      if (P == CHARPTR)
		WITH->UU.U1.TYP = 4;
	      else {
		if (P == BOOLPTR)
		  WITH->UU.U1.TYP = 5;
		else {
		  if ((TYPFORM)P->UU.U0.FORM == SYMBOLIC)
		    WITH->UU.U1.TYP = 1;
		  else
		    WITH->UU.U1.TYP = 6;
		}
	      }
	    }
	  }
	}
	WITH->UU.U1.RLA = ADR;
	WITH->UU.U1.ACCESS = ACC;
	PUT(PMDFILE, _REC_PMDFILE);
	PMCTR += 2;
      }
    }
    N = WITH1->NXTEL;
  }
}  /*OUTPMD*/

#undef DIRECT
#undef INDIRECT

Local Void ENTERBODY(LINK)
struct LOC_BODY *LINK;
{
  /*GENERATES PROCEDURE PROLOG CODE AND PROCEDURE ENTRY CODE, CODE
   TO INITIALIZE EFETS AND TO OPEN LOCAL FILES, AND CODE TO INITIALIZE
   LOCAL CLASSPOINTERS*/
  SHRTINT IT2;
  long FORLIM;
  CONTEXTTABLE *WITH;

  CA = 0;
  CP = 4;
  LCX = 0;
  if (LEVEL != 0) {  /*PROCEDURE ENTRY CODE*/
    GEN15(62L, 0L, 5L, 0L);
    GEN15(16L, 0L, 0L, 18L);
    GEN15(10L, 7L, 7L, 0L);
    GEN15(54L, 5L, 6L, 0L);
    GEN30(41L, 7L, 5L, 1L);
    GEN30(49L, 6L, 6L, 0L);
    LINK->LCA = CA;
    LINK->LCP = CP;
    if (LEVEL != 1)
      GEN15(46L, 6L, 5L, 0L);
    FORLIM = FILTOP;
    /*CODE TO INITIALIZE EFETS*/
    for (IT = FILEV[LEVEL]; IT <= FORLIM; IT++) {
      WITH = FILPTS[IT];
      memcpy(LINK->LCH, WITH->NAME, sizeof(CWORD));
      IT2 = 7;
      while (LINK->LCH[IT2-1] == ' ')
	IT2--;
      RP = 0;
      LDCST(INTBITS(WITH->NAME));
      GEN15(35L, 0L, 0L, IT2 * 6);
      GEN15(9L, 6L, 0L, 1L);
      GEN30(41L, 6L, 5L, WITH->UU.U3.VADDR + 2L);
      GEN30(57L, 6L, 0L,
	    (long)SEXT(WITH->UU.U3.VTYPE->UU.U0.UU.FELTYPE->UU.U0.SIZE, 18));
      if (WITH->UU.U3.VTYPE->UU.U0.UU.FELTYPE != CHARPTR) {
	GEN15(35L, 0L, 0L, 1L);
	GEN15(16L, 0L, 0L, 56L);
	GEN15(10L, 6L, 6L, 0L);
      }
      GEN30(41L, 6L, 5L, WITH->UU.U3.VADDR + 1L);
      GEN30(57L, 6L, 5L, WITH->UU.U3.VADDR + 7L);
      GEN30(41L, 6L, 5L, WITH->UU.U3.VADDR + 3L);
      GEN30(58L, 6L, 6L, (long)SEXT(WITH->UU.U3.VTYPE->UU.U0.SIZE, 18));
      GEN30(41L, 6L, 5L, WITH->UU.U3.VADDR + 6L);
    }
  } else {
    /*MAIN PROGRAM, SAVE X7,B5 SET B6*/
    GEN15(46L, 7L, 5L, 0L);
    GEN15(62L, 7L, 5L, 0L);
    GEN30(41L, 7L, 0L, (long)PARMLIST.ICO);
    GEN30(49L, 6L, 5L, 0L);
    LINK->LCA = CA;
    LINK->LCP = CP;
    LC = 1;
  }
  if (STOFLCHECK)
    GEN30(6L, 6L, 4L, (long)STOFLERR);
  /*CODE TO OPEN FILES*/
  if (LEVEL == 0)
    IT2 = 0;
  else
    IT2 = 5;
  FORLIM = FILTOP;
  for (IT = FILEV[LEVEL]; IT <= FORLIM; IT++) {
    GEN30(49L, 7L, IT2, FILPTS[IT]->UU.U3.VADDR + 1L);
    JUMPTO(OPN);
  }
  FORLIM = PFTOP;
  /*CODE TO INITIALIZE CLASSPOINTERS*/
  for (IT = PILEV[LEVEL]; IT <= FORLIM; IT++) {
    IT1 = PFL[IT];
    GEN30(57L, 7L, IT2, IT1 + 1);
    GEN30(41L, 7L, IT2, IT1);
  }
  PFTOP = PILEV[LEVEL] - 1;
  TCT = 0;
  TMAX = 0;
  CLABIX = 0;
}  /*ENTERBODY*/

#undef OPN

Local Void ERRMESSAGE(ALF, VAL)
Char *ALF;
SHRTINT VAL;
{
  ERROR(43L);
/* p2c: pascal_p2c.pas, line 3689:
 * Note: Null character at end of sprintf control string [148] */
  printf("");
  PRTERR();
/* p2c: pascal_p2c.pas, line 3690:
 * Note: Null character at end of sprintf control string [148] */
  printf(" %.6s%6ld", ALF, VAL);
  if (!EOLFLAG)
    printf("%*c", (int)(CHCNT + 8), ' ');
}  /*ERRMESSAGE*/

Local Void LEAVEBODY(LINK)
struct LOC_BODY *LINK;
{
  long IT2, FORLIM;
  _REC_LABTAB *WITH;
  _REC_EXTAB *WITH1;
  long FORLIM1;

  if (LEVEL == 0)
    IT2 = 0;
  else
    IT2 = 5;
  FORLIM = FILTOP;
  for (IT = FILEV[LEVEL]; IT <= FORLIM; IT++) {
    GEN30(49L, 7L, IT2, FILPTS[IT]->UU.U3.VADDR + 1L);
    JUMPTO(CLS);
  }
  FILTOP = FILEV[LEVEL] - 1;
  if (LEVEL != 0) {   /*PROCEDURE EXIT CODE*/
    GEN30(41L, 1L, 5L, 1L);
    GEN15(54L, 6L, 5L, 0L);
    GEN15(51L, 7L, 1L, 0L);
    GEN15(16L, 1L, 0L, 42L);
    GEN15(51L, 5L, 1L, 0L);
  } else {
    GEN15(46L, 1L, 5L, 0L);
    GEN15(51L, 7L, 1L, 0L);
  }
  /*MAIN, RETURN TO MONITOR*/
  GEN30(2L, 7L, 0L, 0L);
  LC += TMAX;
  INS(LC, LINK->LCP, LINK->LCA);
  FORLIM = CLABIX;
  for (IT = 1; IT <= FORLIM; IT++) {
    WITH = &LABTAB[IT-1];
    /*WITH, FOR*/
    if (SEXT(WITH->FLD2, 18) > 0) {   /*UNDECLARED  LABEL*/
      ERRMESSAGE("LABEL:    ", (long)SEXT(WITH->LABVAL, 18));
      UNDLAB[SEXT(WITH->FLD2, 18) - 1].SUCC = CHNIX;
      CHNIX = SEXT(WITH->FLD3, 18);
    }  /*IF FLD2 > 0*/
  }
  FORLIM = CEXTABIX;
  for (IT = LINK->FSTIX; IT <= FORLIM; IT++) {
    WITH1 = &EXTAB[IT-1];
    /*WITH, FOR*/
    FORLIM1 = CLABIX;
    for (IT1 = 1; IT1 <= FORLIM1; IT1++) {
      WITH = &LABTAB[IT1-1];
      if (SEXT(WITH->LABVAL, 18) == SEXT(WITH1->EXVAL, 18)) {
	if (SEXT(WITH->FLD2, 18) > 0)
	  ERRMESSAGE("EXIT:     ", (long)SEXT(WITH1->EXVAL, 18));
	else {
	  IT2 = JMPCST;
	  INSERT((long)LC, 30L, &IT2);
	  INSERT((long)SEXT(WITH->FLD3, 18), 0L, &IT2);
	  JMPTAB[SEXT(WITH1->JMPTABIX, 18)] = IT2;
	}
	goto _L2;
      }  /*IF, WITH, FOR*/
    }
    ERRMESSAGE("EXIT:     ", (long)SEXT(WITH1->EXVAL, 18));
_L2: ;
  }
  CEXTABIX = LINK->FSTIX - 1;
  WRITOUT();
  if (LEVEL == 0) {   /* WRITE OUT JUMPTABLE */
    for (IT = 0; IT <= JMPMAX; IT++)
      fwrite(&JMPTAB[IT], sizeof(long), 1, PASCLGO);
  }
}  /*LEAVEBODY*/

#undef CLS


/* ------------------------------------------------------------ */

Static Void BODY(SURRPTR, FIRSTENTRY)
CONTEXTTABLE *SURRPTR, *FIRSTENTRY;
{
  struct LOC_BODY V;

  long I, J;
  RG3 OLDLEV;
  SHRTINT LC1;
  CONTEXTTABLE *PROCPTR, *LFIRSTENTRY;
  long FORLIM;
  _REC_EXTAB *WITH;
  CONTEXTTABLE *WITH1;
  _REC_DISPLAY *WITH2;
  long TEMP;

  if (setjmp(V._JL1))
    goto _L1;
  PILEV[LEVEL] = PFTOP + 1;
  V.VLC = 0;
  FILEV[LEVEL] = FILTOP + 1;
  V.FSTIX = CEXTABIX + 1;
_L1:
  if (NO == 40) {   /*LABEL*/
    do {
      INSYMBOL();
      if (NO != 2 || CL != 1) {   /*IF (NO=2) AND (CL=1)*/
	ERROR(61L);
	goto _L3;
      }
      if (IVAL >= TWOTO17) {
	ERROR(100L);
	goto _L2;
      }
      FORLIM = CEXTABIX;
      for (IT = V.FSTIX; IT <= FORLIM; IT++) {
	if (SEXT(EXTAB[IT-1].EXVAL, 18) == IVAL) {
	  ERROR(77L);
	  goto _L2;
	}
      }
      if (CEXTABIX == MAXEXLABS)
	ERROR(78L);
      else {
	CEXTABIX++;
	if (JMPIX > JMPMAX)
	  ERROR(91L);
	else {
	  WITH = &EXTAB[CEXTABIX-1];
	  WITH->EXVAL = IVAL;
	  WITH->JMPTABIX = JMPIX;
	  JMPTAB[JMPIX] = LEVEL;
	  JMPIX++;
	}
      }
_L2:
      INSYMBOL();
    } while (NO == 15);
    /*,*/
_L3:
    FINDSEMICOLON(&V);
  }
  if (NO == 41) {   /* CONST */
    INSYMBOL();
    while (NO == 1) {
      do {
	SRCHREC(NEXT);
	if (CTPTR != NULL)
	  ERROR(8L);
	ALLOC(V.P, KONST, ACTUAL);
/* p2c: pascal_p2c.pas, line 3763:
 * Warning: Symbol 'ALLOC' is not defined [221] */
	WITH1 = V.P;
	memcpy(WITH1->NAME, AVAL, sizeof(alfa));
	WITH1->NXTEL = NEXT;
	WITH1->KLASS = (unsigned)KONST;
	WITH1->UU.U1.CONKIND = (unsigned)ACTUAL;
	NEXT = V.P;
	INSYMBOL();
	if (NO != 8 || CL != 6)
	  ERROR(4L);
	else
	  INSYMBOL();
	if (NO == 36) {   /* NIL */
	  V.P->UU.U1.CONTYPE = NILPTR;
	  V.P->UU.U1.UU.U0.VALUES = NILVAL;
	  INSYMBOL();
	} else {
	  INCONST(&I, V.N, NEXT->NXTEL);
	  V.P->UU.U1.CONTYPE = V.N;
	  V.P->UU.U1.UU.U0.VALUES = I;
	}
	while (NO == 15) {
	  INSYMBOL();
	  if (NO == 1)
	    break;
	  ERROR(11L);
	  SKIP(15L);
	}
      } while (NO == 1);
      FINDSEMICOLON(&V);
    }  /* WHILE NO = 1 */
  }  /*CONST*/
  if (NO == 37) {   /*TYPE*/
    INSYMBOL();
    while (NO == 1) {
      SRCHREC(NEXT);
      if (CTPTR != NULL)
	ERROR(8L);
      memcpy(V.TYPID, AVAL, sizeof(alfa));
      INSYMBOL();
      if (NO != 8 || CL != 6)
	ERROR(4L);
      else
	INSYMBOL();
      ERR = false;
      TYPEDECL(&V.TL, V.P);
      if (!ERR) {
	if (strncmp(V.P->NAME, BLANK, sizeof(alfa)))
	  ERROR(96L);
	else {
	  memcpy(V.P->NAME, V.TYPID, sizeof(alfa));
	  V.P->NXTEL = NEXT;
	  NEXT = V.P;
	}
      }
      FINDSEMICOLON(&V);
    }
  }  /*TYPE*/
  if (NO == 43)   /*VAR*/
    VARDECL(&V);
  else
    V.VLC = LC;
  if (PTX > 0) {
    ERROR(12L);
/* p2c: pascal_p2c.pas, line 3804:
 * Note: Null character at end of sprintf control string [148] */
    printf("");
    PRTERR();
    for (PTX--; PTX >= 0; PTX--)
      printf(" CLASS-ID%.10s", PTLIST[PTX].HNAME);
/* p2c: pascal_p2c.pas, line 3806:
 * Note: Null character at end of sprintf control string [148] */
    if (!EOLFLAG)
      printf("%*c", (int)(CHCNT + 8), ' ');
  }
  if (LEVEL == 0) {
    FORLIM = FILTOP;
    for (I = 0; I <= FORLIM; I++) {
      WITH1 = FILPTS[I];
      if (WITH1->UU.U3.VTYPE->UU.U0.UU.FELTYPE == CHARPTR) {
	IT = 0;
	LC += ALFALENG;
	for (J = 1; J <= ALFALENG; J++)   /*CHAR BUFFER*/
	  PUT(PASCLGO, long);
      } else
	IT = 1;
      PUT(PASCLGO, long);   /* P-PTR */
      J = WITH1->UU.U3.VLEVEL * 2 + IT;   /*SPECIFIER*/
      WITH1->UU.U3.VLEVEL = 0;
      memcpy(V.LCH, WITH1->NAME, sizeof(CWORD));
      IT1 = 7;
      while (V.LCH[IT1-1] == ' ')
	IT1--;
      IT1 = (ALFALENG - IT1) * 6;
      APPEND(&J, 55L,
	     (long)SEXT(WITH1->UU.U3.VTYPE->UU.U0.UU.FELTYPE->UU.U0.SIZE, 18));
      fwrite(&J, sizeof(long), 1, PASCLGO);
      /* SPECIFIER + LRL */
      J = INTBITS(WITH1->NAME);
      APPEND(&J, -IT1, 0L);
      APPEND(&J, IT1, 0L);
      fwrite(&J, sizeof(long), 1, PASCLGO);
      /* LFN */
      PUTFBUF(PASCLGO, long, WITH1->UU.U3.VADDR);
      for (J = 1; J <= 3; J++)   /*FIRST,IN,OUT*/
	PUT(PASCLGO, long);
      TEMP = WITH1->UU.U3.VADDR + SEXT(WITH1->UU.U3.VTYPE->UU.U0.SIZE, 18);
      fwrite(&TEMP, sizeof(long), 1, PASCLGO);
      /* LIMIT */
      WITH1->UU.U3.VADDR = LC;
      LC += 7;
    }
    PARMLIST.LPJMPTAB = LC;
    if (V.VLC != LC) {
      for (J = 0; J <= JMPMAX; J++)
	PUT(PASCLGO, long);
    } else
      V.VLC = LC + JMPMAX + 1;
    LC += JMPMAX + 1;
    IC = LC;
    PARMLIST.LOADPT = V.VLC;   /*LOADPT*/
  }
  DP = false;
  if (NO == 45 || NO == 44) {   /*FUNCTION OR PROCEDURE*/
    if (SURRPTR != NULL) {
      if (SURRPTR->UU.U2.PROCADDR == 0) {
	if (JMPIX > JMPMAX)
	  ERROR(91L);
	else {
	  SURRPTR->UU.U2.PROCADDR = PARMLIST.LPJMPTAB + JMPIX;
	  JMPIX++;
	}
      }
    }
    do {
      V.LL = NO;
      ERR = false;
      OLDLEV = LEVEL;
      if (LEVEL < MAXLEVEL)
	LEVEL++;
      else
	ERROR(76L);
      INSYMBOL();
      if (NO != 1) {
	ERROR(11L);
	LEVEL = OLDLEV;
	goto _L1;
      }
      LC1 = LC;
      SRCHREC(NEXT);
      if (CTPTR != NULL) {
	if ((IDCLASS)CTPTR->KLASS != PROC) {
	  ERROR(8L);
	  CTPTR = NULL;
	}
      }
      if (CTPTR == NULL) {   /* UNDECLARED PROCEDURE */
	ALLOC(PROCPTR, PROC);
/* p2c: pascal_p2c.pas, line 3859:
 * Warning: Symbol 'ALLOC' is not defined [221] */
	memcpy(PROCPTR->NAME, AVAL, sizeof(alfa));
	PROCPTR->NXTEL = NEXT;
	PROCPTR->KLASS = (unsigned)PROC;
	PROCPTR->UU.U2.PROCTYPE = PROCPTR;
	PROCPTR->UU.U2.PROCKIND = (unsigned)ACTUAL;
	PROCPTR->UU.U2.PROCLEVEL = LEVEL - 1;
	DISPLAY[TOP].FNAME = PROCPTR;
	NEXT = NULL;
	INSYMBOL();
	if (V.LL == 44) {   /* FUNCTION */
	  if (NO != 9)
	    ERROR(29L);
	  else {
	    LC = 3;
	    INSYMBOL();
	    FORMPARM(&V);
	  }
	  if (ERR)
	    PROCPTR->UU.U2.PROCTYPE = NULL;
	  if (NO == 10) {
	    INSYMBOL();
	    if (NO != 19)
	      ERROR(10L);
	    else
	      INSYMBOL();
	    if (NO != 1) {
	      ERROR(11L);
	      SKIP(49L);
	      PROCPTR->UU.U2.PROCTYPE = NULL;
	    } else {
	      SEARCH();
	      if (CTPTR != NULL) {
		if ((IDCLASS)CTPTR->KLASS != TYPES) {
		  ERROR(93L);
		  CTPTR = NULL;
		} else {
		  if (CTPTR->UU.U0.FORM > (long)POWER) {
		    ERROR(93L);
		    CTPTR = NULL;
		  }
		}
	      } else
		ERROR(12L);
	      PROCPTR->UU.U2.PROCTYPE = CTPTR;
	      INSYMBOL();
	    }
	  }  /*NO = 10*/
	}
	/* FUNCTION */
	else {   /* PARMLIST */
	  LC = 3;
	  if (NO == 9) {
	    INSYMBOL();
	    FORMPARM(&V);
	    if (ERR)
	      PROCPTR->UU.U2.PROCTYPE = NULL;
	    if (NO == 10)
	      INSYMBOL();
	  }  /*NO = 9*/
	}
	/* PROCEDURE */
	if (NO != 16) {
	  PROCPTR->UU.U2.PROCTYPE = NULL;
	  ERROR(58L);
	  SKIP(16L);
	} else
	  INSYMBOL();
	PROCPTR->UU.U2.FORMALS = NEXT;
	if (NO == 1 && !strncmp(AVAL, "FORWARD   ", sizeof(alfa))) {
	  NEXT = PROCPTR;
	  if (JMPIX > JMPMAX)
	    ERROR(91L);
	  else {
	    PROCPTR->UU.U2.PROCADDR = PARMLIST.LPJMPTAB + JMPIX;
	    JMPIX++;
	  }
	  PROCPTR->UU.U2.SEGSIZE = -LC;
	  /* SEGSIZE < 0 SIGNIFIES FORWARD-DECLARATION */
	  INSYMBOL();
	} else {
	  TOP = LEVEL + 1;
	  WITH2 = &DISPLAY[TOP];
	  WITH2->FNAME = NEXT;
	  WITH2->OCCUR = (unsigned)BLOCK;
	  PROCPTR->UU.U2.PROCADDR = 0;
	  ALLOC(LFIRSTENTRY, DUMMYCLASS);
/* p2c: pascal_p2c.pas, line 3918:
 * Warning: Symbol 'ALLOC' is not defined [221] */
	  BODY(PROCPTR, LFIRSTENTRY);
	}
      }
      /* NEW PROC */
      else {
	if (SEXT(CTPTR->UU.U2.SEGSIZE, 18) >= 0)
	      /* PREV. DECL NOT FORWARD */
		ERROR(16L);
	INSYMBOL();
	if (NO == 9) {   /* IGNORE PARM-LIST */
	  ERROR(23L);
	  do {
	    SKIP(10L);
	    if (NO == 16 || NO == 45 || NO == 44 || NO == 43 || NO == 41)
	      INSYMBOL();
	  } while (NO == 16 || NO == 45 || NO == 44 || NO == 43 || NO == 41);
	  if (NO != 10)
	    ERROR(9L);
	  else
	    INSYMBOL();
	}
	if (NO == 15)
	  SKIP(16L);
	if (NO != 16)
	  ERROR(58L);
	else
	  INSYMBOL();
	if (NO == 1 && !strncmp(AVAL, "FORWARD   ", sizeof(alfa)))
	      /*AGAIN FORWARD*/
		INSYMBOL();
	else {
	  PROCPTR = CTPTR;
	  LC = -SEXT(PROCPTR->UU.U2.SEGSIZE, 18);
	  NEXT = PROCPTR->UU.U2.FORMALS;
	  TOP = LEVEL + 1;
	  WITH2 = &DISPLAY[TOP];
	  WITH2->FNAME = NEXT;
	  WITH2->OCCUR = (unsigned)BLOCK;
	  ALLOC(LFIRSTENTRY, DUMMYCLASS);
/* p2c: pascal_p2c.pas, line 3944:
 * Warning: Symbol 'ALLOC' is not defined [221] */
	  BODY(PROCPTR, LFIRSTENTRY);
	  /*NOT FORWARD*/
	}
      }
      /* PROC ID ALREADY DECLARED */
      /* OLD PROCEDURE */
      LC = LC1;
      LEVEL = OLDLEV;
      FINDSEMICOLON(&V);
    } while (NO == 45 || NO == 44);
  }  /* FUNCTION OR PROCEDURE */
  DISPLAY[TOP].FNAME = NEXT;
  if (NO != 21) {
    ERROR(24L);
    SKIP(49L);
    while ((unsigned long)NO < 32 && ((1L << NO) & 0x410000L) != 0) {
      INSYMBOL();
      SKIP(49L);
    }
    if (NO == 45 || NO == 44 || NO == 43 || NO == 41 || NO == 40 || NO == 37)
      goto _L1;
  } else {
    if (SURRPTR == NULL)
      PARMLIST.ENTRYPT = IC;
    else {
      if (SURRPTR->UU.U2.PROCADDR != 0) {
	I = IC;
	APPEND(&I, 30L, EQJMP);
	JMPTAB[SURRPTR->UU.U2.PROCADDR - PARMLIST.LPJMPTAB] = I;
      }
      SURRPTR->UU.U2.PROCADDR = IC;
    }
  }
  V.N = NEXT;
  ERR = false;
  while (V.N != NULL) {
    if ((IDCLASS)V.N->KLASS != PROC) {
      V.N = NULL;
      break;
    }
    if (SEXT(V.N->UU.U2.SEGSIZE, 18) < 0) {   /*FORWARD, NO REAL DECLARATION*/
      if (!ERR) {
	ERROR(12L);
/* p2c: pascal_p2c.pas, line 3975:
 * Note: Null character at end of sprintf control string [148] */
	printf("");
	PRTERR();
      }
/* p2c: pascal_p2c.pas, line 3976:
 * Note: Null character at end of sprintf control string [148] */
      printf(" PROCEDURE%2c%.10s", ' ', V.N->NAME);
    }
    V.N = V.N->NXTEL;
  }
  /*IF, WHILE*/
  if (ERR) {
    if (!EOLFLAG)
      printf("%*c", (int)(CHCNT + 8), ' ');
  }
  if (PARMLIST.ERRFLAG == false)
    OUTPMD(SURRPTR, &V);
  ENTERBODY(&V);
  COMPSTAT();
  LEAVEBODY(&V);
  if (SURRPTR != NULL)
    SURRPTR->UU.U2.SEGSIZE = LC;
  ALLOC(V.P, DUMMYCLASS);
/* p2c: pascal_p2c.pas, line 3986:
 * Warning: Symbol 'ALLOC' is not defined [221] */
  if (V.P > MAXCTP)
    MAXCTP = V.P;
  rewind(FIRSTENTRY);
  TOP = LEVEL;
  NEXT = DISPLAY[TOP].FNAME;
}  /* BODY */


/* ------------------------------------------------------------ */
/* M A I N   P R O G R A M */

main(argc, argv)
int argc;
Char *argv[];
{
  CONTEXTTABLE *WITH;
  _REC_DISPLAY *WITH1;

  PASCAL_MAIN(argc, argv);
  PMDFILE = NULL;
  PASCLGO = NULL;
  /* INITIALIZE CONTEXT-TABLE */
  ALLOC(NILPTR, TYPES, POINTER);   /*TYPE OF NIL*/
/* p2c: pascal_p2c.pas, line 3998:
 * Warning: Symbol 'ALLOC' is not defined [221] */
  WITH = NILPTR;
  memcpy(WITH->NAME, BLANK, sizeof(alfa));
  WITH->NXTEL = NULL;
  WITH->KLASS = (unsigned)TYPES;
  WITH->UU.U0.SIZE = 0;
  WITH->UU.U0.FORM = (unsigned)POINTER;
  WITH->UU.U0.UU.U2.DOMAIN = NULL;
  WITH->UU.U0.UU.U2.ELTYPE = NULL;
  NEXT = NULL;
  for (IT = 0; IT <= 11; IT++) {   /*TEXT, WEOR, GET, PUT, RESET, ALLOC*/
    ALLOC(PT, PROC);   /*PACK, UNPACK, INSERT, APPEND, READ, WRITE*/
/* p2c: pascal_p2c.pas, line 4005:
 * Warning: Symbol 'ALLOC' is not defined [221] */
    WITH = PT;
    memcpy(WITH->NAME, INITNAM[IT], sizeof(alfa));
    WITH->NXTEL = NEXT;
    WITH->KLASS = (unsigned)PROC;
    WITH->UU.U2.PROCTYPE = PT;
    WITH->UU.U2.FORMALS = NULL;
    WITH->UU.U2.PROCADDR = 0;
    WITH->UU.U2.PROCLEVEL = 0;
    WITH->UU.U2.SEGSIZE = IT;
    WITH->UU.U2.PROCKIND = (unsigned)ACTUAL;
    NEXT = PT;
  }
  for (IT = 1; IT <= 9; IT++) {
    ALLOC(PT, PROC);   /*ODD, INT, CHR, EOF, ABS, SQR, TRUNC,*/
/* p2c: pascal_p2c.pas, line 4014:
 * Warning: Symbol 'ALLOC' is not defined [221] */
    WITH = PT;   /*PRED, SUCC*/
    memcpy(WITH->NAME, INITNAM[IT+11], sizeof(alfa));
    WITH->NXTEL = NEXT;
    WITH->KLASS = (unsigned)PROC;
    WITH->UU.U2.PROCTYPE = NILPTR;
    WITH->UU.U2.FORMALS = NULL;
    WITH->UU.U2.PROCADDR = 0;
    WITH->UU.U2.PROCLEVEL = 0;
    WITH->UU.U2.SEGSIZE = IT;
    WITH->UU.U2.PROCKIND = (unsigned)ACTUAL;
    NEXT = PT;
  }
  ALLOC(CTPTR, TYPES, FILES);   /*TYPE OF INPUT AND OUTPUT*/
/* p2c: pascal_p2c.pas, line 4022:
 * Warning: Symbol 'ALLOC' is not defined [221] */
  WITH = CTPTR;
  memcpy(WITH->NAME, BLANK, sizeof(alfa));
  WITH->NXTEL = NULL;
  WITH->KLASS = (unsigned)TYPES;
  WITH->UU.U0.SIZE = 513;
  WITH->UU.U0.FORM = (unsigned)FILES;
  FILPTS[1] = CTPTR;
  for (IT = 22; IT <= 23; IT++) {
    ALLOC(PT, VARS);   /*INPUT, OUTPUT*/
/* p2c: pascal_p2c.pas, line 4028:
 * Warning: Symbol 'ALLOC' is not defined [221] */
    WITH = PT;
    memcpy(WITH->NAME, INITNAM[IT+1], sizeof(alfa));
    WITH->NXTEL = NEXT;
    WITH->KLASS = (unsigned)VARS;
    WITH->UU.U3.VTYPE = CTPTR;
    WITH->UU.U3.VKIND = (unsigned)ACTUAL;
    if (IT == 22)
      WITH->UU.U3.VADDR = INPT;
    else
      WITH->UU.U3.VADDR = OUTPT;
    WITH->UU.U3.VLEVEL = 0;
    NEXT = PT;
  }
  ALLOC(ALFAPTR, TYPES, SYMBOLIC);   /*ALFA*/
/* p2c: pascal_p2c.pas, line 4037:
 * Warning: Symbol 'ALLOC' is not defined [221] */
  WITH = ALFAPTR;
  memcpy(WITH->NAME, INITNAM[25], sizeof(alfa));
  WITH->NXTEL = NEXT;
  WITH->KLASS = (unsigned)TYPES;
  WITH->UU.U0.SIZE = 1;
  WITH->UU.U0.FORM = (unsigned)SYMBOLIC;
  WITH->UU.U0.UU.U1.FCONST = NULL;
  WITH->UU.U0.UU.U1.PWSET = NULL;
  WITH->UU.U0.UU.U1.BITSIZE = WORDLENGTH;
  ALLOC(REALPTR, TYPES, SYMBOLIC);   /*REAL*/
/* p2c: pascal_p2c.pas, line 4043:
 * Warning: Symbol 'ALLOC' is not defined [221] */
  WITH = REALPTR;
  memcpy(WITH->NAME, INITNAM[26], sizeof(alfa));
  WITH->NXTEL = ALFAPTR;
  WITH->KLASS = (unsigned)TYPES;
  WITH->UU.U0.SIZE = 1;
  WITH->UU.U0.FORM = (unsigned)SYMBOLIC;
  WITH->UU.U0.UU.U1.FCONST = NULL;
  WITH->UU.U0.UU.U1.PWSET = NULL;
  WITH->UU.U0.UU.U1.BITSIZE = WORDLENGTH;
  ALLOC(CHARPTR, TYPES, SYMBOLIC);   /*CHAR*/
/* p2c: pascal_p2c.pas, line 4049:
 * Warning: Symbol 'ALLOC' is not defined [221] */
  WITH = CHARPTR;
  memcpy(WITH->NAME, INITNAM[27], sizeof(alfa));
  WITH->NXTEL = REALPTR;
  WITH->KLASS = (unsigned)TYPES;
  WITH->UU.U0.SIZE = 1;
  WITH->UU.U0.FORM = (unsigned)SYMBOLIC;
  WITH->UU.U0.UU.U1.BITSIZE = 7;
  ALLOC(PT, KONST, ACTUAL);   /*' '*/
/* p2c: pascal_p2c.pas, line 4054:
 * Warning: Symbol 'ALLOC' is not defined [221] */
  WITH = PT;
  memcpy(WITH->NAME, BLANK, sizeof(alfa));
  WITH->NXTEL = NULL;
  WITH->KLASS = (unsigned)KONST;
  WITH->UU.U1.CONTYPE = CHARPTR;
  WITH->UU.U1.CONKIND = (unsigned)ACTUAL;
  WITH->UU.U1.UU.U0.VALUES = 63;
  WITH->UU.U1.UU.U0.SUCC = NULL;
  CHARPTR->UU.U0.UU.U1.FCONST = PT;
  ALLOC(BOOLPTR, TYPES, SYMBOLIC);   /*BOOLEAN*/
/* p2c: pascal_p2c.pas, line 4060:
 * Warning: Symbol 'ALLOC' is not defined [221] */
  WITH = BOOLPTR;
  memcpy(WITH->NAME, INITNAM[28], sizeof(alfa));
  WITH->NXTEL = CHARPTR;
  WITH->KLASS = (unsigned)TYPES;
  WITH->UU.U0.SIZE = 1;
  WITH->UU.U0.FORM = (unsigned)SYMBOLIC;
  WITH->UU.U0.UU.U1.BITSIZE = 2;
  ALLOC(PT, KONST, ACTUAL);   /*CHR(0)*/
/* p2c: pascal_p2c.pas, line 4065:
 * Warning: Symbol 'ALLOC' is not defined [221] */
  WITH = PT;
  memcpy(WITH->NAME, INITNAM[21], sizeof(alfa));
  WITH->NXTEL = BOOLPTR;
  WITH->KLASS = (unsigned)KONST;
  WITH->UU.U1.CONTYPE = CHARPTR;
  WITH->UU.U1.CONKIND = (unsigned)ACTUAL;
  WITH->UU.U1.UU.U0.VALUES = 0;
  WITH->UU.U1.UU.U0.SUCC = NULL;
  NEXT = PT;
  CTPTR = NULL;
  for (IT = 0; IT <= 1; IT++) {
    ALLOC(PT, KONST, ACTUAL);   /*FALSE, TRUE*/
/* p2c: pascal_p2c.pas, line 4073:
 * Warning: Symbol 'ALLOC' is not defined [221] */
    WITH = PT;
    memcpy(WITH->NAME, INITNAM[IT+29], sizeof(alfa));
    WITH->NXTEL = NEXT;
    WITH->KLASS = (unsigned)KONST;
    WITH->UU.U1.CONTYPE = BOOLPTR;
    WITH->UU.U1.CONKIND = (unsigned)ACTUAL;
    WITH->UU.U1.UU.U0.VALUES = IT;
    WITH->UU.U1.UU.U0.SUCC = CTPTR;
    CTPTR = PT;
    NEXT = PT;
  }
  BOOLPTR->UU.U0.UU.U1.FCONST = NEXT;
  ALLOC(INTPTR, TYPES, NUMERIC);   /*INTEGER*/
/* p2c: pascal_p2c.pas, line 4081:
 * Warning: Symbol 'ALLOC' is not defined [221] */
  WITH = INTPTR;
  memcpy(WITH->NAME, INITNAM[31], sizeof(alfa));
  WITH->NXTEL = NEXT;
  WITH->KLASS = (unsigned)TYPES;
  WITH->UU.U0.SIZE = 1;
  WITH->UU.U0.FORM = (unsigned)NUMERIC;
  WITH->UU.U0.UU.U0.MIN = 1152780767118491648.0;
  WITH->UU.U0.UU.U0.MAX = 140737488355327.0;
  WITH->UU.U0.UU.U0.BITS = WORDLENGTH;
  PREDEFP = INTPTR;
  for (IT = 1; IT <= 2; IT++) {
    ALLOC(PT, TYPES, POWER);   /*SET OF CHAR, POWERSET BOOLEAN*/
/* p2c: pascal_p2c.pas, line 4089:
 * Warning: Symbol 'ALLOC' is not defined [221] */
    WITH = PT;
    memcpy(WITH->NAME, BLANK, sizeof(alfa));
    WITH->NXTEL = NULL;
    WITH->KLASS = (unsigned)TYPES;
    WITH->UU.U0.SIZE = 1;
    WITH->UU.U0.FORM = (unsigned)POWER;
    if (IT == 1) {   /*POWERSET BOOLEAN*/
      WITH->UU.U0.UU.U3.ELSET = BOOLPTR;
      BOOLPTR->UU.U0.UU.U1.PWSET = PT;
      WITH->UU.U0.UU.U3.PWBITS = 3;
    } else {
      WITH->UU.U0.UU.U3.ELSET = CHARPTR;
      CHARPTR->UU.U0.UU.U1.PWSET = PT;
      WITH->UU.U0.UU.U3.PWBITS = WORDLENGTH;
    }
  }
  /*SET OF CHAR*/
  ALLOC(UNDECPTR, VARS);   /*TYPE OF ALL UNDECLARED IDES*/
/* p2c: pascal_p2c.pas, line 4101:
 * Warning: Symbol 'ALLOC' is not defined [221] */
  WITH = UNDECPTR;
  memcpy(WITH->NAME, BLANK, sizeof(alfa));
  WITH->NXTEL = NULL;
  WITH->KLASS = (unsigned)VARS;
  WITH->UU.U3.VTYPE = NULL;
  WITH->UU.U3.VKIND = (unsigned)ACTUAL;
  WITH->UU.U3.VADDR = 0;
  WITH->UU.U3.VLEVEL = 0;
  ALLOC(PNUMPTR, TYPES, POWER);   /*TYPE OF SET OF 0..58*/
/* p2c: pascal_p2c.pas, line 4106:
 * Warning: Symbol 'ALLOC' is not defined [221] */
  WITH = PNUMPTR;
  memcpy(WITH->NAME, BLANK, sizeof(alfa));
  WITH->NXTEL = NULL;
  WITH->KLASS = (unsigned)TYPES;
  WITH->UU.U0.SIZE = 1;
  WITH->UU.U0.FORM = (unsigned)POWER;
  WITH->UU.U0.UU.U3.ELSET = INTPTR;
  WITH->UU.U0.UU.U3.PWBITS = WORDLENGTH;
  ALLOC(LAMPTR, TYPES, POWER);   /*TYPE OF [ ]*/
/* p2c: pascal_p2c.pas, line 4111:
 * Warning: Symbol 'ALLOC' is not defined [221] */
  WITH = LAMPTR;
  memcpy(WITH->NAME, BLANK, sizeof(alfa));
  WITH->NXTEL = NULL;
  WITH->KLASS = (unsigned)TYPES;
  WITH->UU.U0.SIZE = 1;
  WITH->UU.U0.FORM = (unsigned)POWER;
  WITH->UU.U0.UU.U3.ELSET = NULL;
  WITH->UU.U0.UU.U3.PWBITS = 0;
  ALLOC(CTPTR, KONST, FORMAL);   /*TYPE OF PARAM OF EXTERNAL PROCEDURES*/
/* p2c: pascal_p2c.pas, line 4116:
 * Warning: Symbol 'ALLOC' is not defined [221] */
  WITH = CTPTR;
  memcpy(WITH->NAME, BLANK, sizeof(alfa));
  WITH->NXTEL = NULL;
  WITH->KLASS = (unsigned)KONST;
  WITH->UU.U1.CONTYPE = REALPTR;
  WITH->UU.U1.CONKIND = (unsigned)FORMAL;
  WITH->UU.U1.UU.U1.CADDR = 3;
  WITH->UU.U1.UU.U1.CLEVEL = 0;
  NEXT = INTPTR;
  ALLOC(PT, KONST, ACTUAL);   /*ALFALENG*/
/* p2c: pascal_p2c.pas, line 4123:
 * Warning: Symbol 'ALLOC' is not defined [221] */
  WITH = PT;
  memcpy(WITH->NAME, INITNAM[22], sizeof(alfa));
  WITH->NXTEL = NEXT;
  WITH->KLASS = (unsigned)KONST;
  WITH->UU.U1.CONTYPE = INTPTR;
  WITH->UU.U1.CONKIND = (unsigned)ACTUAL;
  WITH->UU.U1.UU.U0.VALUES = 10;
  WITH->UU.U1.UU.U0.SUCC = NULL;
  NEXT = PT;
  for (IT = 32; IT <= 37; IT++) {
    ALLOC(PT, PROC);   /*SIN, COS, EXP, LN, SQRT, ARCTAN*/
/* p2c: pascal_p2c.pas, line 4131:
 * Warning: Symbol 'ALLOC' is not defined [221] */
    WITH = PT;
    memcpy(WITH->NAME, INITNAM[IT+1], sizeof(alfa));
    WITH->NXTEL = NEXT;
    WITH->KLASS = (unsigned)PROC;
    WITH->UU.U2.PROCTYPE = REALPTR;
    WITH->UU.U2.PROCKIND = (unsigned)ACTUAL;
    WITH->UU.U2.PROCLEVEL = 0;
    WITH->UU.U2.FORMALS = CTPTR;
    WITH->UU.U2.PROCADDR = IT + 58;
    if (IT <= 33)
      WITH->UU.U2.SEGSIZE = 0;
    else
      WITH->UU.U2.SEGSIZE = IT - 33;
    NEXT = PT;
  }
  EXTPTR = NEXT;
  FILPTS[1]->UU.U0.UU.FELTYPE = CHARPTR;

  P_addset(P_expset(DIGITS, 0L), '0');
  P_addset(DIGITS, '1');
  P_addset(DIGITS, '2');
  P_addset(DIGITS, '3');
  P_addset(DIGITS, '4');
  P_addset(DIGITS, '5');
  P_addset(DIGITS, '6');
  P_addset(DIGITS, '7');
  P_addset(DIGITS, '8');
  P_addset(DIGITS, '9');
  for (IT = 0; IT <= 3; IT++)
    PARMLIST.ERRNRS[IT] = 0;
  ERRINX = 0;
  EOLFLAG = true;
  PARMLIST.ERRFLAG = false;
  DP = true;
  PRCODE = false;
  PMCTR = 0;
  LASTLINK = 0;   /*FOR PMDFILE*/
  ASSCHECK = true;
  INXCHECK = true;
  DIVCHECK = true;
  STOFLCHECK = true;
  ROUNDING = true;
  KK = ALFALENG;
  FILTOP = -1;
  JMPIX = 0;
  CEXTABIX = 0;
  PTX = 0;
  CHNIX = 1;
  B6DPL = 3;
  for (IT = 1; IT < UNDMAX; IT++)
    UNDLAB[IT-1].SUCC = IT + 1;
  UNDLAB[UNDMAX-1].SUCC = 0;
  IC = PARMLIST.ICO + 1;
  MAXCTP = LAMPTR;
  PARMLIST.EXTFLAGS = 0;
  WITH1 = DISPLAY;
  WITH1->FNAME = EXTPTR;
  WITH1->OCCUR = (unsigned)BLOCK;
  NO = 0;
  LC = IC;
  NEXTCH();
  INSYMBOL();
  do {
    DISPLAY[1].FNAME = NULL;
    DISPLAY[1].OCCUR = (unsigned)BLOCK;
    TOP = 1;
    LEVEL = 0;
    NEXT = NULL;
    ALLOC(PT, DUMMYCLASS);
/* p2c: pascal_p2c.pas, line 4166:
 * Warning: Symbol 'ALLOC' is not defined [221] */
    LC = IC;
    PFTOP = -1;

    /*********************
     *                   */
    BODY(NULL, PT);
    /*  COMPILE USER PROGRAM
     *                   *
     *********************/

    if (LEVEL == 0 && NO != 17)   /* . */
      ERROR(54L);
  } while (NO != 17);
  while (!EOLFLAG)
    NEXTCH();
  if (ERRINX > 0)
    PRTERR();
  if (PRCODE)
    PRJMPTAB();
  GETFBUF(PMDFILE, _REC_PMDFILE).PKIND = PROCDR;
  GETFBUF(PMDFILE, _REC_PMDFILE).UU.UM1.COUNT = LASTLINK;
  PUT(PMDFILE, _REC_PMDFILE);   /*LINK TO MAIN PROGRAM ENTRY*/
/* p2c: pascal_p2c.pas, line 4180:
 * Note: Null character at end of sprintf control string [148] */
  printf("");
  PARMLIST.LIMCODE = IC;
  if (PASCLGO != NULL)
    fclose(PASCLGO);
  if (PMDFILE != NULL)
    fclose(PMDFILE);
  exit(EXIT_SUCCESS);
}



/* End. */
