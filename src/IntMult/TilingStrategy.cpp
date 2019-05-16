#include "IntMult/TilingStrategy.hpp"

namespace flopoco {


TilingStrategy::TilingStrategy(int wX, int wY, int wOut, bool signedIO, BaseMultiplierCollection *baseMultiplierCollection) :
	wX(wX), wY(wY), wOut(wOut), signedIO(signedIO), baseMultiplierCollection(baseMultiplierCollection)
{
}

void TilingStrategy::printSolution()
{
    for (auto& tile : solution)
    {
        BaseMultiplierCategory::Parametrization& parametrization = tile.first;
        multiplier_coordinates_t& coordinates = tile.second;

        cerr << "multiplier of type " << parametrization.getMultType() << " placed at (" << coordinates.first << "," << coordinates.second << ") of size (" << parametrization.getTileXWordSize() << ", " << parametrization.getTileYWordSize() << ")" << endl;
    }

}

void TilingStrategy::printSolutionTeX(ofstream &outstream)
{
    cerr << "Dumping multiplier schema in multiplier.tex\n";
    outstream << "\\documentclass{standalone}\n\\usepackage{tikz}\n\n\\begin{document}\n\\begin{tikzpicture}[yscale=-1,xscale=-1]\n";
    outstream << "\\draw[thick] (0, 0) rectangle (" << wX << ", " << wY << ");\n";
    for (size_t i = 0 ; i < static_cast<size_t>(wY) ; ++i) {
        outstream << "\\draw[dotted, very thin, gray] (0, " << i <<") -- (" << wX << ", " << i << ");\n";
    }
    for (size_t i = 0 ; i < static_cast<size_t>(wX) ; ++i) {
        outstream << "\\draw[dotted, very thin, gray] ("<< i <<", 0) -- (" << i << ", " << wY << ");\n";
    }
    for (auto& tile : solution) {
        auto& parametrization = tile.first;
        auto& coordinates = tile.second;
        int xstart = coordinates.first;
        int ystart = coordinates.second;
        int xend = xstart + static_cast<int>(parametrization.getTileXWordSize());
        int yend = ystart + static_cast<int>(parametrization.getTileYWordSize());
        outstream << "\\draw[fill=gray, fill opacity=0.3] (" << xstart << ", " << ystart << ") rectangle (" <<
                     xend << ", " << yend << ");\n";
        cerr << "Got one tile at (" << xstart << ", " << ystart << ") of size (" << parametrization.getTileXWordSize() << ", " << parametrization.getTileYWordSize() << ").\n";
    }
    outstream << "\\end{tikzpicture}\n\\end{document}\n";
}
}   //end namespace flopoco
