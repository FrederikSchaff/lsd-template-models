/*************************************************************
* backend_pop.cpp
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

void pop_map::add_person(object *uID, object *f_uID, object *m_uID){
  add_person(uID->unique_id(), f_uID != NULL ? f_uID->unique_id() : -1, m_uID != NULL ? m_uID->unique_id() : -1);
}

void pop_map::add_person(int uID, int f_uID, int m_uID)
{
  bool female = RND > 0.5 ? true : false;
  const double age = 0.0;
  const double &t_birth = t_now;
  persons.emplace_hint(persons.end(),uID,pop_person(uID, f_uID, m_uID, female, t_birth, age, model->age_of_death() ) );
  if (true == female)
    females_by_birth.emplace_hint(females_by_birth.end(),t_birth,uID);
  else
    males_by_birth.emplace_hint(males_by_birth.end(),t_birth,uID);

  VERBOSE_MODE(TEST_POP_MODULE && uID < 50){
    char test_msg[300];
    sprintf(test_msg,"\ncalled add_person(). Added person with ID %i, father %i, mother %i, age %g, death age %g, gender %i",persons.at(uID).uID,persons.at(uID).f_uID,persons.at(uID).m_uID,persons.at(uID).age,persons.at(uID).d_age,persons.at(uID).female);
    plog(test_msg);
  }
  if (f_uID > 0)
    add_child(f_uID, uID);
  if (m_uID > 0)
    add_child(m_uID, uID);
}

char pop_map::advance_age_or_die(int uID)
{
  if (persons.at(uID).age == persons.at(uID).d_age){
    return 'd';
  } else {
    persons.at(uID).age+=t_unit;
    return 'a';
  }
}

void pop_map::advance_time()
{
  t_now += t_unit;
  std::vector<int> will_die;
  //Cycle through living persons (fe)male, age and delete as necessary.
//   auto const &next_p = males_by_birth.begin(); //deleting invalidates the current iterator
  std::multimap< double, int >::iterator current_m = males_by_birth.begin(); //deleting invalidates the current iterator
  while (current_m != males_by_birth.end())
  {
    int c_uID = current_m->second;
    std::advance(current_m,1);
    if (advance_age_or_die(c_uID) == 'd')
      will_die.push_back(c_uID);
  }
  std::multimap< double, int >::iterator current_f = females_by_birth.begin();
  while (current_f != females_by_birth.end())
  {
    int c_uID = current_f->second;
    std::advance(current_f,1);
    if (advance_age_or_die(c_uID) == 'd')
      will_die.push_back(c_uID);
  }
  for (auto const &item : will_die){
    person_dies(item);
  }
}

void pop_map::person_dies(object *uID){
  person_dies(uID->unique_id());
}

void pop_map::person_dies(int uID)
{
  if (false == persons.at(uID).alive){
    error_hard( "Error in 'pop_map::person_dies(int uID)'", "The person that should die is already declared dead",
							"check your code to prevent this situation." );
    return;
  }

  std::multimap<double,int>::iterator to_del;
  //find first position in (fe)male subset with correct age
  persons.at(uID).alive = false; //set to dead
  if (true == persons.at(uID).female)
    to_del = females_by_birth.find(persons.at(uID).t_birth);
  else
    to_del = males_by_birth.find(persons.at(uID).t_birth);

  //commence to correct person (multiple persons with same age may exist)
  while (to_del->second != uID)
    to_del++;

  //check, can be deactivated if tested sufficiently
  if (to_del->first != persons.at(uID).t_birth){
    error_hard( "Error in 'pop_map::person_dies(int uID)'", "The person that should die could not be found",
							"contact the developer." );
    return;
  }

  //erase person from the subset
  if (true == persons.at(uID).female)
    females_by_birth.erase(to_del);
  else
    males_by_birth.erase(to_del);

  VERBOSE_MODE(TEST_POP_MODULE && uID < 50){
    char test_msg[300];
    sprintf(test_msg,"\ncalled person_dies(). Delete person with ID %i, father %i, mother %i, age %g, death age %g, gender %i",uID,persons.at(uID).f_uID,persons.at(uID).m_uID,persons.at(uID).age,persons.at(uID).d_age,persons.at(uID).female);
    plog(test_msg);
  }

}

void pop_map::person_set_d_age(object *uID, double d_age){
  person_set_d_age(uID->unique_id(), d_age);
}

void pop_map::person_set_d_age(int uID, double d_age)
{
  double adjusted_d_age = floor(d_age*t_unit)
        + ( ( RND < floor(d_age*t_unit) - d_age*t_unit ) ? t_unit : 0.0  );

  VERBOSE_MODE(TEST_POP_MODULE && uID < 50){
    char test_msg[300];
    sprintf(test_msg,"\ncalled person_set_d_age(). Adjust age of person ID %i with age %g and former death age %g to new death age %g (t_unit %g).",
      uID,persons.at(uID).age,persons.at(uID).d_age,adjusted_d_age, t_unit);
    plog(test_msg);
  }

  persons.at(uID).d_age=adjusted_d_age;
}

void pop_map::add_child(int uID, int c_uID){
  persons.at(uID).children.c_uIDs.push_back(c_uID);
}

object* pop_map::mother_of(object *uID){
  return mother_of( uID->unique_id() );
}

object* pop_map::mother_of(int uID)
{
  int m_uID = persons.at(uID).m_uID;
  if (m_uID > 0)
    return root->obj_by_unique_id( m_uID );
  return NULL; //no father case
}

object* pop_map::father_of(object *uID) {
  return father_of( uID->unique_id() );
}

object* pop_map::father_of(int uID)
{
  int f_uID = persons.at(uID).f_uID;
  if (f_uID > 0)
    return root->obj_by_unique_id( f_uID );
  return NULL; //no father case
}

object* pop_map::first_child_of(object *uID){
  return first_child_of( uID->unique_id() );
}

object* pop_map::first_child_of(int uID)
{
  //initialisation
  persons.at(uID).children.child=0;
  //get child
  return next_child_of(uID);
}

object* pop_map::next_child_of(object *uID){
  return next_child_of( uID->unique_id() );
}

object* pop_map::next_child_of(int uID)
{
  if (persons.at(uID).children.child < persons.at(uID).children.c_uIDs.size())
    return root->obj_by_unique_id( persons.at(uID).children.c_uIDs.at( persons.at(uID).children.child++ ) );
  return NULL;
}

//get alive father of last child. If the father of the last child is dead, return NULL.
object* pop_map::alive_last_mgenitor(object *uID){
  return alive_last_mgenitor(uID->unique_id());
}

object* pop_map::alive_last_mgenitor(int uID){
  if (persons.at(uID).children.c_uIDs.size()==0){
    return NULL; //no children yet, hence no father
  } else {
    int lastchild = persons.at(uID).children.c_uIDs.back();
    int lastmgenitor = persons.at(lastchild).f_uID;
    if ( lastmgenitor == -1 ) {
      return NULL; //no father
    } else {
      if (true == persons.at(lastmgenitor).alive){
        return root->obj_by_unique_id(lastmgenitor); //found the father
      } else {
        return NULL; //former male genitor is dead
      }
    }
  }
}

double pop_map::age_of(object *uID)
{
  return  age_of( uID->unique_id() );
}

double pop_map::age_of(int uID){
  return persons.at(uID).age;
}

double pop_map::d_age_of(object *uID)
{
  return  d_age_of( uID->unique_id() );
}

double pop_map::d_age_of(int uID){
  return persons.at(uID).d_age;
}

bool pop_map::is_female(object *uID)
{
  return  is_female( uID->unique_id() );
}

bool pop_map::is_female(int uID){
  return persons.at(uID).female;
}

bool pop_map::is_alive(object *uID)
{
  return  is_alive( uID->unique_id() );
}

bool pop_map::is_alive(int uID){
  return persons.at(uID).alive;
}


object* pop_selection::first()
{
  it_selection = selection.begin();
  if (it_selection == selection.end())
    return NULL;
  else
    return root->obj_by_unique_id(it_selection->second);
}

object* pop_selection::next()
{
  if (++it_selection == selection.end() )
    return NULL;
  else
    return root->obj_by_unique_id(it_selection->second);
}

object* pop_map::random_person(int gender, double age_low, double age_high )
{
  return  pop_selection(this, gender, age_low, age_high).first();
}


std::string pop_map::person_info(object *uID){
  return person_info(uID->unique_id());
}

std::string pop_map::person_info(int uID){
  std::string buffer =  "\n Person info:";
  buffer += "\n\t uID: "      + std::to_string(persons.at(uID).uID);
  buffer += "\n\t Status: "   + std::string(persons.at(uID).alive ? "alive" : "dead");
  buffer += "\n\t mother: "   + std::to_string(persons.at(uID).m_uID);
  buffer += "\n\t father: "   + std::to_string(persons.at(uID).f_uID);
  buffer += "\n\t gender: "   + std::string(persons.at(uID).female ? "female" : "male");
  buffer += "\n\t age: "      + std::to_string(persons.at(uID).age);
  buffer += "\n\t age of death: "  + std::to_string(persons.at(uID).d_age);
  return buffer;
}


  /* a function that returns the family degree, defined as follows:
    -1 - flag error.

    0 - none of the below/tested ones
    1 - siblings
    2 - parent-child
    3 - grandchild-grandparent
	  4 - nephew - uncle/aunt
    5 - cousin - cousin
  */
int pop_map::family_degree(object *m_uID, object *f_uID, int max_tested_degree){
  return family_degree(m_uID->unique_id(), f_uID->unique_id(), max_tested_degree);
}

int pop_map::family_degree(int m_uID, int f_uID, int max_tested_degree)
{
  bool verbose_logging = true;

  VERBOSE_MODE(verbose_logging){
    PLOG("\nPopulation Model :   : ext_pop::check_if_incest : called with mother %i, father %i and max degree %i",m_uID,f_uID,max_tested_degree);
  }

  if (max_tested_degree == -1) {max_tested_degree=5;} //test everything as default

  int tested_degree = 0;
  if (max_tested_degree == 0){ return 0;  } //check if at end


  //test if orphan
  if (m_uID < 0 && f_uID < 0) { //no parents

    VERBOSE_MODE(verbose_logging){
      PLOG("\nt .. full orphan");
    }
    return 0; //full orphan
  }

  //set-up
  pop_person* c_mother = NULL; //default: no mother
  if (m_uID >= 0) {
    c_mother = &persons.at(m_uID);
  }
  pop_person* c_father = NULL;
  if (f_uID >= 0) {
    c_father = &persons.at(f_uID);
  }


  //Start serious testing

  //mother siblings
  if (c_mother->m_uID != -1 && c_mother->m_uID == c_father->m_uID) {
    return 1;
  }
  //father siblings
  if (c_mother->f_uID != -1 && c_mother->f_uID == c_father->f_uID) {
    return 1;
  }
    VERBOSE_MODE(verbose_logging){
      PLOG("\nt .. not siblings");
    }
  tested_degree++;
  if (max_tested_degree == tested_degree){ return 0; }

  //parent-child?
  if ( (c_mother->f_uID != -1 && c_mother->f_uID == f_uID)
    || (c_father->m_uID != -1 && c_father->m_uID == m_uID)
     ){
    return 2;
  }
    VERBOSE_MODE(verbose_logging){
      PLOG(", nor parent-child");
    }
  tested_degree++;
  if (max_tested_degree == tested_degree){ return 0; }

  //grandchild-grandparent?
  if ( (/* grandchild-grandpas */
         (c_mother->f_uID != -1 && persons.at(c_mother->f_uID).f_uID != -1 && persons.at(c_mother->f_uID).f_uID == f_uID)
      || (c_mother->m_uID != -1 && persons.at(c_mother->m_uID).f_uID != -1 && persons.at(c_mother->m_uID).f_uID == f_uID)
       ) ||
       (/*grandson-grandmas */
         (c_father->f_uID != -1 && persons.at(c_father->f_uID).m_uID != -1 && persons.at(c_father->f_uID).m_uID == m_uID)
      || (c_father->m_uID != -1 && persons.at(c_father->m_uID).m_uID != -1 && persons.at(c_father->m_uID).m_uID == m_uID)
       )
     ){
    return 3;
  }

    VERBOSE_MODE(verbose_logging){
      PLOG(", nor grandchild-grandparent");
    }
  tested_degree++;
  if (max_tested_degree == tested_degree){ return 0; }

  //Aunt/Uncle - Nephew
  if ( (/*niece-uncles*/
                (c_mother->f_uID != -1 && persons.at(c_mother->f_uID).f_uID != -1 && c_father->f_uID != -1 && c_father->f_uID == persons.at(c_mother->f_uID).f_uID)
            ||  (c_mother->f_uID != -1 && persons.at(c_mother->f_uID).m_uID != -1 && c_father->m_uID != -1 && c_father->m_uID == persons.at(c_mother->f_uID).m_uID)
            ||  (c_mother->m_uID != -1 && persons.at(c_mother->m_uID).f_uID != -1 && c_father->f_uID != -1 && c_father->f_uID == persons.at(c_mother->m_uID).f_uID)
            ||  (c_mother->m_uID != -1 && persons.at(c_mother->m_uID).m_uID != -1 && c_father->m_uID != -1 && c_father->m_uID == persons.at(c_mother->m_uID).m_uID)
       ) ||
       (/*aunt-nephews*/
                (c_mother->f_uID != -1 && c_father->f_uID != -1 && persons.at(c_father->f_uID).f_uID != -1 && c_mother->f_uID == persons.at(c_father->f_uID).f_uID)
            ||  (c_mother->f_uID != -1 && c_father->m_uID != -1 && persons.at(c_father->m_uID).f_uID != -1 && c_mother->f_uID == persons.at(c_father->m_uID).f_uID)
            ||  (c_mother->m_uID != -1 && c_father->f_uID != -1 && persons.at(c_father->f_uID).m_uID != -1 && c_mother->m_uID == persons.at(c_father->f_uID).m_uID)
            ||  (c_mother->m_uID != -1 && c_father->m_uID != -1 && persons.at(c_father->m_uID).m_uID != -1 && c_mother->m_uID == persons.at(c_father->m_uID).m_uID)
       )
     ){
    return 4;
  }
    VERBOSE_MODE(verbose_logging){
      PLOG(", nor aunt/uncle-niece/nephew");
    }
  tested_degree++;
  if (max_tested_degree == tested_degree){ return 0; }

  //Cousins
  if (
       (/*cousins*/
                (c_mother->f_uID != -1 && persons.at(c_mother->f_uID).f_uID != -1 && c_father->f_uID != -1 && persons.at(c_father->f_uID).f_uID != -1 && persons.at(c_father->f_uID).f_uID == persons.at(c_mother->f_uID).f_uID)
            ||  (c_mother->f_uID != -1 && persons.at(c_mother->f_uID).m_uID != -1 && c_father->f_uID != -1 && persons.at(c_father->f_uID).m_uID != -1 && persons.at(c_father->f_uID).m_uID == persons.at(c_mother->f_uID).m_uID)
            ||  (c_mother->m_uID != -1 && persons.at(c_mother->m_uID).f_uID != -1 && c_father->f_uID != -1 && persons.at(c_father->f_uID).f_uID != -1 && persons.at(c_father->f_uID).f_uID == persons.at(c_mother->m_uID).f_uID)
            ||  (c_mother->m_uID != -1 && persons.at(c_mother->m_uID).m_uID != -1 && c_father->f_uID != -1 && persons.at(c_father->f_uID).m_uID != -1 && persons.at(c_father->f_uID).m_uID == persons.at(c_mother->m_uID).m_uID)
       ) ||
       (/*         */
                (c_mother->f_uID != -1 && persons.at(c_mother->f_uID).f_uID != -1 && c_father->m_uID != -1 && persons.at(c_father->m_uID).f_uID != -1 && persons.at(c_father->m_uID).f_uID == persons.at(c_mother->f_uID).f_uID)
            ||  (c_mother->f_uID != -1 && persons.at(c_mother->f_uID).m_uID != -1 && c_father->m_uID != -1 && persons.at(c_father->m_uID).m_uID != -1 && persons.at(c_father->m_uID).m_uID == persons.at(c_mother->f_uID).m_uID)
            ||  (c_mother->m_uID != -1 && persons.at(c_mother->m_uID).f_uID != -1 && c_father->m_uID != -1 && persons.at(c_father->m_uID).f_uID != -1 && persons.at(c_father->m_uID).f_uID == persons.at(c_mother->m_uID).f_uID)
            ||  (c_mother->m_uID != -1 && persons.at(c_mother->m_uID).m_uID != -1 && c_father->m_uID != -1 && persons.at(c_father->m_uID).m_uID != -1 && persons.at(c_father->m_uID).m_uID == persons.at(c_mother->m_uID).m_uID)
       )
     ){
    return 5;
  }
    VERBOSE_MODE(verbose_logging){
      PLOG(", nor cousins");
    }
  tested_degree++;
  if (max_tested_degree == tested_degree){ return 0; }

  //lesser degree
  return 0;
}

// Check the family degree of the relationship
  //Check if there is potential of incest with given "degree" - only direct biology.
  // allowed degree: 0 - incest allowed, 1 - siblings, 2 - also parent-child, 3 - also grandchild-grandparent, 4 - also nephew - uncle/aunt, 5 - also cousin - cousin.
  // we compare be-directional, to also catch if a mother would have a child with a (grand)child.
  //returns true if there is incest
  bool pop_map::check_if_incest(object *m_uID, object *f_uID, int prohibited_degree){
    return check_if_incest(m_uID->unique_id(), f_uID->unique_id(), prohibited_degree);
  }

  bool pop_map::check_if_incest(int m_uID, int f_uID, int prohibited_degree)
  {
    bool verbose_logging = true;
    if (prohibited_degree == 0){
      return false; //no prohibited incest.
    }
    int kinship_degree = family_degree(m_uID, f_uID, prohibited_degree);

    if (kinship_degree >= prohibited_degree){
      VERBOSE_MODE(verbose_logging){
        PLOG("\nPopulation Model :   : ext_pop::check_if_incest : Maximum forbidden degree is %i",prohibited_degree);
        switch (kinship_degree){

          case 0: PLOG("\n\t... No relevant family relation. ERROR this should not happen.");
                  break;
          case 1: PLOG("\n\t... siblings.");
                  break;
          case 2: PLOG("\n\t... parent-child.");
                  break;
          case 3: PLOG("\n\t... grandchild-grandparent.");
                  break;
          case 4: PLOG("\n\t... nephew - uncle/aunt.");
                  break;
          case 5: PLOG("\n\t... cousin - cousin.");
                  break;
        }
      }

      return true; //incest
    } else {
      return false;
    }
  }

  double pop_model_BLL::uncond_sr(double age) //chance to live up to year.
  {
    //bll (1), survival function
    return (exp(-beta * age) - alpha)/(1.0-alpha);
  }

  double pop_model_BLL::const_pop_fert(double n) //fertility rate to keep population constant if in equilibrium.
  {
    //inverse of life expectancy times n
    double raw = (n/exp_life) * t_unit;
    double adjusted = floor(raw) + ( RND < (raw - floor(raw) ) ? 1.0 : 0.0);
    return adjusted;
  }

  double pop_model_BLL::age_of_death()
  {
    double raw =  - ( log( RND * (1-alpha) + alpha ) / beta ); //from solving m(a) for a.
    double prob = raw*t_unit - floor(raw * t_unit);
    double adjusted = floor(raw * t_unit)/t_unit + ( RND < prob ? 1.0 : 0.0 );
    //PLOG("\nRaw %g, prob %g, adjusted %g",raw,prob,adjusted);
    return adjusted;
  }
