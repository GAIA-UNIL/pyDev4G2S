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

#include <iostream>
#include <cmath>
#include <chrono>
#include <random>
#include "utils.hpp"
#include "DataImage.hpp"
#include "jobManager.hpp"

#include "pyDevSamplingModule.hpp"
#include "simulation4pyDev.hpp"

enum simType
{
	vectorSim,
	fullSim,
	augmentedDimSim
};


void printHelp(){
	printf ("that is the help");
}

int main(int argc, char const *argv[]) {

	std::multimap<std::string, std::string> arg=g2s::argumentReader(argc,argv);

	char logFileName[2048]={0};
	std::vector<std::string> sourceFileNameVector;
	std::string targetFileName;
	std::string kernelFileName;
	std::string simuationPathFileName;
	std::string idImagePathFileName;

	std::string outputFilename;
	std::string outputIndexFilename;


	jobIdType uniqueID=-1;
	bool run=true;


	// manage report file
	FILE *reportFile=NULL;
	if (arg.count("-r") > 1)
	{
		fprintf(reportFile,"only one rapport file is possible\n");
		run=false;
	}else{
		if(arg.count("-r") ==1){
			if(!strcmp((arg.find("-r")->second).c_str(),"stderr")){
				reportFile=stderr;
			}
			if(!strcmp((arg.find("-r")->second).c_str(),"stdout")){
				reportFile=stdout;
			}
			if (reportFile==NULL) {
				strcpy(logFileName,(arg.find("-r")->second).c_str());
				reportFile=fopen((arg.find("-r")->second).c_str(),"a");
				setvbuf ( reportFile , nullptr , _IOLBF , 0 ); // maybe  _IONBF


				jobIdType logId;
				if(sscanf(logFileName,"logs/%u.log",&logId)==1){
					std::to_string(logId);
					uniqueID=logId;
				}
			}
			if (reportFile==NULL){
				fprintf(stderr,"Impossible to open the rapport file\n");
				run=false;
			}
		}
	}
	arg.erase("-r");
	for (int i = 0; i < argc; ++i)
	{
		fprintf(reportFile,"%s ",argv[i]);
	}
	fprintf(reportFile,"\n");

	// LOOK FOR STANDARD PARAMETER

	unsigned nbThreads=1;
	unsigned nbThreadsOverTi=1;
	unsigned nbThreadsLastLevel=1;
	unsigned totalNumberOfThreadVailable=1;
	bool verbose=false;


	#if _OPENMP
		totalNumberOfThreadVailable=omp_get_max_threads();
	#endif	
	
	if (arg.count("-j") >= 1)
	{
		std::multimap<std::string, std::string>::iterator jobsString=arg.lower_bound("-j");

		if(jobsString!=arg.upper_bound("-j")){
			float nbThreadsLoc=atof((jobsString->second).c_str());
			if(std::roundf(nbThreadsLoc) != nbThreadsLoc){
				nbThreadsLoc=std::max(std::floor(nbThreadsLoc*totalNumberOfThreadVailable),1.f);
			}
			nbThreads=(int)(nbThreadsLoc);
			++jobsString;
		}
		if(jobsString!=arg.upper_bound("-j")){
			float nbThreadsOverTiLoc=atof((jobsString->second).c_str());
			if(std::roundf(nbThreadsOverTiLoc) != nbThreadsOverTiLoc){
				nbThreadsOverTiLoc=std::max(std::floor(nbThreadsOverTiLoc*totalNumberOfThreadVailable),1.f);
			}
			nbThreadsOverTi=(int)(nbThreadsOverTiLoc);
			++jobsString;
		}
		if(jobsString!=arg.upper_bound("-j")){
			float nbThreadsLastLevelLoc=atof((jobsString->second).c_str());
			if(std::roundf(nbThreadsLastLevelLoc) != nbThreadsLastLevelLoc){
				nbThreadsLastLevelLoc=std::max(std::floor(nbThreadsLastLevelLoc*totalNumberOfThreadVailable),1.f);
			}
			nbThreadsLastLevel=(int)(nbThreadsLastLevelLoc);
			++jobsString;
		}
	}
	arg.erase("-j");

	if (arg.count("--jobs") >= 1)
	{
		std::multimap<std::string, std::string>::iterator jobsString=arg.lower_bound("--jobs");
		if(jobsString!=arg.upper_bound("--jobs")){
			float nbThreadsLoc=atof((jobsString->second).c_str());
			if(std::roundf(nbThreadsLoc) != nbThreadsLoc){
				nbThreadsLoc=std::max(std::floor(nbThreadsLoc*totalNumberOfThreadVailable),1.f);
			}
			nbThreads=(int)(nbThreadsLoc);
			++jobsString;
		}
		if(jobsString!=arg.upper_bound("--jobs")){
			float nbThreadsOverTiLoc=atof((jobsString->second).c_str());
			if(std::roundf(nbThreadsOverTiLoc) != nbThreadsOverTiLoc){
				nbThreadsOverTiLoc=std::max(std::floor(nbThreadsOverTiLoc*totalNumberOfThreadVailable),1.f);
			}
			nbThreadsOverTi=(int)(nbThreadsOverTiLoc);
			++jobsString;
		}
		if(jobsString!=arg.upper_bound("--jobs")){
			float nbThreadsLastLevelLoc=atof((jobsString->second).c_str());
			if(std::roundf(nbThreadsLastLevelLoc) != nbThreadsLastLevelLoc){
				nbThreadsLastLevelLoc=std::max(std::floor(nbThreadsLastLevelLoc*totalNumberOfThreadVailable),1.f);
			}
			nbThreadsLastLevel=(int)(nbThreadsLastLevelLoc);
			++jobsString;
		}
	}
	arg.erase("--jobs");

	if(nbThreads<1)
	#if _OPENMP
		nbThreads=totalNumberOfThreadVailable;
	#else
		nbThreads=1;
	#endif

	if ((arg.count("-h") == 1)|| (arg.count("--help") == 1))
	{
		printHelp();
		return 0;
	}
	arg.erase("-h");

	if ((arg.count("-v") == 1) || (arg.count("--verbose") == 1))
	{
		verbose=true;
	}
	arg.erase("--verbose");


	std::string pathPA;
	std::string filePA;
	std::string classPA;

	if (arg.count("-pa") > 0)
	{
		std::string pythonAlgorithPath=arg.find("-pa")->second;
		std::size_t foundEndPath = pythonAlgorithPath.find_last_of("/");
		std::size_t foundEndFile = pythonAlgorithPath.find_last_of(":");
		
		if(foundEndPath==std::string::npos){
			pathPA=std::string("./../PyDevAlgo/");
		}else
		{
			pathPA=pythonAlgorithPath.substr(0,foundEndPath);
			if(pathPA[0]!='/')
				pathPA=std::string("./../PyDevAlgo/")+pathPA;
		}
		if(foundEndFile==std::string::npos)
		{
			filePA=pythonAlgorithPath.substr(foundEndPath+1,-1);
			classPA=filePA;
		}else{
			filePA=pythonAlgorithPath.substr(foundEndPath+1,foundEndFile-foundEndPath-1);
			classPA=pythonAlgorithPath.substr(foundEndFile+1,-1);
		}
		
	}else{	
		fprintf(reportFile,"error no python algorithm\n");
		run=false;
	}
	arg.erase("-pa");


	// LOOK FOR DATA FILES
	//look for training images
	if (arg.count("-ti") > 0)
	{
		std::multimap<std::string, std::string>::iterator it;
	    for (it=arg.equal_range("-ti").first; it!=arg.equal_range("-ti").second; ++it)
	    {
	    	sourceFileNameVector.push_back(it->second);
	    }
	}
	arg.erase("-ti");

	//look for destination images (hard data)
	if (arg.count("-di") ==1)
	{
		targetFileName=arg.find("-di")->second;
	}else{	
		fprintf(reportFile,"error target\n");
		run=false;
	}
	arg.erase("-di");

	//look for -ki			: kernel image 
	if (arg.count("-ki") ==1)
	{
		kernelFileName=arg.find("-ki")->second;
	}else{	
		fprintf(reportFile,"non critical error : no kernel \n");
	}
	arg.erase("-ki");

	//look for -sp			: simulation path 
	if (arg.count("-sp") ==1)
	{
		simuationPathFileName=arg.find("-sp")->second;
	}else{	
		fprintf(reportFile,"non critical error : no simulation path\n");
	}
	arg.erase("-sp");

	bool useUniqueTI4Sampling=false;
	//look for -ii			: image of training index
	if (arg.count("-ii") ==1)
	{
		idImagePathFileName=arg.find("-ii")->second;
		useUniqueTI4Sampling=true;
	}
	arg.erase("-ii");






	// LOOK FOR OUTPUT
	if (arg.count("-o") ==1)
	{
		outputFilename=arg.find("-o")->second;
		run=false;
	}else{
		outputFilename=std::to_string(uniqueID);
		outputIndexFilename=std::string("id_")+std::to_string(uniqueID);
	}
	arg.erase("-o");

	if (arg.count("-oi") ==1)
	{
		outputIndexFilename=arg.find("-oi")->second;
	}
	arg.erase("-oi");

	

	// autoSave
	unsigned interval=0;
	jobIdType previousID=0;
	if (arg.count("-as") >0)
	{
		std::multimap<std::string, std::string>::iterator autoSaveParam=arg.lower_bound("-as");
		if(autoSaveParam!=arg.upper_bound("-as")){
			interval=atoi((autoSaveParam->second).c_str());
			++autoSaveParam;
		}
		if(autoSaveParam!=arg.upper_bound("-as")){
			previousID=atol((autoSaveParam->second).c_str());
			++autoSaveParam;
		}
	}
	arg.erase("-as");


	// LOOK FOR SETINGS
	bool noVerbatim=false;
	bool fullStationary=false;
	
	std::vector<unsigned> nbNeighbors;						// number of nighbors QS, DS ...
	unsigned seed=std::chrono::high_resolution_clock::now().time_since_epoch().count();
	g2s::DistanceType searchDistance=g2s::EUCLIDIEN;
	bool requestFullSimulation=false;
	bool conciderTiAsCircular=false;
	bool circularSimulation=false;
	bool augmentedDimentionSimulation=false;

	if (arg.count("-fs") == 1)
	{
		requestFullSimulation=true;
	}
	arg.erase("-fs");

	if (arg.count("-n") >= 1)
	{
		for (auto val=arg.lower_bound("-n"); val!=arg.upper_bound("-n"); val++){
			nbNeighbors.push_back(atoi((val->second).c_str()));
		}
	}
	arg.erase("-n");

	if (arg.count("-s") == 1)
	{
		seed=atoi((arg.find("-s")->second).c_str());
	}
	arg.erase("-s");

	if (arg.count("-wd") == 1)
	{
		searchDistance=g2s::KERNEL;
	}
	arg.erase("-wd");

	if (arg.count("-ed") == 1)
	{
		searchDistance=g2s::EUCLIDIEN;
	}
	arg.erase("-ed");

	if (arg.count("-md") == 1)
	{
		searchDistance=g2s::MANAHTTAN;
	}
	arg.erase("-md");

	if (arg.count("-fastAndRisky") == 1)
	{
		fullStationary=true;
	}
	arg.erase("-fastAndRisky");
	if (arg.count("-far") == 1)
	{
		fullStationary=true;
	}
	arg.erase("-far");

	if (arg.count("-cti") == 1)
	{
		conciderTiAsCircular=true;
	}
	arg.erase("-cti");

	if (arg.count("-csim") == 1)
	{
		circularSimulation=true;
	}
	arg.erase("-csim");

	if (arg.count("-adsim") == 1)
	{
		augmentedDimentionSimulation=true;
	}
	arg.erase("-adsim");

	//add extra paremetre here
	float alpha=0;
	g2s::KernelType kernelTypeForGeneration=g2s::UNIFORM;
	int kernelSize=-1;
	if (arg.count("-kernel") == 1)
	{
		//TODO implement the selecteur 
		// UNIFORM,
		// TRIANGULAR,
		// EXPONENTIAL,
		// EPANECHNIKOV,
		// QUARTIC,
		// TRIWEIGHT,
		// TRICUBE,
		// GAUSSIAN,
		// COSINE,
		// LOGISTIC,
		// SIGMOID,
		// SILVERMAN
	}
	arg.erase("-kernel");
	if (arg.count("-ks") == 1)
	{
		kernelSize=atof((arg.find("-ks")->second).c_str());
	}
	arg.erase("-ks");
	if (arg.count("-alpha") == 1)
	{
		alpha=atof((arg.find("-alpha")->second).c_str());
	}
	arg.erase("-alpha");


	bool withGPU=false;
	if (arg.count("-W_GPU") == 1)
	{
		withGPU=true;//atof((arg.find("-W_GPU")->second).c_str());
	}
	arg.erase("-W_GPU");

	bool withCUDA=false;
	std::vector<int> cudaDeviceList;
	#ifdef WITH_CUDA
	if (arg.count("-W_CUDA") >= 1)
	{
		withCUDA=true;
		int cudaDeviceAvailable=0;
		cudaGetDeviceCount(&cudaDeviceAvailable);
		std::multimap<std::string, std::string>::iterator deviceString=arg.lower_bound("-W_CUDA");
		if(deviceString==arg.upper_bound("-W_CUDA")){
			for (int i = 0; i < cudaDeviceAvailable; ++i)
			{
				cudaDeviceList.push_back(i);
			}
		}
		while(deviceString!=arg.upper_bound("-W_CUDA")){
			int deviceId=atoi((deviceString->second).c_str());
			cudaDeviceList.push_back(deviceId);
			deviceString++;
		}
	}
	arg.erase("-W_CUDA");
	#endif


	// precheck | check what is mandatory

	if(nbNeighbors.size()<=0){
		run=false;
		fprintf(reportFile, "%s\n", "number of neighbor parameter not valide" );
	}

	// print all ignored parameters
	for (std::multimap<std::string, std::string>::iterator it=arg.begin(); it!=arg.end(); ++it){
		fprintf(reportFile, "%s %s <== ignored !\n", it->first.c_str(), it->second.c_str());
	}

	if(!run){
		fprintf(reportFile, "simulation interupted !!\n");
		return 0;
	}

	std::mt19937 randomGenerator(seed);

	std::vector<g2s::DataImage > TIs;

	for (size_t i = 0; i < sourceFileNameVector.size(); ++i)
	{
		TIs.push_back(g2s::DataImage::createFromFile(sourceFileNameVector[i]));
	}

	g2s::DataImage DI=g2s::DataImage::createFromFile(targetFileName);

	if(DI._dims.size()<=TIs[0]._dims.size()) // auto desactivate of the dimention augmentation, if the dimention is not good
		augmentedDimentionSimulation=false;


	g2s::DataImage kernel;
	g2s::DataImage simulationPath;
	g2s::DataImage idImage;

	if(kernelFileName.empty()) {
		std::vector<unsigned> maxSize=DI._dims;
		if(TIs.size()>0)
			maxSize=TIs[0]._dims;
		if(kernelSize!=-1){
			for (size_t i = 0; i < maxSize.size(); ++i)
			{
				maxSize[i]=kernelSize;
			}
		}else{
			for (size_t j = 0; j < TIs.size(); ++j)
			{
				for (size_t i = 0; i < maxSize.size(); ++i)
				{
					maxSize[i]=std::min(TIs[j]._dims[i]/2+1,maxSize[i]);
				}
			}
		}
		std::vector<float> variableWeight(DI._nbVariable);
		for (size_t i = 0; i < variableWeight.size(); ++i)
		{
			variableWeight[i]=1;
		}
		std::vector<float> alphas(DI._nbVariable);
		for (size_t i = 0; i < alphas.size(); ++i)
		{
			alphas[i]=alpha;
		}
		std::vector<g2s::KernelType> kernelsTypeFG(DI._nbVariable);
		for (size_t i = 0; i < kernelsTypeFG.size(); ++i)
		{
			kernelsTypeFG[i]=kernelTypeForGeneration;
		}
		kernel=g2s::DataImage::genearteKernel(kernelsTypeFG, maxSize, variableWeight, alphas);
	}
	else {
		kernel=g2s::DataImage::createFromFile(kernelFileName);
		if(kernel._dims.size()-1==TIs[0]._dims.size()){
			kernel.convertFirstDimInVariable();
		}
	}

	std::vector<std::vector<int> > pathPosition;
	pathPosition.push_back(std::vector<int>(0));
	for (size_t i = 0; i < kernel._dims.size(); ++i)
	{
		unsigned originalSize=pathPosition.size();
		int sizeInThisDim=(kernel._dims[i]+1)/2;
		pathPosition.resize(originalSize*(2*sizeInThisDim-1));
		for (unsigned int k = 0; k < originalSize; ++k)
		{
			pathPosition[k].push_back(0);
		}
		for (int j = 1; j < sizeInThisDim; ++j)
		{
			std::copy ( pathPosition.begin(), pathPosition.begin()+originalSize, pathPosition.begin()+originalSize*(-1+2*j+0) );
			std::copy ( pathPosition.begin(), pathPosition.begin()+originalSize, pathPosition.begin()+originalSize*(-1+2*j+1) );
			for (unsigned int k = originalSize*(-1+2*j+0); k < originalSize*(-1+2*j+1); ++k)
			{
				pathPosition[k][i]=j;
			}
			for (unsigned int k = originalSize*(-1+2*j+1); k < originalSize*(-1+2*j+2); ++k)
			{
				pathPosition[k][i]=-j;
			}
		}
	}

	g2s::DataImage wieghtKernel=kernel.emptyCopy(true);
	if(searchDistance==g2s::EUCLIDIEN){
		for (unsigned int i = 0; i < wieghtKernel.dataSize(); ++i)
		{
			wieghtKernel._data[i]=-wieghtKernel.distance2ToCenter(i);
		}
	}
	if(searchDistance==g2s::KERNEL){
		unsigned nbV=kernel._nbVariable;
		for (unsigned int i = 0; i < wieghtKernel.dataSize(); ++i)
		{
			for (unsigned int j = 0; j < nbV; ++j)
			{
				if(fabs(kernel._data[i*nbV+j])>wieghtKernel._data[i])wieghtKernel._data[i]=fabs(kernel._data[i*nbV+j]);
			}
		}
	}

	unsigned center=0;
	g2s::DataImage* wieghtKernelPtr=wieghtKernel.ptr();
	for (int i =  wieghtKernelPtr->_dims.size()-1; i>=0 ; i--)
	{
		center=center*wieghtKernelPtr->_dims[i]+(wieghtKernelPtr->_dims[i]-1)/2;
	}

	std::sort(pathPosition.begin(),pathPosition.end(),[wieghtKernelPtr, center](std::vector<int> &a, std::vector<int> &b){
		unsigned l1,l2;
		wieghtKernelPtr->indexWithDelta(l1, center, a);
		wieghtKernelPtr->indexWithDelta(l2, center, b);
		return wieghtKernelPtr->_data[l1] > wieghtKernelPtr->_data[l2];
	});

	unsigned simulationPathSize=0;
	unsigned* simulationPathIndex=nullptr;
	unsigned beginPath=0;
	bool fullSimulation=false;

	if(simuationPathFileName.empty()) {  //todo, need to be redsign to handle augmentedDimentionSimulation
		//fprintf(stderr, "generate simulation path\n");
		if (requestFullSimulation)
		{
			simulationPathSize=DI.dataSize();
			fullSimulation=true;
		}else{
			simulationPathSize=DI.dataSize()/DI._nbVariable;
			fullSimulation=false;
		}
		simulationPathIndex=(unsigned *)malloc(sizeof(unsigned)*simulationPathSize);
		for (unsigned i = 0; i < simulationPathSize; ++i)
		{
			simulationPathIndex[i]=i;
		}

		if (fullSimulation)
		{
			for (unsigned int i = 0; i < simulationPathSize; ++i)
			{
				if(!std::isnan(DI._data[i])){
					std::swap(simulationPathIndex[beginPath],simulationPathIndex[i]);
					beginPath++;
				}
			}

		}else{
			for (unsigned int i = 0; i < simulationPathSize; ++i)
			{
				bool valueSeted=true;
				for (unsigned int j = 0; j < DI._nbVariable; ++j)
				{
					if(std::isnan(DI._data[i*DI._nbVariable+j]))valueSeted=false;
				}
				if(valueSeted)
				{
					std::swap(simulationPathIndex[beginPath],simulationPathIndex[i]);
					beginPath++;
				}
			}
		}
		std::shuffle(simulationPathIndex+beginPath, simulationPathIndex + simulationPathSize, randomGenerator );
	}
	else {
		simulationPath=g2s::DataImage::createFromFile(simuationPathFileName);
		simulationPathSize=simulationPath.dataSize();
		bool dimAgree=true;
		fullSimulation=false;
		if(simulationPath._dims.size()!=DI._dims.size()){
			if(simulationPath._dims.size()-1==DI._dims.size()){
				simulationPath.convertFirstDimInVariable();
				fullSimulation=true;
			}else dimAgree=false;
		}
		for (size_t i = 0; i < simulationPath._dims.size(); ++i)
		{
			if(simulationPath._dims[i]!=DI._dims[i])dimAgree=false;
		}
		if(!dimAgree){
			fprintf(reportFile, "%s\n", "dimension bettwen simulation path and destination grid disagree");
			return 0;
		}

		simulationPathIndex=(unsigned *)malloc(sizeof(unsigned)*simulationPathSize);
		std::iota(simulationPathIndex,simulationPathIndex+simulationPathSize,0);
		float* simulationPathData=simulationPath._data;
		std::sort(simulationPathIndex, simulationPathIndex+simulationPathSize,
			[simulationPathData](unsigned i1, unsigned i2) {return simulationPathData[i1] < simulationPathData[i2];});

		//Search begin path
		for ( beginPath=0 ; beginPath < simulationPathSize; ++beginPath)
		{
			float value=simulationPathData[simulationPathIndex[beginPath]];
			if((!std::isinf(value))||(value>0)) break;
		}

	}

	g2s::DataImage id=g2s::DataImage::createFromFile(std::string("im_2_")+std::to_string(previousID)+std::string(".auto_bk"));

	if(id._dims.size()<1){
		id=DI.emptyCopy(!fullSimulation);
		id.setEncoding(g2s::DataImage::EncodingType::UInteger);
		memset(id._data,0,sizeof(unsigned)*simulationPathSize);
	}else{
		DI=g2s::DataImage::createFromFile(std::string("im_1_")+std::to_string(previousID)+std::string(".auto_bk"));
	}
	
	unsigned* importDataIndex=(unsigned *)id._data;
	float* seedForIndex=( float* )malloc( sizeof(float) * simulationPathSize );
	std::uniform_real_distribution<float> uniformDitributionOverSource(0.f,1.f);

	for ( unsigned int i = 0; i < simulationPathSize; ++i)
	{
		seedForIndex[i]=uniformDitributionOverSource(randomGenerator);
		if(seedForIndex[i]==1.f)seedForIndex[i]=uniformDitributionOverSource(randomGenerator);
	}


	// id Image

	if (!idImagePathFileName.empty())
	{
		idImage=g2s::DataImage::createFromFile(idImagePathFileName);
	}

	// init pyDev
	pyDevSamplingModule pySM(pathPA, filePA, classPA, &TIs, &kernel, &DI, &simulationPath, nbThreads, nbThreadsOverTi, nbThreadsLastLevel, useUniqueTI4Sampling);
	pySM.build(nbNeighbors);
	// run QS

	auto begin = std::chrono::high_resolution_clock::now();

	simType st=vectorSim;
	if(fullSimulation) st=fullSim;
	if(augmentedDimentionSimulation) st=augmentedDimSim;

	auto autoSaveFunction=[](g2s::DataImage &id, g2s::DataImage &DI, std::atomic<bool>  &computationIsDone, unsigned interval, jobIdType uniqueID){
			unsigned last=0;
			while (!computationIsDone)
			{
				if(last>=interval){
					id.write(std::string("im_2_")+std::to_string(uniqueID)+std::string(".auto_bk"));
					DI.write(std::string("im_1_")+std::to_string(uniqueID)+std::string(".auto_bk"));
					last=0;
				}
				last++;
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}
		};


	std::thread saveThread;
	std::atomic<bool> computationIsDone(false);
	if(interval>0){
		saveThread=std::thread(autoSaveFunction, std::ref(id), std::ref(DI), std::ref(computationIsDone), interval, uniqueID);
	}

	// switch (st){
	// 	// case fullSim:
	// 	// 	fprintf(reportFile, "%s\n", "full sim");
	// 	// 	simulationFull(reportFile, DI, TIs, pySM, pathPosition, simulationPathIndex+beginPath, simulationPathSize-beginPath, (useUniqueTI4Sampling ? &idImage : nullptr ),
	// 	// 		seedForIndex, importDataIndex, nbNeighbors, categoriesValues, nbThreads, fullStationary, circularSimulation);
	// 	// break;
	// 	case vectorSim:
			fprintf(reportFile, "%s\n", "vector sim");
			simulation4pyDev(reportFile, DI, TIs, pySM, pathPosition, simulationPathIndex+beginPath, simulationPathSize-beginPath, (useUniqueTI4Sampling ? &idImage : nullptr ),
				seedForIndex, importDataIndex, nbNeighbors, nbThreads, fullStationary, circularSimulation);
	// 	break;
	// }
 
	auto end = std::chrono::high_resolution_clock::now();
	computationIsDone=true;
	double time = 1.0e-6 * std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
	fprintf(reportFile,"compuattion time: %7.2f s\n", time/1000);
	fprintf(reportFile,"compuattion time: %.0f ms\n", time);

	// free memory

	// to remove later
	id.write(outputIndexFilename);
	DI.write(outputFilename);
	//end to remove

	// new filename 
	id.write(std::string("im_2_")+std::to_string(uniqueID));
	DI.write(std::string("im_1_")+std::to_string(uniqueID));

	if(saveThread.joinable()){
		saveThread.join();
	}

	free(simulationPathIndex);
	simulationPathIndex=nullptr;


	return 0;
}