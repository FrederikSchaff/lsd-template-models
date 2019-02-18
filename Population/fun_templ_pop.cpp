//#define EIGENLIB			// uncomment to use Eigen linear algebra library
//#define NO_POINTER_INIT	// uncomment to disable pointer checking

#include "fun_head_fast.h"


/******************************************************************************/
/* Some debugging tools.                                                      */
/* (un)comment to switches to turn capabilities on(off)                       */
/* if commented, no loss in performance                                       */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
// #define SWITCH_TEST_OFF   //(un)comment to switch on(off)
// #define SWITCH_VERBOSE_OFF  //(un)comment to switch on(off)
#define TRACK_SEQUENCE_MAX_T 1000 //number of steps for tracking
#define SWITCH_PAJEK_OFF //(un)comment to switch on(off)
#define SWITCH_TRACK_SEQUENCE_OFF //(un)comment to switch on(off)


/******************************************************************************/
/*         Population model backend                                           */
/*  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "lsd-modules/debug-tools/validate.h"
#ifndef SWITCH_PAJEK_OFF
#include "lsd-modules/PajekFromCpp/PajekFromCpp_head.h" //before backend!
#endif
#include "lsd-modules/pop/backend_pop_LSD.h"

/* -------------------------------------------------------------------------- */

MODELBEGIN


/*----------------------------------------------------------------------------*/
/*           General Helpers                                                  */
/*----------------------------------------------------------------------------*/

////////////////////////
TEQUATION("Updating_Scheme")
/*  Controls the flow of events at any single simulation tick. This should always
    be the first equation called in any model! */
//TRACK_SEQUENCE
V("Init_Global"); //once then parameter

/* Before all else, the population is updated */
V("Pop_age");     //existing generation ages. Some persons may die.
V("Pop_birth");   //If the population model generates new agents, that happens here.

VERBOSE_MODE(true){
    cur = POP_RANDOM_PERSON('x',10,30);
    PLOG("\nRandomly selecting an agent within age 10 and 30.");
    if (cur==NULL) {
      PLOG("\n NO SUCH AGENT");
    } else {
    PLOG("\nInfos according to new method: ID %g, m_ID %g, f_ID %g, age %g, d_age %g, gender %g",
      UIDS(cur),POP_MOTHERS(cur)!=NULL?UIDS(POP_MOTHERS(cur)):-1.0,POP_FATHERS(cur)!=NULL?UIDS(POP_FATHERS(cur)):-1.0,POP_AGES(cur),POP_D_AGES(cur),POP_FEMALES(cur)  );
    }

    cur=POP_RANDOM_PERSON('m',t,t+2); //male
    PLOG("\nTest: Random agent should have gender %s and age between %i and %i","male",t,t+2);
    if (cur==NULL){
      PLOG("\nTest: No such agent exists");
    } else {
      PLOG("\nTest: Returned agent: ID %g  gender %s and age %g",UIDS(cur),POP_FEMALES(cur)==true?"female":"male",POP_AGES(cur));
    }

    cur=POP_RANDOM_PERSON('f',t,t+2); //female
    PLOG("\nTest: Random agent should have gender %s and age between %i and %i","female",t,t+2);
    if (cur==NULL){
      PLOG("\nTest: No such agent exists");
    } else {
      PLOG("\nTest: Returned agent: ID %g  gender %s and age %g",UIDS(cur),POP_FEMALES(cur)==true?"female":"male",POP_AGES(cur));
    }

    cur=POP_RANDOM_PERSON('x',-1,-1); //doesn't matter
    PLOG("\nTest: Random agent should have gender %s and age between %i and %i","unspecified",t,t+2);
    if (cur==NULL){
      PLOG("\nTest: No such agent exists");
    } else {
      PLOG("\nTest: Returned agent: ID %g  gender %s and age %g",UIDS(cur),POP_FEMALES(cur)==true?"female":"male",POP_AGES(cur));
    }
}

VERBOSE_MODE(true){
    POP_FAMILY_DEGREE(POP_RANDOM_PERSON('f',-1,-1),POP_RANDOM_PERSON('m',-1,-1));
    POP_CHECK_INCEST(POP_RANDOM_PERSON('f',-1,-1),POP_RANDOM_PERSON('m',-1,-1),3);
}
  //END: Test new population module

RESULT(0.0)

////////////////////////
TEQUATION("Init_Global")
/*  This it the main initialisation function, calling all initialisation action
    necessary. */

int model_type = V("Model_Type"); //allow testing different models.
int n_generation = 0;

if (model_type == 1)
{
    INIT_POPULATION_MODULE("BLL", 0.0, 1.0, 0.5, V("m1_alpha"), V("m1_beta")); //Model, t_start, t_unit, femaleRatio, par1, par2
    n_generation = POP_CONSTN_BIRTH(V("Pop_const_n")); //size of the first generation
}
else
{
    INIT_POPULATION_MODULE("NONE", 0.0, 1.0, 0.5); //Model, t_start, t_unit, femaleRatio
    n_generation = V("Pop_const_n") / V("m0_maxLife") * 2;
}

INIT_SPACE_ROOT_WRAP(100, 100, 15); //initialise grid space as in BHSC The Grid Size needs to be squared, but is otherwise just a performance parameter.

object* to_delete = SEARCH("Agent"); //Template person will die.
V("Pop_birth"); // a new agent is added NOW.
DELETE(to_delete);

PARAMETER
RESULT(0)

/******************************************************************************/
/*           Population modul linking - see description.txt                   */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

TEQUATION("age")
//TRACK_SEQUENCE
RESULT(POP_AGE)

TEQUATION("female")
//TRACK_SEQUENCE
PARAMETER
RESULT(POP_FEMALE)

TEQUATION("Death")
/* Kill the agent */
//TRACK_SEQUENCE
DELETE(p)
//   POP_INFO
RESULT(0.0)


TEQUATION("Pop_birth")
/* Add next generation.*/
//TRACK_SEQUENCE
int model_type = V("Model_Type"); //e.g., 1 == BLL
int n_generation = 0;
if (model_type == 1)
{
    n_generation = POP_CONSTN_BIRTH(V("Pop_const_n")); //size of the first generation
}
else
{
    n_generation = V("Pop_const_n") / V("m0_maxLife") * 2;
}

//add new agents
for (int i = 0; i < n_generation; i++)
{
    object* ptrAgent = ADDOBJLS(p->up, "Agent", 0); //create LSD agent
    object* mother = POP_RANDOM_PERSON('f', V("fertility_low"), V("fertility_high")); //find a mother
    object* father = NULL; //place holder - no father (yet)

    if (mother != NULL) { //check if there is a child already,
        // PLOG("\n Check POP_MGENITORS");
        father = POP_MGENITORS(mother);
        // if (father==NULL)
        // PLOG("\n no father");
    }

    if (father == NULL) { //no father yet
        father = POP_RANDOM_PERSON('m', 15, 65);
    }

    POP_ADD_PERSON_WPARENTS(ptrAgent, mother, father); //Add to population module

    //Add to space for wedding ring model
    if (NULL != father && NULL != mother) {
        ADD_TO_SPACE_CENTER_SHARES(ptrAgent, mother, father);
    }
    else if (NULL != mother) {
        ADD_TO_SPACE_SHARES(ptrAgent, mother);
    }
    else if (NULL != father) {
        ADD_TO_SPACE_SHARES(ptrAgent, father);
    }
    else {
        ADD_TO_SPACE_RNDS(ptrAgent, root); //Add to space at random position
    }

    if (V("Model_Type") == 0) {
        POP_SET_DAGES(ptrAgent, RND * V("m0_maxLife")); //in the no model case, chose random age between 0 and 100.
    }

    VS(ptrAgent, "Init_Agent");
}

RESULT(double(n_generation) )


///////////////////////////////
TEQUATION("Pop_age")
/* Each agents get older one year. */
//TRACK_SEQUENCE

POP_ADVANCE_TIME //let agents age
//Kill agents that are dead
double alive = 0.0;
CYCLE_SAFES(p->up, cur, "Agent")
{
    if (false == POP_ALIVES(cur)) {
        if (0.0 < alive || NULL != cur->next ) {
            VS(cur, "Death");
        }
        else {
            PLOG("\nAt time %i: Simulation at premature end. Last agent would have died.", t);
            ABORT
        }
    }
    else {
        alive++;
    }
}
RESULT(alive)


/***** Now the wedding ring model ****/

TEQUATION("Init_Agent")
/*  Initialise a new agent
    - define agent characteristics
*/

int ro_type = uniform_int(1, 5); //kind of shift of age interval for relevant others.
double gamma = RND * V("wr_gamma"); //strength of shift
double hwidth = RND * V("wr_hintvl"); //size of half of interval

double a_center;
switch (ro_type)
{
case 1:
    a_center = - gamma;
    break;
case 2:
    a_center = - gamma / 2.0;
    break;
case 3:
    a_center = 0.0;
    break;
case 4:
    a_center = gamma / 2.0;
    break;
case 5:
    a_center = gamma;
    break;
}
WRITE("ro_a_low", a_center - hwidth); //absolute distance to age younger relevant others
WRITE("ro_a_high", a_center + hwidth);
PARAMETER
RESULT(0.0)

TEQUATION("Relevant_Other")
/*  This is a FUNCTION that reports to the caller if the callee (the agent which holds the function) is a relevant other.

    A person is relevant to the other person, if it is (a) within physical distance, emulating the social distance and (b) within age distance.
    We may disable the check for (a) if we search within the relevant distance ex ante.

*/
double is_relevant = 0.0;

RESULT(is_relevant)

TEQUATION("psearch_radius")
/*  This is the search radius for the partner search in the Wedding Ring model.
    It is set in 0,1 (relative)
*/
//TRACK_SEQUENCE
double sp = V("Social_Pressure");
double ai_P = V("age_influence");
double distance_wd = V("distance_wd");

double psearch_radius = sp * ai_P * distance_wd;

RESULT(distance_wd)

TEQUATION("age_accept_low")
/* Calculate the lower bound of acceptable age */
bool alt_model = V("WD_alt_model") > 0.0 ? true : false; //define social pressure on share of people married, as in the original, or on share of people with children.
double age_accept_low = POP_AGE - V("Social_Pressure") * V("age_influence") * V("c_WR");
if (alt_model)
{
    age_accept_low = max( age_accept_low, ( POP_FEMALE ? V("fertility_m_low") : V("fertility_f_low") ) );
}
RESULT(age_accept_low)

TEQUATION("age_accept_high")
/* Calculate the lower bound of acceptable age */
bool alt_model = V("WD_alt_model") > 0.0 ? true : false; //define social pressure on share of people married, as in the original, or on share of people with children.
double age_accept_high = POP_AGE + V("Social_Pressure") * V("age_influence") * V("c_WR");
if (alt_model)
{
    age_accept_high = max( age_accept_high, ( POP_FEMALE ? V("fertility_m_high") : V("fertility_f_high") ) );
}
RESULT(age_accept_high)

TEQUATION("age_influence")
/*  A factor that decides on the size of the socio-spatial network based on the age of the person. */
//TRACK_SEQUENCE
double age_influence;
double age = POP_AGE;

if ( age > 64 )
{
    age_influence = 0.1;
}
else if ( age > 60 )
{
    age_influence = 6.5 - 0.1 * age;
}
else if ( age > 38 )
{
    age_influence = 0.5;
}
else if ( age > 33 )
{
    age_influence = 4.3 - 0.1 * age;
}
else if ( age > 20 )
{
    age_influence = 0.1;
}
else if ( age >= 16 )
{
    age_influence = 0.2 * age - 3.1;
}
else
{
    age_influence = 0.0;
}

RESULT(age_influence)

TEQUATION("Social_Pressure")
/*  This is the social presure that rests upon the individual to get a partner and start getting (more) children
    At this point, the parameters alpha and beta are hard coded as in the papers [2,3], because the parameters it self are not useful to interpret.
*/

bool alt_model = V("WD_alt_model") > 0.0 ? true : false; //define social pressure on share of people married, as in the original, or on share of people with children.
double pocm = 0.0;
double total = 0.0;
double age_low = POP_AGE - V("ro_a_low"); //relevant others minimum age
double age_high = POP_AGE - V("ro_a_high");
double rho = V("rho_WD"); //chance to take

//In a first step, calculate size of set of relevant others
FCYCLE_NEIGHBOUR(cur, "Agent", V("distance_wd"))   //first check distance
{
    if ( POP_AGES(cur) < age_low   //skip if to young
            || POP_AGES(cur) > age_high //skip if to old
            || rho > RND ) { //skip by chance
        continue; //skip
    }

    total++;

    if (alt_model) {
        if ( POP_NCHILDRENS(cur) > 0 )
            pocm++;
        else if ( VS(cur, "Partner_Status") == 1.0 )
            pocm++;
    }
}

double beta = 7.0; //V("beta_WR");
double alpha = 0.5;// V("alpha_WR");
double temp = exp(beta * (pocm - alpha));
double sp = temp / (1 + temp);

RESULT(sp)

TEQUATION("Partner_Status")
/* Is there currently a partner? 0: No, 1: Yes. Also check if partner is dead. */
double has_partner = 0.0; //no partner yet
double partner_id = V("PartnerID");
if (partner_id >= 0)
{
    object* Partner = SEARCH_UID(partner_id);
    if (Partner == NULL) {
        has_partner = -1.0; //partner dead
    }
    else {
        has_partner = 1.0; //partner alive
    }
}
RESULT(has_partner)

TEQUATION("Potential_Partner")
/* This is a function that reports to the caller if the callee is a suitable match, which implies also that the callee finds the caller suitable. */
double is_match = 0.0; //no
double distance = DISTANCE(c);
if ( POP_FEMALE != POP_FEMALES(c)
        && distance < V("psearch_radius")
        && distance < VS(c, "psearch_radius")
        && V("age_accept_low") <= POP_AGES(c)
        && POP_AGES(c) <= V("age_accept_high")
        && VS(c, "age_accept_low") <= POP_AGE
        && POP_AGE <= VS(c, "age_accept_high") )
{
    is_match = 1.0;
}
RESULT(is_match)


/******************************************************************************/
/*  Population modul linking end                                              */
/*----------------------------------------------------------------------------*/

/* Theory specific below */










MODELEND




void close_sim(void)
{
    PAJEK_POP_LINEAGE_SAVE
}


