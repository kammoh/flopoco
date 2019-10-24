#include "IntMult/TilingStrategy.hpp"
#include "IntMultiplier.hpp"

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

	void TilingStrategy::printSolutionTeX(ofstream &outstream, int wTrunc, bool triangularStyle)
	{
		cerr << "Dumping multiplier schema in multiplier.tex\n";
		outstream << "\\documentclass{standalone}\n\\usepackage{tikz}\n\n\\begin{document}\n\\begin{tikzpicture}[yscale=-1,xscale=-1]\n";
		if(triangularStyle)
		{
			outstream << "\\draw[thick] (0, 0) -- ("<< wX << ", 0) -- ("<<(wX+wY) <<", " << wY<<") -- ("<< wY << ", " << wY <<") -- cycle;\n";
			for (size_t i = 1 ; i < static_cast<size_t>(wY) ; ++i) {
				outstream << "\\draw[dotted, thin, gray] ("<<i<<", " << i <<") -- (" << wX + i << ", " << i << ");\n";
			}
			for (size_t i = 1 ; i < static_cast<size_t>(wX) ; ++i) {
				outstream << "\\draw[dotted, thin, gray] ("<< i <<", 0) -- (" << (wY + i) << ", " << wY << ");\n";
			}
			for (auto& tile : solution) {
				auto& parametrization = tile.first;
				auto& coordinates = tile.second;
				int xstart = coordinates.first + coordinates.second;
				int ystart = coordinates.second;
				int xend = xstart + static_cast<int>(parametrization.getTileXWordSize());
				int yend = ystart + static_cast<int>(parametrization.getTileYWordSize());
				int deltaY = static_cast<int>(parametrization.getTileYWordSize());
				string color = (parametrization.isSignedMultX() || parametrization.isSignedMultY()) ? "red" : "blue";
				outstream << "\\draw[fill="<< color <<", fill opacity=0.3] (" << xstart << ", " << ystart << ") -- (" <<
					xend << ", " << ystart << ") -- ("<< xend + deltaY <<", "<< yend<<") -- ("<< xstart + deltaY <<", "<< yend <<")--cycle;\n";
				cerr << "Got one tile at (" << xstart << ", " << ystart << ") of size (" << parametrization.getTileXWordSize() << ", " << parametrization.getTileYWordSize() << ").\n";
			}

			int offset = IntMultiplier::prodsize(wX, wY) - wOut;

			if (offset > 0) {
				float startY = (wX <= offset) ? (offset - wX) + 0.5  : 0;
				float endY =  (offset >= wY) ? wY : offset + 0.5;
				outstream << "\\draw[ultra thick, green] (" << offset << ".5, " << startY << ") -- (" << offset << ".5, " << endY << ");" << endl;
			}

			if (wTrunc > offset) {
				int truncOffset = IntMultiplier::prodsize(wX, wY) - wTrunc;
				float startY = (wX <= truncOffset) ? (truncOffset - wX) + 0.5  : 0;
				float endY =  (truncOffset >= wY) ? wY : truncOffset + 0.5;
				outstream << "\\draw[ultra thick, brown] (" << truncOffset << ".5, " << startY << ") -- (" << truncOffset << ".5, " << endY << ");" << endl;
			}

			for (size_t i = 0 ; i < static_cast<size_t>(wX) ; ++i) {
				for (size_t j = 0 ; j < static_cast<size_t>(wY) ; ++j) {
					string color = (j+i >= (size_t)offset) ? "black" : "purple";
					outstream << "\\fill["<< color <<"] ("<<(j+i + 1)<<", " << j <<".5) circle (0.125);\n";
				}
			}

			outstream << "\\draw[red, thick] (0, 0) -- ("<< wX << ", 0) -- ("<<(wX+wY) <<", " << wY<<") -- ("<< wY << ", " << wY <<") -- cycle;\n";
		} else {
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
				for (int y = ystart; y < yend; y++){        // for not rectangular tiles, every field in its maximum outline has to be checked if it is covered
                    for (int x = xstart; x < xend; x++){
                        if(parametrization.shapeValid(x-xstart,y-ystart)){
                            outstream << "\\fill[fill=gray, fill opacity=0.3] (" << x << ", " << y << ") rectangle (" <<
                                      x+1 << ", " << y+1 << ");\n";
                            if((parametrization.shapeValid(x-xstart,y-ystart) && x-xstart == 0) || (x-xstart > 0 && !parametrization.shapeValid(x-xstart-1,y-ystart)))      //draw outline of individual tile, if neighbouring tile is not covered by it
                                outstream << "\\draw (" << x << "," << y << ") -- (" << x << "," << y+1 <<");\n";
                            if((parametrization.shapeValid(x-xstart,y-ystart) && y-ystart == 0) || (y-ystart > 0 && !parametrization.shapeValid(x-xstart,y-ystart-1)))
                                outstream << "\\draw (" << x << "," << y << ") -- (" << x+1 << "," << y <<");\n";
                            if(!parametrization.shapeValid(x-xstart+1,y-ystart))
                                outstream << "\\draw (" << x+1 << "," << y << ") -- (" << x+1 << "," << y+1 <<");\n";
                            if(!parametrization.shapeValid(x-xstart,y-ystart+1))
                                outstream << "\\draw (" << x << "," << y+1 << ") -- (" << x+1 << "," << y+1 <<");\n";
                        }
                    }
                }
/*				outstream << "\\draw[fill=gray, fill opacity=0.3] (" << xstart << ", " << ystart << ") rectangle (" <<
					xend << ", " << yend << ");\n";
*/				cerr << "Got one tile at (" << xstart << ", " << ystart << ") of size (" << parametrization.getTileXWordSize() << ", " << parametrization.getTileYWordSize() << ").\n";
			}
		}
		outstream << "\\end{tikzpicture}\n\\end{document}\n";
	}
}   //end namespace flopoco
