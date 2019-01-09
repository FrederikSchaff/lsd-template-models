//#define EIGENLIB			// uncomment to use Eigen linear algebra library
//#define NO_POINTER_INIT	// uncomment to disable pointer checking

#include "fun_head_fast.h"


/******************************************************************************/
/* Some debugging tools.                                                      */
/* (un)comment to switches to turn capabilities on(off)                       */
/* if commented, no loss in performance                                       */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
// #define SWITCH_TEST_OFF   //(un)comment to switch on(off)
#define SWITCH_VERBOSE_OFF  //(un)comment to switch on(off)
#define TRACK_SEQUENCE_MAX_T 5 //number of steps for tracking
#define SWITCH_TRACK_SEQUENCE_OFF //(un)comment to switch on(off)
#define SWITCH_PAJEK_OFF //(un)comment to switch on(off)


/******************************************************************************/
/*         Population model backend                                           */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Not doc yet at this pos.
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#include "lsd-modules/debug-tools/validate.h"
#ifndef SWITCH_PAJEK_OFF
  #include "lsd-modules/PajekFromCppFixVer/PajekFromCpp_head.h" //before backend!
#endif
#include "lsd-modules/pop/backend_pop_LSD.h"

/* -------------------------------------------------------------------------- */

MODELBEGIN


/*----------------------------------------------------------------------------*/
/*           General Helpers                                                  */
/*----------------------------------------------------------------------------*/

////////////////////////
EQUATION("Updating_Scheme")
/* Controls the flow of events at any single simulation tick. This should always
   be the first equation called in any model! */
TRACK_SEQUENCE

  if (t==1){
    V("Init_Global");
  }

  /* Before all else, the population is updated */
    V("Pop_age");     //existing generation ages. Some persons may die.
    V("Pop_birth");   //If the population model generates new agents, that happens here.

  //BEGIN: Test new population module
  VERBOSE_MODE(true){
    cur = RNDDRAW_FAIR("Agent");
    PLOG("\nInfos according to new method: ID %g, m_ID %g, f_ID %g, age %g, d_age %g, gender %g",
      UIDS(cur),POP_MOTHERS(cur)!=NULL?UIDS(POP_MOTHERS(cur)):-1.0,POP_FATHERS(cur)!=NULL?UIDS(POP_FATHERS(cur)):-1.0,POP_AGES(cur),POP_D_AGES(cur),POP_FEMALES(cur)  );

    cur = POP_RANDOM_PERSON(10,30);
    PLOG("\nRandomly selecting an agent within age 10 and 30.");
    if (cur==NULL) {
      PLOG("\n NO SUCH AGENT");
    } else {
    PLOG("\nInfos according to new method: ID %g, m_ID %g, f_ID %g, age %g, d_age %g, gender %g",
      UIDS(cur),POP_MOTHERS(cur)!=NULL?UIDS(POP_MOTHERS(cur)):-1.0,POP_FATHERS(cur)!=NULL?UIDS(POP_FATHERS(cur)):-1.0,POP_AGES(cur),POP_D_AGES(cur),POP_FEMALES(cur)  );
    }

    cur=POP_RANDOM_PERSONM(t,t+2); //male
    PLOG("\nTest: Random agent should have gender %s and age between %i and %i","male",t,t+2);
    if (cur==NULL){
      PLOG("\nTest: No such agent exists");
    } else {
      PLOG("\nTest: Returned agent: ID %g  gender %s and age %g",UIDS(cur),POP_FEMALES(cur)==true?"female":"male",POP_AGES(cur));
    }

    cur=POP_RANDOM_PERSONF(t,t+2); //female
    PLOG("\nTest: Random agent should have gender %s and age between %i and %i","female",t,t+2);
    if (cur==NULL){
      PLOG("\nTest: No such agent exists");
    } else {
      PLOG("\nTest: Returned agent: ID %g  gender %s and age %g",UIDS(cur),POP_FEMALES(cur)==true?"female":"male",POP_AGES(cur));
    }

    cur=POP_RANDOM_PERSON(-1,-1); //doesn't matter
    PLOG("\nTest: Random agent should have gender %s and age between %i and %i","unspecified",t,t+2);
    if (cur==NULL){
      PLOG("\nTest: No such agent exists");
    } else {
      PLOG("\nTest: Returned agent: ID %g  gender %s and age %g",UIDS(cur),POP_FEMALES(cur)==true?"female":"male",POP_AGES(cur));
    }
  }

  VERBOSE_MODE(true){
    POP_FAMILY_DEGREE(POP_RANDOM_PERSONF(-1,-1),POP_RANDOM_PERSONM(-1,-1));
    POP_CHECK_INCEST(POP_RANDOM_PERSONF(-1,-1),POP_RANDOM_PERSONM(-1,-1),3);
  }
  //END: Test new population module

  #ifdef ABMAT_USE_ANALYSIS
    V("ABMAT_UPDATE"); //Take descriptive statistics
  #endif
RESULT(0.0)

////////////////////////
EQUATION("Init_Global")
/* This it the main initialisation function, calling all initialisation action
necessary. */

  int model_type = V("Model_Type"); //allow testing different models.
  int n_generation = 0;

  if (model_type == 1){
  INIT_POPULATION_MODULE("BLL", 0.0,  1.0, V("m1_alpha"), V("m1_beta"));
  n_generation = POP_CONSTN_BIRTH(V("Pop_const_n")); //size of the first generation
  } else {
  INIT_POPULATION_MODULE("NONE", 0.0,  1.0);
  n_generation = V("Pop_const_n")/V("m0_maxLife")*2;
  }

  object *to_delete = SEARCH("Agent"); //Template person will die.
  for (int i = 0; i < n_generation; i++){
    V("New_Agent"); // a new agent is added.
  }
  DELETE(to_delete);

  #ifdef ABMAT_USE_ANALYSIS
    V("ABMAT_INIT"); //Initialise ABMAT
  #endif
PARAMETER
RESULT(0)

/******************************************************************************/
/*           Population modul linking - see description.txt                   */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

////////////////////////
EQUATION("New_Agent")
/* Create and initialise a new agent */

  TRACK_SEQUENCE

  object *ptrAgent = ADDOBJL("Agent", 0); //create LSD agent
  object *mother = POP_RANDOM_PERSONF(15,35); //find a mother
  object *father = NULL; //place holder - no father (yet)
  if (mother != NULL){
    //check if there is a child already, if yes, check if father lives, if yes, take him
    father = POP_MGENITORS(mother);
  }
  if (father == NULL){
    //no father yet
    father = POP_RANDOM_PERSONM(15,65);
  }

  POP_ADD_PERSON_WPARENTS(ptrAgent,mother,father);

  if (V("Model_Type") == 0){
    POP_SET_DAGES(ptrAgent, RND*V("m0_maxLife")); //in the no model case, chose random age between 0 and 100.
  }

  //add heritage stuff



   //BEGIN: Test new population module
   // POP_ADD_PERSON( c, POP_GET_RAGENTFS(c), POP_GET_RAGENTMS(c), 0, 99 )

   //END: Test new population module


   //WRITE("father",ID); //if there shall be a father, provide the ID here.
   //WRITE("mother",ID); //if there shall be a mother, provide the ID here.

RESULT(0)

EQUATION("Age")
RESULT(POP_AGE)

EQUATION("female")
RESULT(POP_FEMALE)

EQUATION("Death")
/* Kill the agent */
TRACK_SEQUENCE
  DELETE(p)
//   POP_INFO
RESULT(0.0)


EQUATION("Pop_birth")
/* Add next generation.*/
TRACK_SEQUENCE
  int model_type = V("Model_Type"); //allow testing different models.
  int n_generation=0;
  if (model_type == 1){
    n_generation = POP_CONSTN_BIRTH(V("Pop_const_n")); //size of the first generation
  } else {
    n_generation = V("Pop_const_n")/V("m0_maxLife")*2;
  }
  for (int i = 0; i < n_generation; i++){
    V("New_Agent"); // a new agent is added.
  }

RESULT(double(n_generation) )

EQUATION("Pop_age")
/* Each agents get older one year. */
TRACK_SEQUENCE

  POP_ADVANCE_TIME
  //Kill agents that are dead
    double alive = 0.0;
    CYCLE_SAFES(p->up,cur,"Agent"){
      if (false == POP_ALIVES(cur)){
        if (0.0 < alive || NULL != cur->next ){
          VS(cur,"Death");
        } else {
          PLOG("\nAt time %i: Simulation at premature end. Last agent would have died.",t);
          ABORT
        }
      } else {
        alive++;
      }
    }
RESULT(alive)

/******************************************************************************/
/*  Population modul linking end                                              */
/*----------------------------------------------------------------------------*/

/* Theory specific below */










MODELEND




void close_sim(void)
{
PAJEK_POP_LINEAGE_SAVE
}


