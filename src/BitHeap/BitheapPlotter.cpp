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


	void BitheapPlotter::takeSnapshot()
	{
		if(!(cp==0.0 && cycle==0))
		{
			if(compress)
			{
				unsigned size=snapshots.size();
				Snapshot* s = new Snapshot(bh->bits, bh->getMinWeight(), bh->getMaxWeight(), bh->getMaxHeight(), compress, cycle, cp);
				bool proceed=true;

				if (size==0)
				{
					snapshots.push_back(s);
				}else
				{
					vector<Snapshot*>::iterator it = snapshots.begin();

					it++;
					while(proceed)
					{
						if (it==snapshots.end() || (*s < **it))
						{ // test in this order to avoid segfault!
							snapshots.insert(it, s);
							proceed=false;
						}else
						{
							it++;
						}
					}
				}
			}
		}
	}



	void BitheapPlotter::plotBitHeap()
	{
		drawInitialBitheap();
		drawBitheapCompression();
	}


	void BitheapPlotter::drawInitialBitheap()
	{
		initializePlotter(true);

		int offsetY = 0;
		int turnaroundX = snapshots[0]->bits.size() * 10 + 80;


		offsetY += 20 + snapshots[0]->maxHeight * 10;

		drawInitialConfiguration(snapshots[0], offsetY, turnaroundX);

		closePlotter(offsetY, turnaroundX)
	}



	void BitheapPlotter::drawBitheapCompression()
	{
		initializePlotter(false);

		int offsetY = 0;
		int turnaroundX = snapshots[snapshots.size()-1]->maxWeight * 10 + 100;

		//lastStage=snapshots[0]->stage;

		bool timeCondition;


		for(unsigned i=0; i< snapshots.size(); i++)
		{
			if(snapshots[i]->didCompress)
			{
				timeCondition = true;
				if (i > snapshots.size()-3)
					timeCondition = false;

				offsetY += 15 + snapshots[i]->maxHeight * 10;
				drawConfiguration(snapshots[i]->bits, i,snapshots[i]->cycle, snapshots[i]->cp,
						snapshots[i]->minWeight, offsetY, turnaroundX, timeCondition);

				if (i!=snapshots.size()-1)
				{
					int j=i+1;

					while(snapshots[j]->didCompress==false)
						j++;

					int c = snapshots[j]->cycle - snapshots[i]->cycle;

					fig << "<line x1=\"" << turnaroundX + 150 << "\" y1=\""
						<< offsetY + 10 << "\" x2=\"" << 50
						<< "\" y2=\"" << offsetY + 10 << "\" style=\"stroke:lightsteelblue;stroke-width:1\" />" << endl;

					while(c>0)
					{
						offsetY += 10;
						fig << "<line x1=\"" << turnaroundX + 150 << "\" y1=\""
							<< offsetY  << "\" x2=\"" << 50
							<< "\" y2=\"" << offsetY  << "\" style=\"stroke:midnightblue;stroke-width:2\" />" << endl;

						c--;
					}
				}
			}
		}

		closePlotter(offsetY, turnaroundX);
	}


	void BitheapPlotter::initializePlotter(bool isInitial)
	{
		ostringstream figureFileName;

		if(isInitial)
			figureFileName << "BitHeap_initial_" << bh->getName()  << ".svg";
		else
			figureFileName << "BitHeap_compression_" << bh->getName()  << ".svg";

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
		int cnt = 0;
		vector<WeightedBit*> orderedBits;

		fig << "<line x1=\"" << turnaroundX + 150 << "\" y1=\""
				<< offsetY +10 << "\" x2=\"" << turnaroundX - bits.size()*10 - 50
				<< "\" y2=\"" << offsetY +10 << "\" style=\"stroke:lightsteelblue;stroke-width:1\" />" << endl;

		for(unsigned i=minWeight; i<bits.size(); i++)
		{
			if(bits[i].size()>0)
			{
				for(list<WeightedBit*>::iterator bit = bits[i].begin(); bit!=bits[i].end(); ++bit)
				{
					if(orderedBits.size()==0)
					{
						orderedBits.push_back((*bit));
					}
					bool proceed=true;
					vector<WeightedBit*>::iterator iterBit = orderedBits.begin();

					while(proceed)
					{
						if (iterBit==orderedBits.end())
						{
							orderedBits.push_back((*bit));
							proceed=false;
						}
						else
						{
							if( (**bit) == (**iterBit))
							{
								proceed=false;
							}
							else
							{
								if( (**bit) < (**iterBit))
								{
									orderedBits.insert(iterBit, *bit);
									proceed=false;
								}
								else
								{
									iterBit++;
								}
							}

						}
					}
				}
			}
		}

		for(unsigned i=0; i<bits.size(); i++)
		{
			if(bits[i].size()>0)
			{
				cnt = 0;
				for(list<WeightedBit*>::iterator it = bits[i].begin(); it!=bits[i].end(); ++it)
				{
					color=0;

					for (unsigned j=0; j<orderedBits.size(); j++)
					{
						if ( (**it) == (*orderedBits[j]) )
							color=j;
					}

					int cy = (*it)->getCycle();
					double cp = (*it)->getCriticalPath(cy)*100000000000;

					drawBit(cnt, i, turnaroundX, offsetY, color, cy, cp, (*it)->getName());
					cnt++;
				}
			}
		}
	}


	void BitheapPlotter::drawConfiguration(Snapshot *snapshot, int offsetY, int turnaroundX)
	{
		int cnt = 0;
		int ci,c1,c2,c3;							//print cp as a number as a rational number, in nanoseconds
		int cpint = criticalPath * 1000000000000;

		c3 = cpint % 10;
		cpint = cpint / 10;
		c2 = cpint % 10;
		cpint = cpint / 10;
		c1 = cpint % 10;
		cpint = cpint / 10;
		ci = cpint % 10;

		if(nr == 0)
		{
			fig << "<text x=\"" << turnaroundX + 50 << "\" y=\"" << offsetY + 3
					<< "\" fill=\"midnightblue\">" << "before first compression" << "</text>" << endl;
		}else if(nr == snapshots.size()-1)
		{
			fig << "<text x=\"" << turnaroundX + 50 << "\" y=\"" << offsetY + 3
					<< "\" fill=\"midnightblue\">" << "before final addition" << "</text>" << endl;
		}else if(nr == snapshots.size()-2)
		{
			fig << "<text x=\"" << turnaroundX + 50 << "\" y=\"" << offsetY + 3
					<< "\" fill=\"midnightblue\">" << "before 3-bit height additions" << "</text>" << endl;
		}else
		{
			fig << "<text x=\"" << turnaroundX + 50 << "\" y=\"" << offsetY + 3
					<< "\" fill=\"midnightblue\">" << cycle << "</text>" << endl
					<< "<text x=\"" << turnaroundX + 80 << "\" y=\"" << offsetY + 3
					<< "\" fill=\"midnightblue\">" << ci << "." << c1 << c2 << c3 << " ns"  << "</text>" << endl;
		}

		turnaroundX -= minWeight*10;

		for(unsigned i=0; i<bits.size(); i++)
		{
			if(bits[i].size()>0)
			{
				cnt = 0;
				for(list<WeightedBit*>::iterator it = bits[i].begin(); it!=bits[i].end(); ++it)
				{
					int cy = (*it)->getCycle();
					double cp = (*it)->getCriticalPath(cy);

					if (timeCondition)
					{
						if ((cy<cycle) || ((cy==cycle) && (cp<=criticalPath)))
						{
							cp = cp * 1000000000000;  //picoseconds
							drawBit(cnt, i, turnaroundX, offsetY, (*it)->getType(), cy, cp, (*it)->getName());
							cnt++;
						}
					}
					else
					{
						drawBit(cnt, i, turnaroundX, offsetY, (*it)->getType(), cy, cp, (*it)->getName());
						cnt++;
					}
				}
			}
		}
	}


	void BitheapPlotter::drawBit(int cnt, int turnaroundX, int offsetY, int color, Bit *bit)
	{
		const std::string colors[] = { "#97bf04","#0f1af2",
			"orange", "#f5515c",  "lightgreen", "fuchsia", "indianred"};
		int index = color % 7;
		int ci,c1,c2,c3;	//print cp as a number, as a rational number, in nanoseconds

		c3 = cp % 10;
		cp = cp / 10;
		c2 = cp % 10;
		cp = cp / 10;
		c1 = cp % 10;
		cp = cp / 10;
		ci = cp % 10;

		fig << "<circle cx=\"" << turnaroundX - w*10 - 5 << "\""
			<< " cy=\"" << offsetY - cnt*10 - 5 << "\""
			<< " r=\"3\""
			<< " fill=\"" << colors[index] << "\" stroke=\"black\" stroke-width=\"0.5\""
			<< " onmousemove=\"ShowTooltip(evt, \'" << name << ", " << cycle << " : " << ci << "." << c1 << c2 << c3 << " ns\')\""
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

