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

#ifndef PY_DEV_SAMPLING_MODULE_HPP
#define PY_DEV_SAMPLING_MODULE_HPP

#include "computeDeviceModule.hpp"
#include "samplingModule.hpp"
#include <Python.h>
#include <numpy/arrayobject.h>
#include "python3_interface.hpp"

int init_numpy(){
     import_array(); // PyError if not successful
     return 0;
}

class pyDevSamplingModule: public SamplingModule {

private:
	std::vector<g2s::DataImage> *_TIs;
	bool _completeTIs=true;
	unsigned _nbThreadOverTI=1;
	unsigned _threadRatio=1;
	bool _noVerbatim=false;
	bool _useUniqueTI=false;
	PyObject * _pythonSamplingClass;
	PyObject* samplingNameObject=PyUnicode_FromString((char*) "sample");
public:
	
	pyDevSamplingModule( std::string pathPA, std::string filePA, std::string classPA, std::vector<g2s::DataImage> *TIs, g2s::DataImage* kernel, g2s::DataImage* di, g2s::DataImage* simulationPath, unsigned nbThread, unsigned nbThreadOverTI=1, unsigned threadRatio=1, bool useUniqueTI=false):SamplingModule(nullptr,kernel)
	{
		// fprintf(stderr, "arrived until here\n");
		
		Py_Initialize();
		init_numpy();
		
		PyObject * sys = PyImport_ImportModule("sys");
		PyObject * path = PyObject_GetAttrString(sys, "path");
		Py_XDECREF(sys);
		// PyList_Append(path, PyUnicode_FromString("."));
		PyList_Append(path, PyUnicode_FromString(pathPA.c_str()));
		Py_XDECREF(path);

		
		// {
		// 	PyObject * ModuleString = PyUnicode_FromString((char*) "os");
		// 	PyObject * Module = PyImport_Import(ModuleString);
		// 	PyObject * Dict = PyModule_GetDict(Module);
		// 	PyObject * Func = PyDict_GetItemString(Dict, "getcwd");
		// 	PyObject * Result = PyObject_CallFunction(Func, NULL);
		// }

		InerfaceTemplatePython3 inerfaceTemplatePython3;

		PyObject* tis=PyList_New(TIs->size());
		for (int i = 0; i < TIs->size(); ++i)
		{
			PyObject* imageNumpy=std::any_cast<PyObject*>(inerfaceTemplatePython3.convert2NativeMatrix(TIs->at(i)));
			PyList_SET_ITEM(tis,i,imageNumpy);
		}
		
		long size=(di->_types.size());
		PyObject* type=type=PyArray_SimpleNew(1,&size,NPY_INT32);
		int32_t *typePtr=(int32_t*)PyArray_DATA(type);
		for (size_t i = 0; i < size; ++i)
		{
			switch(di->_types[i]){
				case g2s::DataImage::VaraibleType::Continuous:
				typePtr[i]=0;
				break;
				case g2s::DataImage::VaraibleType::Categorical:
				typePtr[i]=1;
				break;
			}
		}


		PyObject* createArgs=PyTuple_New(5);
		PyTuple_SetItem(createArgs,0,tis);
		PyTuple_SetItem(createArgs,1,std::any_cast<PyObject*>(inerfaceTemplatePython3.convert2NativeMatrix(*kernel)));
		PyTuple_SetItem(createArgs,2,std::any_cast<PyObject*>(inerfaceTemplatePython3.convert2NativeMatrix(*di)));
		PyTuple_SetItem(createArgs,3,type);
		if(simulationPath->_dims.size()<1)
			PyTuple_SetItem(createArgs,4,Py_None);	
		else
			PyTuple_SetItem(createArgs,4,std::any_cast<PyObject*>(inerfaceTemplatePython3.convert2NativeMatrix(*simulationPath)));
		

		PyObject * ModuleString = PyUnicode_FromString(filePA.c_str());
		PyObject * Module = PyImport_Import(ModuleString);
		Py_XDECREF(ModuleString);
		PyObject * Dict = PyModule_GetDict(Module);
		Py_XDECREF(Module);
		PyObject * object = PyDict_GetItemString(Dict, classPA.c_str());
		Py_XDECREF(Dict);
		_pythonSamplingClass = PyObject_CallObject(object,createArgs);
		Py_XDECREF(object);

		
	}

	~pyDevSamplingModule(){
		Py_XDECREF(_pythonSamplingClass);
		Py_Finalize();
	};

	inline void build(std::vector<unsigned> nbNeighbors){


		PyObject* ns=PyList_New(nbNeighbors.size());
		
		for (int i = 0; i < nbNeighbors.size(); ++i)
		{
			PyList_SET_ITEM(ns,i,PyLong_FromLong(nbNeighbors[i]));
		}
		
		PyGILState_STATE state=PyGILState_Ensure();
		
		PyObject_CallMethodObjArgs(_pythonSamplingClass, PyUnicode_FromString((char*) "build"),ns,NULL);
		
		PyGILState_Release(state);
		Py_XDECREF(ns);
	}

	inline matchLocation sample(std::vector<std::vector<int>> neighborArrayVector, std::vector<std::vector<float> > neighborValueArrayVector,float seed, matchLocation verbatimRecord, unsigned moduleID=0, bool fullStationary=false, unsigned variableOfInterest=0, int idTI4Sampling=-1){

		
		matchLocation result;
		// result.TI=selectedTI;
		// result.index=indexInTI;

		return result;
	}

	narrownessMeasurment narrowness(std::vector<std::vector<int>> neighborArrayVector, std::vector<std::vector<float> > neighborValueArrayVector,float seed, unsigned moduleID=0, bool fullStationary=false){
		narrownessMeasurment a;
		return a;
	}

	// SamplingModule::simValues importIndex;
	// 	importIndex.index=INT_MAX;
	// 	// importIndex.values=nullptr;

	simValues sampleValue(std::vector<std::vector<int>> neighborArrayVector, std::vector<std::vector<float> > neighborValueArrayVector,float seed, unsigned index, unsigned moduleID=0, bool fullStationary=false, unsigned variableOfInterest=0, int idTI4Sampling=-1){
		PyObject* position=PyList_New(neighborArrayVector.size());
		for (int i = 0; i < neighborArrayVector.size(); ++i)
		{
			PyObject* coordinates=PyList_New(neighborArrayVector[i].size());
			for (int j = 0; j < neighborArrayVector[i].size(); ++j)
			{
				PyList_SET_ITEM(coordinates,j,PyLong_FromLong(neighborArrayVector[i][j]));
			}
			PyList_SET_ITEM(position,i,coordinates);
		}

		PyObject* values=PyList_New(neighborValueArrayVector.size());
		for (int i = 0; i < neighborValueArrayVector.size(); ++i)
		{
			PyObject* coordinates=PyList_New(neighborValueArrayVector[i].size());
			for (int j = 0; j < neighborValueArrayVector[i].size(); ++j)
			{
				PyList_SET_ITEM(coordinates,j,PyFloat_FromDouble(neighborValueArrayVector[i][j]));
			}
			PyList_SET_ITEM(values,i,coordinates);
		}
		PyObject* resultObject;
		// 
		#pragma omp critical(python)
		{
			PyGILState_STATE state=PyGILState_Ensure();
			resultObject=PyObject_CallMethodObjArgs(_pythonSamplingClass, samplingNameObject,position,values,PyFloat_FromDouble(seed),PyLong_FromLong(index),PyLong_FromLong(moduleID),PyBool_FromLong(fullStationary),PyLong_FromLong(variableOfInterest),PyLong_FromLong(idTI4Sampling),NULL);
			PyGILState_Release(state);
		}
		Py_XDECREF(position);
		Py_XDECREF(values);

		// fprintf(stderr,"Result: %s\n", PyBytes_AS_STRING(PyUnicode_AsEncodedString(PyObject_Repr(resultObject), "utf-8", "~E~")));
		if(!PyTuple_Check(resultObject) || PyTuple_GET_SIZE(resultObject)!=2){
			fprintf(stderr, "is sample return is not a tuple, or the tuple is not of size 2\n");
		}
		PyObject* valuesObject=PyTuple_GetItem(resultObject,0);
		PyObject* indexObject=PyTuple_GetItem(resultObject,1);

		if(!PyList_Check(valuesObject) && !(PyArray_Check(valuesObject) && PyArray_ISFLOAT(valuesObject)) && !PyFloat_Check(valuesObject)){
			fprintf(stderr, "The first output of the tuple need to be a value, a list or a numpy array of floats\n");
		}
		if(!PyLong_Check(indexObject)){
			fprintf(stderr, "The second output of the tuple need to be a integer\n");
		}
		simValues result;
		result.index=PyLong_AsLong(indexObject);
		if(PyList_Check(valuesObject)) {
			for (int i = 0; i < PyList_Size(valuesObject); ++i)
			{
				result.values.push_back(PyFloat_AsDouble(PyList_GetItem(valuesObject,i)));
			}
		}
		else if(PyArray_Check(valuesObject)) {
			valuesObject=PyArray_ContiguousFromAny(valuesObject,NPY_FLOAT64,0,0);
			double *ptr=(double*)PyArray_DATA(valuesObject);
			for (int i = 0; i < PyArray_SIZE(valuesObject); ++i)
			{
				result.values.push_back(ptr[i]);
			}
			Py_XDECREF(valuesObject);
		}
		else {
				result.values.push_back(PyFloat_AsDouble(valuesObject));
		}
		Py_XDECREF(resultObject);
		return result;
	}
};
#endif // PY_DEV_SAMPLING_MODULE_HPP
