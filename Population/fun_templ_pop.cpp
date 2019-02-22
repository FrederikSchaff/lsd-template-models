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
double updating_scheme_ret = 0.0;
{
    V("Init_Global"); //once then parameter

    /* Before all else, the population is updated */
    V("Pop_age");     //existing generation ages. Some persons may die.
    V("Pop_birth");   //If the population model generates new agents, that happens here.

    VERBOSE_MODE(false) {
        cur = POP_RANDOM_PERSON('x', 10, 30);
        PLOG("\nRandomly selecting an agent within age 10 and 30.");
        if (cur == NULL) {
            PLOG("\n NO SUCH AGENT");
        }
        else {
            PLOG("\nInfos according to new method: ID %g, m_ID %g, f_ID %g, age %g, d_age %g, gender %g",
                 UIDS(cur), POP_MOTHERS(cur) != NULL ? UIDS(POP_MOTHERS(cur)) : -1.0, POP_FATHERS(cur) != NULL ? UIDS(POP_FATHERS(cur)) : -1.0, POP_AGES(cur), POP_D_AGES(cur), POP_FEMALES(cur)  );
        }

        cur = POP_RANDOM_PERSON('m', t, t + 2); //male
        PLOG("\nTest: Random agent should have gender %s and age between %i and %i", "male", t, t + 2);
        if (cur == NULL) {
            PLOG("\nTest: No such agent exists");
        }
        else {
            PLOG("\nTest: Returned agent: ID %g  gender %s and age %g", UIDS(cur), POP_FEMALES(cur) == true ? "female" : "male", POP_AGES(cur));
        }

        cur = POP_RANDOM_PERSON('f', t, t + 2); //female
        PLOG("\nTest: Random agent should have gender %s and age between %i and %i", "female", t, t + 2);
        if (cur == NULL) {
            PLOG("\nTest: No such agent exists");
        }
        else {
            PLOG("\nTest: Returned agent: ID %g  gender %s and age %g", UIDS(cur), POP_FEMALES(cur) == true ? "female" : "male", POP_AGES(cur));
        }

        cur = POP_RANDOM_PERSON('x', -1, -1); //doesn't matter
        PLOG("\nTest: Random agent should have gender %s and age between %i and %i", "unspecified", t, t + 2);
        if (cur == NULL) {
            PLOG("\nTest: No such agent exists");
        }
        else {
            PLOG("\nTest: Returned agent: ID %g  gender %s and age %g", UIDS(cur), POP_FEMALES(cur) == true ? "female" : "male", POP_AGES(cur));
        }
    }

    VERBOSE_MODE(false) {
        POP_FAMILY_DEGREE(POP_RANDOM_PERSON('f', -1, -1), POP_RANDOM_PERSON('m', -1, -1));
        POP_CHECK_INCEST(POP_RANDOM_PERSON('f', -1, -1), POP_RANDOM_PERSON('m', -1, -1), 3);
    }
    //END: Test new population module
}
RESULT(updating_scheme_ret)

////////////////////////
TEQUATION("Init_Global")
/*  This it the main initialisation function, calling all initialisation action
    necessary. */
double init_global_ret = 0.0;
{

    int model_type = V("Model_Type"); //allow testing different models.
    int n_generation = 0;

    if (model_type == 1) {
        INIT_POPULATION_MODULE("BLL", 0.0, 1.0, 0.5, V("m1_alpha"), V("m1_beta")); //Model, t_start, t_unit, femaleRatio, par1, par2
        n_generation = POP_CONSTN_BIRTH(V("Pop_const_n")); //size of the first generation
    }
    else {
        INIT_POPULATION_MODULE("NONE", 0.0, 1.0, 0.5); //Model, t_start, t_unit, femaleRatio
        n_generation = V("Pop_const_n") / V("m0_maxLife") * 2;
    }

    INIT_SPACE_ROOT_WRAP(100, 100, 15); //initialise grid space as in BHSC The Grid Size needs to be squared, but is otherwise just a performance parameter.
    INIT_LAT_GISS(root, 0);

    object* to_delete = SEARCH("Agent"); //Template person will die.
    V("Pop_birth"); // a new agent is added NOW.
    DELETE(to_delete);

    PARAMETER
}
RESULT(init_global_ret)

/******************************************************************************/
/*           Template Model Start - see description.txt                   */
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

TEQUATION("age")
//TRACK_SEQUENCE
double age_ret = 0.0;
{
    age_ret = POP_AGE;
}
RESULT(age_ret)

TEQUATION("female")
//TRACK_SEQUENCE
double female_ret = 0.0;
{
    female_ret = (POP_FEMALE == true) ? 1.0 : 0.0;
    PARAMETER
}
RESULT(female_ret)

EQUATION("ID")
double ID_ret = 0.0;
{
    ID_ret = UID;
    PARAMETER
}
RESULT(ID_ret)

EQUATION("mother")
double mother_ret = 0.0;
{
    int mother_ret = -1;
    cur = POP_MOTHER;
    if (cur != NULL)
        mother_ret = UIDS(cur);
    PARAMETER
}
RESULT(mother_ret)

EQUATION("father")
double father_ret = 0.0;
{
    father_ret = -1;
    cur = POP_FATHER;
    if (cur != NULL)
        father_ret = UIDS(cur);
    PARAMETER
}
RESULT(father_ret)

EQUATION("nChildren")
double nChildren_ret = POP_NCHILDREN;
RESULT(nChildren_ret)

TEQUATION("Death")
/* Kill the agent */
//TRACK_SEQUENCE
const double death_ret = 0.0;
{
    DELETE(p);
    //   PLOG(POP_INFO)
}
RESULT(death_ret)

TEQUATION("IsFertile")
/*  Determine if the agent can become a parent based on its age
    and if it is female also based on the time of the last delivery.*/

double isfertile_ret = 0.0;
{
    double age = POP_AGE;
    const double timeToNewBirth = 3.0;
    if (true == POP_FEMALE) {
        if ( age >= V("fertility_low_f") && age <= V("fertility_high_f")
                && POP_LASTDELIVERY + timeToNewBirth < T ) {
            isfertile_ret = 1.0;
        }
    }
    else { //male
        if ( age >= V("fertility_low_m") && age <= V("fertility_high_m") ) {
            isfertile_ret = 1.0;
        }
    }
}
RESULT(isfertile_ret)

TEQUATION("PotentialMother")
/*  Function. Provides information on whether the women can become a mother.
    It needs to be within fertility range, have a partner within fertility range
    and the delivery of the last child needs be long enough in the past. */

//Find_Partner ensures that it is first looked for a partner. The status is reported.
double pot_mother = ( V("Find_Partner") == 1.0 ) && ( V("IsFertile") == 1.0 ) ?  1.0 : 0.0;

RESULT(pot_mother)


TEQUATION("Pop_birth")
/* Add next generation.*/
double pop_birth_ret = 0;
{
    //TRACK_SEQUENCE
    int model_type = V("Model_Type"); //e.g., 1 == BLL
    int n_generation = 0;
    if (model_type == 1) {
        n_generation = POP_CONSTN_BIRTH(V("Pop_const_n")); //size of the first generation
    }
    else {
        n_generation = V("Pop_const_n") / V("m0_maxLife") * 2;
    }

    pop_birth_ret = n_generation; //just to save this info

    //Add new agents
    //first: use set of potential mothers in random order
    POP_RCYCLE_PERSON_CND(cur, 'f', V("fertility_low"), V("fertility_high"), "PotentialMother", '=', 1.0) {
        if (n_generation == 0) {
            break; //done.
        }
        V_CHEAT("Add_Agent", cur);
        --n_generation;
    }
    //second: left-overs
    for (int i = 0; i < n_generation; i++) {
        V_CHEAT("Add_Agent", root);
    }
    
    WRITE("StorckChildRate",(double)n_generation/pop_birth_ret);
}
RESULT( pop_birth_ret )

TEQUATION("Add_Agent")
/*  An equation that is called via fake-caller (thus passing the mother) and creates a new agent.
    If the fake-caller is root, there are no parents.
*/
double add_agent_ret = CURRENT + 1;
{
    object* ptrAgent = ADDOBJLS(p->up, "Agent", 0); //create LSD agent
    object* mother = c; //caller
    object* father = NULL; //place holder - no father (yet)
    if (mother != root) {
        father = SEARCH_UID(VS(mother, "PartnerID"));
        if (father == NULL)
            PLOG("\nERROR! See %s in %s : %s", __func__, __FILE__, __LINE__);
    }
    POP_ADD_PERSON_WPARENTS(ptrAgent, father, mother); //Add to population module
    //Add to space for wedding ring model
    if (NULL != father && NULL != mother) {
        ADD_TO_SPACE_CENTER_SHARES(ptrAgent, mother, father);
    }
    else {
        ADD_TO_SPACE_RNDS(ptrAgent, root); //Add to space at random position
    }
    SET_LAT_PRIORITYS(ptrAgent,0);
    SET_LAT_COLORS(ptrAgent,1000);

    if (V("Model_Type") == 0) {
        POP_SET_DAGES(ptrAgent, RND * V("m0_maxLife")); //in the no model case, chose random age between 0 and 100.
    }
    VS(ptrAgent, "Init_Agent");
}
RESULT( add_agent_ret ) //Number of new persons created.

TEQUATION("StorckChildRate")
/*  Measure the share of children that are born without parents. Normalised in [0,1]
*/

RESULT(0.0)

///////////////////////////////
TEQUATION("Pop_age")
/* Each agents get older one year. */
//TRACK_SEQUENCE
double pop_age_ret = CURRENT; //number of persons alive
{
    POP_ADVANCE_TIME //let agents age
    //Kill agents that are dead
    int alive = 0;
    CYCLE_SAFES(p->up, cur, "Agent") {
        if (false == POP_ALIVES(cur)) {
            if (0 < alive || NULL != cur->next ) {
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
    pop_age_ret = alive;
}
RESULT(pop_age_ret)


/***** Now the wedding ring model ****/

TEQUATION("Init_Agent")
/*  Initialise a new agent
    - define agent characteristics
*/
double init_agent_ret = T;
{
    int ro_type = uniform_int(1, 5); //kind of shift of age interval for relevant others.
    double gamma = RND * V("wr_gamma"); //strength of shift
    double hwidth = RND * V("wr_hintvl"); //size of half of interval

    double a_center;
    switch (ro_type) {
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
}
RESULT(init_agent_ret)


TEQUATION("psearch_radius")
/*  This is the search radius for the partner search in the Wedding Ring model.
    It is set in 0,1 (relative)
*/
//TRACK_SEQUENCE
double psearch_radius_ret = 0.0;
{
    double sp = V("Social_Pressure");
    double ai_P = V("age_influence");
    double distance_wd = V("distance_wd");

    psearch_radius_ret = sp * ai_P * distance_wd;
}
RESULT(psearch_radius_ret)

TEQUATION("age_accept_low")
/* Calculate the lower bound of acceptable age */
double age_accept_low_ret = 0.0;
{
    bool alt_model = V("WD_alt_model") == 0.0 ? false : true; //define social pressure on share of people married, as in the original, or on share of people with children (true).
    age_accept_low_ret = POP_AGE - V("Social_Pressure") * V("age_influence") * V("c_WR");
    if (alt_model) {
        age_accept_low_ret = max( age_accept_low_ret, ( POP_FEMALE ? V("fertility_m_low") : V("fertility_f_low") ) );
    }
}
RESULT(age_accept_low_ret)

TEQUATION("age_accept_high")
/* Calculate the lower bound of acceptable age */
double age_accept_high_ret = 0.0;
{
    bool alt_model = V("WD_alt_model") == 0.0 ? false : true; //define social pressure on share of people married, as in the original, or on share of people with children (true).
    double age_accept_high_ret = POP_AGE + V("Social_Pressure") * V("age_influence") * V("c_WR");
    if (alt_model) {
        age_accept_high_ret = max( age_accept_high_ret, ( POP_FEMALE ? V("fertility_m_high") : V("fertility_f_high") ) );
    }
}
RESULT(age_accept_high_ret)

TEQUATION("age_influence")
/*  A factor that decides on the size of the socio-spatial network based on the age of the person. */
//TRACK_SEQUENCE
double age_influence_ret = 0.0;
{
    double age = POP_AGE;

    if ( age > 64 ) {
        age_influence_ret = 0.1;
    }
    else if ( age > 60 ) {
        age_influence_ret = 6.5 - 0.1 * age;
    }
    else if ( age > 38 ) {
        age_influence_ret = 0.5;
    }
    else if ( age > 33 ) {
        age_influence_ret = 4.3 - 0.1 * age;
    }
    else if ( age > 20 ) {
        age_influence_ret = 0.1;
    }
    else if ( age >= 16 ) {
        age_influence_ret = 0.2 * age - 3.1;
    }
    else {
        age_influence_ret = 0.0;
    }
}
RESULT(age_influence_ret)

TEQUATION("Social_Pressure")
/*  This is the social presure that rests upon the individual to get a partner and start getting (more) children
    At this point, the parameters alpha and beta are hard coded as in the papers [2,3], because the parameters it self are not useful to interprete.
*/
SET_LOCAL_CLOCK_RF
double social_pressure_ret = 0.0;
{
    bool alt_model = V("WD_alt_model") == 0.0 ? false : true; //define social pressure on share of people married, as in the original, or on share of people with children (true).
    double pocm = 0.0;
    double total = 0.0;
    double age_low = POP_AGE - V("ro_a_low"); //relevant others minimum age
    double age_high = POP_AGE - V("ro_a_high");
    double rho = V("rho_WD"); //chance to take

    //In a first step, calculate size of set of relevant others
    FCYCLE_NEIGHBOUR(cur, "Agent", V("distance_wd")) { //first check distance

        //Check if relevant other
        if ( POP_AGES(cur) < age_low   //skip if to young
                || POP_AGES(cur) > age_high //skip if to old
                || rho > RND ) { //skip by chance
            continue; //skip
        }

        total++;

        //check if putting pressure
        if (alt_model && POP_NCHILDRENS(cur) > 0 ) {
            pocm++;
        }
        else if ( VS(cur, "Partner_Status") == 1.0 ) {
            pocm++;
        }

    }

    const double beta = 7.0; //V("beta_WR");
    const double alpha = 0.5;// V("alpha_WR");
    double temp = exp(beta * (pocm - alpha));
    social_pressure_ret = temp / (1 + temp);
    REPORT_LOCAL_CLOCK_CND(0.02);
}
RESULT(social_pressure_ret)

TEQUATION("Partner_Status")
/* Function. Is there currently a partner? 0: No, 1: Yes. Also check if partner is dead. */
double partner_status_ret = 0.0; //no partner yet
{
    double partner_id = V("PartnerID");
    if (partner_id > 0) {
        object* Partner = SEARCH_UID(partner_id);
        if (Partner == NULL) {
            partner_status_ret = -1.0; //partner dead
        }
        else {
            partner_status_ret = 1.0; //partner alive
        }
    }
}
RESULT(partner_status_ret)

TEQUATION("Potential_Partner")
/*  This is a function that reports to the caller if the callee is a suitable match,
    which implies also that the callee finds the caller suitable. */
double potential_partner_ret = 0.0;
{
    double is_match = 0.0; //no
    bool alt_model = V("WD_alt_model") == 0.0 ? false : true; //define social pressure on share of people married, as in the original, or on share of people with children (true).
    double distance = DISTANCE(c);
    bool is_free = alt_model ? ( V("Partner_Status") != 1 ) : ( V("Partner_Status") == 0 ); //in original model: only if unmarried. in alt model: if currently no partner.
    const int prohibited_degree = 5; //maximum degree of relatedness that is prohibited (i.e. most distant relatedness not allowed. 5 == cousinship)
    bool not_of_kin = alt_model ? ( false == POP_CHECK_INCEST(c, p, prohibited_degree) ) : true;

    if (    true == is_free //can couple
            && POP_FEMALE != POP_FEMALES(c) //different sex
            && true == not_of_kin //are not of same kin
            && distance < V("psearch_radius")  //within social/spatial range
            && distance < VS(c, "psearch_radius")
            && V("age_accept_low") <= POP_AGES(c) //within age range
            && POP_AGES(c) <= V("age_accept_high")
            && VS(c, "age_accept_low") <= POP_AGE
            && POP_AGE <= VS(c, "age_accept_high") ) {
        is_match = 1.0;
    }
    potential_partner_ret = is_match;
}
RESULT(potential_partner_ret)

TEQUATION("Find_Partner")
/*  If the agent does not yet have a partner, it actively searches for a partner.
    In the original wedding ring model it searches for a partner if it didn't have one yet,
    i.e., if the partner died it will still not look for a new one.
    In the alternative model it searches for a partner if it is in fertility range only
    and if it doesn't have a partner.

    Note: Currently we do not break-up a partnership if one of the partners leaves fertility range.
*/
SET_LOCAL_CLOCK_RF
double find_partner_ret = 0.0;
{
    bool alt_model = V("WD_alt_model") == 0.0 ? false : true; //define social pressure on share of people married, as in the original, or on share of people with children (true).
    bool is_free = alt_model ? ( V("Partner_Status") != 1 ) : ( V("Partner_Status") == 0 ); //in original model: only if unmarried. in alt model: if currently no partner.

    if (true == is_free) {
        //randomly cycle through neighbours in partner range, mating with the first one that fits.
        RCYCLE_NEIGHBOUR(cur, "Agent", V("psearch_radius")) { //first check distance
            if (VS(cur, "Potential_Partner") == 1.0) {
                WRITE("PartnerID", UIDS(cur));
                WRITES(cur, "PartnerID", UID);
                PLOG("\n Matching agents %s -- %s ", POP_INFO, POP_INFOS(cur) );
                is_free = false;
                break; //found a partner, end search
            }
        }
    }
    find_partner_ret = V("Partner_Status");
}
REPORT_LOCAL_CLOCK_CND(0.02);
RESULT(find_partner_ret)

/******************************************************************************/
/*  Template Model End                                                        */
/*----------------------------------------------------------------------------*/

/* User specific below */










MODELEND




void close_sim(void)
{
    PAJEK_POP_LINEAGE_SAVE
}


