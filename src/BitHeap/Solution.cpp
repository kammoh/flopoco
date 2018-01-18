
#include "Solution.hpp"


using namespace std;

namespace flopoco{


	Solution::Solution()
	{
		status = BitheapSolutionStatus::EMPTY;
	}

	void Solution::addCompressor(unsigned int stage, unsigned int column, BasicCompressor* compressor, int middleLength){

		//make sure that the position (stage and column) exist
		if(stage >= comps.size()){
			comps.resize(stage + 1);
		}
		if(column >= comps[stage].size()){
			comps[stage].resize(column + 1);
		}

		for(unsigned int i = 0; i < comps[stage][column].size(); i++){
			if(comps[stage][column][i].compressor == compressor){
				if(comps[stage][column][i].middleLength == middleLength){
					comps[stage][column][i].amount += 1;
					return;
				}
			}
		}

		//new Entry, because it does not exists
		Entry tempEntry;
		tempEntry.stage = stage;
		tempEntry.column = column;
		tempEntry.compressor = compressor;
		tempEntry.amount = 1;
		tempEntry.middleLength = middleLength;
		comps[stage][column].push_back(tempEntry);

	}



	void Solution::cleanUpStagesAndColumns(){

		//first find the number of stages with compressors in them
		int fullStages; //number of stages with compressors
		bool foundFullStage = false;
		for(fullStages = comps.size(); fullStages > 0; fullStages--){
			unsigned int tempS = (unsigned) (fullStages - 1);
			for(unsigned int c = 0; c < comps[tempS].size(); c++){
				if(comps[tempS][c].size() > 0){
					foundFullStage = true;
					break;
				}
			}
			if(foundFullStage){
				break;
			}
		}
		//delete the rest
		if(fullStages < comps.size()){
			comps.resize(fullStages);
		}

		//go through every stage and delete empty columns if there is no bigger
		//column with compressors in it
		for(unsigned int s = 0; s < comps.size(); s++){
			int maxColumn = (unsigned) (comps[s].size() - 1);
			while(maxColumn >= 0){
				if(comps[s][maxColumn].size() > 0){
					break;
				}
				else{
					maxColumn--;
				}
			}
			if(maxColumn + 1 != comps[s].size()){
				comps[s].resize(maxColumn + 1);
			}
		}
	}

	vector<pair<BasicCompressor*, unsigned int> > Solution::getCompressorsAtPosition(unsigned int stage, unsigned int column){

		if(comps.size() > stage && comps[stage].size() > column){
			vector<pair<BasicCompressor*, unsigned int> > returnList;

			for(unsigned int i = 0; i < comps[stage][column].size(); i++){
				for(unsigned int j = 0; j < comps[stage][column][i].amount; j++){
					pair<BasicCompressor*, unsigned int> tempPair;
					tempPair.first =  comps[stage][column][i].compressor;
					tempPair.second = comps[stage][column][i].middleLength;
					returnList.push_back(tempPair);
				}
			}
			return returnList;
		}
		else{
			vector<pair<BasicCompressor*, unsigned int> > emptyList;
			return emptyList;
		}
	}

	int Solution::getNumberOfStages(){
		return comps.size();
	}
	int Solution::getNumberOfColumnsAtStage(unsigned int stage){
		if(comps.size() >= stage){
			return -1;
		}
		return comps[stage].size();
	}

	vector<unsigned int> Solution::getEmptyInputsByStage(unsigned int stage){
		if(stage > emptyInputs.size()){
			vector<unsigned int> emptyVector;
			return emptyVector;
		}
		else{
			return emptyInputs[stage];
		}
	}

	void Solution::setEmptyInputsByRemainingBits(unsigned int stage, vector<int> remainingBits){
		if(emptyInputs.size() <= stage){
			emptyInputs.resize(stage + 1);
		}
		vector<unsigned int> tempVector; //contains the new values
		for(unsigned int c = 0; c < remainingBits.size(); c++){
			if(remainingBits[c] >= 0){
				tempVector.push_back(0);
			}
			else if(remainingBits[c] < 0){
				int amount = remainingBits[c] * (-1);
				tempVector.push_back((unsigned int) amount);
			}
		}
		emptyInputs[stage] = tempVector;
	}

	BitheapSolutionStatus Solution::getSolutionStatus(){
		return status;
	}

	void Solution::setSolutionStatus(BitheapSolutionStatus stat){
		status = stat;
	}

	void Solution::markSolutionAsComplete(){
		if(status == BitheapSolutionStatus::MIXED_PARTIAL){
			status = BitheapSolutionStatus::MIXED_COMPLETE;
		}
		else if(status == BitheapSolutionStatus::OPTIMAL_PARTIAL){
			status = BitheapSolutionStatus::OPTIMAL_COMPLETE;
		}
		else if(status == BitheapSolutionStatus::HEURISTIC_PARTIAL){
			status = BitheapSolutionStatus::HEURISTIC_COMPLETE;
		}
		else if(status == BitheapSolutionStatus::EMPTY){
			cout << "tried to mark a Solution as complete, although it is empty" << endl;
		}
		//else: already complete/compression not needed.
	}





}
