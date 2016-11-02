/*
   A class used for plotting bitheaps

   This file is part of the FloPoCo project

Author : Florent de Dinechin, Kinga Illyes, Bogdan Popa, Matei Istoan

Initial software.
Copyright © INSA de Lyon, ENS-Lyon, INRIA, CNRS, UCBL
2016.
All rights reserved.

*/

#include "BitheapPlotter.hpp"


using namespace std;

namespace flopoco
{

	BitheapPlotter::Snapshot::Snapshot(vector<vector<Bit*> > bits_, int maxHeight_, int cycle_, double criticalPath_):
		maxHeight(maxHeight_), cycle(cycle_), criticalPath(criticalPath_)
	{
		for(unsigned int i=0; i<bits_.size(); i++)
		{
			vector<Bit*> newColumn;

			if(bits_[i].size() > 0)
			{
				for(vector<Bit*>::iterator it = bits_[i].begin(); it!=bits_[i].end(); ++it)
				{
					Bit* b = new Bit(*it);
					newColumn.push_back(b);
				}
			}

			bits.push_back(newColumn);
		}
	}


	bool operator< (BitheapPlotter::Snapshot& b1, BitheapPlotter::Snapshot& b2)
	{
		return ((b1.cycle < b2.cycle) || (b1.cycle==b2.cycle && b1.criticalPath<b2.criticalPath));
	}

	bool operator<= (BitheapPlotter::Snapshot& b1, BitheapPlotter::Snapshot& b2)
	{
		return ((b1.cycle<b2.cycle) || (b1.cycle==b2.cycle && b1.criticalPath<=b2.criticalPath));
	}

	bool operator== (BitheapPlotter::Snapshot& b1, BitheapPlotter::Snapshot& b2)
	{
		return (b1.cycle==b2.cycle && b1.criticalPath==b2.criticalPath);
	}

	bool operator!= (BitheapPlotter::Snapshot& b1, BitheapPlotter::Snapshot& b2)
	{
		return ((b1.cycle!=b2.cycle) || (b1.cycle==b2.cycle && b1.criticalPath!=b2.criticalPath));
	}

	bool operator> (BitheapPlotter::Snapshot& b1, BitheapPlotter::Snapshot& b2)
	{
		return ((b1.cycle>b2.cycle) || (b1.cycle==b2.cycle && b1.criticalPath>b2.criticalPath));
	}

	bool operator>= (BitheapPlotter::Snapshot& b1, BitheapPlotter::Snapshot& b2)
	{
		return ((b1.cycle>b2.cycle) || (b1.cycle==b2.cycle && b1.criticalPath>=b2.criticalPath));
	}


	BitheapPlotter::BitheapPlotter(BitheapNew* bitheap_) :
			bitheap(bitheap_)
	{
		srcFileName = bitheap_->getName() + ":BitheapPlotter";
	}



	BitheapPlotter::~BitheapPlotter()
	{

	}


	void BitheapPlotter::takeSnapshot(Bit *soonestBit)
	{
		Snapshot* s = new Snapshot(bitheap->getBits(), bitheap->getMaxHeight(), soonestBit->signal->getCycle(), soonestBit->signal->getCriticalPath());

		snapshots.push_back(s);
	}



	void BitheapPlotter::plotBitHeap()
	{
		drawInitialBitheap();
		drawBitheapCompression();
	}


	void BitheapPlotter::drawInitialBitheap()
	{
		int offsetY, turnaroundX;

		initializePlotter(true);

		offsetY += 20 + snapshots[0]->maxHeight * 10;
		turnaroundX = snapshots[0]->bits.size() * 10 + 80;

		drawInitialConfiguration(snapshots[0], offsetY, turnaroundX);

		closePlotter(offsetY, turnaroundX);
	}


	void BitheapPlotter::drawBitheapCompression()
	{
		initializePlotter(false);

		int offsetY = 0;
		int turnaroundX = snapshots[snapshots.size()-1]->bits.size() * 10 + 100;

		for(unsigned int i=0; i<snapshots.size(); i++)
		{
			offsetY += 15 + snapshots[i]->maxHeight * 10;

			drawConfiguration(i, snapshots[i], offsetY, turnaroundX);

			fig << "<line x1=\"" << turnaroundX + 150 << "\" y1=\"" << offsetY + 10
					<< "\" x2=\"" << 50 << "\" y2=\"" << offsetY + 10
					<< "\" style=\"stroke:lightsteelblue;stroke-width:1\" />" << endl;
		}

		closePlotter(offsetY, turnaroundX);
	}


	void BitheapPlotter::initializePlotter(bool isInitial)
	{
		ostringstream figureFileName;

		if(isInitial)
			figureFileName << "BitHeap_initial_" << bitheap->getName()  << ".svg";
		else
			figureFileName << "BitHeap_compression_" << bitheap->getName()  << ".svg";

		FILE* pfile;
		pfile  = fopen(figureFileName.str().c_str(), "w");
		fclose(pfile);

		fig.open (figureFileName.str().c_str(), ios::trunc);

		fig << "<?xml version=\"1.0\" standalone=\"no\"?>" << endl
				<< "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"" << endl
				<< "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">" << endl
				<< "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\" onload=\"init(evt)\" >" << endl;

		addECMAFunction();
	}


	void BitheapPlotter::closePlotter(int offsetY, int turnaroundX)
	{
		fig << "<line x1=\"" << turnaroundX + 30 << "\" y1=\"" << 20 << "\" x2=\"" << turnaroundX + 30
				<< "\" y2=\"" << offsetY +30 << "\" style=\"stroke:midnightblue;stroke-width:1\" />" << endl;

		fig << "<rect class=\"tooltip_bg\" id=\"tooltip_bg\" x=\"0\" y=\"0\" rx=\"4\" ry=\"4\" width=\"55\" height=\"17\" visibility=\"hidden\"/>" << endl;
		fig << "<text class=\"tooltip\" id=\"tooltip\" x=\"0\" y=\"0\" visibility=\"hidden\">Tooltip</text>" << endl;

		fig << "</svg>" << endl;

		fig.close();
	}


	void BitheapPlotter::drawInitialConfiguration(Snapshot* snapshot, int offsetY, int turnaroundX)
	{
		int color = 0;

		fig << "<line x1=\"" << turnaroundX + 150 << "\" y1=\""
				<< offsetY +10 << "\" x2=\"" << turnaroundX - snapshot->bits.size()*10 - 50
				<< "\" y2=\"" << offsetY +10 << "\" style=\"stroke:lightsteelblue;stroke-width:1\" />" << endl;

		for(unsigned int i=0; i<snapshot->bits.size(); i++)
		{
			if(snapshot->bits[i].size() > 0)
			{
				for(unsigned int j=0; j<snapshot->bits[i].size(); j++)
				{
					drawBit(j, turnaroundX, offsetY, snapshot->bits[i][j]->signal->getCycle(), snapshot->bits[i][j]);
				}
			}
		}
	}


	void BitheapPlotter::drawConfiguration(int index, Snapshot *snapshot, int offsetY, int turnaroundX)
	{
		int cnt = 0;
		int ci,c1,c2,c3;										//print cp as a number as a rational number, in nanoseconds
		int cpint = snapshot->criticalPath * 1000000000000;

		c3 = cpint % 10;
		cpint = cpint / 10;
		c2 = cpint % 10;
		cpint = cpint / 10;
		c1 = cpint % 10;
		cpint = cpint / 10;
		ci = cpint % 10;

		if(index == 0)
		{
			fig << "<text x=\"" << turnaroundX + 50 << "\" y=\"" << offsetY + 3
					<< "\" fill=\"midnightblue\">" << "initial bitheap contents" << "</text>" << endl;
		}else if(index == snapshots.size()-1)
		{
			fig << "<text x=\"" << turnaroundX + 50 << "\" y=\"" << offsetY + 3
					<< "\" fill=\"midnightblue\">" << "before final addition" << "</text>" << endl;
		}else
		{
			fig << "<text x=\"" << turnaroundX + 50 << "\" y=\"" << offsetY + 3
					<< "\" fill=\"midnightblue\">" << snapshot->cycle << "</text>" << endl
					<< "<text x=\"" << turnaroundX + 80 << "\" y=\"" << offsetY + 3
					<< "\" fill=\"midnightblue\">" << ci << "." << c1 << c2 << c3 << " ns"  << "</text>" << endl;
		}

		for(unsigned int i=0; i<snapshot->bits.size(); i++)
		{
			if(snapshot->bits[i].size() > 0)
			{
				for(unsigned int j=0; j<snapshot->bits[i].size(); j++)
				{
					drawBit(j, turnaroundX, offsetY, snapshot->bits[i][j]->signal->getCycle(), snapshot->bits[i][j]);
				}
			}
		}
	}


	void BitheapPlotter::drawBit(int index, int turnaroundX, int offsetY, int color, Bit *bit)
	{
		const std::string colors[] = { "darkblue", "mediumblue", "blue", "royalblue", "dodgerblue", "cornflowerblue", "teal",
				"darkcyan", "deepskyblue", "darkturquoise", "turquoise", "skyblue", "lightskyblue", "cyan"};
		int colorIndex = color % 14;
		int ci,c1,c2,c3;	//print cp as a number, as a rational number, in nanoseconds
		int criticalPath = (int)(bit->signal->getCriticalPath() * 1000000000000);

		c3 = criticalPath % 10;
		criticalPath /= 10;
		c2 = criticalPath % 10;
		criticalPath /= 10;
		c1 = criticalPath % 10;
		criticalPath /= 10;
		ci = criticalPath % 10;

		fig << "<circle cx=\"" << turnaroundX - bit->weight*10 - 5 << "\""
			<< " cy=\"" << offsetY - index*10 - 5 << "\""
			<< " r=\"3\""
			<< " fill=\"" << colors[colorIndex] << "\" stroke=\"black\" stroke-width=\"0.5\""
			<< " onmousemove=\"ShowTooltip(evt, \'" << bit->name << ", " << bit->signal->getCycle() << " : " << ci << "." << c1 << c2 << c3 << " ns\')\""
			<< " onmouseout=\"HideTooltip(evt)\" />" << endl;
	}


	void BitheapPlotter::addECMAFunction()
	{
		fig << endl;
		fig << "<style>" << endl <<
				tab << ".caption{" << endl <<
				tab << tab << "font-size: 15px;" << endl <<
				tab << tab << "font-family: Georgia, serif;" << endl <<
				tab << "}" << endl <<
				tab << ".tooltip{" << endl <<
				tab << tab << "font-size: 12px;" << endl <<
				tab << "}" << endl <<
				tab << ".tooltip_bg{" << endl <<
				tab << tab << "fill: white;" << endl <<
				tab << tab << "stroke: black;" << endl <<
				tab << tab << "stroke-width: 1;" << endl <<
				tab << tab << "opacity: 0.85;" << endl <<
				tab << "}" << endl <<
				"</style>" << endl;
		fig << endl;
		fig << "<script type=\"text/ecmascript\"> <![CDATA[  " << endl <<
				tab << "function init(evt) {" << endl <<
				tab << tab << "if ( window.svgDocument == null ) {" << endl <<
				tab << tab << tab << "svgDocument = evt.target.ownerDocument;" << endl <<
				tab << tab << "}" << endl <<
				tab << tab << "tooltip = svgDocument.getElementById('tooltip');" << endl <<
				tab << tab << "tooltip_bg = svgDocument.getElementById('tooltip_bg');" << endl <<
				tab << "}" << endl <<
				tab << "function ShowTooltip(evt, mouseovertext) {" << endl <<
				tab << tab << "tooltip.setAttributeNS(null,\"x\",evt.clientX+10);" << endl <<
				tab << tab << "tooltip.setAttributeNS(null,\"y\",evt.clientY+30);" << endl <<
				tab << tab << "tooltip.firstChild.data = mouseovertext;" << endl <<
				tab << tab << "tooltip.setAttributeNS(null,\"visibility\",\"visible\");" << endl <<
				endl <<
				tab << tab << "length = tooltip.getComputedTextLength();" << endl <<
				tab << tab << "tooltip_bg.setAttributeNS(null,\"width\",length+8);" << endl <<
				tab << tab << "tooltip_bg.setAttributeNS(null,\"x\",evt.clientX+7);" << endl <<
				tab << tab << "tooltip_bg.setAttributeNS(null,\"y\",evt.clientY+18);" << endl <<
				tab << tab << "tooltip_bg.setAttributeNS(null,\"visibility\",\"visible\");" << endl <<
				tab << "}" << endl <<
				tab << "function HideTooltip(evt) {" << endl <<
				tab << tab << "tooltip.setAttributeNS(null,\"visibility\",\"hidden\");" << endl <<
				tab << tab << "tooltip_bg.setAttributeNS(null,\"visibility\",\"hidden\");" << endl <<
				tab << "}]]>" << endl <<
				"</script>" << endl;
		fig << endl;
	}

}

