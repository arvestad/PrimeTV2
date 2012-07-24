#include  "StrStrMap.hh"

#include <string.h>
#include <set>
#include <map>
#include <iostream>

#include "AnError.hh"

// Author: Lars Arvestad, � the MCMC-club, SBC, all rights reserved
//namespace beep
//{
  using namespace std;

  //---------------------------------------------------------------------
  // 
  // Construct / Destruct
  //
  //---------------------------------------------------------------------
  StrStrMap::StrStrMap() 
    : avbildning()
  {
  }

  StrStrMap::~StrStrMap() 
  {
  }

  StrStrMap::StrStrMap(const StrStrMap& sm)
    : avbildning(sm.avbildning)
  {    
  }

  StrStrMap& 
  StrStrMap::operator=(const StrStrMap &sm)
  {
    if(&sm != this)
      {
	avbildning = sm.avbildning;
      }
    return *this;
  }

  //---------------------------------------------------------------------
  // 
  // Interface
  //
  //---------------------------------------------------------------------

  // Adding relations
  void
  StrStrMap::insert(const string &x, const string &y)
  {
    avbildning.insert(pair<string,string>(x, y));
  }

  void
  StrStrMap::change(const string &x, const string &y)
  {
    if(avbildning.find(x) != avbildning.end())
      {
	avbildning[x] = y;
      }
    else
      {
	avbildning.insert(pair<string,string>(x, y));
      }
  }

  // Retrieval
  string
  StrStrMap::find(const string &s) const
  {
    map<string, string>::const_iterator iter;

    iter = avbildning.find(s);
    if (iter == avbildning.end())
      {
	return "";
      }
    else 
      {
	return iter->second;
      }
  }

  std::string
  StrStrMap::getNthItem(unsigned idx) const
  {
    for (map<string,string>::const_iterator i = avbildning.begin();
	 i != avbildning.end();
	 i++)
      {
	if (idx == 0)
	  {
	    return i->first;
	  }
	else
	  {
	    idx--;
	  }
      }
    PROGRAMMING_ERROR("Out of bounds.");
    return("");  
  }

  // reset map
  void
  StrStrMap::clearMap()
  {
    avbildning.clear();
  }

  // Diagnostics. Find how many relations are stored
  unsigned
  StrStrMap::size() const
  {
    return avbildning.size();
  }
  unsigned 
  StrStrMap::reverseSize() const
  {
    set<string> reverse;
    for(map<string,string>::const_iterator i = avbildning.begin();
	i != avbildning.end(); i++)
      {
	if(reverse.find(i->second) != reverse.end())
	  reverse.insert(i->second);
      }
    return reverse.size();
  }


  //---------------------------------------------------------------------
  // Output, mainly for debugging
  //---------------------------------------------------------------------
  // Write map to ostream, one string pair per line.
  std::ostream& 
  operator<<(std::ostream &o, const StrStrMap &m)
  {
    string res;

    for (map<string,string>::const_iterator i = m.avbildning.begin();
	 i != m.avbildning.end();
	 i++)
      {
	res.append(i->first + "\t" + i->second + "\n");
      }
    return o << res;
  }

//}//end namespace beep
