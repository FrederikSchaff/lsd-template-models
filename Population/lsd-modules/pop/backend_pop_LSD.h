/*************************************************************
* backend_pop_LSD.h
* Copyright: Frederik Schaff
* Version: 0.11 (jan 2019)
*
* Licence: MIT
*
*
*  LSD Population module - backend for LSD (least LSD-GIS)
*  written by Frederik Schaff, Ruhr-University Bochum
*
*  for infos on LSD see https://github.com/marcov64/Lsd
*
*  The complete package has the following files:
*  [1] readme.md         ; readme file with instructions and information
*                          on the underlying model.
*  [2] fun_templ_pop.cpp ; a template file for the user model, containing the
*                          links to the population backend (which needs to be place in a folder "lsd-modules/pop").
*  [3] lsd-modules\pop\backend_pop.h     ; contains the c++ declarations and new macros.
*  [4] lsd-modules\pop\backend_pop.cpp   ; contains the c++ core code for the pop backend.
*  [5] LICENSE ; the MIT licence
*  [6] LSD_macros_pop.html      ; The documentation
*
*  Currently implemented population model:
*  BLL : Boucekkine, Raouf; La Croix, David de; Licandro, Omar (2002):
*  Vintage Human Capital, Demographic Trends, and Endogenous Growth.
*  In Journal of Economic Theory 104 (2), pp. 340–375.
*  DOI: 10.1006/jeth.2001.2854.
*
*
*************************************************************/

#if !defined( LSD_GIS )
  #error The LSD population module is intended to work with LSD_GIS by Frederik Schaff.
#endif

#ifndef MODULE_POPULATION //GUARD
  #define MODULE_POPULATION
#endif

const bool TEST_POP_MODULE = false; //switch for testing

#include "backend_pop.h"
#include "backend_pop.cpp"



/*****
 *  MACROS
 *  Some macros for easy access in standard LSD manner.
 *
 ******/

pop_map *population; //create global pop_map;

#define INIT_POPULATION_MODULE(model, t_start,  ... ) { population = new pop_map(model, t_start, __VA_ARGS__ ); }
//to do: Change to VARGS stuff.

#define POP_INITN_GENERATIONS population->init_generations(); //number of generations it takes to initialise the model
#define POP_CONSTN_BIRTH(n) population->const_pop_fert(n);

#define POP_ADVANCE_TIME { population->advance_time(); }

//POP_ADD_PERSON macro: Make objects unique if not already. Then add this one to the pop-model.
#define POP_ADD_PERSON_WPARENTS( obj, obj_f, obj_m) \
  { \
    if (!obj->is_unique()) { obj->declare_as_unique( obj->label ); } \
    population->add_person(  obj, obj_f, obj_m); \
  }
#define POP_ADD_PERSON_WPARENT( obj_f, obj_m ) POP_ADD_PERSON_PARENTS(p, obj_f, obj_m )

#define POP_ADD_PERSONS(obj) POP_ADD_PERSON_PARENTS( obj, NULL, NULL)
#define POP_ADD_PERSON       POP_ADD_PERSON_PARENTS( p, NULL, NULL)

#define POP_DIE_PERSONS( obj ) population->person_dies( obj );
#define POP_DIE_PERSON population->person_dies( p );

#define POP_SET_DAGE( obj, d_age ) { population->person_set_d_age( p, d_age ); }
#define POP_SET_DAGES( obj, d_age ) { population->person_set_d_age( obj, d_age ); }

#define POP_ALIVE         population->is_alive( p )
#define POP_ALIVES( obj ) population->is_alive( obj )

#define POP_AGE population->age_of( p )
#define POP_AGES( obj ) population->age_of( obj )

#define POP_D_AGE         population->d_age_of( p )
#define POP_D_AGES( obj ) population->d_age_of( obj )

#define POP_FEMALE         population->is_female( p )
#define POP_FEMALES( obj ) population->is_female( obj )

#define POP_FATHER         population->father_of( p )
#define POP_FATHERS( obj ) population->father_of( obj )

#define POP_MOTHER         population->mother_of( p )
#define POP_MOTHERS( obj ) population->mother_of( obj )

#define POP_MGENITOR          population->alive_last_mgenitor(p)
#define POP_MGENITORS( obj ) population->alive_last_mgenitor(obj)

#define POP_CYCLE_CHILDREN for(object* c_child = population->first_child_of(p); c_child != NULL; c_child = population->next_child_of(p))
#define POP_CYCLE_CHILDRENS ( obj ) for(object* c_child = population->first_child_of(obj); c_child != NULL; c_child = population->next_child_of(obj))

#define POP_INFO           { plog(population->person_info(p).c_str()); }
#define POP_INFOS ( obj )  { population->person_info( obj ).c_str(); }


//Return random agents alive and with gender as specified

#define POP_RANDOM_PERSON(min_age, max_age) population->random_person(-1, min_age, max_age)
#define POP_RANDOM_PERSONF(min_age, max_age) population->random_person(1, min_age, max_age)
#define POP_RANDOM_PERSONM(min_age, max_age) population->random_person(0, min_age, max_age)

#define POP_FAMILY_DEGREE( obj1, obj2 ) population->family_degree( obj1, obj2, -5);
#define POP_CHECK_INCEST( obj1, obj2, prohibDegree ) population->check_if_incest( obj1, obj2, prohibDegree )


//A set of macros in combination with pajek
//not yet adjusted!

#ifdef PAJEKFROMCPP
//to do: Make class-dependent
//Add "child" relation
  //Cycle through all agents, alive or dead, and add them to the network.
  //Then cycle through all again and add links to children.
  //use as y the time of birth, as x the # at birth
  #define PAJEK_POP_LINEAGE_SAVE \
    if (population != NULL && population->n_persons() > 0){ \
    PAJ_STATIC("Pajek","POP_MODEL",RND_SEED,"Lineage"); \
    int x = 0;     \
    int last_born = 0;      \
    for (auto it_item = population->it_persons_begin(); it_item != population->it_persons_end(); it_item++) {    \
      if (last_born > it_item->second.t_birth){  \
        last_born = it_item->second.t_birth;    \
        x = 0;   \
      } else {    \
        x++;      \
      } \
      PAJ_S_ADD_V_C(T,it_item->second.uID,"Person",last_born,x,last_born,it_item->second.female?"woman":"man",it_item->second.children.c_uIDs.size(),it_item->second.children.c_uIDs.size(),it_item->second.female?"Red":"Blue")   \
    }        \
    for (auto it_item = population->it_persons_begin(); it_item != population->it_persons_end(); it_item++) {    \
      for (auto child : it_item->second.children.c_uIDs) {      \
        PAJ_S_ADD_A(T,it_item->second.uID,"Person",child,"Person",1.0,"Child"); \
      }              \
    } \
    PAJ_S_SAVE \
    }

#else
  #define PAJEK_POP_LINEAGE_SAVE
#endif
