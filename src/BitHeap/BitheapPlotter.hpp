/*
   A class used for plotting various drawings in SVG format

   This file is part of the FloPoCo project

Author : Florent de Dinechin, Kinga Illyes, Bogdan Popa, Matei Istoan

Initial software.
Copyright © INSA de Lyon, ENS-Lyon, INRIA, CNRS, UCBL
2016.
All rights reserved.

*/

#ifndef BITHEAPPLOTTER_HPP
#define BITHEAPPLOTTER_HPP

#include <vector>
#include <iostream>
#include <fstream>

#include "BitheapNew.hpp"
#include "Bit.hpp"


namespace flopoco
{

	class BitheapPlotter
	{

	public:

		class Snapshot
		{
			public:

				Snapshot(vector<vector<Bit*> > bits, int maxHeight, int cycle, double criticalPath);


				/**
				 * ordering by availability time
				 */
				friend bool operator< (Snapshot& b1, Snapshot& b2);
				/**
				 * ordering by availability time
				 */
				friend bool operator<= (Snapshot& b1, Snapshot& b2);
				/**
				 * ordering by availability time
				 */
				friend bool operator== (Snapshot& b1, Snapshot& b2);
				/**
				 * ordering by availability time
				 */
				friend bool operator!= (Snapshot& b1, Snapshot& b2);
				/**
				 * ordering by availability time
				 */
				friend bool operator> (Snapshot& b1, Snapshot& b2);
				/**
				 * ordering by availability time
				 */
				friend bool operator>= (Snapshot& b1, Snapshot& b2);


				vector<vector<Bit*> > bits;                          /*< the bits inside a bitheap at the given moment in time wen the snapshot is taken */
				int maxHeight;                                       /*< the height of the bitheap at the given moment in time wen the snapshot is taken */
				int cycle;                                           /*< the cycle of the soonest bit at the given moment in time wen the snapshot is taken */
				double criticalPath;                                 /*< the critical path of the soonest bit at the given moment in time wen the snapshot is taken */

				string srcFileName;
		};

		/**
		 * @brief constructor
		 */
		BitheapPlotter(BitheapNew* bitheap);

		/**
		 * @brief destructor
		 */
		~BitheapPlotter();

		/**
		 * @brief take a snapshot of the bitheap's current state
		 */
		void takeSnapshot(Bit *soonestBit);

		/**
		 * @brief plot all the bitheap's stages
		 * 			including the initial content and all the compression stages
		 */
		void plotBitHeap();


	private:

		/**
		 * @brief draws the initial bitheap
		 */
		void drawInitialBitheap();

		/**
		 * @brief draws the stages of the bitheap's compression
		 */
		void drawBitheapCompression();

		/**
		 * @brief initialize the plotter
		 */
		void initializePlotter(bool isInitial);

		/**
		 * @brief add the closing instructions to the plotter
		 */
		void closePlotter(int offsetY, int turnaroundX);

		/**
		 * @brief draw the initial content of the bitheap, before the start of the compression
		 */
		void drawInitialConfiguration(Snapshot *snapshot, int offsetY, int turnaroundX);

		/**
		 * @brief draw the state of the bitheap
		 */
		void drawConfiguration(int index, Snapshot *snapshot, int offsetY, int turnaroundX);

		/**
		 * @brief draws a single bit
		 */
		void drawBit(int index, int turnaroundX, int offsetY, int color, Bit *bit);

		/*
		 * @brief write the SVG initialization functions
		 */
		void addECMAFunction();

		ofstream fig;
		ofstream fig2;

		vector<Snapshot*> snapshots;

		string srcFileName;

		BitheapNew* bitheap;
	};
}

#endif
