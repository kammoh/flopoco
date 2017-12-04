#ifndef GENERICTABLE_HPP
#define GENERICTABLE_HPP

#include <vector>

#include "../Table.hpp"

namespace flopoco{

	class GenericTable : public Table
	{
	public:
		GenericTable(Target* target, int wIn, int wOut, vector<mpz_class> values, bool logicTable = false);

		~GenericTable();

		mpz_class function(int x);

		vector<mpz_class> values; //the values to be stored in the table
	};

}
#endif //GENERICTABLE_HPP

