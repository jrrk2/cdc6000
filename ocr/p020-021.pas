    CASE KIND OF
    VARBL: BEGIN IF (FRP > 5) AND PCKD THEN
             BEGIN PCKD := FALSE ; TRANSFER(FATTR,1) ;
               IT := BITADR + BITSZ ; BT := IT <> WORDLENGTH ;
               IF BT THEN GEN15(20B,1,0,IT) ;
               GEN15(43B,0,0,WORDLENGTH-BITSZ) ; GEN15(11B,1,0,1) ;
               GEN15(15B,FRP,FRP,0) ; GEN15(12B,FRP,FRP,1) ;
               IF BT THEN GEN15(20B,FRP,0,WORDLENGTH-IT) ;
               GEN15(54B,FRP,1,0)
             END ELSE
             BEGIN LRP := FRP ;
               IF ACCESS = DRCT THEN
               BEGIN IF FRP < 5 THEN
                 BEGIN RP := RP + 1 ; LRP := RP END ELSE
                 IF FRP = 5 THEN ERROR(33) ;
                 IF BREG IN [0,LEVEL] THEN
                 BEGIN IF BREG <> 0 THEN REG := 5 ELSE REG := 0 ;
                   GEN30(51B,LRP,REG,DPLMT) ;
                 END ELSE
                 BEGIN LOADBASE(5,BREG) ; GEN30(52B,LRP,5,DPLMT) END ;
               END ELSE
               IF ACCESS = INXD THEN
               BEGIN IF BREG IN [0,LEVEL] THEN
                 BEGIN IF BREG <> 0 THEN REG := 5 ELSE REG := 0 ;
                   IF DPLMT = 0 THEN GEN15(53B,LRP,RP,REG) ELSE
                   BEGIN IF REG = 0 THEN GEN30(52B,LRP,RP,DPLMT) ELSE
                     BEGIN GEN30(62B,7,RP,DPLMT) ;
                       GEN15(56B,LRP,7,5)
                     END
                   END
                 END ELSE
                 BEGIN GEN30(62B,7,RP,DPLMT) ;
                   LOADBASE(5,BREG) ; GEN15(53B,LRP,5,7)
                 END
               END ELSE
               BEGIN IF DPLMT = 0 THEN GEN15(53B,LRP,RP,0)
                                  ELSE GEN30(52B,LRP,RP,DPLMT)
               END ;
               IF PCKD THEN
               BEGIN IF BITADR <> 0 THEN GEN15(20B,LRP,0,BITADR) ;
                 GEN15(21B,LRP,0,WORDLENGTH-BITSZ) ; PCKD := FALSE
               END
             END
           END ;
    SVAL : BEGIN IF FRP < 5 THEN
             BEGIN RP := RP + 1 ; LRP := RP END ELSE
             IF FRP > 5 THEN LRP := FRP ELSE ERROR(33) ;
             IF VAL = 0 THEN GEN15(13B,LRP,0,0)
                        ELSE GEN30(71B,LRP,0,VAL)
           END ;
    LVAL : BEGIN IF CTERM <> 0 THEN
             BEGIN GEN30(71B,0,0,CTERM) ; GEN15(36B,FRP,RP,0) END ELSE
             IF RP <> FRP THEN BXIXJ(FRP,RP)
           END ;
    LCOND: BEGIN IF ARITH THEN
             BEGIN IF JMP <= 1 THEN
               BEGIN GEN15(43B,FRP,0,0) ; AT := IC + 1 ;
                 GEN30(03B,JMP,0,AT) ; GEN30(71B,FRP,0,1) ; NOOP ;
               END ELSE
               BEGIN GEN15(43B,0,0,1) ;
                 IF JMP = 2 THEN GEN15(11B,FRP,0,RP)
                            ELSE GEN15(15B,FRP,0,RP) ;
                 GEN15(20B,FRP,0,1)
               END
             END ELSE
             BEGIN IF JMP <> 0 THEN
               BEGIN GEN15(43B,0,0,59) ; GEN15(17B,FRP,0,RP) END ELSE
               IF RP <> FRP THEN BXIXJ(FRP,RP)
             END
           END
    END (*CASE KIND*)
END (*TRANSFER*) ;

PROCEDURE LOADADR(FATTR : ATTR ; FRP : SHRTINT) ;
(*GENERATES CODE FOR LOADING THE ADDRESS OF THE VARIABLE DESCRIBED
 BY FATTR INTO X-FRP*)
  VAR REG : SHRTINT ;
BEGIN WITH FATTR DO
  IF TYPTR <> NIL THEN
  IF PCKD THEN ERROR(87) ELSE
  CASE ACCESS OF
  DRCT  : IF BREG IN [0,LEVEL] THEN
            BEGIN IF BREG <> 0 THEN REG := 5 ELSE REG := 0 ;
              GEN30(71B,FRP,REG,DPLMT)
            END ELSE
            BEGIN LOADBASE(5,BREG) ; GEN30(72B,FRP,5,DPLMT) END ;
  INDRCT: IF DPLMT <> 0 THEN GEN30(72B,FRP,RP,DPLMT) ELSE
            IF RP <> FRP THEN BXIXJ(FRP,RP) ;
  INXD  : IF BREG IN [0,LEVEL] THEN
            BEGIN IF BREG <> 0 THEN REG := 5 ELSE REG := 0 ;
              IF DPLMT = 0 THEN GEN15(73B,FRP,RP,REG) ELSE
              BEGIN IF REG = 0 THEN GEN30(72B,FRP,RP,DPLMT) ELSE
                BEGIN GEN30(62B,7,RP,DPLMT) ; GEN15(76B,FRP,7,5) END ;
              END
            END ELSE
            BEGIN GEN30(62B,7,RP,DPLMT) ;
              LOADBASE(5,BREG) ; GEN15(73B,FRP,5,7)
            END ;
  END
END (*LOADADR*) ;

PROCEDURE GENJP(FJPADDR : ADDRESS) ;
(* ACCORDING TO THE GLOBAL ATTRIBUTE RECORD GATTR (DEFINING AN
   EXPRESSION), THIS PROCEDURE PERFORMS CODE GENERATION FOR A
