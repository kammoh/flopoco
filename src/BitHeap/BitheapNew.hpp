/*
  A class to manage and compress heaps of weighted bits in FloPoCo

  This file is part of the FloPoCo project

  Author : Florent de Dinechin, Matei Istoan

  Initial software.
  Copyright © INSA de Lyon, INRIA, CNRS, UCBL,
  2012-2016.
  All rights reserved.

*/
#ifndef __BITHEAPNEW_HPP
#define __BITHEAPNEW_HPP


#include <vector>
#include <sstream>

#include "Operator.hpp"
#include "Signal.hpp"

#include "BitHeap/Bit.hpp"
#include "BitHeap/Compressor.hpp"

#include "IntAddSubCmp/IntAdder.hpp"

#include "Table.hpp"
#include "DualTable.hpp"


#define COMPRESSION_TYPE 0


namespace flopoco{



	class Plotter;

	class BitheapNew
	{


	public:

		//the compression strategy needs access to the bitheap
		friend class CompressionStrategy;

		/**
		 * @brief The constructor for an signed/unsigned integer bitheap
		 * @param op                the operator in which the bitheap is being built
		 * @param size              the size (maximum weight of the heap, or number of columns) of the bitheap
		 * @param isSigned          whether the bitheap is signed, or not (unsigned, by default)
		 * @param name              a description of the heap that will be integrated into its unique name (empty by default)
		 * @param compressionType	the type of compression applied to the bit heap:
		 *								0 = using only compressors (default),
		 *								1 = using only an adder tree,
		 *								2 = using a mix of the two, with an
		 *									addition tree at the end of the
		 *									compression
		 */
		BitheapNew(Operator* op, unsigned size, bool isSigned = false, string name = "", int compressionType = COMPRESSION_TYPE);

		/**
		 * @brief The constructor for an signed/unsigned fixed-point bitheap
		 * @param op                the operator in which the bitheap is being built
		 * @param msb               the msb of the bitheap (maximum weight at which a bit can be inserted)
		 * @param lsb               the lsb of the bitheap (minimum weight at which a bit can be inserted)
		 * @param isSigned          whether the bitheap is signed, or not (unsigned, by default)
		 * @param name              a description of the heap that will be integrated into its unique name (empty by default)
		 * @param compressionType	the type of compression applied to the bit heap:
		 *								0 = using only compressors (default),
		 *								1 = using only an adder tree,
		 *								2 = using a mix of the two, with an
		 *									addition tree at the end of the
		 *									compression
		 */
		BitheapNew(Operator* op, int msb, int lsb, bool isSigned = false, string name = "", int compressionType = COMPRESSION_TYPE);


		~BitheapNew();


		/**
		 * @brief add a bit to the bit heap.
		 * @param weight             the weight of the bit to be added. It should be non-negative,
		 *                           for an integer bitheap, or between msb and lsb for a fixed-point bitheap.
		 * @param rhsAssignment      the right-hand side VHDL code defining this bit.
		 * @param comment            a VHDL comment for this bit
		 */
		void addBit(int weight, string rhsAssignment, string comment = "");

		/**
		 * @brief add a bit to the bit heap.
		 * @param weight             the weight of the bit to be added. It should be non-negative,
		 *                           for an integer bitheap, or between msb and lsb for a fixed-point bitheap.
		 * @param signal             the signal out of which a bit is added to the bitheap.
		 * @param offset             the offset at which the bit is in the signal. for one-bit signals, this parameter is ignored.
		 * @param comment            a VHDL comment for this bit
		 */
		void addBit(int weight, Signal *signal, int offset = 0, string comment = "");

		/**
		 * @brief add a bit to the bit heap. convenience function for adding signals directly to the bitheap.
		 * @param signal             the signal out of which a bit is added to the bitheap.
		 * @param offset             the offset at which the bit is in the signal. for one-bit signals, this parameter is ignored.
		 *                           (by default 0, the lsb of the signal)
		 * @param weight             the weight of the bit to be added. It should be non-negative,
		 *                           for an integer bitheap, or between msb and lsb for a fixed-point bitheap.
		 *                           (by default 0, the lsb of the bitheap)
		 * @param comment            a VHDL comment for this bit
		 */
		void addBit(Signal *signal, int offset = 0, int weight = 0, string comment = "");

		/**
		 * @brief remove a bit from the bitheap.
		 * @param weight  the weight of the bit to be removed
		 * @param dir if dir==0 the bit will be removed from the beginning of the list
		 *            if dir==1 the bit will be removed from the end of the list
		 */
		void removeBit(int weight, int dir = 1);

		/**
		 * @brief remove a bit from the bitheap.
		 * @param weight  the weight of the bit to be removed
		 * @param bit the bit to be removed
		 */
		void removeBit(int weight, Bit* bit);

		/**
		 * @brief remove a bit from the bitheap.
		 * @param weight  the weight of the bit to be removed
		 * @param count the number of bits to remove
		 * @param dir if dir==0 the bit will be removed from the beginning of the list
		 *            if dir==1 the bit will be removed from the end of the list
		 */
		void removeBits(int weight, unsigned count = 1, int dir = 1);

		/**
		 * @brief remove a bit from the bitheap.
		 * @param msb the column up until which to remove the bits
		 * @param lsb the column from which to remove the bits
		 * @param count the number of bits to remove
		 * @param dir if dir==0 the bit will be removed from the beginning of the list
		 *            if dir==1 the bit will be removed from the end of the list
		 */
		void removeBits(int msb, int lsb, unsigned count = 1, int dir = 1);

		/**
		 * @brief remove the bits that have already been marked as compressed
		 */
		void removeCompressedBits();

		/**
		 * @brief remove a bit from the bitheap.
		 * @param weight the weight at which the bit is
		 * @param number the number of the bit in the respective column
		 * @param type how the bit should be marked
		 */
		void markBit(int weight, unsigned number, Bit::BitType type);

		/**
		 * @brief remove a bit from the bitheap.
		 * @param bit the bit to mark
		 * @param type how the bit should be marked
		 */
		void markBit(Bit* bit, Bit::BitType type);

		/**
		 * @brief remove a bit from the bitheap.
		 * @param msb the column up until which to mark the bits
		 * @param lsb the column up until which to mark the bits
		 * @param number the number of bits to mark in each column
		 *               (by default=-1, i.e. mark all bits)
		 * @param type how the bits should be marked
		 */
		void markBits(int msb, int lsb, Bit::BitType type, int number = -1);

		/**
		 * @brief remove a bit from the bitheap.
		 * @param bits the bits to mark
		 * @param type how the bits should be marked
		 */
		void markBits(vector<Bit*> bits, Bit::BitType type);

		/**
		 * @brief add a constant 1 to the bit heap.
		 * All the constant bits are added to the constantBits mpz,
		 * so we don't generate hardware to compress constants.
		 * @param weight            the weight of the 1 bit to be added
		 *                          (by default 0)
		 */
		void addConstantOneBit(int weight = 0);

		/**
		 * @brief subtract a constant 1 from the bitheap.
		 * @param weight            the weight of the 1 to be subtracted
		 *                          (by default 0)
		 */
		void subConstantOneBit(int weight);

		/**
		 * @brief add a constant to the bitheap. It will be added to the constantBits mpz,
		 * so we don't generate hardware to compress constants.
		 * @param constant          the value to be added
		 * @param weight            the weight of the LSB of constant
		 *                          (by default 0)
		 */
		void addConstant(mpz_class constant, int weight = 0);

		/**
		 * @brief add a constant to the bitheap. It will be added to the constantBits mpz,
		 * so we don't generate hardware to compress constants.
		 * @param constant          the value to be added
		 * @param msb               the msb of the format of the constant
		 * @param lsb               the lsb of the format of the constant
		 * @param weight            the weight of the LSB of constant
		 *                          (by default 0)
		 */
		void addConstant(mpz_class constant, int msb, int lsb, int weight = 0);

		/**
		 * @brief subtract a constant to the bitheap. It will be subtracted from the constantBits mpz,
		 * so we don't generate hardware to compress constants.
		 * @param constant          the value to be subtracted
		 * @param weight            the weight of the LSB of constant
		 *                          (by default 0)
		 */
		void subConstant(mpz_class constant, int weight = 0);

		/**
		 * @brief subtract a constant to the bitheap. It will be subtracted to the constantBits mpz,
		 * so we don't generate hardware to compress constants.
		 * @param constant          the value to be subtracted
		 * @param msb               the msb of the format of the constant
		 * @param lsb               the lsb of the format of the constant
		 * @param weight            the weight of the LSB of constant
		 *                          (by default 0)
		 */
		void subConstant(mpz_class constant, int msb, int lsb, int weight = 0);

		/**
		 * @brief add to the bitheap a signal, considered as an unsigned integer
		 * @param signalName        the name of the signal being added
		 * @param size              the size of the signal being added
		 *                          (by default 1)
		 * @param weight            the weight the signal is added to the bitheap
		 *                          (by default 0)
		 */
		void addUnsignedBitVector(string signalName, unsigned size = 1, int weight = 0);

		/**
		 * @brief add to the bitheap a signal, considered as an unsigned integer
		 * @param signalName        the name of the signal being added
		 * @param msb               the msb from which to add bits of the signal
		 * @param lsb               the lsb up until which to add bits of the signal
		 * @param weight            the weight the signal is added to the bitheap
		 *                          (by default 0)
		 */
		void addUnsignedBitVector(string signalName, int msb, int lsb, int weight = 0);

		/**
		 * @brief add to the bitheap a signal, considered as an unsigned integer
		 * @param signal            the signal being added
		 * @param weight            the weight the signal is added to the bitheap
		 *                          (by default 0)
		 */
		void addUnsignedBitVector(Signal* signal, int weight = 0);

		/**
		 * @brief add to the bitheap a signal, considered as an unsigned integer
		 * @param signal            the signal being added
		 * @param msb               the msb from which to add bits of the signal
		 * @param lsb               the lsb up until which to add bits of the signal
		 * @param weight            the weight the signal is added to the bitheap
		 *                          (by default 0)
		 */
		void addUnsignedBitVector(Signal* signal, int msb, int lsb, int weight = 0);

		/**
		 * @brief subtract from the bitheap a signal, considered as an unsigned integer
		 * @param signalName        the name of the signal being added
		 * @param size              the size of the signal being subtracted
		 *                          (by default 1)
		 * @param weight            the weight the signal is added to the bitheap
		 *                          (by default 0)
		 */
		void subtractUnsignedBitVector(string signalName, unsigned size = 1, int weight = 0);

		/**
		 * @brief subtract from the bitheap a signal, considered as an unsigned integer
		 * @param signalName        the name of the signal being added
		 * @param msb               the msb from which to consider the bits of the signal
		 * @param lsb               the lsb up until which to consider bits of the signal
		 * @param weight            the weight the signal is added to the bitheap
		 *                          (by default 0)
		 */
		void subtractUnsignedBitVector(string signalName, int msb, int lsb, int weight = 0);

		/**
		 * @brief subtract from the bitheap a signal, considered as an unsigned integer
		 * @param signal            the signal being added
		 * @param weight            the weight the signal is added to the bitheap
		 *                          (by default 0)
		 */
		void subtractUnsignedBitVector(Signal* signal, int weight = 0);

		/**
		 * @brief subtract from the bitheap a signal, considered as an unsigned integer
		 * @param signal            the signal being added
		 * @param msb               the msb from which to consider the bits of the signal
		 * @param lsb               the lsb up until which to consider bits of the signal
		 * @param weight            the weight the signal is added to the bitheap
		 *                          (by default 0)
		 */
		void subtractUnsignedBitVector(Signal* signal, int msb, int lsb, int weight = 0);

		/**
		 * @brief add to the bitheap a signal, considered as a signed integer. the size includes the sign bit.
		 * @param signalName        the name of the signal being added
		 * @param size              the size of the signal being added
		 *                          (by default 1)
		 * @param weight            the weight the signal is added to the bitheap
		 *                          (by default 0)
		 */
		void addSignedBitVector(string signalName, unsigned size = 1, int weight = 0);

		/**
		 * @brief add to the bitheap a signal, considered as an signed integer
		 * @param signalName        the name of the signal being added
		 * @param msb               the msb from which to add bits of the signal
		 * @param lsb               the lsb up until which to add bits of the signal
		 * @param weight            the weight the signal is added to the bitheap
		 *                          (by default 0)
		 */
		void addSignedBitVector(string signalName, int msb, int lsb, int weight = 0);

		/**
		 * @brief add to the bitheap a signal, considered as an signed integer
		 * @param signal            the signal being added
		 * @param weight            the weight the signal is added to the bitheap
		 *                          (by default 0)
		 */
		void addSignedBitVector(Signal* signal, int weight = 0);

		/**
		 * @brief add to the bitheap a signal, considered as a signed integer. the size includes the sign bit.
		 * @param signal            the signal being added
		 * @param msb               the msb from which to add bits of the signal
		 * @param lsb               the lsb up until which to add bits of the signal
		 * @param weight            the weight the signal is added to the bitheap
		 *                          (by default 0)
		 */
		void addSignedBitVector(Signal* signal, int msb, int lsb, int weight = 0);

		/**
		 * @brief subtract from the bitheap a signal, considered as a signed integer. the size includes the sign bit.
		 * @param signalName        the name of the signal being subtracted
		 * @param size              the size of the signal being subtracted
		 *                          (by default 1)
		 * @param weight            the weight the signal is subtracted from the bitheap
		 *                          (by default 0)
		 */
		void subtractSignedBitVector(string signalName, unsigned size = 1, int weight = 0);

		/**
		 * @brief subtract from the bitheap a signal, considered as a signed integer. the size includes the sign bit.
		 * @param signalName        the name of the signal being added
		 * @param msb               the msb from which to add bits of the signal
		 * @param lsb               the lsb up until which to add bits of the signal
		 * @param weight            the weight the signal is added to the bitheap
		 *                          (by default 0)
		 */
		void subtractSignedBitVector(string signalName, int msb, int lsb, int weight = 0);

		/**
		 * @brief subtract from the bitheap a signal, considered as a signed integer.
		 * @param signal            the signal being subtracted
		 * @param weight            the weight the signal is subtracted from the bitheap
		 *                          (by default 0)
		 */
		void subtractSignedBitVector(Signal* signal, int weight = 0);

		/**
		 * @brief subtract from the bitheap a signal, considered as a signed integer.
		 * @param signal            the signal being subtracted
		 * @param msb               the msb from which to subtract the bits of the signal
		 * @param lsb               the lsb up until which to subtract the bits of the signal
		 * @param weight            the weight the signal is subtracted from the bitheap
		 *                          (by default 0)
		 */
		void subtractSignedBitVector(Signal* signal, int msb, int lsb, int weight = 0);


		/**
		 * @brief resize an integer bitheap to the new given size
		 * @param size              the new size of the bitheap
		 */
		void resizeBitheap(unsigned size);

		/**
		 * @brief resize a fixed-point bitheap to the new given format
		 * @param msb               the new msb of the bitheap
		 * @param lsb               the new lsb of the bitheap
		 */
		void resizeBitheap(int msb, int lsb);


		/**
		 * @brief merge the bitheap given as argument to this one
		 * NOTE: only bitheaps belonging to the same parent operator can be merged together
		 * @param bitheap           the bitheap to merge into this one
		 */
		void mergeBitheap(BitheapNew* bitheap);


		/**
		 * @brief generate the VHDL for the bit heap.
		 * Uses the compression strategy set by the user, or the one created by default.
		 * To be called last by operators using BitHeap.
		 */
		void startCompression();

		/**
		 * @brief return the name of the compressed sum
		 */
		string getSumName();

		/**
		 * @brief returns the name of the compressed sum, with the range (msb, lsb)
		 * @param msb the msb for the range
		 * @param lsb the lsb for the range
		 */
		string getSumName(int msb, int lsb);


		/**
		 * @brief return the maximum height of the bitheap
		 */
		unsigned getMaxHeight();

		/**
		 * @brief return the height a column (bits not yet compressed)
		 * @param weight the column
		 */
		unsigned getColumnHeight(int weight);

		/**
		 * @brief return the delay of a compression stage
		 * (there can be several compression staged in a cycle)
		 */
		double getCompressorDelay();

		/**
		 * @brief return the plotter object
		 */
		Plotter* getPlotter();

		/**
		 * @brief return the parent operator of this bitheap
		 */
		Operator* getOp();

		/**
		 * @brief return the UID of the bit heap
		 */
		int getGUid();

		/**
		 * @brief return the unique name of the bit heap
		 */
		string getName();

		/**
		 * @brief set if the bitheap is signed
		 */
		void setIsSigned(bool newIsSigned);

		/**
		 * @brief is the bitheap signed
		 */
		bool getIsSigned();

		/**
		 * @brief return a fresh uid for a bit of weight w
		 * @param weight            the weight of the bit
		 */
		int newBitUid(unsigned weight);

		/**
		 * @brief print the informations regarding a column
		 * @param weight the weight of the column
		 */
		void printColumnInfo(int weight);

		/**
		 * @brief print the informations regarding the whole bitheap
		 */
		void printBitHeapStatus();

	protected:

		/**
		 * @brief factored code for initializing a bitheap inside the constructor
		 */
		void initialize();

		/**
		 * @brief insert a bit into a column of bits, so that the column is ordered
		 * in increasing lexicographical order on (cycle, critical path)
		 * @param bit the bit to insert
		 * @param bitsColumn the column of bits into which to insert the bit
		 */
		void insertBitInColumn(Bit* bit, vector<Bit*> bitsColumn);

		void initializeDrawing();

		void closeDrawing(int offsetY);

		void drawConfiguration(int offsetY);

		void drawBit(int cnt, int w, int turnaroundX, int offsetY, int c);

		void concatenateLSBColumns();

	public:
		int msb;                                    /**< The maximum weight a bit can have inside the bitheap */
		int lsb;                                    /**< The minimum weight a bit can have inside the bitheap */
		unsigned size;                              /**< The size of the bitheap */
		int height;                                 /**< The current maximum height of any column of the bitheap */
		string name;                                /**< The name of the bitheap */

		bool isSigned;                              /**< Is the bitheap signed, or unsigned */
		bool isFixedPoint;                          /**< Is the bitheap fixed-point, or integer */

	private:
		Operator* op;

		vector<vector<Bit*> > bits;                 /**< The bits currently contained in the bitheap, ordered into columns by weight in the bitheap,
		                                                 and by arrival time of the bits, i.e. lexicographic order on (cycle, cp), inside each column. */
		vector<vector<Bit*> > history;              /**< All the bits that have been added (and possibly removed at some point) to the bitheap. */
		mpz_class constantBits;						/**< The sum of all the constant bits that need to be added to the bit heap
				                                                 (constants added to the bitheap, for rounding, two's complement etc) */

		ostringstream vhdlCode;                     /**< The VHDL code buffer */

		CompressionStrategy* compressionStrategy;   /**< The compression strategy used to compress the bitheap */
		bool isCompressed;                          /**< Has the bitheap already been compressed, or not */
		int compressionType;						/**< The type of compression performed:
		                                                 0=using only compressors, 1=using adder trees, 2=mixed, using compressors and adders*/

		vector<int> bitUID;                         /**< A unique identifier for the bits in this bitheap (for each column) */
		int guid;                                   /**< The global UID for this bit heap, useful in operators managing several bit heaps */

		ofstream fileFig;                           /**< The file stream for the bitheap and bitheap compression figures */
		ostringstream fig;                          /**< The stream for the bitheap and bitheap compression figures */
		bool drawCycleLine;                         /**< Draw lines between cycle, or not */
		int drawCycleNumber;                        /**< Draw the cycle number, or not */
		Plotter* plotter;                           /**< The plotter used to draw the bitheap and bitheap compression process */


		// For error reporting to work
		string srcFileName;
		string uniqueName_;
	};


}
#endif
