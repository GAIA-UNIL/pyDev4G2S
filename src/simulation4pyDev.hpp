/*
 * G2S
 * Copyright (C) 2018, Mathieu Gravey (gravey.mathieu@gmail.com) and UNIL (University of Lausanne)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SIMULATION_4_PY_DEV_HPP
#define SIMULATION_4_PY_DEV_HPP

#include "computeDeviceModule.hpp"
#include "samplingModule.hpp"
#include <thread>


void simulation4pyDev(FILE *logFile,g2s::DataImage &di, std::vector<g2s::DataImage> &TIs, pyDevSamplingModule &samplingModule,
 std::vector<std::vector<int> > &pathPosition, unsigned* solvingPath, unsigned numberOfPointToSimulate, g2s::DataImage *ii, float* seedAray, unsigned* importDataIndex, std::vector<unsigned> numberNeighbor,
  unsigned nbThreads=1, bool fullStationary=false, bool circularSim=false){

	int displayRatio=std::max(numberOfPointToSimulate/100,1u);
	unsigned* posterioryPath=(unsigned*)malloc( sizeof(unsigned) * di.dataSize()/di._nbVariable);
	memset(posterioryPath,255,sizeof(unsigned) * di.dataSize()/di._nbVariable);
	for (unsigned int i = 0; i < di.dataSize()/di._nbVariable; ++i)
	{
		bool withNan=false;
		for (unsigned int j = 0; j < di._nbVariable; ++j)
		{
			withNan|=std::isnan(di._data[i*di._nbVariable+j]);
		}
		if(!withNan)
			posterioryPath[i]=0;
	}
	for (unsigned int i = 0; i < numberOfPointToSimulate; ++i)
	{
		posterioryPath[solvingPath[i]]=i;
	}
	
	unsigned numberOfVariable=di._nbVariable;

	#pragma omp parallel for num_threads(nbThreads) schedule(dynamic,1) default(none) firstprivate(displayRatio, circularSim, fullStationary, numberOfVariable,\
		numberOfPointToSimulate,posterioryPath, solvingPath, seedAray, numberNeighbor, importDataIndex, logFile, ii) shared( pathPosition, di, samplingModule, TIs)
	for (unsigned int indexPath = 0; indexPath < numberOfPointToSimulate; ++indexPath){
		

		unsigned moduleID=0;
		#if _OPENMP
			moduleID=omp_get_thread_num();
		#endif
		unsigned currentCell=solvingPath[indexPath];
		float localSeed=seedAray[indexPath];

		bool withDataInCenter=false;
		bool withOnlyData=true;

		for (unsigned int i = 0; i < di._nbVariable; ++i)
		{
			withDataInCenter|=!std::isnan(di._data[currentCell*di._nbVariable+i]);
			withOnlyData&=!std::isnan(di._data[currentCell*di._nbVariable+i]);
		}

		if(withOnlyData) continue;


		std::vector<unsigned> numberOfNeighborsProVariable(di._nbVariable);
		std::vector<std::vector<int> > neighborArrayVector;
		std::vector<std::vector<float> > neighborValueArrayVector;
		{
			unsigned positionSearch=0;
			while((numberNeighbor.size()>1||(neighborArrayVector.size()<numberNeighbor[0]))&&(positionSearch<pathPosition.size())){
				unsigned dataIndex;
				std::vector<int> vectorInDi=pathPosition[positionSearch];
				vectorInDi.resize(di._dims.size(),0);
				if(di.indexWithDelta(dataIndex, currentCell, vectorInDi) || circularSim)
				{
					//add for
					if(posterioryPath[dataIndex]<=indexPath){
						unsigned numberOfNaN=0;
						float val;
						while(true) {
							numberOfNaN=0;
							for (unsigned int i = 0; i < di._nbVariable; ++i)
							{
								#pragma omp atomic read
								val=di._data[dataIndex*di._nbVariable+i];
								numberOfNaN+=(numberOfNeighborsProVariable[i]<numberNeighbor[i%numberNeighbor.size()]) && std::isnan(val);
							}
							if((numberOfNaN==0)||(posterioryPath[dataIndex]==indexPath))break;
							std::this_thread::sleep_for(std::chrono::microseconds(250));
						}

						std::vector<float> data(di._nbVariable);
						unsigned cpt=0;
						for (unsigned int i = 0; i < di._nbVariable; ++i)
						{
							if((numberOfNeighborsProVariable[i]<numberNeighbor[i%numberNeighbor.size()]))
							{
								#pragma omp atomic read
								val=di._data[dataIndex*di._nbVariable+i];
								data[i]=val;
								cpt++;
								numberOfNeighborsProVariable[i]++;
							}else{
								data[i]=std::nanf("0");
							}
						}
						neighborValueArrayVector.push_back(data);
						neighborArrayVector.push_back(pathPosition[positionSearch]);
						if(cpt==0) break;
					}
				}
				positionSearch++;
			}
		}
		// conversion from one variable to many
		for (size_t j = 0; j < neighborValueArrayVector.size(); ++j)
		{
			std::vector<float> data(numberOfVariable);
			unsigned id=0;
			unsigned idCategorie=0;
			for (unsigned int i = 0; i < di._nbVariable; ++i)
			{
				data[id]=neighborValueArrayVector[j][i];
				id++;
			}
			neighborValueArrayVector[j]=data;
		}

		SamplingModule::simValues importIndex;
		importIndex.index=INT_MAX;
		// importIndex.values=nullptr;

		importIndex=samplingModule.sampleValue(neighborArrayVector,neighborValueArrayVector,localSeed,indexPath,moduleID,fullStationary,0,(ii!=nullptr ? int(ii->_data[currentCell]):-1));
		// import data
		//memcpy(di._data+currentCell*di._nbVariable,TIs[importIndex.TI]._data+importIndex.index*TIs[importIndex.TI]._nbVariable,TIs[importIndex.TI]._nbVariable*sizeof(float));
		importDataIndex[currentCell]=importIndex.index;
		//fprintf(stderr, "write %d\n", importDataIndex[currentCell]);
		for (unsigned int j = 0; j < di._types.size(); ++j)
		{
			if(std::isnan(di._data[currentCell*di._nbVariable+j])){
				#pragma omp atomic write
				di._data[currentCell*di._nbVariable+j]=importIndex.values[j];
			}
		}
		if(indexPath%(displayRatio)==0)
			fprintf(logFile, "progress : %.2f%%\n",float(indexPath)/numberOfPointToSimulate*100);
	}

	free(posterioryPath);
}

/*
void simulationFull4pyDev(FILE *logFile,g2s::DataImage &di, std::vector<g2s::DataImage> &TIs, SamplingModule &samplingModule,
 std::vector<std::vector<int> > &pathPosition, unsigned* solvingPath, unsigned numberOfPointToSimulate, g2s::DataImage *ii, float* seedAray, unsigned* importDataIndex, std::vector<unsigned> numberNeighbor,
  unsigned nbThreads=1, bool fullStationary=false, bool circularSim=false){

	int displayRatio=std::max(numberOfPointToSimulate/100,1u);
	unsigned* posterioryPath=(unsigned*)malloc( sizeof(unsigned) * di.dataSize());
	memset(posterioryPath,255,sizeof(unsigned) * di.dataSize());
	for (unsigned int i = 0; i < di.dataSize(); ++i)
	{
		bool withNan=false;
		for (unsigned int j = 0; j < di._nbVariable; ++j)
		{
			withNan|=std::isnan(di._data[i]);
		}
		if(!withNan)
			posterioryPath[i]=0;
	}
	for (unsigned int i = 0; i < numberOfPointToSimulate; ++i)
	{
		posterioryPath[solvingPath[i]]=i;
	}
	
	unsigned numberOfVariable=di._nbVariable;
	for (size_t i = 0; i < categoriesValues.size(); ++i)
	{
		numberOfVariable+=categoriesValues[i].size()-1;
	}
	#pragma omp parallel for num_threads(nbThreads) schedule(dynamic,1) default(none) firstprivate(displayRatio,circularSim, fullStationary, numberOfVariable, categoriesValues, numberOfPointToSimulate, \
		posterioryPath, solvingPath, seedAray, numberNeighbor, importDataIndex, logFile, ii) shared( pathPosition, di, samplingModule, TIs)
	for (unsigned int indexPath = 0; indexPath < numberOfPointToSimulate; ++indexPath){
		

		unsigned moduleID=0;
		#if _OPENMP
			moduleID=omp_get_thread_num();
		#endif
		unsigned currentCell=solvingPath[indexPath];
		if(!std::isnan(di._data[currentCell])) continue;
		float localSeed=seedAray[indexPath];

		unsigned currentVariable=currentCell%di._nbVariable;
		unsigned currentPosition=currentCell/di._nbVariable;

		std::vector<unsigned> numberOfNeighborsProVariable(di._nbVariable);
		std::vector<std::vector<int> > neighborArrayVector;
		std::vector<std::vector<float> > neighborValueArrayVector;
		{
			unsigned positionSearch=0;
			while((numberNeighbor.size()>1||(neighborArrayVector.size()<numberNeighbor[0]))&&(positionSearch<pathPosition.size())){
				unsigned dataIndex;
				std::vector<int> vectorInDi=pathPosition[positionSearch];
				vectorInDi.resize(di._dims.size(),0);
				if(di.indexWithDelta(dataIndex, currentPosition, vectorInDi) || circularSim)
				{
					bool needToBeadd=false;
					for (unsigned int i = 0; i < di._nbVariable; ++i)
					{
						needToBeadd|=(numberOfNeighborsProVariable[i]<numberNeighbor[i%numberNeighbor.size()])&&(posterioryPath[dataIndex*di._nbVariable+i]<indexPath) ;
					}
					//add for
					if(needToBeadd){
						unsigned numberOfNaN=0;
						float val;
						while(true) {
							numberOfNaN=0;
							for (unsigned int i = 0; i < di._nbVariable; ++i)
							{
								#pragma omp atomic read
								val=di._data[dataIndex*di._nbVariable+i];
								numberOfNaN+=(numberOfNeighborsProVariable[i]<numberNeighbor[i%numberNeighbor.size()])&&(posterioryPath[dataIndex*di._nbVariable+i]<indexPath) && std::isnan(val);
							}
							if(numberOfNaN==0)break;
							std::this_thread::sleep_for(std::chrono::microseconds(250));
						}

						std::vector<float> data(di._nbVariable);
						unsigned cpt=0;
						for (unsigned int i = 0; i < di._nbVariable; ++i)
						{
							if((numberOfNeighborsProVariable[i]<numberNeighbor[i%numberNeighbor.size()])&&(posterioryPath[dataIndex*di._nbVariable+i]<indexPath))
							{
								#pragma omp atomic read
								val=di._data[dataIndex*di._nbVariable+i];
								data[i]=val;
								cpt++;
								numberOfNeighborsProVariable[i]++;
							}else{
								data[i]=std::nanf("0");
							}
						}
						neighborValueArrayVector.push_back(data);
						neighborArrayVector.push_back(pathPosition[positionSearch]);
						if(cpt==0) break;
					}
				}
				positionSearch++;
			}
		}
		// conversion from one variable to many
		for (size_t j = 0; j < neighborValueArrayVector.size(); ++j)
		{
			std::vector<float> data(numberOfVariable);
			unsigned id=0;
			unsigned idCategorie=0;
			for (unsigned int i = 0; i < di._nbVariable; ++i)
			{
				if(di._types[i]==g2s::DataImage::Continuous){
					
					data[id]=neighborValueArrayVector[j][i];
					id++;
				}
				if(di._types[i]==g2s::DataImage::Categorical){
					for (size_t k = 0; k < categoriesValues[idCategorie].size(); ++k)
					{
						data[id] = (neighborValueArrayVector[j][i] == categoriesValues[idCategorie][k]);
						id++;
					}
					idCategorie++;
				}
			}
			neighborValueArrayVector[j]=data;
		}


		SamplingModule::matchLocation importIndex;

		importIndex.TI=0;
		importIndex.index=INT_MAX;

		if(neighborArrayVector.size()>1){
			SamplingModule::matchLocation verbatimRecord;
			verbatimRecord.TI=TIs.size();
			importIndex=samplingModule.sample(neighborArrayVector,neighborValueArrayVector,localSeed,verbatimRecord,moduleID, fullStationary, currentVariable ,(ii!=nullptr ? int(ii->_data[currentCell]):-1));
		}else{

			// sample from the marginal
			unsigned cumulated=0;
			for (size_t i = 0; i < TIs.size(); ++i)
			{
				cumulated+=TIs[i].dataSize();
			}
			
			unsigned position=int(floor(localSeed*(cumulated/TIs[0]._nbVariable)));

			cumulated=0;
			for (size_t i = 0; i < TIs.size(); ++i)
			{
				if(position*TIs[0]._nbVariable<cumulated+TIs[i].dataSize()){
					importIndex.TI=i;
					importIndex.index=position-cumulated/TIs[0]._nbVariable;
					break;
				}else{
					cumulated+=TIs[i].dataSize();
				}
			}

			bool hasNaN=std::isnan(TIs[importIndex.TI]._data[importIndex.index*TIs[importIndex.TI]._nbVariable+currentVariable]); 
		
			if(hasNaN){ // nan safe, much slower
				unsigned cumulated=0;
				for (size_t i = 0; i < TIs.size(); ++i)
				{
					for (unsigned int k = 0; k < TIs[i].dataSize()/TIs[i]._nbVariable; ++k)
					{
						bool locHasNan=false;
						int j=currentVariable;
						{
							locHasNan|=std::isnan(TIs[i]._data[k*TIs[i]._nbVariable+j]);
						}
						cumulated+=!locHasNan;
					}
				}
				if(cumulated==0)fprintf(logFile, "error - no available data for variable: %d", currentVariable);
				unsigned position=int(floor(localSeed*(cumulated/TIs[0]._nbVariable)))*TIs[0]._nbVariable;

				cumulated=0;

				for (size_t i = 0; i < TIs.size(); ++i)
				{
					for (unsigned int k = 0; k < TIs[i].dataSize()/TIs[i]._nbVariable; ++k)
					{
						bool locHasNan=false;
						int j=currentVariable;
						{
							locHasNan|=std::isnan(TIs[i]._data[k*TIs[i]._nbVariable+j]);
						}
						cumulated+=!locHasNan;
						if(position<=cumulated){
							importIndex.TI=i;
							importIndex.index=k;
							break;
						}
					}
					if(position<=cumulated)break;
				}
			}
		}
		// import data
		//memcpy(di._data+currentCell*di._nbVariable,TIs[importIndex.TI]._data+importIndex.index*TIs[importIndex.TI]._nbVariable,TIs[importIndex.TI]._nbVariable*sizeof(float));
		importDataIndex[currentCell]=importIndex.index*TIs.size()+importIndex.TI;
		//fprintf(stderr, "write %d\n", importDataIndex[currentCell]);
		
		if(std::isnan(di._data[currentCell])){
			#pragma omp atomic write
			di._data[currentCell]=TIs[importIndex.TI]._data[importIndex.index*TIs[importIndex.TI]._nbVariable+currentVariable];
		}
		if(indexPath%(displayRatio)==0)fprintf(logFile, "progress : %.2f%%\n",float(indexPath)/numberOfPointToSimulate*100);
	}
	free(posterioryPath);
}
*/


#endif // SIMULATION_4_PY_DEV_HPP