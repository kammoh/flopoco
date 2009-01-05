#ifndef OTHERLOGTABLE_HPP
#define OTHERLOGTABLE_HPP

#include "../Table.hpp"

class OtherLogTable : public Table
{
 public:
  OtherLogTable(int wIn, int wOut, int p, int which);
  
  ~OtherLogTable();
  
  mpz_class function(int x);

  int    double2input(double x);

  double input2double(int x);
  
  mpz_class double2output(double x);

  double output2double(mpz_class x);

  int p;

  // Which table is that ? useful because the first is different from the others.
  int which;

  int outputPrecision;

private:

};






#endif //OTHERLOGTABLE_HPP

